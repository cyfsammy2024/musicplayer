#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDir>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_player(new Player(this)),
    m_playlistModel(new PlaylistModel(this)),
    m_currentPlayMode(Player::Sequential),
    m_updatingProgressBar(false),
    m_lyricsList(),
    m_equalizerWindow(nullptr),
    m_lyricsWindow(nullptr),
    m_tagEditorWindow(nullptr),
    m_equalizerDock(nullptr),
    m_lyricsDock(nullptr),
    m_tagEditorDock(nullptr)
{
    ui->setupUi(this);

    // 三个独立 dock，从上到下依次排列在右侧
    m_lyricsWindow = new LyricsWindow(this);
    m_lyricsDock = new QDockWidget(QStringLiteral("歌词"), this);
    m_lyricsDock->setObjectName("LyricsDock");
    m_lyricsDock->setWidget(m_lyricsWindow);
    m_lyricsDock->setMinimumWidth(400);
    m_lyricsDock->setFeatures(QDockWidget::DockWidgetMovable |
                              QDockWidget::DockWidgetFloatable |
                              QDockWidget::DockWidgetClosable);
    m_lyricsDock->hide();

    m_equalizerWindow = nullptr;
    m_equalizerDock = nullptr;

    m_tagEditorWindow = nullptr;
    m_tagEditorDock = new QDockWidget(QStringLiteral("标签编辑"), this);
    m_tagEditorDock->setObjectName("TagEditorDock");
    m_tagEditorDock->setFeatures(QDockWidget::DockWidgetMovable |
                                  QDockWidget::DockWidgetFloatable |
                                  QDockWidget::DockWidgetClosable);
    m_tagEditorDock->hide();

    // 依次添加：歌词（上）、标签编辑器（下）
    addDockWidget(Qt::RightDockWidgetArea, m_lyricsDock);
    splitDockWidget(m_lyricsDock, m_tagEditorDock, Qt::Vertical);

    m_player->setMediaList(m_playlistModel->mediaList());

    ui->playlistView->setModel(m_playlistModel);
    ui->playlistView->verticalHeader()->hide(); // 隐藏自带行号列，使用模型中的序号列
    ui->playlistView->setColumnWidth(PlaylistModel::Number, 50);
    ui->playlistView->setColumnWidth(PlaylistModel::FileName, 400);
    ui->playlistView->setColumnWidth(PlaylistModel::Duration, 100);
    ui->playlistView->setColumnWidth(PlaylistModel::FileSize, 100);

    // 选中按整行 + 扩展多选（Ctrl/Shift），并启用右键自定义菜单（删除选中）
    ui->playlistView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->playlistView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->playlistView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_player, &Player::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_player, &Player::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_player, &Player::playbackStateChanged, this, &MainWindow::onStateChanged);
    connect(m_player, &Player::sourceChanged, this, &MainWindow::onCurrentMediaChanged);
    connect(m_player, &Player::currentIndexChanged, this, &MainWindow::onCurrentIndexChanged);
    connect(m_player, &Player::lyricsChanged, this, &MainWindow::onLyricsChanged);
    connect(m_player->lyricsManager(), &LyricsManager::allLyricsChanged, this, &MainWindow::onAllLyricsChanged);
    connect(ui->playlistView, &QTableView::doubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(ui->playlistView, &QWidget::customContextMenuRequested, this, &MainWindow::onPlaylistContextMenu);

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
    m_currentPosition = position;
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
        m_lyricsWindow->setLyricsText("歌词将显示在这里");
    } else {
        m_lyricsWindow->setLyricsText(allLyrics);
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

    // 保存停靠窗口布局
    settings.setValue("dockLayout", QMainWindow::saveState());
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

    // 恢复停靠窗口布局
    QByteArray dockLayout = settings.value("dockLayout").toByteArray();
    if (!dockLayout.isEmpty()) {
        QMainWindow::restoreState(dockLayout);
    }
}

