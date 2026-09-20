/*
    This file is part of GTgram.

    GTgram is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GTgram is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GTgram. If not, see <http://www.gnu.org/licenses/>.

    The voice-call handling in this file is adapted from RooTelegram-SailfishOS
    (https://github.com/RootGPT-YouTube/RooTelegram-SailfishOS, GPLv3), which
    built it on top of tgcalls (LGPLv3, https://github.com/TelegramMessenger/tgcalls).
*/
#include "callmanager.h"

#include "tdlibwrapper.h"
#include "mceinterface.h"
#include "systemcallbridge.h"
#include "videocalls/callvideorenderer.h"
#include <QTimer>

#define DEBUG_MODULE CallManager
#include "debuglog.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <pulse/pulseaudio.h>
#include <QMetaObject>
#include <QStringList>
#include <QStandardPaths>
#include <tgcalls/Instance.h>
#include <tgcalls/InstanceImpl.h>
#include <tgcalls/StaticThreads.h>
#include <tgcalls/VideoCaptureInterface.h>
#include <tgcalls/v2/InstanceV2Impl.h>
#include <tgcalls/v2/InstanceV2ReferenceImpl.h>

namespace {
const auto RegisterLegacyInstance = tgcalls::Register<tgcalls::InstanceImpl>();
const auto RegisterV2Instance = tgcalls::Register<tgcalls::InstanceV2Impl>();
const auto RegisterV2ReferenceInstance = tgcalls::Register<tgcalls::InstanceV2ReferenceImpl>();
}

CallManager::CallManager(TDLibWrapper *tdLibWrapper, MceInterface *mceInterface, QObject *parent)
    : QObject(parent)
    , tdLibWrapper(tdLibWrapper)
    , mceInterface(mceInterface)
    , systemCallBridge(new SystemCallBridge(this))
    , m_audioUnmuteTimer(new QTimer(this))
    , m_displayOnTimer(new QTimer(this))
    , currentCallId(0)
    , currentUserId(0)
    , currentIsOutgoing(false)
    , currentIsVideo(false)
    , remoteVideoRenderer(new CallVideoRenderer(this))
    , localVideoRenderer(new CallVideoRenderer(this))
    , m_frontCamera(true)
    , m_remoteVideoActive(false)
    , m_pulseMainloop(nullptr)
    , m_pulseContext(nullptr)
    , m_speakerOn(false)
{
    Q_UNUSED(RegisterLegacyInstance);
    Q_UNUSED(RegisterV2Instance);
    Q_UNUSED(RegisterV2ReferenceInstance);

    if (!this->tdLibWrapper) {
        WARN("CallManager initialized without TDLibWrapper");
        return;
    }

    connect(this->tdLibWrapper, &TDLibWrapper::callUpdated, this, &CallManager::handleCallUpdated);
    connect(this->tdLibWrapper, &TDLibWrapper::callSignalingDataReceived, this, &CallManager::handleCallSignalingDataReceived);

    // Commands from the SYSTEM call UI (answer/hangup) need translating into
    // TDLib actions, otherwise those buttons do nothing; speaker/mute map
    // straight onto our own PulseAudio-facing methods.
    connect(systemCallBridge, &SystemCallBridge::answerRequested, this, &CallManager::handleSystemAnswerRequested);
    connect(systemCallBridge, &SystemCallBridge::hangupRequested, this, &CallManager::handleSystemHangupRequested);
    connect(systemCallBridge, &SystemCallBridge::speakerModeRequested, this, &CallManager::setSpeakerphoneOn);
    connect(systemCallBridge, &SystemCallBridge::muteMicrophoneRequested, this, &CallManager::setMicrophoneMuted);

    // The WebRTC playout stream appears in PulseAudio shortly after the call
    // connects - retry routing it every 500ms until found, then stop.
    m_audioUnmuteTimer->setInterval(500);
    connect(m_audioUnmuteTimer, &QTimer::timeout, this, [this]() {
        if (routeWebrtcToCallSink()) {
            setSpeakerphoneOn(m_speakerOn);
            m_audioUnmuteTimer->stop();
        }
    });

    // MCE guarantees a display blanking pause for ~60s at a time; renew it
    // periodically while ringing.
    m_displayOnTimer->setInterval(50000);
    connect(m_displayOnTimer, &QTimer::timeout, this, [this]() {
        if (this->mceInterface) {
            this->mceInterface->displayBlankingPause();
        }
    });
}

