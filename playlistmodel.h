#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include <QUrl>
#include <QFileInfo>
#include <QList>
#include <QHash>

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        Number = 0,
        FileName,
        Duration,
        FileSize,
        ColumnCount
    };

    explicit PlaylistModel(QObject *parent = nullptr);
    ~PlaylistModel();

    QList<QUrl> mediaList() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addMedia(const QList<QUrl> &urls);
    void clear();
    void removeRowsAt(const QList<int> &rows);
    void importPlaylist(const QString &fileName);
    void exportPlaylist(const QString &fileName) const;
    void savePlaylist() const;
    void loadPlaylist();

private:
    QList<QUrl> m_mediaList;
    QList<QFileInfo> m_fileInfos;
    QHash<QString, qint64> m_durations; // 缓存：文件路径 → 毫秒

    QString formatDuration(qint64 duration) const;
    QString formatFileSize(qint64 size) const;
};

#endif // PLAYLISTMODEL_H