void MainWindow::onLyricsChanged(const QString &lyrics)
{
    if (lyrics.isEmpty()) {
        m_lyricsWindow->setLyricsText("歌词将显示在这里");
        return;
    }

    // 按当前播放时间定位歌词索引（m_lyricsList 来自 getAllLyrics()，按时间升序）。
    // 不再按文本精确匹配——LRC 中重复行（同一句出现在多个时间点）会卡在首处，
    // 按时间定位可命中当前播放段落对应的正确索引。
    int currentIndex = -1;
    for (int i = 0; i < m_lyricsList.size(); ++i) {
        if (m_lyricsList[i].first <= m_currentPosition) {
            currentIndex = i;
        } else {
            break; // 遇到未来时间戳即停（列表升序）
        }
    }

    // 构建歌词HTML，使用表格布局实现上下左右居中
    QString lyricsHtml = "<html><body style=\"margin: 0; padding: 0; height: 100%; display: table; width: 100%;\"><div style=\"display: table-cell; vertical-align: middle; text-align: center;\">";

    if (currentIndex != -1) {
        // 只显示当前歌词加上前后各2句，当前歌词字号放大一倍并高亮
        int startIndex = qMax(0, currentIndex - 2);
        int endIndex = qMin(currentIndex + 2, static_cast<int>(m_lyricsList.size()) - 1);
        for (int i = startIndex; i <= endIndex; ++i) {
            if (i == currentIndex) {
                lyricsHtml += "<p style=\"font-size: 2em; color: red; margin: 10px 0; white-space: pre-wrap;\">" + m_lyricsList[i].second + "</p>";
            } else {
                lyricsHtml += "<p style=\"margin: 10px 0; white-space: pre-wrap;\">" + m_lyricsList[i].second + "</p>";
            }
        }
    } else {
        // 列表非空但无时间戳 <= 当前位置（理论上不应到达，兜底显示全部歌词）
        for (const auto &lyric : m_lyricsList) {
            lyricsHtml += "<p style=\"margin: 10px 0; white-space: pre-wrap;\">" + lyric.second + "</p>";
        }
    }
    lyricsHtml += "</div></body></html>";

    // white-space: pre-wrap 保留纯文本歌词（无时间戳的单条目）的换行，
    // 避免 HTML 默认空白折叠把多行压成一段红字。
    m_lyricsWindow->setLyricsHtml(lyricsHtml);
}

void MainWindow::on_clearPlaylistButton_clicked()
{
    // 清空播放列表
    m_playlistModel->clear();
    m_player->setMediaList(QList<QUrl>());
    
    // 停止播放
    m_player->stop();
    
    // 清空歌词显示
    m_lyricsWindow->setLyricsText("歌词将显示在这里");
    m_lyricsList.clear();
    
    // 重置当前播放歌曲标签
    ui->currentSongLabel->setText("当前播放: 无");
    
    // 重置进度条
    ui->progressBar->setValue(0);
    ui->timeLabel->setText("0:00 / 0:00");
}

void MainWindow::onRemoveSelected()
{
    QItemSelectionModel *sel = ui->playlistView->selectionModel();
    if (!sel) return;
    QModelIndexList idxs = sel->selectedRows();
    if (idxs.isEmpty()) return;

    QList<int> rows;
    for (const QModelIndex &i : idxs) rows << i.row();

    // 先在 Player 上删除并处理当前索引/切歌，再在模型上删除刷新 UI。
    // 两边各自维护 m_mediaList 副本，用同一组（删除前）行号删除，最终一致。
    m_player->removeMediaRows(rows);
    m_playlistModel->removeRowsAt(rows);

    // 列表被删空时重置 UI（参考 on_clearPlaylistButton_clicked）
    if (m_playlistModel->mediaList().isEmpty()) {
        m_lyricsWindow->setLyricsText("歌词将显示在这里");
        m_lyricsList.clear();
        ui->currentSongLabel->setText("当前播放: 无");
        ui->progressBar->setValue(0);
        ui->timeLabel->setText("0:00 / 0:00");
    }
}

void MainWindow::onPlaylistContextMenu(const QPoint &pos)
{
    QItemSelectionModel *sel = ui->playlistView->selectionModel();
    QModelIndex idx = ui->playlistView->indexAt(pos);
    // 右键在未选中的有效行上：单选该行，使"删除选中"作用于右键所在行
    if (idx.isValid() && sel && !sel->isSelected(idx)) {
        sel->clearSelection();
        sel->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    const bool hasSelection = sel && !sel->selectedRows().isEmpty();
    if (!idx.isValid() && !hasSelection) return; // 空白处且无选中不弹菜单

    QMenu menu(this);
    QAction *delAct = menu.addAction("删除选中");
    delAct->setEnabled(hasSelection);
    menu.addSeparator();
    QAction *editTagsAct = menu.addAction("编辑标签");
    editTagsAct->setEnabled(idx.isValid());

    QAction *chosen = menu.exec(ui->playlistView->viewport()->mapToGlobal(pos));
    if (chosen == delAct) onRemoveSelected();
    else if (chosen == editTagsAct && idx.isValid()) {
        QList<QUrl> list = m_playlistModel->mediaList();
        int row = idx.row();
        if (row >= 0 && row < list.size()) {
            QString path = list.at(row).toLocalFile();
            if (!path.isEmpty()) {
                on_actionEditTags_triggered_forPath(path, false);
            }
        }
    }
}

void MainWindow::on_actionCheck_Update_triggered()
{
    // 打开 GitHub releases 页面（仓库地址来自 git remote）
    QDesktopServices::openUrl(QUrl("https://github.com/cyfsammy2024/musicplayer/releases"));
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "关于",
        "音乐播放器 " APP_VERSION "\n\n"
        "基于 Qt6 (C++) 的桌面音乐播放器\n"
        "支持格式：MP3 / WAV / FLAC / OGG / M4A\n\n"
        "功能：内嵌歌词解析、播放列表管理、均衡器、播放模式切换");
}

