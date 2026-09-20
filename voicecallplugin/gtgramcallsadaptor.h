/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#ifndef GTGRAMCALLSADAPTOR_H
#define GTGRAMCALLSADAPTOR_H

#include <QDBusAbstractAdaptor>

class GTgramVoiceCallProvider;

/*
 * D-Bus face of the provider, exposed on the org.nemomobile.voicecall
 * service (which the host process, voicecall-manager, already owns) at
 * object path /gtgram/calls.
 *
 * Why this and not a name of our own: the app is confined by sailjail
 * behind an xdg-dbus-proxy, and can only talk to names granted by a
 * permission. Phone.permission already grants exactly:
 *     dbus-user.talk      org.nemomobile.voicecall
 *     dbus-user.broadcast org.nemomobile.voicecall=org.nemomobile.voicecall.<any>
 * A private service name would not be reachable, and the sandbox's signal
 * filter only accepts interfaces starting with org.nemomobile.voicecall -
 * hence the interface name below, which is not cosmetic.
 */
class GTgramCallsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.nemomobile.voicecall.GTgram")

public:
    explicit GTgramCallsAdaptor(GTgramVoiceCallProvider *provider);

public Q_SLOTS:
    void newCall(const QString &callerName, bool incoming);
    void callReady();
    void discardCall();
    void ping();

Q_SIGNALS:
    void answerRequested();
    void hangupRequested();
    void speakerModeRequested(bool on);
    void muteMicrophoneRequested(bool muted);

private:
    GTgramVoiceCallProvider *m_provider;
};

#endif
