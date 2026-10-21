/*
    Copyright (C) 2026 RootGPT, adapted for GTgram

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
*/
#include "unifiedpushmanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <unistd.h>

#define DEBUG_MODULE UnifiedPushManager
#include "debuglog.h"

namespace {
    const QString CONNECTOR_SERVICE("org.unifiedpush.Connector.gtgram");
    const QString CONNECTOR_PATH("/org/unifiedpush/Connector");
    const QString DISTRIBUTOR_PREFIX("org.unifiedpush.Distributor.");
    const QString DISTRIBUTOR_PATH("/org/unifiedpush/Distributor");
    const QString DISTRIBUTOR_INTERFACE("org.unifiedpush.Distributor2");
    const QString EXTRA_REGISTER("unifiedPush:register");
    const QString EXTRA_REGISTER_SIMPLE("unifiedPush:registerSimple");
    const QString EXTRA_UNREGISTER("unifiedPush:unregister");
    const QString EXTRA_SYNC("unifiedPush:sync");
    const QString KEY_WEBPUSH_AUTH("webPushAuth");
    const QString KEY_TOKEN("token");
    const QString KEY_ENDPOINT("endpoint");

    // P-256 public key for Telegram's Web Push. Telegram refuses a Web Push
    // without a valid key to encrypt for. We never read the content (it is only
    // a wake-up), so the private key was generated and DESTROYED (2026-10-18, by
    // RooTelegram): nobody can read what passes through the push server.
    const QString WEBPUSH_P256DH("BCPfaIo_5uw438vh7kcwLG7kqU8yCPSnFl1DlpKj0Jabsz4K_ZR19sLORIHMfJLn_d3Cpk8KtgA672i3MsFYG0E");

    QVariantMap registerDeviceRequest(const QVariantMap &deviceToken, const QString &extra)
    {
        QVariantMap request;
        request.insert(QStringLiteral("@type"), QStringLiteral("registerDevice"));
        request.insert(QStringLiteral("device_token"), deviceToken);
        request.insert(QStringLiteral("other_user_ids"), QVariantList());
        request.insert(QStringLiteral("@extra"), extra);
        return request;
    }

    // A process without a visible window quits after IDLE_QUIET_MS without
    // activity. One woken by a push lives at least WAKE_MIN_LIFE_MS, to give TDLib
    // time to connect and download, and at most WAKE_MAX_QUIET_MS when the
    // connection never comes up.
    const qint64 IDLE_QUIET_MS = 10000;
    const qint64 WAKE_MIN_LIFE_MS = 20000;
    const qint64 WAKE_MAX_QUIET_MS = 60000;
    const qint64 CONNECTED_SETTLE_MS = 10000;

    qint64 nowMs()
    {
        return QDateTime::currentMSecsSinceEpoch();
    }
}

const char *UnifiedPushManager::WAKE_ENVIRONMENT_VARIABLE = "GTGRAM_PUSH_WAKE";

UnifiedPushConnectorAdaptor::UnifiedPushConnectorAdaptor(UnifiedPushManager *manager)
    : QDBusAbstractAdaptor(manager), manager(manager)
{
}

QVariantMap UnifiedPushConnectorAdaptor::NewEndpoint(const QVariantMap &args)
{
    manager->handleNewEndpoint(args.value(KEY_TOKEN).toString(), args.value(KEY_ENDPOINT).toString());
    return QVariantMap();
}

QVariantMap UnifiedPushConnectorAdaptor::Message(const QVariantMap &args)
{
    // The content is not needed, it is only a wake-up. Answer at once with the
    // id so the distributor considers it delivered.
    manager->handleMessage(args.value(KEY_TOKEN).toString());
    QVariantMap reply;
    reply.insert(QStringLiteral("id"), args.value(QStringLiteral("id")).toString());
    return reply;
}

QVariantMap UnifiedPushConnectorAdaptor::Unregistered(const QVariantMap &args)
{
    manager->handleUnregistered(args.value(KEY_TOKEN).toString());
    return QVariantMap();
}

