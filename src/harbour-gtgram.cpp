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

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QScopedPointer>
#include <QQuickView>
#include <QtQml>
#include <QQmlContext>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QSysInfo>
#include <QSettings>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include "appsettings.h"
#include "debuglog.h"
#include "debuglogjs.h"
#include "tdlibfile.h"
#include "tdlibwrapper.h"
#include "chatpermissionfiltermodel.h"
#include "chatlistmodel.h"
#include "chatmodel.h"
#include "namedaction.h"
#include "notificationmanager.h"
#include "mceinterface.h"
#ifdef GTGRAM_VOICE_CALLS
#include "callmanager.h"
#endif
#include "dbusadaptor.h"
#include "processlauncher.h"
#include "stickermanager.h"
#include "textfiltermodel.h"
#include "boolfiltermodel.h"
#include "tgsplugin.h"
#include "gtgramutils.h"
#include "knownusersmodel.h"
#include "contactsmodel.h"

// The default filter can be overridden by QT_LOGGING_RULES envinronment variable, e.g.
// QT_LOGGING_RULES="gtgram.*=true" harbour-gtgram
#if defined (QT_DEBUG) || defined(DEBUG)
#  define DEFAULT_LOG_FILTER "gtgram.*=true"
#else
#  define DEFAULT_LOG_FILTER "gtgram.*=false"
#endif

#include "appversion.h"
#ifndef APP_VERSION
#  define APP_VERSION "unknown"
#endif

Q_IMPORT_PLUGIN(TgsIOPlugin)

