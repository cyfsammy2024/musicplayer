#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include "player.h"
#include "playlistmodel.h"
#include "equalizerwindow.h"
#include "lyricswindow.h"
#include "tageditorwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_playButton_clicked();
    void on_pauseButton_clicked();
    void on_stopButton_clicked();
    void on_previousButton_clicked();
    void on_nextButton_clicked();
    void on_volumeSlider_valueChanged(int value);
    void on_progressBar_valueChanged(int value);
    void on_playModeButton_clicked();
    void on_actionImport_File_triggered();
    void on_actionImport_Folder_triggered();
    void on_actionExport_Playlist_triggered();
    void on_actionImport_Playlist_triggered();
    void on_clearPlaylistButton_clicked();
    void on_actionEqualizer_triggered();
    void on_actionLyrics_triggered();
    void on_actionEditTags_triggered();

    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onStateChanged(QMediaPlayer::PlaybackState state);
    void onPlaylistChanged();
    void onCurrentMediaChanged(const QUrl &source);
    void onPlaylistItemDoubleClicked(const QModelIndex &index);
    void onLyricsChanged(const QString &lyrics);
    void onAllLyricsChanged(const QList<QPair<qint64, QString>> &lyricsList);
    void onCurrentIndexChanged(int index);
    void onEqualizerSettingsChanged(const QList<int> &settings);
    void onPlaylistContextMenu(const QPoint &pos);
    void onRemoveSelected();
    void on_actionCheck_Update_triggered();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    Player *m_player;
    PlaylistModel *m_playlistModel;
    Player::PlayMode m_currentPlayMode;
    bool m_updatingProgressBar;
    QList<QPair<qint64, QString>> m_lyricsList; // 存储所有歌词的列表
    qint64 m_currentPosition = 0; // 当前播放位置（毫秒），用于按时间定位歌词索引
    EqualizerWindow *m_equalizerWindow;
    LyricsWindow *m_lyricsWindow;
    QDockWidget *m_equalizerDock;
    QDockWidget *m_lyricsDock;
    QSize m_sizeBeforeLyrics;   // 开启歌词面板前的主窗口尺寸，关闭后恢复
    QSize m_sizeBeforeEqualizer; // 开启均衡器面板前的主窗口尺寸

    void updatePlayModeButton();
    void updateProgressBar();
    void saveState();
    void loadState();
};

#endif // MAINWINDOW_H
