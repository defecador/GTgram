/*
    This file is part of GTgram, licensed under the GNU General Public
    License v3.0 or later. See the LICENSE file for details.

    Ported from RooTelegram-SailfishOS (GPLv3).
*/
#ifndef GTGRAMVOICECALLHANDLER_H
#define GTGRAMVOICECALLHANDLER_H

#include <abstractvoicecallhandler.h>
#include <QDateTime>
#include <QTimer>

class GTgramVoiceCallProvider;

/*
 * Represents ONE GTgram call to the system. It carries no audio or video -
 * it only lets voicecall-manager (and so lipstick, mce, and the system call
 * UI) know a call is in progress. Audio keeps flowing through tgcalls
 * inside the app.
 */
class GTgramVoiceCallHandler : public AbstractVoiceCallHandler
{
    Q_OBJECT
public:
    GTgramVoiceCallHandler(const QString &handlerId, const QString &callerName,
                           bool incoming, GTgramVoiceCallProvider *provider,
                           QObject *parent = nullptr);

    AbstractVoiceCallProvider* provider() const override;
    QString handlerId() const override;
    QString lineId() const override;
    QString subscriberId() const override;
    QDateTime startedAt() const override;
    int duration() const override;
    bool isIncoming() const override;
    bool isMultiparty() const override;
    bool isEmergency() const override;
    bool isForwarded() const override;
    bool isRemoteHeld() const override;
    QString parentHandlerId() const override;
    QList<AbstractVoiceCallHandler*> childCalls() const override;
    VoiceCallStatus status() const override;

    void setStatus(VoiceCallStatus status);

public Q_SLOTS:
    // Arrive from the system call UI and are forwarded to the app.
    void answer() override;
    void hangup() override;
    void hold(bool on) override;
    void deflect(const QString &target) override;
    void sendDtmf(const QString &tones) override;
    void merge(const QString &callHandle) override;
    void split() override;
    void filter(VoiceCallFilterAction action) override;

private:
    QString m_handlerId;
    QString m_callerName;
    bool m_incoming;
    GTgramVoiceCallProvider *m_provider;
    VoiceCallStatus m_status;
    QDateTime m_startedAt;
    QTimer m_durationTimer;
};

#endif // GTGRAMVOICECALLHANDLER_H
