# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-gtgram

CONFIG += sailfishapp sailfishapp_i18n

# Version shown in the About page. Read straight from the RPM spec so the spec
# stays the single source of truth and no build number is ever hand-edited here.
# qmake strips quotes before handing the command to the shell, so keep every
# argument space-free: grep returns e.g. "Version:    0.18", which qmake splits
# into a two-element list, and $$last() picks off the value.
VER_LINE = $$system(grep -m1 ^Version: $$PWD/rpm/harbour-gtgram.spec)
REL_LINE = $$system(grep -m1 ^Release: $$PWD/rpm/harbour-gtgram.spec)
APP_VERSION = $$last(VER_LINE)
APP_RELEASE = $$last(REL_LINE)
isEmpty(APP_VERSION): APP_VERSION = unknown
isEmpty(APP_RELEASE): APP_RELEASE = 0
message(Building GTgram $${APP_VERSION}-$${APP_RELEASE})
APP_VERSION_STR = $$sprintf("\"%1-%2\"", $$APP_VERSION, $$APP_RELEASE)
QMAKE_SUBSTITUTES += src/appversion.h.in
INCLUDEPATH += $$OUT_PWD/src

PKGCONFIG += nemonotifications-qt5 zlib

QT += core dbus sql multimedia positioning

DEFINES += QT_STATICPLUGIN

SOURCES += src/harbour-gtgram.cpp \
    src/appsettings.cpp \
    src/boolfiltermodel.cpp \
    src/chatpermissionfiltermodel.cpp \
    src/chatlistmodel.cpp \
    src/chatmodel.cpp \
    src/contactsmodel.cpp \
    src/dbusadaptor.cpp \
    src/dbusinterface.cpp \
    src/emojisearchworker.cpp \
    src/gtgramutils.cpp \
    src/knownusersmodel.cpp \
    src/mceinterface.cpp \
    src/namedaction.cpp \
    src/notificationmanager.cpp \
    src/processlauncher.cpp \
    src/stickermanager.cpp \
    src/tdlibfile.cpp \
    src/tdlibreceiver.cpp \
    src/tdlibwrapper.cpp \
    src/textfiltermodel.cpp \
    src/tgsplugin.cpp