CallManager::~CallManager()
{
    stopInstance();
}

bool CallManager::systemCallUiActive() const
{
    return systemCallBridge && systemCallBridge->isAvailable();
}

QObject *CallManager::remoteVideo() const
{
    return remoteVideoRenderer;
}

QObject *CallManager::localVideo() const
{
    return localVideoRenderer;
}

bool CallManager::frontCamera() const
{
    return m_frontCamera;
}

bool CallManager::remoteVideoActive() const
{
    return m_remoteVideoActive;
}

void CallManager::handleCallUpdated(const QVariantMap &call)
{
    const qlonglong callId = call.value("id").toLongLong();
    if (callId <= 0) {
        WARN("Ignoring call update with invalid ID");
        return;
    }

    currentCallId = callId;
    currentUserId = call.value("user_id").toLongLong();
    currentIsOutgoing = call.value("is_outgoing").toBool();
    currentIsVideo = call.value("is_video").toBool();

    const QVariantMap callState = call.value("state").toMap();
    const QString callStateType = callState.value("@type").toString();
    LOG("Call update received" << callId << callStateType << "outgoing:" << currentIsOutgoing);

    if (callStateType == "callStatePending" && !currentIsOutgoing) {
        // Incoming call: if the voicecall-manager plugin is present, the
        // SYSTEM handles it from here - its own ringing, its own UI (which
        // shows above the PIN too), its own call state towards MCE. Touching
        // MCE ourselves as well would make us a second client fighting over
        // the same global state.
        if (systemCallBridge && systemCallBridge->isAvailable()) {
            systemCallBridge->startCall(callerDisplayName(), true);
        } else if (mceInterface) {
            // Fallback without the plugin: wake the screen (even if off) and
            // lift the touch lock by hand, so the answer UI is visible and
            // usable right away, without unlocking first. Order matters:
            // declare the call state first (it's what makes MCE turn the
            // screen on via its "call" UI exception), then ask for the
            // screen explicitly, and only then lift the touch lock - MCE
            // refuses that while the display is still off, and screen
            // wake-ups are asynchronous, so it needs the "wait for it" variant.
            mceInterface->callStateChange(QStringLiteral("ringing"));
            mceInterface->displayOn();
            mceInterface->tklockUnlockWhenDisplayOn();
            mceInterface->ledPatternActivate(QStringLiteral("PatternIncomingCall"));
        }
        startKeepDisplayOn();
    }

    if (callStateType == "callStateReady") {
        // Connected. With the plugin, the system sets the call state; without
        // it, we do ("active" is what brings proximity.so into play - the
        // screen off when the phone is held to the ear).
        if (systemCallBridge && systemCallBridge->isAvailable()) {
            systemCallBridge->setCallActive();
        } else if (mceInterface) {
            mceInterface->callStateChange(QStringLiteral("active"));
        }
        if (mceInterface) {
            mceInterface->ledPatternDeactivate(QStringLiteral("PatternIncomingCall"));
        }
        ensureInstanceForReadyCall(callState);
    } else if (callStateType == "callStateDiscarded" || callStateType == "callStateError") {
        if (mceInterface) {
            mceInterface->ledPatternDeactivate(QStringLiteral("PatternIncomingCall"));
        }
        stopInstance();
        pendingSignalingData.clear();
    }
}

QString CallManager::callerDisplayName() const
{
    if (!tdLibWrapper || currentUserId == 0) {
        return QStringLiteral("GTgram");
    }
    const QVariantMap user = tdLibWrapper->getUserInformation(QString::number(currentUserId));
    const QString firstName = user.value("first_name").toString();
    const QString lastName = user.value("last_name").toString();
    const QString name = (firstName + QLatin1Char(' ') + lastName).trimmed();
    return name.isEmpty() ? QStringLiteral("GTgram") : name;
}

void CallManager::handleSystemAnswerRequested()
{
    LOG("Answer requested from the system UI, call" << currentCallId);
    if (tdLibWrapper && currentCallId > 0) {
        tdLibWrapper->acceptVoiceCall(currentCallId, currentIsVideo);
    }
}

void CallManager::handleSystemHangupRequested()
{
    LOG("Hangup requested from the system UI, call" << currentCallId);
    if (tdLibWrapper && currentCallId > 0) {
        tdLibWrapper->discardVoiceCall(currentCallId, false, 0, currentIsVideo, 0);
    }
}