UnifiedPushManager::UnifiedPushManager(bool wakeMode, QObject *parent)
    : QObject(parent)
    , adaptor(new UnifiedPushConnectorAdaptor(this))
    , wakeMode(wakeMode)
    , connectorClaimed(false)
    , uiBusy(false)
    , telegramSent(false)
    , status(QStringLiteral("off"))
    , lastActivityMs(nowMs())
    , startedMs(nowMs())
    , connectedSinceMs(0)
    , registeredThisRun(false)
{
    idleTimer.setInterval(2000);
    connect(&idleTimer, &QTimer::timeout, this, &UnifiedPushManager::checkIdle);

    // Before any slow work: the distributor allows 8 seconds per call, process
    // activation included. Also done with push turned off if the distributor
    // started us: it usually brings an "Unregistered" that must be answered, and
    // then the process quits.
    if (isEnabled() || wakeMode) {
        claimConnector();
        idleTimer.start();
    }
    if (isEnabled()) {
        setStatus(QStringLiteral("registering"));
    }
    if (wakeMode) {
        WARN("Process started by a push wake-up");
    }
}

void UnifiedPushManager::setTdLibWrapper(TDLibWrapper *tdLibWrapper)
{
    this->tdLibWrapper = tdLibWrapper;
    connect(tdLibWrapper, &TDLibWrapper::authorizationStateChanged, this, &UnifiedPushManager::handleAuthorizationStateChanged);
    connect(tdLibWrapper, &TDLibWrapper::okReceived, this, &UnifiedPushManager::handleOkReceived);
    connect(tdLibWrapper, &TDLibWrapper::errorReceived, this, &UnifiedPushManager::handleErrorReceived);
    // Notification updates are NOT counted as activity: on an account with many
    // groups they arrive constantly and the process would never quit.
    connect(tdLibWrapper, &TDLibWrapper::connectionStateChanged, this, [this](const TDLibWrapper::ConnectionState &state) {
        if (state == TDLibWrapper::ConnectionReady) {
            if (connectedSinceMs == 0) {
                connectedSinceMs = nowMs();
            }
        } else {
            connectedSinceMs = 0;
        }
    });
    // Registering with the distributor is the job of a "real" start (from the
    // launcher): a process woken in the background takes the name and waits.
    if (isEnabled() && !wakeMode) {
        registerWithDistributor();
    }
}

void UnifiedPushManager::setView(QWindow *view)
{
    this->view = view;
    if (view) {
        connect(view, &QWindow::visibleChanged, this, [this](bool visible) {
            noteActivity();
            // Woken by a push and then opened by the user: that is a "real"
            // start, so register now.
            if (visible && isEnabled() && !registeredThisRun) {
                registerWithDistributor();
            }
        });
    }
}

bool UnifiedPushManager::isEnabled() const
{
    return QFile::exists(markerPath());
}

QString UnifiedPushManager::getStatus() const
{
    return status;
}

QString UnifiedPushManager::getEndpointHost() const
{
    QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
    return QUrl(settings.value(KEY_ENDPOINT).toString()).host();
}

QString UnifiedPushManager::getErrorText() const
{
    return errorText;
}

bool UnifiedPushManager::isUiBusy() const
{
    return uiBusy;
}

void UnifiedPushManager::setUiBusy(bool busy)
{
    if (uiBusy != busy) {
        uiBusy = busy;
        noteActivity();
        emit uiBusyChanged();
    }
}

void UnifiedPushManager::setEnabled(bool enabled)
{
    if (enabled == isEnabled()) {
        return;
    }
    if (enabled) {
        QDir().mkpath(configDirectory());
        QFile marker(markerPath());
        if (marker.open(QIODevice::WriteOnly)) {
            marker.close();
        }
        WARN("Turned on");
        emit enabledChanged();
        claimConnector();
        registerWithDistributor();
        noteActivity();
        idleTimer.start();
    } else {
        WARN("Turned off");
        unregisterFromDistributor();
        if (tdLibWrapper) {
            // An empty endpoint makes Telegram forget this device, for both types.
            QVariantMap simplePush;
            simplePush.insert(QStringLiteral("@type"), QStringLiteral("deviceTokenSimplePush"));
            simplePush.insert(QStringLiteral("endpoint"), QString());
            tdLibWrapper->sendRequest(registerDeviceRequest(simplePush, EXTRA_UNREGISTER));
            QVariantMap webPush;
            webPush.insert(QStringLiteral("@type"), QStringLiteral("deviceTokenWebPush"));
            webPush.insert(QStringLiteral("endpoint"), QString());
            webPush.insert(QStringLiteral("p256dh_base64url"), QString());
            webPush.insert(QStringLiteral("auth_base64url"), QString());
            tdLibWrapper->sendRequest(registerDeviceRequest(webPush, EXTRA_UNREGISTER));
        }
        telegramSent = false;
        // The distributor confirms with Unregistered: if the name were already
        // free that would start a second GTgram process next to this one (TDLib
        // would complain "Can't lock file"), so keep the name for a while.
        QTimer::singleShot(10000, this, [this]() {
            if (!isEnabled()) {
                releaseConnector();
            }
        });
        QFile::remove(markerPath());
        QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
        settings.remove(KEY_ENDPOINT);
        if (!wakeMode) {
            idleTimer.stop();
        }
        setStatus(QStringLiteral("off"));
        emit enabledChanged();
    }
}

