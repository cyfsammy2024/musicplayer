#ifndef TAGUTILS_H
#define TAGUTILS_H

// 共享的二进制标签读写辅助函数。lyricsmanager.cpp（读侧）与 tagmanager.cpp（读写）
// 共用此头文件，避免重复实现。仅含纯字节/文本操作，无 Qt 依赖以外的第三方库。

#include <QByteArray>
#include <QIODevice>
#include <QString>
#include <QStringConverter>
#include <QMap>
#include <QSet>

namespace TagUtils {

// -------- 字节序读取（大端/小端/syncsafe）--------

inline quint32 readBE32(uchar b0, uchar b1, uchar b2, uchar b3) {
    return (quint32(b0) << 24) | (quint32(b1) << 16) | (quint32(b2) << 8) | quint32(b3);
}
inline quint32 readLE32(uchar b0, uchar b1, uchar b2, uchar b3) {
    return (quint32(b3) << 24) | (quint32(b2) << 16) | (quint32(b1) << 8) | quint32(b0);
}
inline quint32 readBE24(uchar b0, uchar b1, uchar b2) {
    return (quint32(b0) << 16) | (quint32(b1) << 8) | quint32(b2);
}
// syncsafe 32 位（ID3v2 size 字段：每字节仅低 7 位有效）
inline quint32 readSyncsafe(uchar b0, uchar b1, uchar b2, uchar b3) {
    return (quint32(b0 & 0x7f) << 21) | (quint32(b1 & 0x7f) << 14)
         | (quint32(b2 & 0x7f) << 7) | quint32(b3 & 0x7f);
}

// -------- 字节序写入 --------

inline void writeBE32(QByteArray &ba, quint32 v) {
    ba.append(char(uchar((v >> 24) & 0xFF)));
    ba.append(char(uchar((v >> 16) & 0xFF)));
    ba.append(char(uchar((v >> 8) & 0xFF)));
    ba.append(char(uchar(v & 0xFF)));
}
inline void writeLE32(QByteArray &ba, quint32 v) {
    ba.append(char(uchar(v & 0xFF)));
    ba.append(char(uchar((v >> 8) & 0xFF)));
    ba.append(char(uchar((v >> 16) & 0xFF)));
    ba.append(char(uchar((v >> 24) & 0xFF)));
}
inline void writeBE24(QByteArray &ba, quint32 v) {
    ba.append(char(uchar((v >> 16) & 0xFF)));
    ba.append(char(uchar((v >> 8) & 0xFF)));
    ba.append(char(uchar(v & 0xFF)));
}
inline void writeBE16(QByteArray &ba, quint16 v) {
    ba.append(char(uchar((v >> 8) & 0xFF)));
    ba.append(char(uchar(v & 0xFF)));
}
// 就地写入小端 32 位到 ba 的 offset 处（ba 必须已有 ≥ offset+4 字节）
inline void writeLE32At(QByteArray &ba, int offset, quint32 v) {
    if (offset + 4 > ba.size()) return;
    uchar *p = reinterpret_cast<uchar *>(ba.data()) + offset;
    p[0] = uchar(v & 0xFF);
    p[1] = uchar((v >> 8) & 0xFF);
    p[2] = uchar((v >> 16) & 0xFF);
    p[3] = uchar((v >> 24) & 0xFF);
}
// syncsafe：每字节仅低 7 位
inline void writeSyncsafe(QByteArray &ba, quint32 v) {
    ba.append(char(uchar((v >> 21) & 0x7F)));
    ba.append(char(uchar((v >> 14) & 0x7F)));
    ba.append(char(uchar((v >> 7) & 0x7F)));
    ba.append(char(uchar(v & 0x7F)));
}

// -------- ID3 文本编解码 --------

// 去 unsynchronisation：把 0xFF 0x00 还原为 0xFF（在解码文本前应用）
QByteArray deUnsynchronise(const QByteArray &in);

// 按 ID3 编码字节解码文本（enc: 0=ISO-8859-1, 1=UTF-16+BOM, 2=UTF-16BE, 3=UTF-8）
QString decodeText(quint8 enc, const QByteArray &raw);

// 编码为 ID3v2 文本帧 body：固定使用 enc=3 (UTF-8) 以避免编码歧义。
// 返回 [1B enc=3][UTF-8 bytes]。
QByteArray encodeId3Text(const QString &s);

// 跳过描述符终结符，返回第一个不属于描述符的字节偏移
// USLT/SYLT 结构：[enc][lang(3)][descriptor 按 enc 终结][lyrics...]
int skipDescriptor(const QByteArray &frame, int start, quint8 enc);

// 判断字节串是否可作为合法帧 ID（A-Z0-9）
bool isFrameId(const QByteArray &id);

// 从已打开的 QIODevice 当前位置读取完整 ID3v2 标签（header+frames），不含音频数据
QByteArray readId3v2Tag(QIODevice &dev);

// -------- Vorbis 注释 --------

// 解析 Vorbis 注释块为字段映射（key 大写）。data 起点 = vendorLen 字节。
// 返回的 map 中 KEY 已 toUpper()。空 value 仍保留以便调用方判断"存在但为空"。
QMap<QString, QString> parseVorbisCommentFields(const QByteArray &data);

// 打包单条 Vorbis 注释字段为 "KEY=UTF-8VALUE" 的字节串（key 会转大写）
QByteArray packVorbisField(const QString &key, const QString &value);

// 构造完整的 Vorbis 注释块（vendor + count + fields），用于 FLAC/OGG 写入。
// vendor 串原样保留（若为空则用 "musicplayer"）。
QByteArray buildVorbisCommentBlock(const QString &vendor,
                                   const QList<QPair<QString, QString>> &fields);

// -------- OGG CRC32 --------

// OGG page CRC32（多项式 0x04C11DB7，初值 0，无 XOR-out，无反射）。
// 覆盖整个 page（header+body），计算前 CRC 字段位置（offset 22-25）应置 0。
quint32 oggCrc32(const QByteArray &data);

// -------- MP4 atom 遍历辅助 --------

// MP4 atom 头信息：解析 8 或 16 字节头，返回 headerLen(8/16) 与 atomSize（含头）。
// 失败返回 false。end 为父 atom body 结束偏移（用于 size=0 时延伸到末尾）。
bool parseMp4AtomHeader(QIODevice &dev, qint64 end, qint64 &headerLen, qint64 &atomSize, QByteArray &type);

// -------- 时长解析（纯文件头，不依赖 QMediaPlayer）--------

// 根据文件扩展名解析音频时长（毫秒）。失败返回 -1。
qint64 getDurationMs(const QString &path);

} // namespace TagUtils

#endif // TAGUTILS_H
