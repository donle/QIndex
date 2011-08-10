# -------------------------------------------------
# Project created by QtCreator 2010-06-13T04:26:52
# -------------------------------------------------
TARGET = QSanguosha
QT += network sql
TEMPLATE = app
CONFIG += warn_on audio joystick qaxcontainer
SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    src/button.cpp \
    src/settings.cpp \
    src/startscene.cpp \
    src/roomscene.cpp \
    src/photo.cpp \
    src/dashboard.cpp \
    src/card.cpp \
    src/pixmap.cpp \
    src/general.cpp \
    src/server.cpp \
    src/engine.cpp \
    src/connectiondialog.cpp \
    src/client.cpp \
    src/carditem.cpp \
    src/room.cpp \
    src/generaloverview.cpp \
    src/player.cpp \
    src/skill.cpp \
    src/cardoverview.cpp \
    src/serverplayer.cpp \
    src/clientplayer.cpp \
    src/standard-cards.cpp \
    src/standard.cpp \
    src/gamerule.cpp \
    src/playercarddialog.cpp \
    src/roomthread.cpp \
    src/optionbutton.cpp \
    src/maneuvering.cpp \
    src/distanceviewdialog.cpp \
    src/configdialog.cpp \
    src/clientlogbox.cpp \
    src/ai.cpp \
    src/aux-skills.cpp \
    src/choosegeneraldialog.cpp \
    src/nativesocket.cpp \
    src/recorder.cpp \
    src/scenario.cpp \
    src/detector.cpp \
    src/clientstruct.cpp \
    src/banpairdialog.cpp \
    src/scenario-overview.cpp \
    src/challengemode.cpp \
    src/joypackage.cpp \
    src/rolecombobox.cpp \
    swig/sanguosha_wrap.cxx \
    src/lua-wrapper.cpp \
    src/window.cpp \
    src/contestdb.cpp \
    src/roomthread3v3.cpp \
    src/cardcontainer.cpp \
    src/roomthread1v1.cpp \
    src/cardeditor.cpp \
    src/generalselector.cpp \
    src/packagingeditor.cpp \
    src/boss-mode-scenario.cpp \
    src/index-package.cpp \
    src/index-cards.cpp
HEADERS += src/mainwindow.h \
    src/button.h \
    src/settings.h \
    src/startscene.h \
    src/roomscene.h \
    src/photo.h \
    src/dashboard.h \
    src/card.h \
    src/pixmap.h \
    src/general.h \
    src/server.h \
    src/engine.h \
    src/connectiondialog.h \
    src/client.h \
    src/carditem.h \
    src/room.h \
    src/generaloverview.h \
    src/player.h \
    src/skill.h \
    src/cardoverview.h \
    src/serverplayer.h \
    src/clientplayer.h \
    src/standard.h \
    src/package.h \
    src/gamerule.h \
    src/playercarddialog.h \
    src/roomthread.h \
    src/optionbutton.h \
    src/maneuvering.h \
    src/distanceviewdialog.h \
    src/configdialog.h \
    src/clientlogbox.h \
    src/ai.h \
    src/aux-skills.h \
    src/choosegeneraldialog.h \
    src/socket.h \
    src/nativesocket.h \
    src/recorder.h \
    src/scenario.h \
    src/detector.h \
    src/clientstruct.h \
    src/banpairdialog.h \
    src/scenario-overview.h \
    src/challengemode.h \
    src/joypackage.h \
    src/rolecombobox.h \
    src/standard-equips.h \
    src/structs.h \
    src/lua-wrapper.h \
    src/window.h \
    src/contestdb.h \
    src/roomthread3v3.h \
    src/cardcontainer.h \
    src/roomthread1v1.h \
    src/cardeditor.h \
    src/generalselector.h \
    src/packagingeditor.h \
    src/boss-mode-scenario.h \
    src/index-package.h \
    src/index-cards.h

FORMS += src/mainwindow.ui \
    src/connectiondialog.ui \
    src/generaloverview.ui \
    src/cardoverview.ui \
    src/configdialog.ui


INCLUDEPATH += include/lua
INCLUDEPATH += include
INCLUDEPATH += src

win32{
    RC_FILE += resource/icon.rc
    LIBS += -L. -llua -lm
}

unix {
    LIBS += -lm -llua
}

CONFIG(audio){
    DEFINES += AUDIO_SUPPORT
    INCLUDEPATH += include/irrKlang
    win32: LIBS += irrKlang.lib
    unix: LIBS += -lphonon
}

CONFIG(joystick){
    DEFINES += JOYSTICK_SUPPORT
    HEADERS += src/joystick.h
    SOURCES += src/joystick.cpp
    win32: LIBS += -lplibjs -lplibul -lwinmm
    unix: LIBS += -lplibjs -lplibul
}

TRANSLATIONS += sanguosha.ts