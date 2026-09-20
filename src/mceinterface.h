/*
    Copyright (C) 2020 Slava Monich et al.

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

#ifndef MCE_INTERFACE_H
#define MCE_INTERFACE_H

#include <QDBusInterface>

class MceInterface : public QDBusInterface
{
public:
    MceInterface(QObject *parent = Q_NULLPTR);

    void ledPatternActivate(const QString &pattern);
    void ledPatternDeactivate(const QString &pattern);
    void displayCancelBlankingPause();
    void displayBlankingPause();
    // Turns the display on if it's off - unlike displayBlankingPause, which only
    // prevents it from turning off.
    void displayOn();
    // Lifts the touch lock (the swipe lockscreen) so an incoming call's UI is
    // usable right away without unlocking first. Does not bypass any security
    // code (that's lipstick's job, not MCE's).
    void tklockUnlock();
    // Like tklockUnlock, but waits until the display is actually on. Needed
    // because MCE refuses tklockUnlock while the display is off
    // ("tkunlock denied due to display=OFF"), and both displayOn() and the
    // automatic wake from the call state change are asynchronous - the first
    // attempt usually still finds the display OFF.
    void tklockUnlockWhenDisplayOn(int maxWaitMs = 3000);
    // Tells MCE a call is in progress. This is what turns on the system's call
    // policies, foremost among them turning the display off when the phone is
    // held to the ear - MCE's proximity.so module looks at the call state, not
    // at audio activity.
    // state: "none" | "ringing" | "active" | "service"
    // type:  "normal" | "emergency"
    // NB: MCE ties this state to the lifetime of THIS D-Bus connection, so if
    // the app dies the state reverts to "none" on its own.
    void callStateChange(const QString &state, const QString &type = QStringLiteral("normal"));
};

#endif // MCE_INTERFACE_H

