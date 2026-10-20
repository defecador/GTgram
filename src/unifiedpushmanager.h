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
#ifndef UNIFIEDPUSHMANAGER_H
#define UNIFIEDPUSHMANAGER_H

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <QWindow>

#include "tdlibwrapper.h"

class UnifiedPushManager;

// The "connector" side of the UnifiedPush D-Bus protocol (version 2): the
// distributor (Foghorn) calls these three methods on /org/unifiedpush/Connector.
// This cannot be done in QML because the method names start with a capital.
class UnifiedPushConnectorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.unifiedpush.Connector2")
public:
    explicit UnifiedPushConnectorAdaptor(UnifiedPushManager *manager);

public slots:
    QVariantMap NewEndpoint(const QVariantMap &args);
    QVariantMap Message(const QVariantMap &args);
    QVariantMap Unregistered(const QVariantMap &args);

private:
    UnifiedPushManager *manager;
};

// Notifications through UnifiedPush (Foghorn) while the app is closed (BETA).
//
// Off: nothing changes.
// On:
//  - when the app is started from the launcher (and when the option is turned
//    on) it registers with the distributor, which answers with an endpoint URL.
//    The endpoint is given to Telegram as a "Web Push" device (registerDevice /
//    deviceTokenWebPush, like Telegram Web). For every message Telegram posts an
//    encrypted push there. We never decrypt it: it is only a wake-up call;
//  - with the app closed, the distributor starts the process through D-Bus
//    activation (org.unifiedpush.Connector.gtgram.service, which sets
//    GTGRAM_PUSH_WAKE). The process runs without a window, TDLib connects and
//    downloads, the usual NotificationManager shows the notifications, and the
//    process quits by itself once it has been idle for a while.
// Measured by RooTelegram on 2026-10-18: with Mozilla (Foghorn's default) the
// wake-up arrives within a second (about 30 s if another Telegram client is
// online); with ntfy.sh Telegram delivered almost nothing.
class UnifiedPushManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(QString endpointHost READ getEndpointHost NOTIFY statusChanged)
    Q_PROPERTY(QString errorText READ getErrorText NOTIFY statusChanged)
    Q_PROPERTY(bool uiBusy READ isUiBusy WRITE setUiBusy NOTIFY uiBusyChanged)

public:
    // status values: "off", "noDistributor", "registering", "registered", "error"
    UnifiedPushManager(bool wakeMode, QObject *parent = nullptr);

    static const char *WAKE_ENVIRONMENT_VARIABLE;

    void setTdLibWrapper(TDLibWrapper *tdLibWrapper);
    void setView(QWindow *view);

    bool isEnabled() const;
    QString getStatus() const;
    QString getEndpointHost() const;
    QString getErrorText() const;
    bool isUiBusy() const;
    void setUiBusy(bool busy);

    Q_INVOKABLE void setEnabled(bool enabled);
    // Registers with the distributor again (the "Try again" button).
    Q_INVOKABLE void refresh();

    // Called by the D-Bus adaptor.
    void handleNewEndpoint(const QString &token, const QString &endpoint);
    void handleMessage(const QString &token);
    void handleUnregistered(const QString &token);

signals:
    void enabledChanged();
    void statusChanged();
    void uiBusyChanged();

private slots:
    void handleAuthorizationStateChanged(const TDLibWrapper::AuthorizationState &authorizationState, const QVariantMap &authorizationStateData);
    void handleOkReceived(const QString &extra);
    void handleErrorReceived(int code, const QString &message, const QString &extra);
    void handleRegisterReply(const QDBusMessage &reply);
    void handleRegisterError(const QDBusError &error, const QDBusMessage &call);
    void checkIdle();

private:
    bool claimConnector();
    void releaseConnector();
    void registerWithDistributor();
    void unregisterFromDistributor();
    QString findDistributor() const;
    void sendEndpointToTelegram();
    void setStatus(const QString &status, const QString &errorText = QString());
    void noteActivity();
    QString configDirectory() const;
    QString markerPath() const;
    QString token();
    QString webPushAuth();

    QPointer<TDLibWrapper> tdLibWrapper;
    QPointer<QWindow> view;
    UnifiedPushConnectorAdaptor *adaptor;
    bool wakeMode;
    bool connectorClaimed;
    bool uiBusy;
    bool telegramSent;
    QString status;
    QString errorText;
    QString distributor;
    QTimer idleTimer;
    qint64 lastActivityMs;
    qint64 startedMs;
    qint64 connectedSinceMs;
    bool registeredThisRun;
};

#endif // UNIFIEDPUSHMANAGER_H
