/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#include "gtgramvoicecallhandler.h"
#include "gtgramvoicecallprovider.h"

#include <QDebug>

GTgramVoiceCallHandler::GTgramVoiceCallHandler(const QString &handlerId, const QString &callerName,
                                               bool incoming, GTgramVoiceCallProvider *provider,
                                               QObject *parent)
    : AbstractVoiceCallHandler(parent)
    , m_handlerId(handlerId)
    , m_callerName(callerName)
    , m_incoming(incoming)
    , m_provider(provider)
    , m_status(incoming ? STATUS_INCOMING : STATUS_DIALING)
{
    m_durationTimer.setInterval(1000);
    connect(&m_durationTimer, &QTimer::timeout, this, [this]() { emit durationChanged(duration()); });
}

AbstractVoiceCallProvider* GTgramVoiceCallHandler::provider() const { return (AbstractVoiceCallProvider*)m_provider; }
QString GTgramVoiceCallHandler::handlerId() const { return m_handlerId; }
// lineId is what the system UI shows as "who is calling".
QString GTgramVoiceCallHandler::lineId() const { return m_callerName; }
QString GTgramVoiceCallHandler::subscriberId() const { return QString(); }
QDateTime GTgramVoiceCallHandler::startedAt() const { return m_startedAt; }

int GTgramVoiceCallHandler::duration() const
{
    return m_startedAt.isValid() ? int(m_startedAt.secsTo(QDateTime::currentDateTime())) : 0;
}

bool GTgramVoiceCallHandler::isIncoming() const { return m_incoming; }
bool GTgramVoiceCallHandler::isMultiparty() const { return false; }
bool GTgramVoiceCallHandler::isEmergency() const { return false; }
bool GTgramVoiceCallHandler::isForwarded() const { return false; }
bool GTgramVoiceCallHandler::isRemoteHeld() const { return false; }
QString GTgramVoiceCallHandler::parentHandlerId() const { return QString(); }
QList<AbstractVoiceCallHandler*> GTgramVoiceCallHandler::childCalls() const { return QList<AbstractVoiceCallHandler*>(); }
AbstractVoiceCallHandler::VoiceCallStatus GTgramVoiceCallHandler::status() const { return m_status; }

void GTgramVoiceCallHandler::setStatus(VoiceCallStatus status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    if (status == STATUS_ACTIVE && !m_startedAt.isValid()) {
        m_startedAt = QDateTime::currentDateTime();
        emit startedAtChanged(m_startedAt);
        m_durationTimer.start();
    }
    if (status == STATUS_DISCONNECTED) {
        m_durationTimer.stop();
    }
    qWarning() << "[GTGRAM-VOICECALL] handler" << m_handlerId << "status ->" << int(status);
    emit statusChanged(m_status);
}

// --- Commands coming from the system UI: relay them to the app ---

void GTgramVoiceCallHandler::answer()
{
    qWarning() << "[GTGRAM-VOICECALL] answer() from system UI";
    emit m_provider->answerRequested();
}

void GTgramVoiceCallHandler::hangup()
{
    qWarning() << "[GTGRAM-VOICECALL] hangup() from system UI";
    emit m_provider->hangupRequested();
    setStatus(STATUS_DISCONNECTED);
}

void GTgramVoiceCallHandler::hold(bool) { /* not supported by Telegram calls */ }
void GTgramVoiceCallHandler::deflect(const QString &) { /* not supported */ }
void GTgramVoiceCallHandler::sendDtmf(const QString &) { /* not supported */ }
void GTgramVoiceCallHandler::merge(const QString &) { /* not supported */ }
void GTgramVoiceCallHandler::split() { /* not supported */ }
void GTgramVoiceCallHandler::filter(VoiceCallFilterAction) { /* no filtering */ }
