QT       += core gui widgets multimedia multimediawidgets

TARGET = musicplayer
TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp \
    playlistmodel.cpp \
    player.cpp \
    lyricsmanager.cpp \
    equalizerwindow.cpp \
    lyricswindow.cpp \
    tagutils.cpp \
    tagmanager.cpp \
    tageditorwindow.cpp

HEADERS += \
    mainwindow.h \
    playlistmodel.h \
    player.h \
    lyricsmanager.h \
    equalizerwindow.h \
    lyricswindow.h \
    tagutils.h \
    tagmanager.h \
    tageditorwindow.h

FORMS += \
    mainwindow.ui

CONFIG += c++17

# 歌词编码转换的链接差异：Linux 由 glibc 内置 iconv；Windows 走系统 API（不使用 iconv）；
# macOS 提供独立的 libiconv，需要显式链接
unix:!linux: LIBS += -liconv

# 版本号统一管理：从根目录 VERSION 文件读取，注入为 APP_VERSION 宏（代码中直接引用）
VERSION_FILE = $$_PRO_FILE_PWD_/VERSION
exists($$VERSION_FILE): APP_VER = $$first($$list($$cat($$VERSION_FILE)))
!exists($$VERSION_FILE): APP_VER = unknown
DEFINES += APP_VERSION=\\\"$$APP_VER\\\"

# FFmpeg includes and libraries (disabled - not used)
# INCLUDEPATH += /usr/include/x86_64-linux-gnu
# LIBS += -L/usr/lib/x86_64-linux-gnu -lavutil -lavcodec -lavformat -lswresample
