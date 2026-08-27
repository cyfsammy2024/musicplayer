#ifndef LYRICSMANAGER_H
#define LYRICSMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QUrl>

class LyricsManager : public QObject
{
    Q_OBJECT
public:
    explicit LyricsManager(QObject *parent = nullptr);

    bool loadLyrics(const QString &filePath);
    bool loadLyricsFromMedia(const QUrl &mediaUrl);
    QString getLyricsAtTime(qint64 time) const;
    QList<qint64> getTimePoints() const;
    QList<QPair<qint64, QString>> getAllLyrics() const;

signals:
    void lyricsChanged(const QString &lyrics);
    void allLyricsChanged(const QList<QPair<qint64, QString>> &lyricsList);

private:
    QMap<qint64, QString> m_lyricsMap;

    // 复用的 LRC 解析入口（字符串输入），带时间戳的内嵌歌词可直接复用
    bool loadLyricsFromString(const QString &content);
    // 从音乐文件二进制中提取内嵌歌词，返回原始歌词文本（可能含 [mm:ss.xx] 时间戳，或纯文本，或空）
    QString extractEmbeddedLyrics(const QString &mediaPath);
    // 各格式解析器
    QString parseId3v2Lyrics(const QByteArray &tagBytes);   // MP3 起始标签 + WAV 的 ID3 chunk 共用
    QString parseFlacLyrics(const QString &path);
    QString parseOggLyrics(const QString &path);
    QString parseMp4Lyrics(const QString &path);
    QString parseRiffId3Lyrics(const QString &path);        // WAV
};

#endif // LYRICSMANAGER_H
