/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#include "gtgramvoicecallproviderfactory.h"
#include "gtgramvoicecallprovider.h"

#include <voicecallmanagerinterface.h>
#include <QDebug>

GTgramVoiceCallProviderFactory::GTgramVoiceCallProviderFactory(QObject *parent)
    : AbstractVoiceCallManagerPlugin(parent), m_manager(nullptr), m_provider(nullptr) {}

GTgramVoiceCallProviderFactory::~GTgramVoiceCallProviderFactory() {}

QString GTgramVoiceCallProviderFactory::pluginId() const { return QStringLiteral("gtgram-voicecall-plugin"); }

bool GTgramVoiceCallProviderFactory::initialize() { return true; }

bool GTgramVoiceCallProviderFactory::configure(VoiceCallManagerInterface *manager)
{
    if (m_provider) {
        return false;
    }
    qWarning() << "[GTGRAM-VOICECALL] configure(): registering the GTgram provider";
    m_manager = manager;
    m_provider = new GTgramVoiceCallProvider(m_manager, this);
    m_manager->appendProvider((AbstractVoiceCallProvider*)m_provider);
    return true;
}

bool GTgramVoiceCallProviderFactory::start() { return true; }
bool GTgramVoiceCallProviderFactory::suspend() { return true; }
bool GTgramVoiceCallProviderFactory::resume() { return true; }
void GTgramVoiceCallProviderFactory::finalize() {}