void CallManager::handleCallSignalingDataReceived(qlonglong callId, const QByteArray &data)
{
    if (callId <= 0 || data.isEmpty()) {
        return;
    }

    if (currentCallId != 0 && callId != currentCallId) {
        LOG("Ignoring signaling data for non-active call" << callId << "active:" << currentCallId);
        return;
    }
    if (currentCallId == 0) {
        currentCallId = callId;
    }

    if (instance) {
        instance->receiveSignalingData(toByteVector(data));
    } else {
        pendingSignalingData.append(data);
    }
}

void CallManager::setMicrophoneMuted(bool muted)
{
    if (instance) {
        instance->setMuteMicrophone(muted);
    }
}

void CallManager::switchCamera()
{
    if (!videoCapture) {
        return;
    }
    m_frontCamera = !m_frontCamera;
    // "" -> front, "back" -> back (see SailfishInterface::makeVideoCapturer).
    videoCapture->switchToDevice(m_frontCamera ? std::string() : std::string("back"), false);
    emit frontCameraChanged();
    // Switching cameras recreates the capturer - re-route/unmute audio just in case.
    routeWebrtcToCallSink();
    m_audioUnmuteTimer->start();
    LOG("Video call: switched camera, front =" << m_frontCamera);
}

void CallManager::setVideoEnabled(bool enabled)
{
    if (!videoCapture || !instance) {
        return;
    }
    if (enabled) {
        videoCapture->setState(tgcalls::VideoState::Active);
        // Re-attach the capture to the instance so the peer receives video again.
        instance->setVideoCapture(videoCapture);
    } else {
        // Detach the capture from the instance so tgcalls SIGNALS to the peer
        // that video is off (they show their own placeholder) instead of
        // leaving them on our last frozen frame. Also stops the camera.
        instance->setVideoCapture(nullptr);
        videoCapture->setState(tgcalls::VideoState::Inactive);
    }
    // Toggling video renegotiates the media and can drop/re-mute the incoming
    // audio stream - re-route and unmute it (retries until it reappears).
    routeWebrtcToCallSink();
    m_audioUnmuteTimer->start();
    LOG("Video call: video enabled =" << enabled);
}

void CallManager::setRemoteVideoActive(bool active)
{
    if (m_remoteVideoActive != active) {
        m_remoteVideoActive = active;
        emit remoteVideoActiveChanged();
        LOG("Video call: remote video active =" << active);
    }
    if (!active) {
        // No more remote video - clear the last frame (the overlay shows the
        // caller's avatar/a placeholder underneath instead).
        remoteVideoRenderer->reset();
    }
}

