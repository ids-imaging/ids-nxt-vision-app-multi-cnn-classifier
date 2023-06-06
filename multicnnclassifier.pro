CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17
QT += core widgets gui dbus

TARGET = multicnnclassifier

SOURCES += main.cpp \
    myapp.cpp \
    myvision.cpp \
    myengine.cpp \
    cnnroiconfig.cpp \
    cnnroihandler.cpp \
    myresultimage.cpp

HEADERS += myapp.h \
    myvision.h \
    myengine.h \
    cnnroiconfig.h \
    cnnroihandler.h \
    myresultimage.h

DEFINES +=
DISTFILES += README.md

#Mandatory params
NXT_SDK = 3.1.0
AVATAR = avatar.png
MANIFEST = manifest.json
TRANSLATION = translation.json

#Optional params
LICENSE = license.txt
# add files within source directory which should be shipped (e.g. libraries, configs, ...)
# To ship cnns place them in the folder cnn. They are installed automatically
DEPLOYFILES = $$PWD/qtlogging.ini $$PWD/DejaVuSans.ttf

include($(NXT_FRAMEWORK)/qmake/nxt_rio.pri)
