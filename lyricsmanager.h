#ifndef LYRICSMANAGER_H
#define LYRICSMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>

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
};

#endif // LYRICSMANAGER_H