// ---- In-process libpulse ---------------------------------------------------
// The app runs sandboxed: an external `pactl` process can't reach the
// PulseAudio server, but a connection made from inside the app's own process
// can (it already plays the call's audio). The real API (header + link
// against libpulse) is used, rather than shelling out, so sinks/ports can be
// enumerated instead of hardcoding device-specific names.
namespace {
// Enumeration result: the first sink that has both a "speaker" port and an
// "earpiece"/"handset"/"receiver" port (skipping sink.null this way).
// Matched by keyword in the port name or description, so it isn't tied to
// any particular device's naming.
struct SinkScan {
    pa_threaded_mainloop *ml = nullptr;
    QString sink;
    QString speaker;
    QString earpiece;
};

void ctxStateCb(pa_context * /*c*/, void *userdata)
{
    pa_threaded_mainloop_signal(static_cast<pa_threaded_mainloop *>(userdata), 0);
}

void sinkInfoCb(pa_context * /*c*/, const pa_sink_info *info, int eol, void *userdata)
{
    SinkScan *scan = static_cast<SinkScan *>(userdata);
    if (eol) {
        pa_threaded_mainloop_signal(scan->ml, 0);
        return;
    }
    if (!info || !scan->sink.isEmpty()) {
        return;
    }
    QString speaker, earpiece;
    for (uint32_t p = 0; p < info->n_ports; ++p) {
        const pa_sink_port_info *port = info->ports[p];
        if (!port || !port->name) {
            continue;
        }
        const QString name = QString::fromUtf8(port->name).toLower();
        const QString desc = QString::fromUtf8(port->description ? port->description : "").toLower();
        if (speaker.isEmpty() && (name.contains("speaker") || desc.contains("speaker"))) {
            speaker = QString::fromUtf8(port->name);
        }
        if (earpiece.isEmpty()
                && (name.contains("earpiece") || name.contains("handset") || name.contains("receiver")
                    || desc.contains("earpiece") || desc.contains("handset") || desc.contains("receiver"))) {
            earpiece = QString::fromUtf8(port->name);
        }
    }
    if (!speaker.isEmpty() && !earpiece.isEmpty()) {
        scan->sink = QString::fromUtf8(info->name);
        scan->speaker = speaker;
        scan->earpiece = earpiece;
    }
}

// Looks for WebRTC's own playout stream (the incoming call audio).
struct SinkInputScan {
    pa_threaded_mainloop *ml = nullptr;
    uint32_t index = PA_INVALID_INDEX;
    bool found = false;
    uint8_t channels = 2;
};

void sinkInputInfoCb(pa_context * /*c*/, const pa_sink_input_info *info, int eol, void *userdata)
{
    SinkInputScan *scan = static_cast<SinkInputScan *>(userdata);
    if (eol) {
        pa_threaded_mainloop_signal(scan->ml, 0);
        return;
    }
    if (!info || !info->proplist) {
        return;
    }
    const char *app = pa_proplist_gets(info->proplist, "application.name");
    if (app && (std::strstr(app, "WEBRTC") || std::strstr(app, "VoiceEngine"))) {
        scan->index = info->index;
        scan->channels = info->volume.channels > 0 ? info->volume.channels : 2;
        scan->found = true;
    }
}

// Reads a sink's current volume by name, to save it before the WebRTC-stream
// volume forcing (below) and restore it once the call ends.
struct SinkVolumeScan {
    pa_threaded_mainloop *ml = nullptr;
    pa_cvolume volume;
    bool found = false;
};

void sinkVolumeInfoCb(pa_context * /*c*/, const pa_sink_info *info, int eol, void *userdata)
{
    SinkVolumeScan *scan = static_cast<SinkVolumeScan *>(userdata);
    if (eol) {
        pa_threaded_mainloop_signal(scan->ml, 0);
        return;
    }
    if (!info) {
        return;
    }
    scan->volume = info->volume;
    scan->found = true;
}
} // namespace

void CallManager::ensurePulseConnection()
{
    if (m_pulseContext) {
        return;
    }
    pa_threaded_mainloop *ml = pa_threaded_mainloop_new();
    if (!ml) {
        WARN("Voice call: PulseAudio mainloop creation failed");
        return;
    }
    pa_threaded_mainloop_start(ml);
    pa_threaded_mainloop_lock(ml);
    pa_context *ctx = pa_context_new(pa_threaded_mainloop_get_api(ml), "harbour-gtgram");
    pa_context_set_state_callback(ctx, &ctxStateCb, ml);
    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    for (;;) {
        const pa_context_state_t st = pa_context_get_state(ctx);
        if (st == PA_CONTEXT_READY) {
            break;
        }
        if (st == PA_CONTEXT_FAILED || st == PA_CONTEXT_TERMINATED) {
            pa_threaded_mainloop_unlock(ml);
            pa_context_unref(ctx);
            pa_threaded_mainloop_stop(ml);
            pa_threaded_mainloop_free(ml);
            WARN("Voice call: PulseAudio context not ready, state" << st);
            return;
        }
        pa_threaded_mainloop_wait(ml);
    }
    SinkScan scan;
    scan.ml = ml;
    pa_operation *op = pa_context_get_sink_info_list(ctx, &sinkInfoCb, &scan);
    if (op) {
        while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(ml);
        }
        pa_operation_unref(op);
    }
    pa_threaded_mainloop_unlock(ml);

    m_pulseMainloop = ml;
    m_pulseContext = ctx;
    m_audioSink = scan.sink;
    m_speakerPort = scan.speaker;
    m_earpiecePort = scan.earpiece;
    if (m_audioSink.isEmpty()) {
        // Fall back to the droid (Halium) port names if enumeration found nothing.
        m_audioSink = QStringLiteral("sink.primary_output");
        m_speakerPort = QStringLiteral("output-speaker");
        m_earpiecePort = QStringLiteral("output-earpiece");
        WARN("Voice call: audio port enumeration empty, using droid fallback names");
    } else {
        LOG("Voice call audio routing: sink" << m_audioSink
            << "speaker" << m_speakerPort << "earpiece" << m_earpiecePort);
    }
}

