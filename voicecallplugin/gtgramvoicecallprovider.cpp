/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#include "gtgramvoicecallprovider.h"
#include "gtgramvoicecallhandler.h"
#include "gtgramcallsadaptor.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDebug>

namespace {
const QString APP_SERVICE("de.ygriega.gtgram");
// The object is exposed on the service the host process (voicecall-manager)
// already owns, org.nemomobile.voicecall - the only one reachable from the
// app, which is sandboxed by sailjail. See gtgramcallsadaptor.h.
const QString PLUGIN_PATH("/gtgram/calls");
// If the app gives no sign of life within this time, the call closes itself:
// better a missed call than a phone with broken audio.
const int WATCHDOG_MS = 120000;
}

GTgramVoiceCallProvider::GTgramVoiceCallProvider(VoiceCallManagerInterface *manager, QObject *parent)
    : AbstractVoiceCallProvider(parent)
    , m_manager(manager)
    , m_voiceCall(nullptr)
    , m_appWatcher(nullptr)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    new GTgramCallsAdaptor(this);
    if (!bus.registerObject(PLUGIN_PATH, this)) {
        qWarning() << "[GTGRAM-VOICECALL] failed to register object" << PLUGIN_PATH;
    }

    // Audio commands from the system UI arrive here as requests to the
    // provider; relay them to the app, which is what actually routes audio.
    connect(m_manager, &VoiceCallManagerInterface::setAudioModeRequested,
            this, [this](const QString &mode) {
                qWarning() << "[GTGRAM-VOICECALL] audio mode requested by system UI:" << mode;
                emit speakerModeRequested(mode != QLatin1String("earpiece"));
            });
    connect(m_manager, &VoiceCallManagerInterface::setMuteMicrophoneRequested,
            this, [this](bool muted) {
                qWarning() << "[GTGRAM-VOICECALL] mute requested by system UI:" << muted;
                emit muteMicrophoneRequested(muted);
            });

    // Safety net 1: close the call if the app disappears from the bus.
    m_appWatcher = new QDBusServiceWatcher(APP_SERVICE, bus,
                                           QDBusServiceWatcher::WatchForUnregistration, this);
    connect(m_appWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &GTgramVoiceCallProvider::handleAppDisappeared);

    // Safety net 2: watchdog.
    m_watchdog.setSingleShot(true);
    m_watchdog.setInterval(WATCHDOG_MS);
    connect(&m_watchdog, &QTimer::timeout, this, &GTgramVoiceCallProvider::handleWatchdogTimeout);

    qWarning() << "[GTGRAM-VOICECALL] provider registered, object on org.nemomobile.voicecall" << PLUGIN_PATH;
}

GTgramVoiceCallProvider::~GTgramVoiceCallProvider()
{
    closeCall("provider destroyed");
}

QString GTgramVoiceCallProvider::providerId() const { return QStringLiteral("gtgram"); }
QString GTgramVoiceCallProvider::providerType() const { return QStringLiteral("voip"); }
QString GTgramVoiceCallProvider::errorString() const { return QString(); }

QList<AbstractVoiceCallHandler*> GTgramVoiceCallProvider::voiceCalls() const
{
    QList<AbstractVoiceCallHandler*> result;
    if (m_voiceCall) {
        result.append((AbstractVoiceCallHandler*)m_voiceCall);
    }
    return result;
}

// Calls aren't dialed from here: the app starts them.
bool GTgramVoiceCallProvider::dial(const QString &) { return false; }

void GTgramVoiceCallProvider::newCall(const QString &callerName, bool incoming)
{
    if (m_voiceCall) {
        qWarning() << "[GTGRAM-VOICECALL] newCall ignored: one is already in progress";
        return;
    }
    qWarning() << "[GTGRAM-VOICECALL] newCall from" << callerName << "incoming:" << incoming;
    m_voiceCall = new GTgramVoiceCallHandler(m_manager->generateHandlerId(), callerName, incoming, this, this);
    m_watchdog.start();
    emit voiceCallAdded((AbstractVoiceCallHandler*)m_voiceCall);
    emit voiceCallsChanged();
}

void GTgramVoiceCallProvider::callReady()
{
    if (!m_voiceCall) {
        return;
    }
    m_voiceCall->setStatus(AbstractVoiceCallHandler::STATUS_ACTIVE);
    m_watchdog.start();
}

void GTgramVoiceCallProvider::discardCall()
{
    closeCall("requested by the app");
}

void GTgramVoiceCallProvider::ping()
{
    if (m_voiceCall) {
        m_watchdog.start();
    }
}

void GTgramVoiceCallProvider::handleAppDisappeared(const QString &service)
{
    Q_UNUSED(service)
    if (m_voiceCall) {
        qWarning() << "[GTGRAM-VOICECALL] app disappeared from the bus with a call open";
        closeCall("app disappeared");
    }
}

void GTgramVoiceCallProvider::handleWatchdogTimeout()
{
    if (m_voiceCall) {
        qWarning() << "[GTGRAM-VOICECALL] watchdog expired without a heartbeat from the app";
        closeCall("watchdog");
    }
}

void GTgramVoiceCallProvider::closeCall(const char *reason)
{
    m_watchdog.stop();
    if (!m_voiceCall) {
        return;
    }
    qWarning() << "[GTGRAM-VOICECALL] closing call:" << reason;
    m_voiceCall->setStatus(AbstractVoiceCallHandler::STATUS_DISCONNECTED);
    const QString handlerId = m_voiceCall->handlerId();
    m_voiceCall->deleteLater();
    m_voiceCall = nullptr;
    emit voiceCallRemoved(handlerId);
    emit voiceCallsChanged();
}
