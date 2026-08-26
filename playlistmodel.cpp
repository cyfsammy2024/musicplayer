#include "playlistmodel.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

PlaylistModel::PlaylistModel(QObject *parent) : QAbstractTableModel(parent)
{
    loadPlaylist();
}

PlaylistModel::~PlaylistModel()
{
    savePlaylist();
}

QList<QUrl> PlaylistModel::mediaList() const
{
    return m_mediaList;
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_mediaList.size();
}

int PlaylistModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        int row = index.row();
        int column = index.column();

        if (row >= m_mediaList.size()) {
            return QVariant();
        }

        QUrl url = m_mediaList[row];
        QFileInfo fileInfo(url.toLocalFile());

        switch (column) {
        case Number:
            return row + 1;
        case FileName:
            return fileInfo.fileName();
        case Duration:
            // 这里需要获取媒体文件的时长，暂时返回空字符串
            return "0:00";
        case FileSize:
            return formatFileSize(fileInfo.size());
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case Number:
            return "序号";
        case FileName:
            return "文件名";
        case Duration:
            return "时长";
        case FileSize:
            return "文件大小";
        default:
            return QVariant();
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void PlaylistModel::addMedia(const QList<QUrl> &urls)
{
    // 收集新的URL，排除已经存在的
    QList<QUrl> newUrls;
    QList<QFileInfo> newFileInfos;
    
    for (const QUrl &url : urls) {
        // 检查URL是否已经存在于播放列表中
        if (!m_mediaList.contains(url)) {
            newUrls.append(url);
            newFileInfos.append(QFileInfo(url.toLocalFile()));
        }
    }
    
    // 只添加新的URL
    if (!newUrls.isEmpty()) {
        beginInsertRows(QModelIndex(), m_mediaList.size(), m_mediaList.size() + newUrls.size() - 1);
        for (int i = 0; i < newUrls.size(); ++i) {
            m_mediaList.append(newUrls[i]);
            m_fileInfos.append(newFileInfos[i]);
        }
        endInsertRows();
    }
}

void PlaylistModel::clear()
{
    beginResetModel();
    m_mediaList.clear();
    m_fileInfos.clear();
    endResetModel();
}

void PlaylistModel::importPlaylist(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QList<QUrl> urls;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            urls.append(QUrl::fromLocalFile(line));
        }
    }

    file.close();
    addMedia(urls);
}

void PlaylistModel::exportPlaylist(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);

    for (int i = 0; i < m_mediaList.size(); ++i) {
        QUrl url = m_mediaList[i];
        out << url.toLocalFile() << "\n";
    }

    file.close();
}

void PlaylistModel::savePlaylist() const
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    if (!dir.exists()) {
        dir.mkpath(appDataDir);
    }

    QString playlistFile = appDataDir + "/playlist.txt";
    exportPlaylist(playlistFile);
}

void PlaylistModel::loadPlaylist()
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString playlistFile = appDataDir + "/playlist.txt";

    QFile file(playlistFile);
    if (!file.exists()) {
        return;
    }

    importPlaylist(playlistFile);
}

QString PlaylistModel::formatDuration(qint64 duration) const
{
    int seconds = duration / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

QString PlaylistModel::formatFileSize(qint64 size) const
{
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}
