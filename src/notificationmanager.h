/*
    Copyright (C) 2020 Sebastian J. Wolf and other contributors

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

#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <nemonotifications-qt5/notification.h>
#include "tdlibwrapper.h"
#include "appsettings.h"
#include "mceinterface.h"

class ChatModel;

class NotificationManager : public QObject
{
    Q_OBJECT
    class ChatInfo;
    class NotificationGroup;

public:

    NotificationManager(TDLibWrapper *tdLibWrapper, AppSettings *appSettings, MceInterface *mceInterface, ChatModel *chatModel);
    ~NotificationManager() override;

public slots:

    void handleUpdateActiveNotifications(const QVariantList &notificationGroups);
    void handleUpdateNotificationGroup(const QVariantMap &notificationGroupUpdate);
    void handleUpdateNotification(const QVariantMap &updatedNotification);
    void handleChatDiscovered(const QString &chatId, const QVariantMap &chatInformation);
    void handleChatTitleUpdated(const QString &chatId, const QString &title);
    void handleChatPhotoUpdated(qlonglong chatId, const QVariantMap &photo);
    void handleFileUpdated(int fileId, const QVariantMap &fileInformation);
    void replyToMessage(const QString &chatId, const QString &messageId, const QString &text);
    void reactToMessage(const QString &chatId, const QString &messageId);
    void handleMessageUnreadReactionsUpdated(qlonglong chatId, qlonglong messageId, const QVariantList &unreadReactions);
    void handleReactionNotificationSettingsUpdated(const QVariantMap &settings);

private:

    void publishNotification(const NotificationGroup *notificationGroup, bool needFeedback);
    void controlLedNotification(bool enabled);
    QString getReactionSenderName(qlonglong chatId, const QVariantMap &senderId) const;
    QString getChatAvatarPath(qlonglong chatId) const;
    NotificationGroup *findGroupForChat(qlonglong chatId) const;
    void updateNotificationGroup(int groupId, qlonglong chatId, int totalCount,
        const QVariantList &addedNotifications,
        const QVariantList &removedNotificationIds = QVariantList(),
        AppSettings::NotificationFeedback feedback = AppSettings::NotificationFeedbackNone);

private:

    TDLibWrapper *tdLibWrapper;
    AppSettings *appSettings;
    MceInterface *mceInterface;
    ChatModel *chatModel;
    QMap<qlonglong,ChatInfo*> chatMap;
    QMap<int,NotificationGroup*> notificationGroups;
    QString appIconFile;
    // Reactions to our own messages, one notification per (chat, message).
    // TDLib never turns these into notification groups without a push
    // service, so they're built here from updateMessageUnreadReactions.
    QMap<QPair<qlonglong,qlonglong>,Notification*> reactionNotifications;
    QString reactionNotificationSource;

};

#endif // NOTIFICATIONMANAGER_H
