/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#ifndef GTGRAMVOICECALLPROVIDER_H
#define GTGRAMVOICECALLPROVIDER_H

#include <abstractvoicecallprovider.h>
#include <voicecallmanagerinterface.h>
#include <QTimer>

class QDBusServiceWatcher;
class GTgramVoiceCallHandler;

/*
 * GTgram's call provider for voicecall-manager.
 *
 * Lives INSIDE the voicecall-manager process, not inside the app: it's
 * driven by the app over D-Bus (org.nemomobile.voicecall / /gtgram/calls,
 * see gtgramcallsadaptor.h for why that name) and relays back what the
 * user does on the system call UI.
 *
 * Two safety nets a naive port would miss: if the app dies with a call
 * open, the handler would otherwise stay ACTIVE forever and the system
 * would keep believing a call is in progress, holding the audio policy in
 * "in call" mode until reboot. So:
 *   1) a QDBusServiceWatcher closes the call if the app disappears from the bus;
 *   2) a watchdog closes it anyway after too long without a heartbeat.
 */
class GTgramVoiceCallProvider : public AbstractVoiceCallProvider
{
    Q_OBJECT
public:
    explicit GTgramVoiceCallProvider(VoiceCallManagerInterface *manager, QObject *parent = nullptr);
    ~GTgramVoiceCallProvider() override;

    QString providerId() const override;
    QString providerType() const override;
    QList<AbstractVoiceCallHandler*> voiceCalls() const override;
    QString errorString() const override;

public Q_SLOTS:
    bool dial(const QString &msisdn) override;

    // --- D-Bus API used by the app ---
    Q_SCRIPTABLE void newCall(const QString &callerName, bool incoming);
    Q_SCRIPTABLE void callReady();
    Q_SCRIPTABLE void discardCall();
    // Heartbeat: pushes the watchdog back. The app sends this while the call is alive.
    Q_SCRIPTABLE void ping();

Q_SIGNALS:
    // Towards the app: the user pressed answer/hangup on the system UI.
    Q_SCRIPTABLE void answerRequested();
    Q_SCRIPTABLE void hangupRequested();
    // Speaker and mute pressed on the system UI. Needed because those buttons
    // go from voicecall-manager to ohm, while the call's audio is routed by
    // the app in PulseAudio: without relaying this, pressing speaker on the
    // system UI would have no effect.
    Q_SCRIPTABLE void speakerModeRequested(bool on);
    Q_SCRIPTABLE void muteMicrophoneRequested(bool muted);

private Q_SLOTS:
    void handleAppDisappeared(const QString &service);
    void handleWatchdogTimeout();

private:
    void closeCall(const char *reason);

    VoiceCallManagerInterface *m_manager;
    GTgramVoiceCallHandler *m_voiceCall;
    QDBusServiceWatcher *m_appWatcher;
    QTimer m_watchdog;
};

#endif // GTGRAMVOICECALLPROVIDER_H