void MainWindow::on_actionEqualizer_triggered()
{
    if (!m_equalizerWindow) {
        m_equalizerWindow = new EqualizerWindow(this);
        connect(m_equalizerWindow, &EqualizerWindow::equalizerSettingsChanged, this, &MainWindow::onEqualizerSettingsChanged);

        m_equalizerDock = new QDockWidget(QStringLiteral("均衡器"), this);
        m_equalizerDock->setObjectName("EqualizerDock");
        m_equalizerDock->setWidget(m_equalizerWindow);
        m_equalizerDock->setFeatures(QDockWidget::DockWidgetMovable |
                                     QDockWidget::DockWidgetFloatable |
                                     QDockWidget::DockWidgetClosable);
        // 均衡器在歌词下方、标签编辑器上方
        splitDockWidget(m_lyricsDock, m_equalizerDock, Qt::Vertical);
        splitDockWidget(m_equalizerDock, m_tagEditorDock, Qt::Vertical);
        m_equalizerDock->hide();
    }
    if (m_equalizerDock->isVisible()) {
        m_equalizerDock->hide();
        if (m_sizeBeforeEqualizer.isValid()) {
            QTimer::singleShot(0, this, [this]() { resize(m_sizeBeforeEqualizer); });
        }
    } else {
        if (!m_equalizerDock->isFloating()) {
            m_sizeBeforeEqualizer = size();
        }
        m_equalizerDock->show();
        m_equalizerDock->raise();
        m_equalizerDock->activateWindow();
    }
}

void MainWindow::on_actionLyrics_triggered()
{
    if (m_lyricsDock->isVisible()) {
        m_lyricsDock->hide();
        if (m_sizeBeforeLyrics.isValid()) {
            QTimer::singleShot(0, this, [this]() { resize(m_sizeBeforeLyrics); });
        }
    } else {
        if (!m_lyricsDock->isFloating()) {
            m_sizeBeforeLyrics = size();
        }
        m_lyricsDock->show();
        m_lyricsDock->raise();
        m_lyricsDock->activateWindow();
    }
}

void MainWindow::on_actionEditTags_triggered()
{
    on_actionEditTags_triggered_forPath(m_playlistModel->mediaList().isEmpty() ? QString()
                              : m_playlistModel->mediaList().at(m_player->currentIndex()).toLocalFile());
}

void MainWindow::on_actionEditTags_triggered_forPath(const QString &path, bool toggle)
{
    if (path.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("标签编辑"),
                             QStringLiteral("播放列表为空，没有可编辑的曲目。"));
        return;
    }
    if (!m_tagEditorWindow) {
        m_tagEditorWindow = new TagEditorWindow(QString(), this);
        connect(m_tagEditorWindow, &TagEditorWindow::tagsSaved, this,
                [this](const QString &filePath) {
                    int cur = m_player->currentIndex();
                    if (cur >= 0 && cur < m_playlistModel->mediaList().size() &&
                        m_playlistModel->mediaList().at(cur).toLocalFile() == filePath) {
                        m_player->lyricsManager()->loadLyricsFromMedia(QUrl::fromLocalFile(filePath));
                    }
                });
        m_tagEditorDock->setWidget(m_tagEditorWindow);
    }
    m_tagEditorWindow->reload(path);

    const bool wasVisible = m_tagEditorDock->isVisible();
    if (toggle) {
        // 菜单触发：切换显示/隐藏
        if (wasVisible) {
            m_tagEditorDock->hide();
            if (m_sizeBeforeTagEditor.isValid()) {
                QTimer::singleShot(0, this, [this]() { resize(m_sizeBeforeTagEditor); });
            }
        } else {
            if (!m_tagEditorDock->isFloating()) {
                m_sizeBeforeTagEditor = size();
            }
            m_tagEditorDock->show();
            m_tagEditorDock->raise();
            m_tagEditorDock->activateWindow();
        }
    } else {
        // 右键触发：始终显示并提升到前台
        if (!m_tagEditorDock->isVisible()) {
            if (!m_tagEditorDock->isFloating()) {
                m_sizeBeforeTagEditor = size();
            }
            m_tagEditorDock->show();
        }
        m_tagEditorDock->raise();
        m_tagEditorDock->activateWindow();
    }
}

void MainWindow::onEqualizerSettingsChanged(const QList<int> &settings)
{
    // 将均衡器设置应用到播放器
    m_player->setEqualizerSettings(settings);
}