void migrateSettings() {
    const QStringList sailfishOSVersion = QSysInfo::productVersion().split(".");
    int sailfishOSMajorVersion = sailfishOSVersion.value(0).toInt();
    int sailfishOSMinorVersion = sailfishOSVersion.value(1).toInt();
    if ((sailfishOSMajorVersion == 4 && sailfishOSMinorVersion >= 4) || sailfishOSMajorVersion > 4) {
        LOG("Checking if we need to migrate settings...");
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/de.ygriega/gtgram/settings.conf", QSettings::NativeFormat);
        if (settings.contains("migrated")) {
            return;
        }
        QSettings oldSettings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/harbour-gtgram/settings.conf", QSettings::NativeFormat);
        const QStringList oldKeys = oldSettings.allKeys();
        if (oldKeys.isEmpty()) {
            return;
        }
        LOG("SailfishOS >= 4.4 and old configuration file detected, migrating settings to new location...");
        for (const QString &key : oldKeys) {
            settings.setValue(key, oldSettings.value(key));
        }

        QDir oldDataLocation(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/harbour-gtgram/harbour-gtgram");
        LOG("Old data directory: " + oldDataLocation.path());
        if (oldDataLocation.exists()) {
            LOG("Old data files detected, migrating files to new location...");
            const int oldDataPathLength = oldDataLocation.absolutePath().length();
            QString dataLocationPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir dataLocation(dataLocationPath);
            QDirIterator oldDataIterator(oldDataLocation, QDirIterator::Subdirectories);
            while (oldDataIterator.hasNext()) {
                oldDataIterator.next();
                QFileInfo currentFileInfo = oldDataIterator.fileInfo();
                if (!currentFileInfo.isHidden()) {
                    const QString subPath = currentFileInfo.absoluteFilePath().mid(oldDataPathLength);
                    const QString targetPath = dataLocationPath + subPath;
                    if (currentFileInfo.isDir()) {
                        LOG("Creating new directory " + targetPath);
                        dataLocation.mkpath(targetPath);
                    } else if(currentFileInfo.isFile()) {
                        LOG("Copying file to " + targetPath);
                        QFile::copy(currentFileInfo.absoluteFilePath(), targetPath);
                    }
                }
            }
        }

        settings.setValue("migrated", true);
    }
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(DEFAULT_LOG_FILTER);

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    QQmlContext *context = view.data()->rootContext();

    migrateSettings();

    const char *uri = "WerkWolf.GTgram";
    qmlRegisterType<TDLibFile>(uri, 1, 0, "TDLibFile");
    qmlRegisterType<NamedAction>(uri, 1, 0, "NamedAction");
    qmlRegisterType<TextFilterModel>(uri, 1, 0, "TextFilterModel");
    qmlRegisterType<BoolFilterModel>(uri, 1, 0, "BoolFilterModel");
    qmlRegisterType<ChatPermissionFilterModel>(uri, 1, 0, "ChatPermissionFilterModel");
    qmlRegisterSingletonType<DebugLogJS>(uri, 1, 0, "DebugLog", DebugLogJS::createSingleton);

    AppSettings *appSettings = new AppSettings(view.data());
    context->setContextProperty("appVersion", QStringLiteral(APP_VERSION));
    context->setContextProperty("appSettings", appSettings);
    qmlRegisterUncreatableType<AppSettings>(uri, 1, 0, "AppSettings", QString());

    MceInterface *mceInterface = new MceInterface(view.data());
    TDLibWrapper *tdLibWrapper = new TDLibWrapper(appSettings, mceInterface, view.data());
    context->setContextProperty("tdLibWrapper", tdLibWrapper);
    qmlRegisterUncreatableType<TDLibWrapper>(uri, 1, 0, "TelegramAPI", QString());

    GTgramUtils *gtgramUtils = new GTgramUtils(view.data());
    context->setContextProperty("gtgramUtils", gtgramUtils);
    qmlRegisterUncreatableType<GTgramUtils>(uri, 1, 0, "GTgramUtilities", QString());

#ifdef GTGRAM_VOICE_CALLS
    CallManager *callManager = new CallManager(tdLibWrapper, mceInterface, view.data());
    context->setContextProperty("callManager", callManager);
    qmlRegisterUncreatableType<CallManager>(uri, 1, 0, "CallManager", QString());
#endif
    context->setContextProperty("voiceCallsAvailable",
#ifdef GTGRAM_VOICE_CALLS
        true
#else
        false
#endif
    );

    DBusAdaptor *dBusAdaptor = tdLibWrapper->getDBusAdaptor();
    context->setContextProperty("dBusAdaptor", dBusAdaptor);

    ChatListModel chatListModel(tdLibWrapper, appSettings);
    context->setContextProperty("chatListModel", &chatListModel);

    ChatModel chatModel(tdLibWrapper);
    context->setContextProperty("chatModel", &chatModel);

    NotificationManager notificationManager(tdLibWrapper, appSettings, mceInterface, &chatModel);
    context->setContextProperty("notificationManager", &notificationManager);

    // Notification quick actions are handled in C++ so they work without the UI
    QObject::connect(dBusAdaptor, &DBusAdaptor::pleaseReplyToMessage,
                     &notificationManager, &NotificationManager::replyToMessage);
    QObject::connect(dBusAdaptor, &DBusAdaptor::pleaseReactToMessage,
                     &notificationManager, &NotificationManager::reactToMessage);

    ProcessLauncher processLauncher;
    context->setContextProperty("processLauncher", &processLauncher);

    StickerManager stickerManager(tdLibWrapper);
    context->setContextProperty("stickerManager", &stickerManager);

    KnownUsersModel knownUsersModel(tdLibWrapper, view.data());
    context->setContextProperty("knownUsersModel", &knownUsersModel);
    QSortFilterProxyModel knownUsersProxyModel(view.data());
    knownUsersProxyModel.setSourceModel(&knownUsersModel);
    knownUsersProxyModel.setFilterRole(KnownUsersModel::RoleFilter);
    knownUsersProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    context->setContextProperty("knownUsersProxyModel", &knownUsersProxyModel);
    
    ContactsModel contactsModel(tdLibWrapper, view.data());
    context->setContextProperty("contactsModel", &contactsModel);
    QSortFilterProxyModel contactsProxyModel(view.data());
    contactsProxyModel.setSourceModel(&contactsModel);
    contactsProxyModel.setFilterRole(ContactsModel::RoleFilter);
    contactsProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    context->setContextProperty("contactsProxyModel", &contactsProxyModel);

    view->setSource(SailfishApp::pathTo("qml/harbour-gtgram.qml"));
    view->show();
    return app->exec();
}
