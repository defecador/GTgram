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
#ifndef CALLMANAGER_H
#define CALLMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QByteArray>
#include <QList>
#include <memory>
#include <vector>
#include <pulse/volume.h>   // pa_cvolume (saved system sink volume, for restore on hang-up)

namespace tgcalls {
class Instance;
class VideoCaptureInterface;
}

class TDLibWrapper;
class MceInterface;
class SystemCallBridge;
class QTimer;
class CallVideoRenderer;

// Owns the tgcalls runtime for one Telegram call (voice or video) at a time:
// builds the tgcalls::Instance once TDLib reports a call as ready,
// feeds/receives its signaling data through TDLibWrapper, and works around a
// couple of Sailfish/Halium PulseAudio quirks (WebRTC's own playout stream
// starts muted, on the wrong sink, and dragging up the system volume) that
// would otherwise make a connected call inaudible. For a video call, also
// owns the camera capturer and the two video renderers (src/videocalls/) -
// system integration (SystemCallBridge, below) stays voice-only either way.
//
// Ringing/connecting the call to the rest of the system goes through
// SystemCallBridge (the native lock-screen answer UI, proximity sensor)
// when its voicecall-manager plugin is available, falling back to plain
// MceInterface calls (screen-on only, no system UI) otherwise.
class CallManager : public QObject
{
    Q_OBJECT
    // Whether the system's own voicecall-manager UI is handling this call -
    // if so, the in-app call screen should mirror it rather than play its
    // own ringtone/vibration on top of the system's.
    Q_PROPERTY(bool systemCallUiActive READ systemCallUiActive CONSTANT)
    // The remote/local video renderers, exposed as plain QObject* so QML's
    // VideoOutput can bind its source directly (e.g. VideoOutput { source:
    // callManager.remoteVideo }) - always valid, whether or not a call is
    // currently video.
    Q_PROPERTY(QObject* remoteVideo READ remoteVideo CONSTANT)
    Q_PROPERTY(QObject* localVideo READ localVideo CONSTANT)
    Q_PROPERTY(bool frontCamera READ frontCamera NOTIFY frontCameraChanged)
    Q_PROPERTY(bool remoteVideoActive READ remoteVideoActive NOTIFY remoteVideoActiveChanged)

public:
    explicit CallManager(TDLibWrapper *tdLibWrapper, MceInterface *mceInterface, QObject *parent = nullptr);
    ~CallManager() override;

    bool systemCallUiActive() const;
    QObject *remoteVideo() const;
    QObject *localVideo() const;
    bool frontCamera() const;
    bool remoteVideoActive() const;

    // No-op if no call is in progress.
    Q_INVOKABLE void setMicrophoneMuted(bool muted);
    Q_INVOKABLE void setSpeakerphoneOn(bool on);
    // No-op outside a video call, or if the camera failed to open.
    Q_INVOKABLE void switchCamera();
    Q_INVOKABLE void setVideoEnabled(bool enabled);

signals:
    void frontCameraChanged();
    void remoteVideoActiveChanged();

private slots:
    void handleCallUpdated(const QVariantMap &call);
    void handleCallSignalingDataReceived(qlonglong callId, const QByteArray &data);
    void handleSystemAnswerRequested();
    void handleSystemHangupRequested();
    // Marshaled off tgcalls' descriptor.remoteMediaStateUpdated callback
    // (webrtc's own thread) onto the Qt/GUI thread via a queued invoke.
    void setRemoteVideoActive(bool active);

private:
    void stopInstance();
    void ensureInstanceForReadyCall(const QVariantMap &callState);
    void startKeepDisplayOn();
    void stopKeepDisplayOn();
    QString callerDisplayName() const;
    std::vector<uint8_t> toByteVector(const QByteArray &data) const;
    QByteArray decodeTdlibBytes(const QString &data) const;

    // In-process PulseAudio connection (the app is sandboxed - an external
    // `pactl` process can't reach the PulseAudio server, but a connection made
    // from inside the app's own process can, reusing the access the app
    // already has for playing call audio).
    void ensurePulseConnection();
    // Moves tgcalls' own PulseAudio playout stream onto the call sink and
    // unmutes it (Halium starts it muted, on the media sink). Returns true
    // once found and routed; called from a retry timer until it succeeds.
    bool routeWebrtcToCallSink();

    TDLibWrapper *tdLibWrapper;
    MceInterface *mceInterface;
    SystemCallBridge *systemCallBridge;
    QTimer *m_audioUnmuteTimer;
    QTimer *m_displayOnTimer;
    std::unique_ptr<tgcalls::Instance> instance;
    qlonglong currentCallId;
    qlonglong currentUserId;
    bool currentIsOutgoing;
    bool currentIsVideo;
    QList<QByteArray> pendingSignalingData;

    std::shared_ptr<tgcalls::VideoCaptureInterface> videoCapture;
    CallVideoRenderer *remoteVideoRenderer;
    CallVideoRenderer *localVideoRenderer;
    bool m_frontCamera;
    bool m_remoteVideoActive;

    // PulseAudio state (pa_threaded_mainloop*/pa_context*; kept as void* here
    // so this header doesn't need <pulse/pulseaudio.h>).
    void *m_pulseMainloop;
    void *m_pulseContext;
    QString m_audioSink;
    QString m_speakerPort;
    QString m_earpiecePort;
    bool m_speakerOn;
    // SFOS's flat-volumes couples the WebRTC stream's volume to its sink's
    // volume: forcing the stream up drags the system sink up with it, and it
    // stays up after the call ends unless restored from what we saved here
    // before forcing it.
    pa_cvolume m_savedSinkVolume;
    bool m_sinkVolumeSaved = false;
};

#endif // CALLMANAGER_H