void UnifiedPushManager::refresh()
{
    if (isEnabled()) {
        claimConnector();
        registerWithDistributor();
    }
}

void UnifiedPushManager::handleNewEndpoint(const QString &token, const QString &endpoint)
{
    if (!isEnabled() || token != this->token()) {
        return;
    }
    WARN("New endpoint from the distributor, server:" << QUrl(endpoint).host());
    QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
    settings.setValue(KEY_ENDPOINT, endpoint);
    settings.sync();
    telegramSent = false;
    sendEndpointToTelegram();
}

void UnifiedPushManager::handleMessage(const QString &token)
{
    Q_UNUSED(token)
    WARN("Wake-up received");
    noteActivity();
    forceTelegramSync();
}

// TDLib does not poll while the app is "offline": after the phone has slept its
// connections can be dead without TDLib noticing, so a wake-up alone changes
// nothing until the app is opened. processPushNotification makes TDLib check
// all connections at once and fetch what is new. The payload is a push TDLib
// cannot use (no chat), so it answers 406 "force loading data from the server",
// which is exactly what is wanted here and is ignored.
void UnifiedPushManager::forceTelegramSync()
{
    if (!tdLibWrapper || tdLibWrapper->getAuthorizationState() != TDLibWrapper::AuthorizationReady) {
        return;
    }
    QVariantMap request;
    request.insert(QStringLiteral("@type"), QStringLiteral("processPushNotification"));
    request.insert(QStringLiteral("payload"), QStringLiteral("{\"loc_key\":\"MESSAGE_MUTED\"}"));
    request.insert(QStringLiteral("@extra"), EXTRA_SYNC);
    tdLibWrapper->sendRequest(request);
}

void UnifiedPushManager::handleUnregistered(const QString &token)
{
    if (token != this->token()) {
        return;
    }
    WARN("The distributor removed our registration");
    QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
    settings.remove(KEY_ENDPOINT);
    if (isEnabled()) {
        // Happens when the distributor's server is changed (e.g. Mozilla -> ntfy):
        // register again and the address on the new server arrives.
        setStatus(QStringLiteral("registering"));
        QTimer::singleShot(2000, this, &UnifiedPushManager::registerWithDistributor);
    }
}

void UnifiedPushManager::handleAuthorizationStateChanged(const TDLibWrapper::AuthorizationState &authorizationState, const QVariantMap &authorizationStateData)
{
    Q_UNUSED(authorizationStateData)
    if (authorizationState == TDLibWrapper::AuthorizationReady) {
        sendEndpointToTelegram();
        if (wakeMode) {
            // Started by a push: fetch what is new right away.
            forceTelegramSync();
        }
    }
}

void UnifiedPushManager::handleOkReceived(const QString &extra)
{
    if (extra == EXTRA_REGISTER_SIMPLE) {
        WARN("Telegram also accepted the Simple Push");
        return;
    }
    if (extra == EXTRA_REGISTER) {
        WARN("Telegram accepted the endpoint");
        setStatus(QStringLiteral("registered"));
    }
}

void UnifiedPushManager::handleErrorReceived(int code, const QString &message, const QString &extra)
{
    if (extra == EXTRA_REGISTER_SIMPLE) {
        WARN("Telegram refused the Simple Push (the Web Push remains):" << code << message);
        return;
    }
    if (extra == EXTRA_REGISTER) {
        WARN("Telegram refused the endpoint:" << code << message);
        telegramSent = false;
        setStatus(QStringLiteral("error"), message);
    }
}

void UnifiedPushManager::handleRegisterReply(const QDBusMessage &reply)
{
    // a{sv} arrives as a QDBusArgument: toMap() would give an empty map.
    const QVariantMap result = qdbus_cast<QVariantMap>(reply.arguments().value(0));
    const QString success = result.value(QStringLiteral("success")).toString();
    if (success == QStringLiteral("REGISTRATION_SUCCEEDED")) {
        LOG("Registered with distributor" << distributor);
        // The endpoint arrives with NewEndpoint, usually before this reply. A
        // token that is already known does not trigger it again: then the
        // saved endpoint is used.
        sendEndpointToTelegram();
    } else {
        const QString reason = result.value(QStringLiteral("reason")).toString();
        WARN("Registration refused by the distributor:" << reason);
        setStatus(QStringLiteral("error"), reason);
    }
}

