#include "tagutils.h"
#include <QFile>
#include <QFileInfo>
#include <functional>

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

// -------- 时长解析实现 --------

// MP3 帧头比特率表（MPEG1 Layer3）
static const int mp3BitrateTable[16] = {
    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};
// 采样率表（MPEG1）
static const int mp3SampleRateTable[4] = { 44100, 48000, 32000, 0 };

static qint64 getMp3Duration(QFile &f) {
    // 跳过 ID3v2
    qint64 dataStart = 0;
    f.seek(0);
    QByteArray header = f.read(10);
    if (header.size() >= 10 && header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
        dataStart = 10 + readSyncsafe(uchar(header[6]), uchar(header[7]),
                                       uchar(header[8]), uchar(header[9]));
    }
    // 扫描前 N 个帧头估算平均比特率
    f.seek(dataStart);
    qint64 fileEnd = f.size();
    int frameCount = 0;
    int totalBitrate = 0;
    qint64 pos = dataStart;

    while (pos + 4 < fileEnd && frameCount < 50) {
        f.seek(pos);
        QByteArray hdr = f.read(4);
        if (hdr.size() < 4) break;
        uchar b0 = uchar(hdr[0]), b1 = uchar(hdr[1]), b2 = uchar(hdr[2]);
        // 帧同步: 11 位全 1
        if ((b0 & 0xFF) != 0xFF || (b1 & 0xE0) != 0xE0) {
            pos++;
            continue;
        }
        int version = (b1 >> 3) & 0x03;  // 01=MPEG1
        int layer = (b1 >> 1) & 0x03;     // 01=Layer3
        if (version != 1 || layer != 1) { pos++; continue; }
        int brIdx = (b2 >> 4) & 0x0F;
        int srIdx = (b2 >> 2) & 0x03;
        int bitrate = mp3BitrateTable[brIdx];
        int sampleRate = mp3SampleRateTable[srIdx];
        if (bitrate == 0 || sampleRate == 0) { pos++; continue; }
        // 帧大小 = 144 * bitrate * 1000 / sampleRate + padding
        int padding = (b2 >> 1) & 0x01;
        int frameSize = 144 * bitrate * 1000 / sampleRate + padding;
        totalBitrate += bitrate;
        frameCount++;
        pos += frameSize;
    }

    if (frameCount > 0 && totalBitrate > 0) {
        int avgBitrate = totalBitrate / frameCount;
        qint64 dataBytes = fileEnd - dataStart;
        return dataBytes * 8 * 1000 / (avgBitrate * 1000);
    }
    return -1;
}

static qint64 getFlacDuration(QFile &f) {
    f.seek(0);
    QByteArray hdr = f.read(4);
    if (hdr.size() < 4) return -1;
    qint64 pos = 4;
    // "fLaC" 标志
    if (hdr[0] != 'f' || hdr[1] != 'L' || hdr[2] != 'a' || hdr[3] != 'C') return -1;

    while (pos < f.size()) {
        f.seek(pos);
        QByteArray blockHdr = f.read(4);
        if (blockHdr.size() < 4) break;
        bool isLast = (blockHdr[0] & 0x80) != 0;
        int blockType = blockHdr[0] & 0x7F;
        int blockLen = readBE32(0, uchar(blockHdr[1]), uchar(blockHdr[2]), uchar(blockHdr[3]));
        pos += 4;

        if (blockType == 0) {
            // STREAMINFO
            QByteArray si = f.read(blockLen);
            if (si.size() >= 18) {
                int sampleRate = (uchar(si[10]) << 12) | (uchar(si[11]) << 4)
                                 | ((uchar(si[12]) >> 4) & 0x0F);
                quint64 totalSamples = (quint64(uchar(si[13]) & 0x0F) << 32)
                                      | (quint64(uchar(si[14])) << 24)
                                      | (quint64(uchar(si[15])) << 16)
                                      | (quint64(uchar(si[16])) << 8)
                                      | quint64(uchar(si[17]));
                if (sampleRate > 0 && totalSamples > 0) {
                    return qint64(totalSamples * 1000 / sampleRate);
                }
            }
            return -1;
        }
        pos += blockLen;
        if (isLast) break;
    }
    return -1;
}

