#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QList>
#include <QUrl>
#include "lyricsmanager.h"

class Player : public QObject
{
    Q_OBJECT
public:
    enum PlayMode { Sequential, Random, RepeatOne, RepeatList };
    explicit Player(QObject *parent = nullptr);
    ~Player();

    void setMediaList(const QList<QUrl> &mediaList);
    QList<QUrl> mediaList() const;

    void play();
    void pause();
    void stop();
    void next();
    void previous();

    qint64 duration() const;
    qint64 position() const;
    void setPosition(qint64 position);

    int volume() const;
    void setVolume(int volume);

    PlayMode playMode() const;
    void setPlayMode(PlayMode mode);

    QMediaPlayer::PlaybackState playbackState() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    LyricsManager *lyricsManager() const;
    
    // 均衡器相关方法
    void setEqualizerSettings(const QList<int> &settings);

 signals:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void playbackStateChanged(QMediaPlayer::PlaybackState state);
    void sourceChanged(const QUrl &source);
    void currentIndexChanged(int index);
    void lyricsChanged(const QString &lyrics);

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPositionChanged(qint64 position);

private:
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QList<QUrl> m_mediaList;
    PlayMode m_playMode;
    int m_currentIndex;
    LyricsManager *m_lyricsManager;
    
    // 均衡器相关成员变量
    QList<int> m_equalizerSettings;
    
    // 均衡器相关方法
    void applyEqualizerSettings();
};

#endif // PLAYER_H