void UnifiedPushManager::handleRegisterError(const QDBusError &error, const QDBusMessage &call)
{
    Q_UNUSED(call)
    WARN("Distributor unreachable:" << error.name() << error.message());
    setStatus(QStringLiteral("error"), error.message());
}

void UnifiedPushManager::checkIdle()
{
    if (!isEnabled() && !wakeMode) {
        return;
    }
    if ((view && view->isVisible()) || uiBusy) {
        lastActivityMs = nowMs();
        return;
    }
    if (!wakeMode) {
        // Started normally and then closed or minimized: Silica ends the process
        // itself when the window closes, a minimized app simply stays.
        return;
    }
    const qint64 now = nowMs();
    const qint64 quietMs = now - lastActivityMs;
    if (quietMs < IDLE_QUIET_MS) {
        return;
    }
    if (now - startedMs < WAKE_MIN_LIFE_MS) {
        return;
    }
    // Quit after staying connected long enough to download and show the
    // notifications; without a network it does not wait forever.
    const bool settled = connectedSinceMs > 0 && now - connectedSinceMs >= CONNECTED_SETTLE_MS;
    if (!settled && quietMs < WAKE_MAX_QUIET_MS) {
        return;
    }
    WARN("No window and nothing to do: quitting");
    idleTimer.stop();
    // Not QCoreApplication::quit(): tearing Qt down with a hidden QML window
    // hangs (measured by RooTelegram: process still alive after 40 s).
    if (tdLibWrapper) {
        tdLibWrapper->closeAndExit();
    } else {
        ::_exit(0);
    }
}

bool UnifiedPushManager::claimConnector()
{
    if (connectorClaimed) {
        return true;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerObject(CONNECTOR_PATH, this, QDBusConnection::ExportAdaptors)) {
        WARN("Cannot export" << CONNECTOR_PATH << bus.lastError().message());
        return false;
    }
    if (!bus.registerService(CONNECTOR_SERVICE)) {
        // Without the UnifiedPush permission (or without Foghorn) sailjail
        // answers ServiceUnknown: it is not a naming error.
        WARN("Cannot take the name" << CONNECTOR_SERVICE << bus.lastError().message());
        bus.unregisterObject(CONNECTOR_PATH);
        return false;
    }
    connectorClaimed = true;
    return true;
}

void UnifiedPushManager::releaseConnector()
{
    if (!connectorClaimed) {
        return;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterService(CONNECTOR_SERVICE);
    bus.unregisterObject(CONNECTOR_PATH);
    connectorClaimed = false;
}

QString UnifiedPushManager::findDistributor() const
{
    QDBusConnectionInterface *busInterface = QDBusConnection::sessionBus().interface();
    if (!busInterface) {
        return QString();
    }
    QStringList names = busInterface->registeredServiceNames().value();
    QDBusMessage listActivatable = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/org/freedesktop/DBus"),
                                                                 QStringLiteral("org.freedesktop.DBus"), QStringLiteral("ListActivatableNames"));
    const QDBusReply<QStringList> activatable = QDBusConnection::sessionBus().call(listActivatable, QDBus::Block, 3000);
    if (activatable.isValid()) {
        names.append(activatable.value());
    }
    for (const QString &name : names) {
        if (name.startsWith(DISTRIBUTOR_PREFIX)) {
            return name;
        }
    }
    return QString();
}

void UnifiedPushManager::registerWithDistributor()
{
    if (!connectorClaimed) {
        // Usually the UnifiedPush permission is missing (Foghorn was not
        // installed when GTgram was first opened after the update).
        setStatus(QStringLiteral("error"), QStringLiteral("D-Bus: ") + CONNECTOR_SERVICE);
        return;
    }
    registeredThisRun = true;
    distributor = findDistributor();
    if (distributor.isEmpty()) {
        WARN("No distributor installed");
        setStatus(QStringLiteral("noDistributor"));
        return;
    }
    // The token must be saved before Register: the distributor calls
    // NewEndpoint before it replies, and we compare against the saved one.
    const QString registrationToken = token();
    setStatus(QStringLiteral("registering"));
    QVariantMap args;
    args.insert(QStringLiteral("service"), CONNECTOR_SERVICE);
    args.insert(QStringLiteral("token"), registrationToken);
    args.insert(QStringLiteral("description"), QStringLiteral("GTgram"));
    QDBusMessage call = QDBusMessage::createMethodCall(distributor, DISTRIBUTOR_PATH, DISTRIBUTOR_INTERFACE, QStringLiteral("Register"));
    call << args;
    // Asynchronous: while we wait for the reply the distributor calls
    // NewEndpoint, and the event loop must be free to serve it.
    QDBusConnection::sessionBus().callWithCallback(call, this, SLOT(handleRegisterReply(QDBusMessage)),
                                                   SLOT(handleRegisterError(QDBusError,QDBusMessage)), 20000);
}