void CallManager::setSpeakerphoneOn(bool on)
{
    m_speakerOn = on;
    ensurePulseConnection();
    pa_context *ctx = static_cast<pa_context *>(m_pulseContext);
    pa_threaded_mainloop *ml = static_cast<pa_threaded_mainloop *>(m_pulseMainloop);
    if (!ctx || !ml || m_audioSink.isEmpty() || pa_context_get_state(ctx) != PA_CONTEXT_READY) {
        WARN("Voice call: PulseAudio not ready, cannot route speakerphone");
        return;
    }
    const QString port = on ? m_speakerPort : m_earpiecePort;
    pa_threaded_mainloop_lock(ml);
    pa_operation *op = pa_context_set_sink_port_by_name(ctx, m_audioSink.toUtf8().constData(),
                                                        port.toUtf8().constData(), nullptr, nullptr);
    if (op) {
        pa_operation_unref(op);
    }
    pa_threaded_mainloop_unlock(ml);
    LOG("Voice call speakerphone" << on << "port" << port);
    // On a video call, the WebRTC stream needs to stay on the call sink and
    // unmuted - the speaker toggle otherwise leaves it muted on deep_buffer.
    if (currentIsVideo) {
        routeWebrtcToCallSink();
    }
}

bool CallManager::routeWebrtcToCallSink()
{
    ensurePulseConnection();
    pa_context *ctx = static_cast<pa_context *>(m_pulseContext);
    pa_threaded_mainloop *ml = static_cast<pa_threaded_mainloop *>(m_pulseMainloop);
    if (!ctx || !ml || pa_context_get_state(ctx) != PA_CONTEXT_READY) {
        return false;
    }
    SinkInputScan scan;
    scan.ml = ml;
    pa_threaded_mainloop_lock(ml);
    pa_operation *op = pa_context_get_sink_input_info_list(ctx, &sinkInputInfoCb, &scan);
    if (op) {
        while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(ml);
        }
        pa_operation_unref(op);
    }
    if (scan.found) {
        // Move the WebRTC stream onto the call sink so that sink's
        // speaker/earpiece ports actually route it (as they do for a regular
        // voice call).
        if (!m_audioSink.isEmpty()) {
            pa_operation *om = pa_context_move_sink_input_by_name(
                ctx, scan.index, m_audioSink.toUtf8().constData(), nullptr, nullptr);
            if (om) {
                pa_operation_unref(om);
            }
        }
        // And unmute it (it's born muted on Halium).
        pa_operation *o2 = pa_context_set_sink_input_mute(ctx, scan.index, 0, nullptr, nullptr);
        if (o2) {
            pa_operation_unref(o2);
        }
        // SFOS's flat-volumes couples the WebRTC stream's volume to the
        // system sink's volume, so forcing the stream up drags the sink up
        // with it and it stays there after the call. Save the sink's
        // "clean" volume once, before forcing - the stream starts at 0% and
        // flat-volumes doesn't lower the sink for that, so the value here is
        // still whatever the user had chosen. Restored in stopInstance().
        if (!m_sinkVolumeSaved && !m_audioSink.isEmpty()) {
            SinkVolumeScan vscan;
            vscan.ml = ml;
            pa_operation *ov = pa_context_get_sink_info_by_name(
                ctx, m_audioSink.toUtf8().constData(), &sinkVolumeInfoCb, &vscan);
            if (ov) {
                while (pa_operation_get_state(ov) == PA_OPERATION_RUNNING) {
                    pa_threaded_mainloop_wait(ml);
                }
                pa_operation_unref(ov);
            }
            if (vscan.found) {
                m_savedSinkVolume = vscan.volume;
                m_sinkVolumeSaved = true;
                LOG("Call: saved system sink volume before WEBRTC boost, avg"
                    << pa_cvolume_avg(&m_savedSinkVolume));
            }
        }
        // Bring it up to 90% (not 100%) once - the stream starts at 0% (-inf
        // dB) and needs to reach an audible level. Not re-forced afterwards
        // (the timer stops as soon as the stream is found), so the user's
        // own volume adjustments during the call aren't overridden.
        pa_cvolume cv;
        pa_cvolume_set(&cv, scan.channels, (PA_VOLUME_NORM * 9) / 10);
        pa_operation *o3 = pa_context_set_sink_input_volume(ctx, scan.index, &cv, nullptr, nullptr);
        if (o3) {
            pa_operation_unref(o3);
        }
    }
    pa_threaded_mainloop_unlock(ml);
    if (scan.found) {
        LOG("Call: routed WebRTC playout to" << m_audioSink << "and unmuted, idx" << scan.index);
    }
    return scan.found;
}

