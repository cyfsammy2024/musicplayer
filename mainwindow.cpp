#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDir>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_player(new Player(this)),
    m_playlistModel(new PlaylistModel(this)),
    m_currentPlayMode(Player::Sequential),
    m_updatingProgressBar(false),
    m_lyricsList(),
    m_equalizerWindow(nullptr)
{
    ui->setupUi(this);

    m_player->setMediaList(m_playlistModel->mediaList());

    ui->playlistView->setModel(m_playlistModel);
    ui->playlistView->setColumnWidth(PlaylistModel::Number, 50);
    ui->playlistView->setColumnWidth(PlaylistModel::FileName, 400);
    ui->playlistView->setColumnWidth(PlaylistModel::Duration, 100);
    ui->playlistView->setColumnWidth(PlaylistModel::FileSize, 100);

    connect(m_player, &Player::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &Player::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &Player::playbackStateChanged, this, &MainWindow::onStateChanged);
    connect(m_player, &Player::sourceChanged, this, &MainWindow::onCurrentMediaChanged);
    connect(m_player, &Player::currentIndexChanged, this, &MainWindow::onCurrentIndexChanged);
    connect(m_player, &Player::lyricsChanged, this, &MainWindow::onLyricsChanged);
    connect(m_player->lyricsManager(), &LyricsManager::allLyricsChanged, this, &MainWindow::onAllLyricsChanged);
    connect(ui->playlistView, &QTableView::doubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);

    loadState();
    updatePlayModeButton();
}

MainWindow::~MainWindow()
{
    saveState();
    delete ui;
}

void MainWindow::on_playButton_clicked()
{
    m_player->play();
}

void MainWindow::on_pauseButton_clicked()
{
    m_player->pause();
}

void MainWindow::on_stopButton_clicked()
{
    m_player->stop();
}

void MainWindow::on_previousButton_clicked()
{
    m_player->previous();
}

void MainWindow::on_nextButton_clicked()
{
    m_player->next();
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
    m_player->setVolume(value);
    ui->volumePercentageLabel->setText(QString("%1%").arg(value));
}

void MainWindow::on_progressBar_valueChanged(int value)
{
    // 只有当进度条的值是由用户手动调整的，而不是由程序自动更新的，才设置播放位置
    if (!m_updatingProgressBar) {
        m_player->setPosition(value * m_player->duration() / 100);
    }
}

void MainWindow::on_playModeButton_clicked()
{
    switch (m_currentPlayMode) {
    case Player::Sequential:
        m_currentPlayMode = Player::Random;
        break;
    case Player::Random:
        m_currentPlayMode = Player::RepeatOne;
        break;
    case Player::RepeatOne:
        m_currentPlayMode = Player::RepeatList;
        break;
    case Player::RepeatList:
        m_currentPlayMode = Player::Sequential;
        break;
    }

    m_player->setPlayMode(m_currentPlayMode);
    updatePlayModeButton();
}

void MainWindow::on_actionImport_File_triggered()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "Import Files", "", "Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a)");
    if (!fileNames.isEmpty()) {
        QList<QUrl> urls;
        for (const QString &fileName : fileNames) {
            urls.append(QUrl::fromLocalFile(fileName));
        }
        m_playlistModel->addMedia(urls);
        m_player->setMediaList(m_playlistModel->mediaList());
    }
}

void MainWindow::on_actionImport_Folder_triggered()
{
    QString folderName = QFileDialog::getExistingDirectory(this, "Import Folder");
    if (!folderName.isEmpty()) {
        QDir dir(folderName);
        QStringList nameFilters = {"*.mp3", "*.wav", "*.flac", "*.ogg", "*.m4a"};
        QStringList fileNames = dir.entryList(nameFilters, QDir::Files);
        QList<QUrl> urls;
        for (const QString &fileName : fileNames) {
            urls.append(QUrl::fromLocalFile(dir.absoluteFilePath(fileName)));
        }
        m_playlistModel->addMedia(urls);
        m_player->setMediaList(m_playlistModel->mediaList());
    }
}

void MainWindow::on_actionExport_Playlist_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Playlist", "", "Playlist Files (*.txt)");
    if (!fileName.isEmpty()) {
        m_playlistModel->exportPlaylist(fileName);
    }
}

void MainWindow::on_actionImport_Playlist_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Playlist", "", "Playlist Files (*.txt)");
    if (!fileName.isEmpty()) {
        m_playlistModel->importPlaylist(fileName);
        m_player->setMediaList(m_playlistModel->mediaList());
    }
}