void UnifiedPushManager::unregisterFromDistributor()
{
    const QString name = distributor.isEmpty() ? findDistributor() : distributor;
    if (name.isEmpty()) {
        return;
    }
    QVariantMap args;
    args.insert(QStringLiteral("token"), token());
    QDBusMessage call = QDBusMessage::createMethodCall(name, DISTRIBUTOR_PATH, DISTRIBUTOR_INTERFACE, QStringLiteral("Unregister"));
    call << args;
    QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
}

void UnifiedPushManager::sendEndpointToTelegram()
{
    if (!isEnabled() || telegramSent || !tdLibWrapper
            || tdLibWrapper->getAuthorizationState() != TDLibWrapper::AuthorizationReady) {
        return;
    }
    QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
    const QString endpoint = settings.value(KEY_ENDPOINT).toString();
    if (endpoint.isEmpty()) {
        return;
    }
    // One request at a time: they come both from NewEndpoint and from the reply
    // to Register, and the second used to be aborted on quit (500 "Request
    // aborted"), leaving the state at "error". A refusal puts it back in play.
    telegramSent = true;
    // Web Push (like Telegram Web) is what Telegram really sends for messages.
    // The body is encrypted and the distributor hands it over without the
    // headers: it is not decrypted, it counts as a wake-up.
    QVariantMap webPush;
    webPush.insert(QStringLiteral("@type"), QStringLiteral("deviceTokenWebPush"));
    webPush.insert(QStringLiteral("endpoint"), endpoint);
    webPush.insert(QStringLiteral("p256dh_base64url"), WEBPUSH_P256DH);
    webPush.insert(QStringLiteral("auth_base64url"), webPushAuth());
    tdLibWrapper->sendRequest(registerDeviceRequest(webPush, EXTRA_REGISTER));
    // Mozilla (Foghorn's default server) refuses the plain Simple Push with
    // HTTP 406, so there only the Web Push is used. Elsewhere Simple Push stays
    // as a fallback.
    if (!QUrl(endpoint).host().endsWith(QStringLiteral("mozilla.com"))) {
        QVariantMap simplePush;
        simplePush.insert(QStringLiteral("@type"), QStringLiteral("deviceTokenSimplePush"));
        simplePush.insert(QStringLiteral("endpoint"), endpoint);
        tdLibWrapper->sendRequest(registerDeviceRequest(simplePush, EXTRA_REGISTER_SIMPLE));
    }
}

void UnifiedPushManager::setStatus(const QString &status, const QString &errorText)
{
    if (this->status != status || this->errorText != errorText) {
        this->status = status;
        this->errorText = errorText;
        emit statusChanged();
    }
}

void UnifiedPushManager::noteActivity()
{
    lastActivityMs = nowMs();
}

QString UnifiedPushManager::configDirectory() const
{
    // The same folder as settings.conf (see AppSettings): sailjail lets it be
    // written and it survives.
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/de.ygriega/gtgram");
}

QString UnifiedPushManager::markerPath() const
{
    return configDirectory() + QStringLiteral("/unifiedpush-enabled");
}

QString UnifiedPushManager::webPushAuth()
{
    // 16-byte Web Push secret, random and fixed per installation.
    QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
    QString value = settings.value(KEY_WEBPUSH_AUTH).toString();
    if (value.isEmpty()) {
        value = QString::fromLatin1(QUuid::createUuid().toRfc4122().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        settings.setValue(KEY_WEBPUSH_AUTH, value);
        settings.sync();
    }
    return value;
}

QString UnifiedPushManager::token()
{
    QSettings settings(configDirectory() + QStringLiteral("/unifiedpush.conf"), QSettings::NativeFormat);
    QString value = settings.value(KEY_TOKEN).toString();
    if (value.isEmpty()) {
        value = QUuid::createUuid().toString().remove(QLatin1Char('{')).remove(QLatin1Char('}'));
        settings.setValue(KEY_TOKEN, value);
        settings.sync();
    }
    return value;
}
