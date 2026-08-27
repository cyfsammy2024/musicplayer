#include "player.h"
#include <QRandomGenerator>
#include <algorithm>

Player::Player(QObject *parent) : QObject(parent),
    m_player(new QMediaPlayer(this)),
    m_audioOutput(new QAudioOutput(this)),
    m_playMode(Sequential),
    m_currentIndex(-1),
    m_lyricsManager(new LyricsManager(this))
{
    m_player->setAudioOutput(m_audioOutput);
    connect(m_player, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &Player::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &Player::playbackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &Player::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::sourceChanged, this, [this](const QUrl &url) {
        emit sourceChanged(url);
    });
}

Player::~Player()
{
    delete m_lyricsManager;
}

void Player::setMediaList(const QList<QUrl> &mediaList)
{
    m_mediaList = mediaList;
    if (!m_mediaList.isEmpty()) {
        setCurrentIndex(0); // 总是自动设置当前索引为0并开始播放音乐
    }
}

QList<QUrl> Player::mediaList() const
{
    return m_mediaList;
}

void Player::removeMediaRows(const QList<int> &rows)
{
    if (rows.isEmpty() || m_mediaList.isEmpty()) return;

    // 去重降序：从后往前删
    QList<int> sorted = rows;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    const int n = m_mediaList.size();
    for (int r : sorted) {
        if (r < 0 || r >= n) return; // 范围非法整体放弃
    }

    const int oldCur = m_currentIndex;
    const bool curRemoved = sorted.contains(oldCur);
    // < oldCur 的删除数：用于把当前索引平移到删除后的新位置
    int removedBefore = 0;
    for (int r : sorted) {
        if (r < oldCur) ++removedBefore;
    }

    for (int r : sorted) m_mediaList.removeAt(r);

    const int newSize = m_mediaList.size();
    int newCur = oldCur - removedBefore;
    if (newCur < 0) newCur = -1;
    else if (newCur >= newSize) newCur = newSize - 1; // 删到末尾越界，回退到末项
    m_currentIndex = newCur;

    if (curRemoved) {
        // 删除的是当前播放项：切到"下一首"（已位于 newCur 位置）并继续播放
        if (newCur >= 0) {
            m_player->setSource(m_mediaList[newCur]);
            m_lyricsManager->loadLyricsFromMedia(m_mediaList[newCur]);
            m_player->play();
        } else {
            // 列表被删空
            m_player->stop();
        }
    }
    // 未删当前项：source 未变，仅索引调整（不打断播放），刷新选中行
    emit currentIndexChanged(m_currentIndex);
}

void Player::play()
{
    m_player->play();
}

void Player::pause()
{
    m_player->pause();
}

void Player::stop()
{
    m_player->stop();
}

void Player::next()
{
    int count = m_mediaList.size();
    if (count == 0) return;

    switch (m_playMode) {
    case Sequential:
        if (m_currentIndex < count - 1) {
            setCurrentIndex(m_currentIndex + 1);
        } else {
            // 末首按"下一首"：回到第一首并播放，让按钮始终有反应。
            // 自动播完末首的停止由 onMediaStatusChanged 处理，保留 Sequential 顺序语义。
            setCurrentIndex(0);
        }
        break;
    case Random:
        setCurrentIndex(QRandomGenerator::global()->bounded(count));
        break;
    case RepeatOne:
        // 手动按"下一首"应切歌（单曲循环语义仅作用于自动播完重播）
        if (m_currentIndex < count - 1) {
            setCurrentIndex(m_currentIndex + 1);
        } else {
            setCurrentIndex(0);
        }
        break;
    case RepeatList:
        if (m_currentIndex < count - 1) {
            setCurrentIndex(m_currentIndex + 1);
        } else {
            setCurrentIndex(0);
        }
        break;
    }
}

void Player::previous()
{
    int count = m_mediaList.size();
    if (count == 0) return;

    switch (m_playMode) {
    case Sequential:
    case RepeatList:
        if (m_currentIndex > 0) {
            setCurrentIndex(m_currentIndex - 1);
        } else {
            setCurrentIndex(count - 1);
        }
        break;
    case Random:
        setCurrentIndex(QRandomGenerator::global()->bounded(count));
        break;
    case RepeatOne:
        // 手动按"上一首"应切歌（单曲循环语义仅作用于自动播完重播）
        if (m_currentIndex > 0) {
            setCurrentIndex(m_currentIndex - 1);
        } else {
            setCurrentIndex(count - 1);
        }
        break;
    }
}

qint64 Player::duration() const
{
    return m_player->duration();
}

qint64 Player::position() const
{
    return m_player->position();
}

void Player::setPosition(qint64 position)
{
    m_player->setPosition(position);
}

int Player::volume() const
{
    return static_cast<int>(m_audioOutput->volume() * 100);
}

void Player::setVolume(int volume)
{
    m_audioOutput->setVolume(volume / 100.0f);
}

Player::PlayMode Player::playMode() const
{
    return m_playMode;
}

void Player::setPlayMode(PlayMode mode)
{
    m_playMode = mode;
}

QMediaPlayer::PlaybackState Player::playbackState() const
{
    return m_player->playbackState();
}

int Player::currentIndex() const
{
    return m_currentIndex;
}

LyricsManager *Player::lyricsManager() const
{
    return m_lyricsManager;
}

void Player::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_mediaList.size()) {
        m_currentIndex = index;
        m_player->setSource(m_mediaList[index]);

        // 尝试从音乐文件中加载歌词
        m_lyricsManager->loadLyricsFromMedia(m_mediaList[index]);

        m_player->play(); // 自动开始播放音乐
        emit currentIndexChanged(index);
    }
}

void Player::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        if (m_playMode == RepeatOne) {
            // 单曲循环：自动播完重播当前（不切歌）
            m_player->setPosition(0);
            m_player->play();
        } else if (m_playMode == Sequential && m_currentIndex >= m_mediaList.size() - 1) {
            // Sequential 自动播完末首应停止（不循环，保留顺序语义）
            m_player->stop();
        } else {
            next();
        }
    }
}

void Player::onPositionChanged(qint64 position)
{
    QString lyrics = m_lyricsManager->getLyricsAtTime(position);
    emit lyricsChanged(lyrics);
}

// 均衡器相关方法
void Player::setEqualizerSettings(const QList<int> &settings)
{
    m_equalizerSettings = settings;
    applyEqualizerSettings();
}

void Player::applyEqualizerSettings()
{
    // 注意：QMediaPlayer原生不支持均衡器功能
    // 要实现真正的均衡器，需要使用第三方库（如FFTW、PortAudio或FFmpeg）
    // 或自定义实现FIR滤波器

    // 这里可以添加代码来应用均衡器设置
    // 例如，将settings传递给音频处理模块
    Q_UNUSED(m_equalizerSettings);
}
