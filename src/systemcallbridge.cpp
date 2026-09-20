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

#include "systemcallbridge.h"

#define DEBUG_MODULE SystemCallBridge
#include "debuglog.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCall>

namespace {
// Our plugin exposes its object ON voicecall-manager's own service - the
// only one the sandboxed app can reach, thanks to the "Phone" permission in
// the .desktop file (dbus-user.talk org.nemomobile.voicecall).
const QString PLUGIN_SERVICE("org.nemomobile.voicecall");
const QString PLUGIN_PATH("/gtgram/calls");
// The interface MUST start with org.nemomobile.voicecall: Phone.permission's
// signal filter only accepts those.
const QString PLUGIN_IFACE("org.nemomobile.voicecall.GTgram");
// The plugin closes the call on its own if it gets no heartbeat; this stays
// comfortably under its threshold.
const int KEEPALIVE_MS = 30000;
}

SystemCallBridge::SystemCallBridge(QObject *parent)
    : QObject(parent)
    , m_plugin(nullptr)
    , m_callDeclared(false)
{
    m_plugin = new QDBusInterface(PLUGIN_SERVICE, PLUGIN_PATH, PLUGIN_IFACE,
                                  QDBusConnection::sessionBus(), this);

    QDBusConnection::sessionBus().connect(PLUGIN_SERVICE, PLUGIN_PATH, PLUGIN_IFACE,
                                          QStringLiteral("answerRequested"),
                                          this, SIGNAL(answerRequested()));
    QDBusConnection::sessionBus().connect(PLUGIN_SERVICE, PLUGIN_PATH, PLUGIN_IFACE,
                                          QStringLiteral("hangupRequested"),
                                          this, SIGNAL(hangupRequested()));
    QDBusConnection::sessionBus().connect(PLUGIN_SERVICE, PLUGIN_PATH, PLUGIN_IFACE,
                                          QStringLiteral("speakerModeRequested"),
                                          this, SIGNAL(speakerModeRequested(bool)));
    QDBusConnection::sessionBus().connect(PLUGIN_SERVICE, PLUGIN_PATH, PLUGIN_IFACE,
                                          QStringLiteral("muteMicrophoneRequested"),
                                          this, SIGNAL(muteMicrophoneRequested(bool)));

    m_keepAlive.setInterval(KEEPALIVE_MS);
    connect(&m_keepAlive, &QTimer::timeout, this, &SystemCallBridge::sendKeepAlive);

    if (isAvailable()) {
        LOG("voicecall plugin available: calls will be integrated with the system");
    } else {
        LOG("voicecall plugin NOT available (voicecall-manager not restarted since install?):"
            << "staying on the MCE path, no system UI");
    }
}

SystemCallBridge::~SystemCallBridge()
{
    endCall();
}

bool SystemCallBridge::isAvailable() const
{
    return m_plugin && m_plugin->isValid();
}

void SystemCallBridge::startCall(const QString &callerName, bool incoming)
{
    if (!isAvailable()) {
        return;
    }
    LOG("Declaring call to the system:" << callerName << "incoming:" << incoming);
    m_plugin->asyncCall(QStringLiteral("newCall"), callerName, incoming);
    m_callDeclared = true;
    m_keepAlive.start();
}

void SystemCallBridge::setCallActive()
{
    if (!isAvailable() || !m_callDeclared) {
        return;
    }
    m_plugin->asyncCall(QStringLiteral("callReady"));
}

void SystemCallBridge::endCall()
{
    m_keepAlive.stop();
    if (!isAvailable() || !m_callDeclared) {
        m_callDeclared = false;
        return;
    }
    LOG("Closing call towards the system");
    m_plugin->asyncCall(QStringLiteral("discardCall"));
    m_callDeclared = false;
}

void SystemCallBridge::sendKeepAlive()
{
    if (isAvailable() && m_callDeclared) {
        m_plugin->asyncCall(QStringLiteral("ping"));
    }
}