void CallManager::startKeepDisplayOn()
{
    if (mceInterface) {
        mceInterface->displayBlankingPause();
    }
    m_displayOnTimer->start();
}

void CallManager::stopKeepDisplayOn()
{
    m_displayOnTimer->stop();
    if (mceInterface) {
        mceInterface->displayCancelBlankingPause();
    }
}

void CallManager::stopInstance()
{
    m_audioUnmuteTimer->stop();
    stopKeepDisplayOn();
    // Single funnel for end-of-call (both callStateDiscarded/Error and the
    // destructor land here): tell the system the call is over, otherwise it
    // would stay convinced one is still in progress.
    if (systemCallBridge) {
        systemCallBridge->endCall();
    }
    if (!(systemCallBridge && systemCallBridge->isAvailable()) && mceInterface) {
        mceInterface->callStateChange(QStringLiteral("none"));
    }
    // Restore the system sink volume saved before the WEBRTC boost - without
    // this, the system volume stays elevated after the call ends.
    if (m_sinkVolumeSaved) {
        pa_context *ctx = static_cast<pa_context *>(m_pulseContext);
        pa_threaded_mainloop *ml = static_cast<pa_threaded_mainloop *>(m_pulseMainloop);
        if (ctx && ml && !m_audioSink.isEmpty() && pa_context_get_state(ctx) == PA_CONTEXT_READY) {
            pa_threaded_mainloop_lock(ml);
            pa_operation *o = pa_context_set_sink_volume_by_name(
                ctx, m_audioSink.toUtf8().constData(), &m_savedSinkVolume, nullptr, nullptr);
            if (o) {
                pa_operation_unref(o);
            }
            pa_threaded_mainloop_unlock(ml);
            LOG("Call ended: restored system sink volume on" << m_audioSink);
        }
        m_sinkVolumeSaved = false;
    }
    if (videoCapture) {
        videoCapture->setState(tgcalls::VideoState::Inactive);
        videoCapture.reset();
    }
    remoteVideoRenderer->reset();
    localVideoRenderer->reset();

    if (!instance) {
        return;
    }
    instance->stop([](tgcalls::FinalState) {
    });
    instance.reset();
}

