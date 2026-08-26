#include "player.h"
#include <QRandomGenerator>

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
            m_player->stop();
        }
        break;
    case Random:
        setCurrentIndex(QRandomGenerator::global()->bounded(count));
        break;
    case RepeatOne:
        // 单曲循环，不需要改变索引
        m_player->setPosition(0);
        m_player->play();
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
        // 单曲循环，不需要改变索引
        m_player->setPosition(0);
        m_player->play();
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
        next();
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