void MainWindow::onPositionChanged(qint64 position)
{
    if (m_player->duration() > 0) {
        m_updatingProgressBar = true;
        int value = static_cast<int>(position * 100 / m_player->duration());
        ui->progressBar->setValue(value);
        m_updatingProgressBar = false;

        int currentSeconds = position / 1000;
        int currentMinutes = currentSeconds / 60;
        currentSeconds %= 60;

        int totalSeconds = m_player->duration() / 1000;
        int totalMinutes = totalSeconds / 60;
        totalSeconds %= 60;

        ui->timeLabel->setText(QString("%1:%2 / %3:%4").arg(currentMinutes).arg(currentSeconds, 2, 10, QChar('0')).arg(totalMinutes).arg(totalSeconds, 2, 10, QChar('0')));
    }
}

void MainWindow::onDurationChanged(qint64 /* duration */)
{
    updateProgressBar();
}

void MainWindow::onStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        ui->playButton->setEnabled(false);
        ui->pauseButton->setEnabled(true);
        break;
    case QMediaPlayer::PausedState:
        ui->playButton->setEnabled(true);
        ui->pauseButton->setEnabled(false);
        break;
    case QMediaPlayer::StoppedState:
        ui->playButton->setEnabled(true);
        ui->pauseButton->setEnabled(false);
        break;
    }
}

void MainWindow::onPlaylistChanged()
{
    m_player->setMediaList(m_playlistModel->mediaList());
}

void MainWindow::updatePlayModeButton()
{
    switch (m_currentPlayMode) {
    case Player::Sequential:
        ui->playModeButton->setText("顺序");
        break;
    case Player::Random:
        ui->playModeButton->setText("随机");
        break;
    case Player::RepeatOne:
        ui->playModeButton->setText("单曲循环");
        break;
    case Player::RepeatList:
        ui->playModeButton->setText("列表循环");
        break;
    }
}

void MainWindow::updateProgressBar()
{
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(0);
    ui->timeLabel->setText("0:00 / 0:00");
}

void MainWindow::onCurrentMediaChanged(const QUrl &source)
{
    QFileInfo fileInfo(source.toLocalFile());
    ui->currentSongLabel->setText(QString("当前播放: %1").arg(fileInfo.fileName()));
}

void MainWindow::onPlaylistItemDoubleClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        int row = index.row();
        m_player->setCurrentIndex(row);
    }
}

void MainWindow::onAllLyricsChanged(const QList<QPair<qint64, QString>> &lyricsList)
{
    m_lyricsList = lyricsList;
    // 初始显示所有歌词
    QString allLyrics;
    for (const auto &lyric : lyricsList) {
        allLyrics += lyric.second + "\n";
    }
    if (allLyrics.isEmpty()) {
        ui->lyricsView->setText("歌词将显示在这里");
    } else {
        ui->lyricsView->setText(allLyrics);
    }
}

void MainWindow::onCurrentIndexChanged(int index)
{
    // 选中播放列表中对应的项
    if (index >= 0) {
        QModelIndex modelIndex = m_playlistModel->index(index, 0);
        ui->playlistView->selectionModel()->clearSelection();
        ui->playlistView->selectionModel()->select(modelIndex, QItemSelectionModel::Select);
        ui->playlistView->scrollTo(modelIndex);
    }
}

void MainWindow::saveState()
{
    // 保存程序状态到配置文件
    QSettings settings("MusicPlayer", "MusicPlayer");
    
    // 保存播放列表
    settings.beginWriteArray("playlist");
    QList<QUrl> mediaList = m_playlistModel->mediaList();
    for (int i = 0; i < mediaList.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("url", mediaList[i].toString());
    }
    settings.endArray();
    
    // 保存当前播放索引
    settings.setValue("currentIndex", m_player->currentIndex());
    
    // 保存播放模式
    settings.setValue("playMode", m_currentPlayMode);
    
    // 保存音量
    settings.setValue("volume", m_player->volume());
    
    // 保存播放状态
    settings.setValue("playbackState", m_player->playbackState());
}