DISTFILES += qml/harbour-gtgram.qml \
    qml/components/AudioPreview.qml \
    qml/components/BackgroundImage.qml \
    qml/components/CallOverlay.qml \
    qml/components/ChatListViewItem.qml \
    qml/components/ContactSync.qml \
    qml/components/DocumentPreview.qml \
    qml/components/GamePreview.qml \
    qml/components/ImagePreview.qml \
    qml/components/InformationEditArea.qml \
    qml/components/InformationTextItem.qml \
    qml/components/InReplyToRow.qml \
    qml/components/InlineQuery.qml \
    qml/components/LocationPreview.qml \
    qml/components/MessageListViewItem.qml \
    qml/components/MessageListViewItemSimple.qml \
    qml/components/MessageOverlayFlickable.qml \
    qml/components/MessagePropertiesLoader.qml \
    qml/components/MessageViaLabel.qml \
    qml/components/MultilineEmojiLabel.qml \
    qml/components/PinnedMessageItem.qml \
    qml/components/PollPreview.qml \
    qml/components/PressEffect.qml \
    qml/components/ProfilePictureList.qml \
    qml/components/ReactionButton.qml \
    qml/components/ReplyMarkupButtons.qml \
    qml/components/StickerPicker.qml \
    qml/components/PhotoTextsListItem.qml \
    qml/components/StickerSetOverlay.qml \
    qml/components/TDLibImage.qml \
    qml/components/TDLibMinithumbnail.qml \
    qml/components/TDLibPhoto.qml \
    qml/components/TDLibThumbnail.qml \
    qml/components/VoiceNoteOverlay.qml \
    qml/components/chatInformationPage/ChatInformationPageContent.qml \
    qml/components/chatInformationPage/ChatInformationProfilePicture.qml \
    qml/components/chatInformationPage/ChatInformationTabItemBase.qml \
    qml/components/chatInformationPage/ChatInformationTabItemDebug.qml \
    qml/components/chatInformationPage/ChatInformationTabItemMembersGroups.qml \
    qml/components/chatInformationPage/ChatInformationTabItemSettings.qml \
    qml/components/chatInformationPage/ChatInformationTabView.qml \
    qml/components/chatInformationPage/EditGroupChatPermissionsColumn.qml \
    qml/components/chatInformationPage/EditSuperGroupSlowModeColumn.qml \
    qml/components/inlineQueryResults/InlineQueryResult.qml \
    qml/components/inlineQueryResults/InlineQueryResultAnimation.qml \
    qml/components/inlineQueryResults/InlineQueryResultArticle.qml \
    qml/components/inlineQueryResults/InlineQueryResultAudio.qml \
    qml/components/inlineQueryResults/InlineQueryResultContact.qml \
    qml/components/inlineQueryResults/InlineQueryResultDefaultBase.qml \
    qml/components/inlineQueryResults/InlineQueryResultDocument.qml \
    qml/components/inlineQueryResults/InlineQueryResultGame.qml \
    qml/components/inlineQueryResults/InlineQueryResultLocation.qml \
    qml/components/inlineQueryResults/InlineQueryResultPhoto.qml \
    qml/components/inlineQueryResults/InlineQueryResultSticker.qml \
    qml/components/inlineQueryResults/InlineQueryResultVenue.qml \
    qml/components/inlineQueryResults/InlineQueryResultVideo.qml \
    qml/components/inlineQueryResults/InlineQueryResultVoiceNote.qml \
    qml/components/messageContent/MessageAnimatedEmoji.qml \
    qml/components/messageContent/MessageAnimation.qml \
    qml/components/messageContent/MessageAudio.qml \
    qml/components/messageContent/MessageContact.qml \
    qml/components/messageContent/MessageContentBase.qml \
    qml/components/messageContent/MessageContentFileInfoBase.qml \
    qml/components/messageContent/MessageDocument.qml \
    qml/components/messageContent/MessageGame.qml \
    qml/components/messageContent/MessageLocation.qml \
    qml/components/messageContent/MessagePhoto.qml \
    qml/components/messageContent/MessagePhotoAlbum.qml \
    qml/components/messageContent/MessagePoll.qml \
    qml/components/messageContent/MessageSticker.qml \
    qml/components/messageContent/MessageVenue.qml \
    qml/components/messageContent/MessageVideoAlbum.qml \
    qml/components/messageContent/MessageVideoNote.qml \
    qml/components/messageContent/MessageVideo.qml \
    qml/components/messageContent/MessageVoiceNote.qml \
    qml/components/messageContent/SponsoredMessage.qml \
    qml/components/messageContent/WebPagePreview.qml \
    qml/components/messageContent/mediaAlbumPage/FullscreenOverlay.qml \
    qml/components/messageContent/mediaAlbumPage/PhotoComponent.qml \
    qml/components/messageContent/mediaAlbumPage/VideoComponent.qml \
    qml/components/messageContent/mediaAlbumPage/ZoomArea.qml \
    qml/components/messageContent/mediaAlbumPage/ZoomImage.qml \
    qml/components/settingsPage/Accordion.qml \
    qml/components/settingsPage/AccordionItem.qml \
    qml/components/settingsPage/ResponsiveGrid.qml \
    qml/components/settingsPage/SettingsAppearance.qml \
    qml/components/settingsPage/SettingsBehavior.qml \
    qml/components/settingsPage/SettingsPrivacy.qml \
    qml/components/settingsPage/SettingsSession.qml \
    qml/components/settingsPage/SettingsStorage.qml \
    qml/components/settingsPage/SettingsUserProfile.qml \
    qml/js/debug.js \
    qml/js/functions.js \
    qml/pages/ChatInformationPage.qml \
    qml/pages/ChatPage.qml \
    qml/pages/ChatSelectionPage.qml \
    qml/pages/CoverPage.qml \
    qml/pages/DebugPage.qml \
    qml/pages/InitializationPage.qml \
    qml/pages/MediaAlbumPage.qml \
    qml/pages/NewChatPage.qml \
    qml/pages/OverviewPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/PollCreationPage.qml \
    qml/pages/PollResultsPage.qml \
    qml/pages/ScheduleMessageDialog.qml \
    qml/pages/ScheduledMessagesPage.qml \
    qml/pages/SearchChatsPage.qml \
    qml/pages/SettingsPage.qml \
    rpm/harbour-gtgram.changes \
    rpm/harbour-gtgram.spec \
    rpm/harbour-gtgram.yaml \
    translations/*.ts \
    bin/restart-voicecall.sh \
    voicecallplugin/harbour-gtgram-voicecall-plugin.pro \
    voicecallplugin/gtgramcallsadaptor.h \
    voicecallplugin/gtgramcallsadaptor.cpp \
    voicecallplugin/gtgramvoicecallhandler.h \
    voicecallplugin/gtgramvoicecallhandler.cpp \
    voicecallplugin/gtgramvoicecallprovider.h \
    voicecallplugin/gtgramvoicecallprovider.cpp \
    voicecallplugin/gtgramvoicecallproviderfactory.h \
    voicecallplugin/gtgramvoicecallproviderfactory.cpp \
    harbour-gtgram.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172 256x256

TRANSLATIONS += translations/harbour-gtgram-de.ts \
                translations/harbour-gtgram-es.ts \
                translations/harbour-gtgram-fi.ts \
                translations/harbour-gtgram-fr.ts \
                translations/harbour-gtgram-hu.ts \
                translations/harbour-gtgram-it.ts \
                translations/harbour-gtgram-pl.ts \
                translations/harbour-gtgram-ru.ts \
                translations/harbour-gtgram-sv.ts \
                translations/harbour-gtgram-sk.ts \
                translations/harbour-gtgram-en.ts \
                translations/harbour-gtgram-zh_CN.ts

equals(QT_ARCH, arm) {
    message(Building ARM)
    TARGET_ARCHITECTURE = armv7hl
}
equals(QT_ARCH, i386) {
    message(Building i486)
    TARGET_ARCHITECTURE = i486
}
equals(QT_ARCH, arm64){
    message(Building aarch64)
    TARGET_ARCHITECTURE = aarch64
}

# Voice calls (tgcalls + tg_owt), opt-in: qmake CONFIG+=gtgram_voicecalls
#
# TDLib only handles call *signaling* - the actual audio needs a WebRTC media
# stack, which for Telegram means tgcalls on top of tg_owt (Telegram's WebRTC
# fork). Both are vendored under vendor/ (NOT committed to git - see
# NOTICE.md for where to get them) rather than built from this .pro, because
# tg_owt is a multi-hundred-MB prebuilt static lib per architecture; only
# aarch64 has a prebuild, so this is aarch64-only for now.
CONFIG += gtgram_voicecalls
gtgram_voicecalls {
    equals(QT_ARCH, arm64) {
        message(Voice calls: tgcalls + tg_owt enabled)
        DEFINES += GTGRAM_VOICE_CALLS
        # tg_owt/tgcalls need C++17 (std::optional); force gnu++17 specifically
        # (not gnu++2a/20) since Qt 5.6's <QtCore/qfutureinterface.h> uses the
        # injected-class-name as a constructor name in a way C++20 rejects.
        QMAKE_CXXFLAGS += -std=gnu++17
        HEADERS += src/callmanager.h src/systemcallbridge.h
        SOURCES += src/callmanager.cpp src/systemcallbridge.cpp
        # Voice-only build: no camera/video capture, so use tgcalls' own no-op
        # PlatformInterface (only the desktop/mobile platforms it ships one
        # for need a real one - Sailfish isn't among them).
        SOURCES += vendor/tgcalls/upstream/tgcalls/platform/fake/FakeInterface.cpp
        HEADERS += vendor/tgcalls/upstream/tgcalls/platform/fake/FakeInterface.h
        # libtg_owt.a was built with H264 support (references openh264's
        # encoder), which the SDK target doesn't provide - it's only reached
        # by video calls, so stub it out rather than bundling openh264.
        SOURCES += src/openh264_stub.cpp
        include(vendor/tg_owt/abseil-cpp.pri)
        include(vendor/tg_owt/tg_owt.pri)
        include(vendor/tgcalls/tgcalls.pri)
        # libtg_owt.a was built against OpenSSL 1.1 (references versioned
        # symbols like SSL_get_peer_certificate@@OPENSSL_1_1_0), so it must
        # link that exact soname rather than the SDK's default OpenSSL 3.x.
        # bundledlibs/<arch>/ ships app-private copies of the codec libs
        # because SFOS target sonames drift across OS versions (e.g. SFOS 5.2
        # ships libvpx.so.12, not the .so.9 this was built against);
        # -rpath-link resolves their further transitive deps (theora/vorbis/
        # webp) without adding them as direct link dependencies.
        LIBS += -L$$PWD/bundledlibs/$${TARGET_ARCHITECTURE} \
                -Wl,-rpath-link,$$PWD/bundledlibs/$${TARGET_ARCHITECTURE} \
                -l:libssl.so.1.1 -l:libcrypto.so.1.1 -lz -lpulse \
                -l:libopus.so.0 \
                -l:libvpx.so.9 \
                -l:libavcodec.so.59 \
                -l:libavutil.so.57
    } else {
        warning(gtgram_voicecalls requested but QT_ARCH is not arm64: scope ignored)
    }
}

INCLUDEPATH += $$PWD/tdlib/include
DEPENDPATH += $$PWD/tdlib/include
LIBS += -L$$PWD/tdlib/$${TARGET_ARCHITECTURE}/lib/ -ltdjson
telegram.files = $$PWD/tdlib/$${TARGET_ARCHITECTURE}/lib
telegram.path = /usr/share/$${TARGET}

gtgram_voicecalls {
    equals(QT_ARCH, arm64) {
        # Ships alongside libtdjson.so in the same directory, which the
        # sailfishapp CONFIG already puts on the binary's rpath.
        bundledlibs.files = $$files($$PWD/bundledlibs/$${TARGET_ARCHITECTURE}/*.so.*)
        bundledlibs.path = /usr/share/$${TARGET}/lib
        INSTALLS += bundledlibs

        # Restarts voicecall-manager after install so it picks up
        # voicecallplugin/ (built and packaged separately - see rpm/*.spec).
        voicecallrestart.files = $$PWD/bin/restart-voicecall.sh
        voicecallrestart.path = /usr/share/$${TARGET}/bin
        INSTALLS += voicecallrestart
    }
}

gui.files = qml
gui.path = /usr/share/$${TARGET}

images.files = images
images.path = /usr/share/$${TARGET}

ICONPATH = /usr/share/icons/hicolor

86.png.path = $${ICONPATH}/86x86/apps/
86.png.files += icons/86x86/harbour-gtgram.png

108.png.path = $${ICONPATH}/108x108/apps/
108.png.files += icons/108x108/harbour-gtgram.png

128.png.path = $${ICONPATH}/128x128/apps/
128.png.files += icons/128x128/harbour-gtgram.png

172.png.path = $${ICONPATH}/172x172/apps/
172.png.files += icons/172x172/harbour-gtgram.png

256.png.path = $${ICONPATH}/256x256/apps/
256.png.files += icons/256x256/harbour-gtgram.png

gtgram.desktop.path = /usr/share/applications/
gtgram.desktop.files = harbour-gtgram.desktop

database.files = db
database.path = /usr/share/$${TARGET}

INSTALLS += telegram 86.png 108.png 128.png 172.png 256.png \
            gtgram.desktop gui images database

HEADERS += \
    src/appsettings.h \
    src/boolfiltermodel.h \
    src/chatpermissionfiltermodel.h \
    src/chatlistmodel.h \
    src/chatmodel.h \
    src/contactsmodel.h \
    src/dbusadaptor.h \
    src/dbusinterface.h \
    src/debuglog.h \
    src/debuglogjs.h \
    src/emojisearchworker.h \
    src/gtgramutils.h \
    src/knownusersmodel.h \
    src/mceinterface.h \
    src/namedaction.h \
    src/notificationmanager.h \
    src/processlauncher.h \
    src/stickermanager.h \
    src/tdlibfile.h \
    src/tdlibreceiver.h \
    src/tdlibsecrets.h \
    src/tdlibwrapper.h \
    src/textfiltermodel.h \
    src/tgsplugin.h

# https://github.com/Samsung/rlottie.git

RLOTTIE_CONFIG = $${PWD}/rlottie/src/vector/config.h
PRE_TARGETDEPS += $${RLOTTIE_CONFIG}
QMAKE_EXTRA_TARGETS += rlottie_config

rlottie_config.target = $${RLOTTIE_CONFIG}
rlottie_config.commands = touch $${RLOTTIE_CONFIG} # Empty config is fine

DEFINES += LOTTIE_THREAD_SUPPORT

INCLUDEPATH += \
    rlottie/inc \
    rlottie/src/vector \
    rlottie/src/vector/freetype

SOURCES += \
    rlottie/src/lottie/lottieanimation.cpp \
    rlottie/src/lottie/lottieitem.cpp \
    rlottie/src/lottie/lottieitem_capi.cpp \
    rlottie/src/lottie/lottiekeypath.cpp \
    rlottie/src/lottie/lottieloader.cpp \
    rlottie/src/lottie/lottiemodel.cpp \
    rlottie/src/lottie/lottieparser.cpp \
    rlottie/src/lottie/zip/zip.cpp

SOURCES += \
    rlottie/src/vector/freetype/v_ft_math.cpp \
    rlottie/src/vector/freetype/v_ft_raster.cpp \
    rlottie/src/vector/freetype/v_ft_stroker.cpp \
    rlottie/src/vector/stb/stb_image.cpp \
    rlottie/src/vector/varenaalloc.cpp \
    rlottie/src/vector/vbezier.cpp \
    rlottie/src/vector/vbitmap.cpp \
    rlottie/src/vector/vbrush.cpp \
    rlottie/src/vector/vdasher.cpp \
    rlottie/src/vector/vdrawable.cpp \
    rlottie/src/vector/vdrawhelper.cpp \
    rlottie/src/vector/vdrawhelper_common.cpp \
    rlottie/src/vector/vdrawhelper_neon.cpp \
    rlottie/src/vector/vdrawhelper_sse2.cpp \
    rlottie/src/vector/vmatrix.cpp \
    rlottie/src/vector/vimageloader.cpp \
    rlottie/src/vector/vinterpolator.cpp \
    rlottie/src/vector/vpainter.cpp \
    rlottie/src/vector/vpath.cpp \
    rlottie/src/vector/vpathmesure.cpp \
    rlottie/src/vector/vraster.cpp \
    rlottie/src/vector/vrle.cpp

NEON = $$system(g++ -dM -E -x c++ - < /dev/null | grep __ARM_NEON__)
SSE2 = $$system(g++ -dM -E -x c++ - < /dev/null | grep __SSE2__)

!isEmpty(NEON) {
    message(Using NEON render functions)
    SOURCES += rlottie/src/vector/pixman/pixman-arm-neon-asm.S
} else {
    !isEmpty(SSE2) {
        message(Using SSE2 render functions)
        SOURCES += rlottie/src/vector/vdrawhelper_sse2.cpp
    } else {
        message(Using default render functions)
    }
}
