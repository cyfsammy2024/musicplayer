QT       += core gui widgets multimedia multimediawidgets

TARGET = musicplayer
TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp \
    playlistmodel.cpp \
    player.cpp \
    lyricsmanager.cpp \
    equalizerwindow.cpp \
    lyricswindow.cpp

HEADERS += \
    mainwindow.h \
    playlistmodel.h \
    player.h \
    lyricsmanager.h \
    equalizerwindow.h \
    lyricswindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

CONFIG += c++17

# FFmpeg includes and libraries (disabled - not used)
# INCLUDEPATH += /usr/include/x86_64-linux-gnu
# LIBS += -L/usr/lib/x86_64-linux-gnu -lavutil -lavcodec -lavformat -lswresample
