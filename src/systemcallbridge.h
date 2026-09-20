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

    Ported from RooTelegram-SailfishOS (GPLv3).
*/

#ifndef SYSTEMCALLBRIDGE_H
#define SYSTEMCALLBRIDGE_H

#include <QObject>
#include <QTimer>

class QDBusInterface;

/*
 * Bridge between GTgram and its plugin inside voicecall-manager.
 *
 * Why this exists: registering a call with voicecall-manager is the ONLY
 * way to make it behave like a system phone call - screen turning on, the
 * answer UI showing ABOVE the lock screen (PIN included), the proximity
 * sensor, screen-off at the ear. Providers can only register as plugins
 * loaded INSIDE the voicecall-manager process - not from outside. The
 * plugin lives in the same project and the same RPM (voicecallplugin/),
 * but runs in a different process, so the two talk over D-Bus.
 *
 * If the plugin isn't there (partial install, voicecall-manager not yet
 * restarted after an update) everything degrades silently: isAvailable()
 * returns false and the caller falls back to the plain MceInterface path.
 */
class SystemCallBridge : public QObject
{
    Q_OBJECT
public:
    explicit SystemCallBridge(QObject *parent = nullptr);
    ~SystemCallBridge() override;

    // true if the plugin is alive on the bus and calls can be declared to it.
    bool isAvailable() const;

    void startCall(const QString &callerName, bool incoming);
    void setCallActive();
    void endCall();

Q_SIGNALS:
    // The user pressed answer/hangup on the SYSTEM call UI.
    void answerRequested();
    void hangupRequested();
    // Speaker/mute pressed on the system UI: applied by us, since we're the
    // ones routing the call's audio in PulseAudio.
    void speakerModeRequested(bool on);
    void muteMicrophoneRequested(bool muted);

private Q_SLOTS:
    void sendKeepAlive();

private:
    QDBusInterface *m_plugin;
    QTimer m_keepAlive;
    bool m_callDeclared;
};

#endif // SYSTEMCALLBRIDGE_H
