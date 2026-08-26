#include "lyricsmanager.h"
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

LyricsManager::LyricsManager(QObject *parent) : QObject(parent)
{
}

bool LyricsManager::loadLyrics(const QString &filePath)
{
    m_lyricsMap.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QRegularExpression timeRegex("\\[(\\d+):(\\d+)(\\.\\d+)?\\]");
    while (!file.atEnd()) {
        QString line = file.readLine().trimmed();
        QRegularExpressionMatchIterator it = timeRegex.globalMatch(line);
        QString lyricsText;

        // 提取歌词文本
        int lastMatchEnd = 0;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            lastMatchEnd = match.capturedEnd();
        }
        if (lastMatchEnd < line.length()) {
            lyricsText = line.mid(lastMatchEnd).trimmed();
        }

        // 重新匹配时间标签
        it = timeRegex.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int minutes = match.captured(1).toInt();
            int seconds = match.captured(2).toInt();
            double milliseconds = 0;
            if (!match.captured(3).isEmpty()) {
                milliseconds = match.captured(3).toDouble() * 1000;
            }
            qint64 time = minutes * 60 * 1000 + seconds * 1000 + static_cast<qint64>(milliseconds);
            m_lyricsMap[time] = lyricsText;
        }
    }

    file.close();
    
    // 发送所有歌词已加载的信号
    emit allLyricsChanged(getAllLyrics());
    
    return !m_lyricsMap.isEmpty();
}

QList<QPair<qint64, QString>> LyricsManager::getAllLyrics() const
{
    QList<QPair<qint64, QString>> lyricsList;
    for (auto it = m_lyricsMap.constBegin(); it != m_lyricsMap.constEnd(); ++it) {
        lyricsList.append(qMakePair(it.key(), it.value()));
    }
    return lyricsList;
}

bool LyricsManager::loadLyricsFromMedia(const QUrl &mediaUrl)
{
    // 尝试从音乐文件所在目录中查找同名的LRC文件
    QString mediaPath = mediaUrl.toLocalFile();
    QString lyricsPath = mediaPath.left(mediaPath.lastIndexOf('.')) + ".lrc";
    return loadLyrics(lyricsPath);
}

QString LyricsManager::getLyricsAtTime(qint64 time) const
{
    if (m_lyricsMap.isEmpty()) {
        return "";
    }

    auto it = m_lyricsMap.lowerBound(time);
    if (it != m_lyricsMap.begin()) {
        --it;
    }

    return it.value();
}

QList<qint64> LyricsManager::getTimePoints() const
{
    return m_lyricsMap.keys();
}
