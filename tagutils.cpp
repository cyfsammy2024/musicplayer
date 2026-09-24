#include "tagutils.h"

namespace TagUtils {

// -------- ID3 文本/帧辅助 --------

QByteArray deUnsynchronise(const QByteArray &in) {
    QByteArray out;
    out.reserve(in.size());
    const int n = in.size();
    for (int i = 0; i < n; ++i) {
        uchar c = uchar(in[i]);
        out.append(char(c));
        // 跳过紧跟 0xFF 的填充 0x00
        if (c == 0xFF && i + 1 < n && uchar(in[i + 1]) == 0x00) {
            ++i;
        }
    }
    return out;
}

QString decodeText(quint8 enc, const QByteArray &raw) {
    if (raw.isEmpty()) return QString();
    switch (enc) {
    case 0: // ISO-8859-1
        return QString::fromLatin1(raw);
    case 3: // UTF-8
        return QString::fromUtf8(raw);
    case 1: { // UTF-16 with BOM
        if (raw.size() >= 2 && uchar(raw[0]) == 0xFE && uchar(raw[1]) == 0xFF) {
            QStringDecoder dec(QStringConverter::Utf16BE);
            return dec.decode(raw.mid(2));
        } else if (raw.size() >= 2 && uchar(raw[0]) == 0xFF && uchar(raw[1]) == 0xFE) {
            QStringDecoder dec(QStringConverter::Utf16LE);
            return dec.decode(raw.mid(2));
        }
        QStringDecoder dec(QStringConverter::Utf16LE);
        return dec.decode(raw);
    }
    case 2: { // UTF-16BE 无 BOM
        QStringDecoder dec(QStringConverter::Utf16BE);
        return dec.decode(raw);
    }
    default:
        return QString::fromUtf8(raw);
    }
}

QByteArray encodeId3Text(const QString &s) {
    QByteArray body;
    body.append(char(0x03)); // enc=3 UTF-8
    body.append(s.toUtf8());
    return body;
}

int skipDescriptor(const QByteArray &frame, int start, quint8 enc) {
    const int n = frame.size();
    if (enc == 1 || enc == 2) {
        for (int i = start; i + 1 < n; i += 2) {
            if (uchar(frame[i]) == 0x00 && uchar(frame[i + 1]) == 0x00) {
                return i + 2;
            }
        }
    } else {
        for (int i = start; i < n; ++i) {
            if (uchar(frame[i]) == 0x00) {
                return i + 1;
            }
        }
    }
    return n;
}

bool isFrameId(const QByteArray &id) {
    if (id.isEmpty()) return false;
    for (int i = 0; i < id.size(); ++i) {
        uchar c = uchar(id[i]);
        bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (!ok) return false;
    }
    return true;
}

QByteArray readId3v2Tag(QIODevice &dev) {
    QByteArray header = dev.read(10);
    if (header.size() < 10 || header[0] != 'I' || header[1] != 'D' || header[2] != '3') {
        return QByteArray();
    }
    quint32 size = readSyncsafe(uchar(header[6]), uchar(header[7]), uchar(header[8]), uchar(header[9]));
    if (size == 0) return QByteArray();
    QByteArray frames = dev.read(size);
    QByteArray tag = header;
    tag += frames;
    return tag;
}

// -------- Vorbis 注释 --------

QMap<QString, QString> parseVorbisCommentFields(const QByteArray &data) {
    QMap<QString, QString> out;
    if (data.size() < 4) return out;
    int pos = 0;
    quint32 vendorLen = readLE32(uchar(data[0]), uchar(data[1]), uchar(data[2]), uchar(data[3]));
    pos += 4;
    if (pos + vendorLen > data.size()) return out;
    pos += int(vendorLen);
    if (pos + 4 > data.size()) return out;
    quint32 count = readLE32(uchar(data[pos]), uchar(data[pos + 1]),
                             uchar(data[pos + 2]), uchar(data[pos + 3]));
    pos += 4;

    for (quint32 i = 0; i < count; ++i) {
        if (pos + 4 > data.size()) break;
        quint32 len = readLE32(uchar(data[pos]), uchar(data[pos + 1]),
                               uchar(data[pos + 2]), uchar(data[pos + 3]));
        pos += 4;
        if (pos + len > data.size()) break;
        QByteArray field = data.mid(pos, int(len));
        pos += int(len);
        int eq = field.indexOf('=');
        if (eq <= 0) continue;
        QString key = QString::fromUtf8(field.left(eq)).toUpper();
        QString val = QString::fromUtf8(field.mid(eq + 1));
        // 同键多次出现：合并为多值（用 '\n' 分隔）；调用方按 key 取首个即可
        if (out.contains(key)) {
            out[key] = out.value(key) + "\n" + val;
        } else {
            out.insert(key, val);
        }
    }
    return out;
}

QByteArray packVorbisField(const QString &key, const QString &value) {
    QByteArray field = key.toUpper().toUtf8();
    field.append('=');
    field.append(value.toUtf8());
    return field;
}

QByteArray buildVorbisCommentBlock(const QString &vendor,
                                   const QList<QPair<QString, QString>> &fields) {
    QByteArray block;
    QByteArray vbytes = vendor.isEmpty() ? QByteArray("musicplayer") : vendor.toUtf8();
    writeLE32(block, quint32(vbytes.size()));
    block.append(vbytes);
    writeLE32(block, quint32(fields.size()));
    for (const auto &f : fields) {
        QByteArray field = packVorbisField(f.first, f.second);
        writeLE32(block, quint32(field.size()));
        block.append(field);
    }
    return block;
}

// -------- OGG CRC32 --------
// 多项式 0x04C11DB7，初值 0，无 XOR-out，无反射。OGG 规范要求。

static const quint32 *oggCrcTable() {
    static quint32 table[256];
    static bool init = false;
    if (!init) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 r = i << 24;
            for (int j = 0; j < 8; ++j) {
                if (r & 0x80000000) {
                    r = (r << 1) ^ 0x04C11DB7;
                } else {
                    r <<= 1;
                }
            }
            table[i] = r;
        }
        init = true;
    }
    return table;
}

