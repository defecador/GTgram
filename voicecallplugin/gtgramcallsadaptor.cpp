/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#include "gtgramcallsadaptor.h"
#include "gtgramvoicecallprovider.h"

GTgramCallsAdaptor::GTgramCallsAdaptor(GTgramVoiceCallProvider *provider)
    : QDBusAbstractAdaptor(provider), m_provider(provider)
{
    setAutoRelaySignals(false);
    // The provider's two signals become D-Bus signals on the interface the
    // sandbox permits.
    connect(m_provider, &GTgramVoiceCallProvider::answerRequested,
            this, &GTgramCallsAdaptor::answerRequested);
    connect(m_provider, &GTgramVoiceCallProvider::hangupRequested,
            this, &GTgramCallsAdaptor::hangupRequested);
    connect(m_provider, &GTgramVoiceCallProvider::speakerModeRequested,
            this, &GTgramCallsAdaptor::speakerModeRequested);
    connect(m_provider, &GTgramVoiceCallProvider::muteMicrophoneRequested,
            this, &GTgramCallsAdaptor::muteMicrophoneRequested);
}

void GTgramCallsAdaptor::newCall(const QString &callerName, bool incoming) { m_provider->newCall(callerName, incoming); }
void GTgramCallsAdaptor::callReady() { m_provider->callReady(); }
void GTgramCallsAdaptor::discardCall() { m_provider->discardCall(); }
void GTgramCallsAdaptor::ping() { m_provider->ping(); }
