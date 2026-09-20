/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#ifndef GTGRAMVOICECALLPROVIDERFACTORY_H
#define GTGRAMVOICECALLPROVIDERFACTORY_H

#include <abstractvoicecallmanagerplugin.h>

class GTgramVoiceCallProvider;

// Entry point loaded by voicecall-manager at startup.
class GTgramVoiceCallProviderFactory : public AbstractVoiceCallManagerPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.nemomobile.voicecall.ManagerPlugin/1.0")
    Q_INTERFACES(AbstractVoiceCallManagerPlugin)

public:
    explicit GTgramVoiceCallProviderFactory(QObject *parent = nullptr);
    ~GTgramVoiceCallProviderFactory() override;

    QString pluginId() const override;

public Q_SLOTS:
    bool initialize() override;
    bool configure(VoiceCallManagerInterface *manager) override;
    bool start() override;
    bool suspend() override;
    bool resume() override;
    void finalize() override;

private:
    VoiceCallManagerInterface *m_manager;
    GTgramVoiceCallProvider *m_provider;
};

#endif
