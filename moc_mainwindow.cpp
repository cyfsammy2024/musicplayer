/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "on_playButton_clicked",
        "",
        "on_pauseButton_clicked",
        "on_stopButton_clicked",
        "on_previousButton_clicked",
        "on_nextButton_clicked",
        "on_volumeSlider_valueChanged",
        "value",
        "on_progressBar_valueChanged",
        "on_playModeButton_clicked",
        "on_actionImport_File_triggered",
        "on_actionImport_Folder_triggered",
        "on_actionExport_Playlist_triggered",
        "on_actionImport_Playlist_triggered",
        "on_clearPlaylistButton_clicked",
        "on_actionEqualizer_triggered",
        "on_actionLyrics_triggered",
        "on_actionEditTags_triggered",
        "onPositionChanged",
        "position",
        "onDurationChanged",
        "duration",
        "onStateChanged",
        "QMediaPlayer::PlaybackState",
        "state",
        "onPlaylistChanged",
        "onCurrentMediaChanged",
        "QUrl",
        "source",
        "onPlaylistItemDoubleClicked",
        "QModelIndex",
        "index",
        "onLyricsChanged",
        "lyrics",
        "onAllLyricsChanged",
        "QList<std::pair<qint64,QString>>",
        "lyricsList",
        "onCurrentIndexChanged",
        "onEqualizerSettingsChanged",
        "QList<int>",
        "settings",
        "onPlaylistContextMenu",
        "QPoint",
        "pos",
        "onRemoveSelected",
        "on_actionCheck_Update_triggered",
        "on_actionAbout_triggered"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_playButton_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pauseButton_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_stopButton_clicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_previousButton_clicked'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_nextButton_clicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_volumeSlider_valueChanged'
        QtMocHelpers::SlotData<void(int)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Slot 'on_progressBar_valueChanged'
        QtMocHelpers::SlotData<void(int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Slot 'on_playModeButton_clicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionImport_File_triggered'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionImport_Folder_triggered'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionExport_Playlist_triggered'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionImport_Playlist_triggered'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_clearPlaylistButton_clicked'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionEqualizer_triggered'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionLyrics_triggered'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionEditTags_triggered'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPositionChanged'
        QtMocHelpers::SlotData<void(qint64)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 20 },
        }}),
        // Slot 'onDurationChanged'
        QtMocHelpers::SlotData<void(qint64)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 22 },
        }}),
        // Slot 'onStateChanged'
        QtMocHelpers::SlotData<void(QMediaPlayer::PlaybackState)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 24, 25 },
        }}),
        // Slot 'onPlaylistChanged'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCurrentMediaChanged'
        QtMocHelpers::SlotData<void(const QUrl &)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 28, 29 },
        }}),
        // Slot 'onPlaylistItemDoubleClicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 31, 32 },
        }}),
        // Slot 'onLyricsChanged'
        QtMocHelpers::SlotData<void(const QString &)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 34 },
        }}),
        // Slot 'onAllLyricsChanged'
        QtMocHelpers::SlotData<void(const QList<QPair<qint64,QString>> &)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 36, 37 },
        }}),
        // Slot 'onCurrentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(38, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 32 },
        }}),
        // Slot 'onEqualizerSettingsChanged'
        QtMocHelpers::SlotData<void(const QList<int> &)>(39, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 40, 41 },
        }}),
        // Slot 'onPlaylistContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(42, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 43, 44 },
        }}),
        // Slot 'onRemoveSelected'
        QtMocHelpers::SlotData<void()>(45, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionCheck_Update_triggered'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actionAbout_triggered'
        QtMocHelpers::SlotData<void()>(47, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_playButton_clicked(); break;
        case 1: _t->on_pauseButton_clicked(); break;
        case 2: _t->on_stopButton_clicked(); break;
        case 3: _t->on_previousButton_clicked(); break;
        case 4: _t->on_nextButton_clicked(); break;
        case 5: _t->on_volumeSlider_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->on_progressBar_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->on_playModeButton_clicked(); break;
        case 8: _t->on_actionImport_File_triggered(); break;
        case 9: _t->on_actionImport_Folder_triggered(); break;
        case 10: _t->on_actionExport_Playlist_triggered(); break;
        case 11: _t->on_actionImport_Playlist_triggered(); break;
        case 12: _t->on_clearPlaylistButton_clicked(); break;
        case 13: _t->on_actionEqualizer_triggered(); break;
        case 14: _t->on_actionLyrics_triggered(); break;
        case 15: _t->on_actionEditTags_triggered(); break;
        case 16: _t->onPositionChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 17: _t->onDurationChanged((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1]))); break;
        case 18: _t->onStateChanged((*reinterpret_cast<std::add_pointer_t<QMediaPlayer::PlaybackState>>(_a[1]))); break;
        case 19: _t->onPlaylistChanged(); break;
        case 20: _t->onCurrentMediaChanged((*reinterpret_cast<std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 21: _t->onPlaylistItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 22: _t->onLyricsChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 23: _t->onAllLyricsChanged((*reinterpret_cast<std::add_pointer_t<QList<std::pair<qint64,QString>>>>(_a[1]))); break;
        case 24: _t->onCurrentIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 25: _t->onEqualizerSettingsChanged((*reinterpret_cast<std::add_pointer_t<QList<int>>>(_a[1]))); break;
        case 26: _t->onPlaylistContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 27: _t->onRemoveSelected(); break;
        case 28: _t->on_actionCheck_Update_triggered(); break;
        case 29: _t->on_actionAbout_triggered(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 25:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<int> >(); break;
            }
            break;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 30)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 30;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 30)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 30;
    }
    return _id;
}
QT_WARNING_POP
