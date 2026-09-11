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

#ifndef DBUSADAPTOR_H
#define DBUSADAPTOR_H

#include <QDBusAbstractAdaptor>

class DBusAdaptor : public QDBusAbstractAdaptor
{

    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.ygriega.gtgram")

public:
    DBusAdaptor(QObject *parent);

signals:
    void pleaseOpenMessage(const QString &chatId, const QString &messageId);
    void pleaseOpenUrl(const QString &url);
    void pleaseReplyToMessage(const QString &chatId, const QString &messageId, const QString &text);
    void pleaseReactToMessage(const QString &chatId, const QString &messageId);

public slots:
    void openMessage(const QString &chatId, const QString &messageId);
    void openUrl(const QStringList &arguments);
    // Invoked by Lipstick when a notification action is used. For the reply
    // action the text typed into the notification is appended by the shell.
    void replyToMessage(const QString &chatId, const QString &messageId, const QString &text);
    void reactToMessage(const QString &chatId, const QString &messageId);

};

#endif // DBUSADAPTOR_H