void MainWindow::loadState()
{
    // 从配置文件加载程序状态
    QSettings settings("MusicPlayer", "MusicPlayer");
    
    // 加载播放列表
    int size = settings.beginReadArray("playlist");
    QList<QUrl> mediaList;
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString urlString = settings.value("url").toString();
        mediaList.append(QUrl(urlString));
    }
    settings.endArray();
    
    if (!mediaList.isEmpty()) {
        // 清空现有播放列表，避免播放列表被翻倍
        m_playlistModel->clear();
        m_playlistModel->addMedia(mediaList);
        m_player->setMediaList(mediaList);
        
        // 加载当前播放索引
        int currentIndex = settings.value("currentIndex", -1).toInt();
        if (currentIndex >= 0 && currentIndex < mediaList.size()) {
            m_player->setCurrentIndex(currentIndex);
        }
    }
    
    // 加载播放模式
    int playMode = settings.value("playMode", Player::Sequential).toInt();
    m_currentPlayMode = static_cast<Player::PlayMode>(playMode);
    m_player->setPlayMode(m_currentPlayMode);
    
    // 加载音量
    int volume = settings.value("volume", 50).toInt();
    m_player->setVolume(volume);
    ui->volumeSlider->setValue(volume);
    ui->volumePercentageLabel->setText(QString("%1%").arg(volume));
    
    // 加载播放状态
    int playbackState = settings.value("playbackState", QMediaPlayer::StoppedState).toInt();
    if (playbackState == QMediaPlayer::PlayingState) {
        m_player->play();
    } else if (playbackState == QMediaPlayer::PausedState) {
        m_player->pause();
    }
}

void MainWindow::onLyricsChanged(const QString &lyrics)
{
    if (lyrics.isEmpty()) {
        ui->lyricsView->setText("歌词将显示在这里");
    } else {
        // 只显示当前歌词加上前后各2句，一行一句且居中，当前歌词字号放大一倍
        QString lyricsHtml;
        int currentIndex = -1;
        
        // 找到当前歌词的索引
        for (int i = 0; i < m_lyricsList.size(); ++i) {
            if (m_lyricsList[i].second == lyrics) {
                currentIndex = i;
                break;
            }
        }
        
        if (currentIndex != -1) {
            // 计算显示的起始和结束索引
            int startIndex = qMax(0, currentIndex - 2);
            int endIndex = qMin(currentIndex + 2, static_cast<int>(m_lyricsList.size()) - 1);
            
            // 构建歌词HTML，使用表格布局实现上下左右居中
            lyricsHtml = "<html><body style=\"margin: 0; padding: 0; height: 100%; display: table; width: 100%;\"><div style=\"display: table-cell; vertical-align: middle; text-align: center;\">";
            for (int i = startIndex; i <= endIndex; ++i) {
                if (i == currentIndex) {
                    // 高亮当前歌词，字号放大一倍
                    lyricsHtml += "<p style=\"font-size: 2em; color: red; margin: 10px 0;\">" + m_lyricsList[i].second + "</p>";
                } else {
                    lyricsHtml += "<p style=\"margin: 10px 0;\">" + m_lyricsList[i].second + "</p>";
                }
            }
            lyricsHtml += "</div></body></html>";
        } else {
            // 如果找不到当前歌词，显示所有歌词
            lyricsHtml = "<html><body style=\"margin: 0; padding: 0; height: 100%; display: table; width: 100%;\"><div style=\"display: table-cell; vertical-align: middle; text-align: center;\">";
            for (const auto &lyric : m_lyricsList) {
                if (lyric.second == lyrics) {
                    // 高亮当前歌词，字号放大一倍
                    lyricsHtml += "<p style=\"font-size: 2em; color: red; margin: 10px 0;\">" + lyric.second + "</p>";
                } else {
                    lyricsHtml += "<p style=\"margin: 10px 0;\">" + lyric.second + "</p>";
                }
            }
            lyricsHtml += "</div></body></html>";
        }
        
        ui->lyricsView->setHtml(lyricsHtml);
    }
}

void MainWindow::on_clearPlaylistButton_clicked()
{
    // 清空播放列表
    m_playlistModel->clear();
    m_player->setMediaList(QList<QUrl>());
    
    // 停止播放
    m_player->stop();
    
    // 清空歌词显示
    ui->lyricsView->setText("歌词将显示在这里");
    m_lyricsList.clear();
    
    // 重置当前播放歌曲标签
    ui->currentSongLabel->setText("当前播放: 无");
    
    // 重置进度条
    ui->progressBar->setValue(0);
    ui->timeLabel->setText("0:00 / 0:00");
}

void MainWindow::on_equalizerButton_clicked()
{
    if (!m_equalizerWindow) {
        m_equalizerWindow = new EqualizerWindow(this);
        connect(m_equalizerWindow, &EqualizerWindow::equalizerSettingsChanged, this, &MainWindow::onEqualizerSettingsChanged);
    }
    m_equalizerWindow->show();
}

void MainWindow::onEqualizerSettingsChanged(const QList<int> &settings)
{
    // 将均衡器设置应用到播放器
    m_player->setEqualizerSettings(settings);
}
