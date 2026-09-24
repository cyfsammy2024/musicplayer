#ifndef TAGMANAGER_H
#define TAGMANAGER_H

// 音乐标签读写管理器。支持 MP3(ID3v2.4) / FLAC / OGG(Vorbis) / M4A(MP4) / WAV(RIFF ID3)
// 共 5 种格式的 7 个标准字段读写，无第三方依赖（纯二进制操作）。
// 写入策略：保留目标字段外的其它帧/注释/atom，仅替换/追加目标字段。
// 写入失败时原文件不受影响（写临时文件后原子 rename）。

#include <QString>
#include <QUrl>

struct TagFields {
    QString title;
    QString artist;
    QString album;
    QString genre;
    QString lyrics;
    int year = 0;   // 0 表示未设置/清空
    int track = 0;  // 0 表示未设置/清空
};

class TagManager
{
public:
    // 读取 path 的 7 个字段到 out。失败返回 false（out 保持默认值）。
    static bool read(const QString &path, TagFields &out);

    // 将 in 中的字段写回 path。失败返回 false，*errMsg（若非空）填写原因供 UI 展示。
    // 成功保证原子性（临时文件 + rename）。空字符串字段会被写入（清空原值）。
    static bool write(const QString &path, const TagFields &in, QString *errMsg = nullptr);

private:
    enum class Format { Mp3, Wav, Flac, Ogg, M4A, Unknown };
    static Format detectFormat(const QString &path);

    // ---- 读取 ----
    static bool readId3v2(const QByteArray &tag, TagFields &out);   // MP3 + WAV 共用
    static bool readFlac(const QString &path, TagFields &out);
    static bool readOgg(const QString &path, TagFields &out);
    static bool readMp4(const QString &path, TagFields &out);

    // ---- 写入 ----
    static bool writeMp3(const QString &path, const TagFields &in, QString *errMsg);
    static bool writeFlac(const QString &path, const TagFields &in, QString *errMsg);
    static bool writeOgg(const QString &path, const TagFields &in, QString *errMsg);
    static bool writeMp4(const QString &path, const TagFields &in, QString *errMsg);
    static bool writeWav(const QString &path, const TagFields &in, QString *errMsg);

    // 构造 ID3v2.4 tag 块（header+frames+padding），用于 MP3/WAV 写入。
    // preserveFrames 为需原样保留的非目标帧字节（已去重，不含目标 7 帧）。
    static QByteArray buildId3v24Tag(const TagFields &in, const QByteArray &preserveFrames);

    // 构造 Vorbis 注释字段列表（key 大写顺序：先保留字段，后追加/覆盖目标 7 字段）
    static QList<QPair<QString, QString>> buildVorbisFields(
        const TagFields &in, const QMap<QString, QString> &preserve);

    // 原子写入：把 outData 写到 path 同目录的临时文件后 rename 覆盖 path。
    static bool atomicWrite(const QString &path, const QByteArray &outData, QString *errMsg);
};

#endif // TAGMANAGER_H