void CallManager::ensureInstanceForReadyCall(const QVariantMap &callState)
{
    if (instance) {
        return;
    }
    if (currentCallId <= 0) {
        WARN("Cannot create call runtime without a valid call ID");
        return;
    }

    const QVariantMap protocol = callState.value("protocol").toMap();
    const QVariantList remoteVersions = protocol.value("library_versions").toList();
    std::vector<std::string> localVersions = tgcalls::Meta::Versions();
    std::reverse(localVersions.begin(), localVersions.end());

    QStringList localVersionList;
    for (std::vector<std::string>::const_iterator it = localVersions.cbegin(); it != localVersions.cend(); ++it) {
        localVersionList.append(QString::fromStdString(*it));
    }

    QString selectedVersion;
    for (QList<QVariant>::const_iterator it = remoteVersions.cbegin(); it != remoteVersions.cend(); ++it) {
        const QString remoteVersion = it->toString();
        if (!remoteVersion.isEmpty() && localVersionList.contains(remoteVersion)) {
            selectedVersion = remoteVersion;
            break;
        }
    }
    if (selectedVersion.isEmpty() && !localVersionList.isEmpty()) {
        selectedVersion = localVersionList.first();
    }
    if (selectedVersion.isEmpty()) {
        WARN("Unable to negotiate a call runtime version");
        return;
    }

    QByteArray encryptionKeyData = decodeTdlibBytes(callState.value("encryption_key").toString());
    if (encryptionKeyData.isEmpty()) {
        WARN("Missing encryption key for ready call state");
        return;
    }

    std::shared_ptr<std::array<uint8_t, tgcalls::EncryptionKey::kSize>> encryptionKey =
            std::make_shared<std::array<uint8_t, tgcalls::EncryptionKey::kSize>>();
    encryptionKey->fill(0);
    const int encryptionBytesCount = std::min(encryptionKeyData.size(), tgcalls::EncryptionKey::kSize);
    std::memcpy(encryptionKey->data(), encryptionKeyData.constData(), static_cast<size_t>(encryptionBytesCount));

    tgcalls::Descriptor descriptor{
        selectedVersion.toStdString(),
        tgcalls::Config(),
        tgcalls::PersistentState(),
        std::vector<tgcalls::Endpoint>(),
        std::unique_ptr<tgcalls::Proxy>(),
        std::vector<tgcalls::RtcServer>(),
        tgcalls::NetworkType::WiFi,
        tgcalls::EncryptionKey(encryptionKey, currentIsOutgoing)
    };

    descriptor.config.initializationTimeout = 30.0;
    descriptor.config.receiveTimeout = 20.0;
    descriptor.config.enableP2P = callState.contains("allow_p2p") ? callState.value("allow_p2p").toBool() : true;
    descriptor.config.allowTCP = true;
    descriptor.config.enableStunMarking = true;
    descriptor.config.enableAEC = true;
    descriptor.config.enableNS = true;
    descriptor.config.enableAGC = true;
    descriptor.config.maxApiLayer = protocol.value("max_layer").toInt();
    if (descriptor.config.maxApiLayer <= 0) {
        descriptor.config.maxApiLayer = tgcalls::Meta::MaxLayer();
    }
    descriptor.config.customParameters = callState.value("custom_parameters").toString().toStdString();

    if (currentIsVideo) {
        m_frontCamera = true; // always start on the front camera
        emit frontCameraChanged();
        m_remoteVideoActive = false; // placeholder until the remote sends video
        emit remoteVideoActiveChanged();
        videoCapture = tgcalls::VideoCaptureInterface::Create(
            tgcalls::StaticThreads::getThreads(), std::string(), false, nullptr);
        descriptor.videoCapture = videoCapture;
        LOG("Video call: created video capture for call" << currentCallId);
    }

    // Tells us when the remote mutes/unmutes their camera, so the overlay can
    // switch between showing their video and a placeholder.
    descriptor.remoteMediaStateUpdated = [this](tgcalls::AudioState, tgcalls::VideoState videoState) {
        const bool active = (videoState == tgcalls::VideoState::Active);
        QMetaObject::invokeMethod(this, "setRemoteVideoActive", Qt::QueuedConnection,
                                  Q_ARG(bool, active));
    };

    descriptor.stateUpdated = [this](tgcalls::State state) {
        // 0=WaitInit 1=WaitInitAck 2=Established 3=Failed 4=Reconnecting
        LOG("tgcalls state for call" << currentCallId << "is" << static_cast<int>(state));
    };
    descriptor.signalingDataEmitted = [this](const std::vector<uint8_t> &data) {
        if (!tdLibWrapper || currentCallId <= 0 || data.empty()) {
            return;
        }
        QByteArray signalingData(reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size()));
        QMetaObject::invokeMethod(tdLibWrapper, "sendCallSignalingData", Qt::QueuedConnection,
                                  Q_ARG(qlonglong, currentCallId),
                                  Q_ARG(QByteArray, signalingData));
    };

    // TDLib's callStateReady carries the relay/WebRTC server list under
    // "servers", not "connections" - the relay list stays empty and calls
    // fail to connect across NAT if the wrong key is read here.
    const QVariantList servers = callState.value("servers").toList();
    for (QList<QVariant>::const_iterator it = servers.cbegin(); it != servers.cend(); ++it) {
        const QVariantMap server = it->toMap();
        const QVariantMap serverType = server.value("type").toMap();
        const QString serverTypeName = serverType.value("@type").toString();

        if (serverTypeName == "callServerTypeTelegramReflector") {
            tgcalls::Endpoint endpoint;
            endpoint.endpointId = server.value("id").toLongLong();
            endpoint.host = tgcalls::EndpointHost{
                server.value("ip_address").toString().toStdString(),
                server.value("ipv6_address").toString().toStdString()
            };
            endpoint.port = static_cast<uint16_t>(server.value("port").toUInt());
            endpoint.type = serverType.value("is_tcp").toBool()
                    ? tgcalls::EndpointType::TcpRelay
                    : tgcalls::EndpointType::UdpRelay;

            const QByteArray peerTag = decodeTdlibBytes(serverType.value("peer_tag").toString());
            if (peerTag.size() >= 16) {
                std::memcpy(endpoint.peerTag, peerTag.constData(), 16);
            }
            descriptor.endpoints.push_back(endpoint);

            // The V2 instances ignore descriptor.endpoints and only look at
            // rtcServers, so also expose the reflector as a relay server.
            // The "reflector" login makes ReflectorRelayPortFactory build a
            // ReflectorPort; the password carries the hex-encoded peer tag
            // (parsed back as hex by ReflectorPort). TCP reflectors are
            // skipped by the V2 ICE config.
            if (!serverType.value("is_tcp").toBool() && peerTag.size() >= 16) {
                const QString reflectorHost = !server.value("ip_address").toString().isEmpty()
                        ? server.value("ip_address").toString()
                        : server.value("ipv6_address").toString();
                tgcalls::RtcServer reflectorServer;
                reflectorServer.id = static_cast<uint8_t>((descriptor.rtcServers.size() % 250) + 1);
                reflectorServer.host = reflectorHost.toStdString();
                reflectorServer.port = static_cast<uint16_t>(server.value("port").toUInt());
                reflectorServer.login = "reflector";
                reflectorServer.password = QString::fromLatin1(peerTag.toHex()).toStdString();
                reflectorServer.isTurn = true;
                reflectorServer.isTcp = false;
                descriptor.rtcServers.push_back(reflectorServer);
            }
        } else if (serverTypeName == "callServerTypeWebrtc") {
            const QString host = !server.value("ip_address").toString().isEmpty()
                    ? server.value("ip_address").toString()
                    : server.value("ipv6_address").toString();
            const int serverId = server.value("id").toInt();
            const uint16_t port = static_cast<uint16_t>(server.value("port").toUInt());
            const bool supportsStun = serverType.value("supports_stun").toBool();
            const bool supportsTurn = serverType.value("supports_turn").toBool();

            if (supportsStun) {
                tgcalls::RtcServer rtcServer;
                rtcServer.id = static_cast<uint8_t>(serverId < 0 ? 0 : (serverId > 255 ? 255 : serverId));
                rtcServer.host = host.toStdString();
                rtcServer.port = port;
                rtcServer.isTurn = false;
                descriptor.rtcServers.push_back(rtcServer);
            }
            if (supportsTurn) {
                tgcalls::RtcServer rtcServer;
                rtcServer.id = static_cast<uint8_t>(serverId < 0 ? 0 : (serverId > 255 ? 255 : serverId));
                rtcServer.host = host.toStdString();
                rtcServer.port = port;
                rtcServer.login = serverType.value("username").toString().toStdString();
                rtcServer.password = serverType.value("password").toString().toStdString();
                rtcServer.isTurn = true;
                rtcServer.isTcp = serverType.value("is_tcp").toBool();
                descriptor.rtcServers.push_back(rtcServer);
            }
        }
    }

    LOG("Creating tgcalls instance for call" << currentCallId
        << "version" << selectedVersion
        << "endpoints" << static_cast<int>(descriptor.endpoints.size())
        << "rtcServers" << static_cast<int>(descriptor.rtcServers.size()));

    instance = tgcalls::Meta::Create(selectedVersion.toStdString(), std::move(descriptor));
    if (!instance) {
        WARN("Failed to create tgcalls instance for call" << currentCallId << "version" << selectedVersion);
        return;
    }

    // Activate the video capture (starts the camera, enables sending video)
    // and attach the render sinks: remote (the other party) + local (preview).
    if (videoCapture) {
        videoCapture->setState(tgcalls::VideoState::Active);
        videoCapture->setOutput(localVideoRenderer->sink());
    }
    if (currentIsVideo) {
        instance->setIncomingVideoOutput(remoteVideoRenderer->sink());
    }
    // Route the incoming audio onto the call sink + unmute (starts muted on
    // Halium) + bring the volume up (starts at 0% on SFOS 5.1). Applies to
    // voice AND video calls.
    routeWebrtcToCallSink();     // immediate attempt
    m_audioUnmuteTimer->start(); // + keep retrying until the stream shows up
    if (currentIsVideo) {
        startKeepDisplayOn(); // keep the screen on for the whole call, not just ringing
    }

    while (!pendingSignalingData.isEmpty()) {
        instance->receiveSignalingData(toByteVector(pendingSignalingData.takeFirst()));
    }
}

std::vector<uint8_t> CallManager::toByteVector(const QByteArray &data) const
{
    std::vector<uint8_t> output;
    output.reserve(static_cast<size_t>(data.size()));
    for (int i = 0; i < data.size(); i++) {
        output.push_back(static_cast<uint8_t>(data.at(i)));
    }
    return output;
}

QByteArray CallManager::decodeTdlibBytes(const QString &data) const
{
    if (data.isEmpty()) {
        return QByteArray();
    }
    QByteArray decoded = QByteArray::fromBase64(data.toUtf8());
    if (decoded.isEmpty()) {
        return data.toUtf8();
    }
    return decoded;
}