quint32 oggCrc32(const QByteArray &data) {
    const quint32 *table = oggCrcTable();
    quint32 crc = 0;
    const int n = data.size();
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    for (int i = 0; i < n; ++i) {
        crc = (crc << 8) ^ table[((crc >> 24) & 0xFF) ^ p[i]];
    }
    return crc;
}

// -------- MP4 atom 头解析 --------

bool parseMp4AtomHeader(QIODevice &dev, qint64 end, qint64 &headerLen, qint64 &atomSize, QByteArray &type) {
    if (dev.pos() + 8 > end) return false;
    QByteArray hdr = dev.read(8);
    if (hdr.size() < 8) return false;
    quint32 size32 = readBE32(uchar(hdr[0]), uchar(hdr[1]), uchar(hdr[2]), uchar(hdr[3]));
    type = hdr.mid(4, 4);
    headerLen = 8;
    atomSize = qint64(size32);
    if (size32 == 1) {
        // 64 位扩展 size
        QByteArray ext = dev.read(8);
        if (ext.size() < 8) return false;
        quint64 hi = readBE32(uchar(ext[0]), uchar(ext[1]), uchar(ext[2]), uchar(ext[3]));
        quint64 lo = readBE32(uchar(ext[4]), uchar(ext[5]), uchar(ext[6]), uchar(ext[7]));
        atomSize = qint64((hi << 32) | lo);
        headerLen = 16;
    } else if (size32 == 0) {
        atomSize = end - dev.pos() + 8 - headerLen; // 延伸到父 atom 末尾
        // 注意：end 是父 body 结束；当前 atom 起点为 dev.pos()-8（已读 8 字节头）
        // atomSize 应覆盖从 atom 起点到 end
        // 上面公式不准，重算：
        qint64 atomStart = dev.pos() - 8;
        atomSize = end - atomStart;
        if (atomSize < headerLen) atomSize = headerLen;
    }
    if (atomSize < headerLen) return false;
    return true;
}

} // namespace TagUtils
