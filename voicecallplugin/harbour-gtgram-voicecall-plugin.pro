TEMPLATE = lib
TARGET = gtgram-voicecall-plugin
QT = core dbus
CONFIG += plugin c++11
CONFIG -= debug_and_release

# voicecall API headers/lib, from the voicecall-qt5-devel package (installed
# in the SDK target via: sfdk tools package-install <target> voicecall-qt5-devel).
INCLUDEPATH += /usr/include/voicecall
LIBS += -lvoicecall

HEADERS += \
    gtgramcallsadaptor.h \
    gtgramvoicecallhandler.h \
    gtgramvoicecallprovider.h \
    gtgramvoicecallproviderfactory.h

SOURCES += \
    gtgramcallsadaptor.cpp \
    gtgramvoicecallhandler.cpp \
    gtgramvoicecallprovider.cpp \
    gtgramvoicecallproviderfactory.cpp

equals(QT_ARCH, arm): target.path = /usr/lib/voicecall/plugins
else: target.path = /usr/lib64/voicecall/plugins
INSTALLS += target