static qint64 getM4aDuration(QFile &f) {
    // 遍历 atom 树查找 mvhd
    f.seek(0);
    qint64 end = f.size();

    std::function<qint64(qint64, qint64, const char *)> findAtom;
    findAtom = [&](qint64 start, qint64 atomEnd, const char *target) -> qint64 {
        qint64 pos = start;
        while (pos + 8 <= atomEnd) {
            f.seek(pos);
            QByteArray hdr = f.read(8);
            if (hdr.size() < 8) return -1;
            qint64 size = readBE32(0, 0, 0, 0);
            size = (quint32(uchar(hdr[0])) << 24) | (quint32(uchar(hdr[1])) << 16)
                 | (quint32(uchar(hdr[2])) << 8) | quint32(uchar(hdr[3]));
            QByteArray type = hdr.mid(4, 4);
            qint64 headerLen = 8;
            if (size == 1) {
                // 64 位 size
                QByteArray ext = f.read(8);
                if (ext.size() < 8) return -1;
                size = (quint64(uchar(ext[0])) << 56) | (quint64(uchar(ext[1])) << 48)
                     | (quint64(uchar(ext[2])) << 40) | (quint64(uchar(ext[3])) << 32)
                     | (quint64(uchar(ext[4])) << 24) | (quint64(uchar(ext[5])) << 16)
                     | (quint64(uchar(ext[6])) << 8) | quint64(uchar(ext[7]));
                headerLen = 16;
            } else if (size == 0) {
                size = atomEnd - pos;
            }
            if (size < headerLen) return -1;

            if (type.size() == 4 && memcmp(type.constData(), target, 4) == 0) {
                return pos; // 找到
            }
            // 递归进入容器 atom
            if (type == "moov" || type == "trak") {
                qint64 found = findAtom(pos + headerLen, pos + size, target);
                if (found >= 0) return found;
            }
            pos += size;
        }
        return -1;
    };

    qint64 mvhdPos = findAtom(0, end, "mvhd");
    if (mvhdPos < 0) return -1;

    // 读取 mvhd atom 内容
    f.seek(mvhdPos);
    QByteArray hdr = f.read(8);
    if (hdr.size() < 8) return -1;
    qint64 size = (quint32(uchar(hdr[0])) << 24) | (quint32(uchar(hdr[1])) << 16)
                 | (quint32(uchar(hdr[2])) << 8) | quint32(uchar(hdr[3]));
    // 跳过 version(1) + flags(3)
    QByteArray body = f.read(size - 8);
    if (body.size() < 24) return -1;

    int version = uchar(body[0]);
    qint64 timescale, duration;
    if (version == 1) {
        // creation(4) + modification(4) + timescale(4) + duration(8)
        timescale = readBE32(0, uchar(body[12]), uchar(body[13]), uchar(body[15]));
        // 修正：正确偏移
        timescale = (quint32(uchar(body[12])) << 24) | (quint32(uchar(body[13])) << 16)
                   | (quint32(uchar(body[14])) << 8) | quint32(uchar(body[15]));
        duration = (quint64(uchar(body[16])) << 56) | (quint64(uchar(body[17])) << 48)
                 | (quint64(uchar(body[18])) << 40) | (quint64(uchar(body[19])) << 32)
                 | (quint64(uchar(body[20])) << 24) | (quint64(uchar(body[21])) << 16)
                 | (quint64(uchar(body[22])) << 8) | quint64(uchar(body[23]));
    } else {
        // version 0: creation(4) + modification(4) + timescale(4) + duration(4)
        timescale = (quint32(uchar(body[12])) << 24) | (quint32(uchar(body[13])) << 16)
                   | (quint32(uchar(body[14])) << 8) | quint32(uchar(body[15]));
        duration = (quint32(uchar(body[16])) << 24) | (quint32(uchar(body[17])) << 16)
                 | (quint32(uchar(body[18])) << 8) | quint32(uchar(body[19]));
    }
    if (timescale > 0 && duration > 0) {
        return duration * 1000 / timescale;
    }
    return -1;
}

static qint64 getWavDuration(QFile &f) {
    // RIFF + size + "WAVE"
    f.seek(0);
    QByteArray hdr = f.read(12);
    if (hdr.size() < 12) return -1;
    if (hdr[0] != 'R' || hdr[1] != 'I' || hdr[2] != 'F' || hdr[3] != 'F') return -1;

    int sampleRate = 0, channels = 0, bitsPerSample = 0;
    quint32 dataSize = 0;

    while (!f.atEnd()) {
        QByteArray chunkHdr = f.read(8);
        if (chunkHdr.size() < 8) break;
        QByteArray chunkId = chunkHdr.mid(0, 4);
        quint32 chunkSize = (quint32(uchar(chunkHdr[4])) << 24)
                           | (quint32(uchar(chunkHdr[5])) << 16)
                           | (quint32(uchar(chunkHdr[6])) << 8)
                           | quint32(uchar(chunkHdr[7]));

        if (chunkId == "fmt ") {
            QByteArray fmt = f.read(chunkSize);
            if (fmt.size() >= 16) {
                channels = (quint16(uchar(fmt[2])) << 8) | uchar(fmt[3]);
                sampleRate = (quint32(uchar(fmt[4])) << 24)
                           | (quint32(uchar(fmt[5])) << 16)
                           | (quint32(uchar(fmt[6])) << 8)
                           | quint32(uchar(fmt[7]));
                // bitsPerSample at offset 14
                bitsPerSample = (quint16(uchar(fmt[14])) << 8) | uchar(fmt[15]);
            }
        } else if (chunkId == "data") {
            dataSize = chunkSize;
            break;
        } else {
            f.seek(f.pos() + chunkSize);
        }
    }

    if (sampleRate > 0 && channels > 0 && bitsPerSample > 0 && dataSize > 0) {
        int byteRate = sampleRate * channels * (bitsPerSample / 8);
        if (byteRate > 0) {
            return qint64(dataSize) * 1000 / byteRate;
        }
    }
    return -1;
}

qint64 getDurationMs(const QString &path) {
    QFileInfo fi(path);
    QString ext = fi.suffix().toLower();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return -1;

    if (ext == "mp3") return getMp3Duration(f);
    if (ext == "flac") return getFlacDuration(f);
    if (ext == "m4a" || ext == "m4p" || ext == "mp4") return getM4aDuration(f);
    if (ext == "wav") return getWavDuration(f);
    // OGG/Opus: 暂不支持，返回 -1
    return -1;
}

} // namespace TagUtils
