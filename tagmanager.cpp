#include "tagmanager.h"
#include "tagutils.h"
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QDir>
#include <QRegularExpression>
#include <QSet>

using namespace TagUtils;

// ============================================================
// 内部辅助：MP4 atom 遍历（写侧需要精确偏移，与 lyricsmanager 的递归读取不同）
// ============================================================
namespace {

// 在 [start, end) 范围内查找 type 指定的直接子 atom，返回其起点偏移与 atomSize（含头）。
// 找不到返回 false。dev 调用后位置不确定。
bool findChildAtom(QIODevice &dev, qint64 start, qint64 end, const QByteArray &type,
                   qint64 &outStart, qint64 &outSize, qint64 &outHeaderLen)
{
    qint64 pos = start;
    while (pos + 8 <= end) {
        if (!dev.seek(pos)) return false;
        QByteArray hdr = dev.read(8);
        if (hdr.size() < 8) return false;
        quint32 size32 = readBE32(uchar(hdr[0]), uchar(hdr[1]), uchar(hdr[2]), uchar(hdr[3]));
        QByteArray t = hdr.mid(4, 4);
        qint64 headerLen = 8;
        qint64 atomSize = qint64(size32);
        if (size32 == 1) {
            QByteArray ext = dev.read(8);
            if (ext.size() < 8) return false;
            quint64 hi = readBE32(uchar(ext[0]), uchar(ext[1]), uchar(ext[2]), uchar(ext[3]));
            quint64 lo = readBE32(uchar(ext[4]), uchar(ext[5]), uchar(ext[6]), uchar(ext[7]));
            atomSize = qint64((hi << 32) | lo);
            headerLen = 16;
        } else if (size32 == 0) {
            atomSize = end - pos;
        }
        if (atomSize < headerLen) return false;
        if (t == type) {
            outStart = pos;
            outSize = atomSize;
            outHeaderLen = headerLen;
            return true;
        }
        pos += atomSize;
    }
    return false;
}

// 从 data 子 atom 中提取 value（跳过 16B data atom 头）
QString readMp4DataAtomValue(const QByteArray &ilstBody)
{
    // ilst 子 atom body 形如: [4B size][4B type][ data atom: 4B size 4B "data" 4B indicator 4B locale value ]
    // ilstBody 已是子 atom 的完整字节（含其 8B 头）
    if (ilstBody.size() < 16) return QString();
    // 跳过子 atom 头 8B，读 data atom
    const int dataAtomStart = 8;
    if (ilstBody.mid(dataAtomStart + 4, 4) != "data") return QString();
    const int valueStart = dataAtomStart + 16; // data atom 头 16B
    if (ilstBody.size() < valueStart) return QString();
    return QString::fromUtf8(ilstBody.mid(valueStart));
}

// 构造一个 iTunes 风格 ilst 子 atom（文本值）：[4B size][4B type][data atom: ...]
QByteArray buildMp4TextAtom(const QByteArray &type4, const QString &value)
{
    QByteArray vbytes = value.toUtf8();
    // data atom = [4B size][4B "data"][4B type-indicator=1][4B locale=0][value]
    QByteArray da;
    writeBE32(da, quint32(16 + vbytes.size()));
    da.append("data");
    writeBE32(da, 1); // type-indicator = 1 (UTF-8)
    writeBE32(da, 0); // locale
    da.append(vbytes);
    QByteArray atom;
    writeBE32(atom, quint32(8 + da.size()));
    atom.append(type4);
    atom.append(da);
    return atom;
}

// 构造 trkn 子 atom（type-indicator=0，8B 值）
QByteArray buildMp4TrknAtom(int track)
{
    QByteArray da;
    writeBE32(da, 24); // size = 16 + 8
    da.append("data");
    writeBE32(da, 0); // type-indicator=0
    writeBE32(da, 0); // locale
    // value: [2B track][2B total][4B 0]
    QByteArray val;
    writeBE16(val, quint16(track > 0 ? track : 0));
    writeBE16(val, 0); // total tracks unknown
    writeBE32(val, 0);
    da.append(val);
    QByteArray atom;
    writeBE32(atom, quint32(8 + da.size()));
    atom.append("trkn");
    atom.append(da);
    return atom;
}

// 从 trkn 子 atom 读 track 号
int readMp4Trkn(const QByteArray &ilstBody)
{
    if (ilstBody.size() < 24 + 8) return 0;
    const int valueStart = 8 + 16; // 子 atom 头 8 + data atom 头 16
    if (ilstBody.size() < valueStart + 2) return 0;
    int track = (uchar(ilstBody[valueStart]) << 8) | uchar(ilstBody[valueStart + 1]);
    return track;
}

} // namespace

// ============================================================
// 公共 API
// ============================================================

TagManager::Format TagManager::detectFormat(const QString &path)
{
    QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == "mp3") return Format::Mp3;
    if (suffix == "wav") return Format::Wav;
    if (suffix == "flac") return Format::Flac;
    if (suffix == "ogg" || suffix == "oga") return Format::Ogg;
    if (suffix == "m4a" || suffix == "mp4" || suffix == "m4p" ||
        suffix == "m4b" || suffix == "alac") return Format::M4A;
    return Format::Unknown;
}

bool TagManager::read(const QString &path, TagFields &out)
{
    out = TagFields();
    Format fmt = detectFormat(path);
    switch (fmt) {
    case Format::Mp3: {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return false;
        QByteArray tag = readId3v2Tag(f);
        if (tag.isEmpty()) return true; // 无 ID3v2 标签，字段全空
        return readId3v2(tag, out);
    }
    case Format::Wav: {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return false;
        QByteArray hdr = f.read(12);
        if (hdr.size() < 12 || hdr.mid(0, 4) != "RIFF" || hdr.mid(8, 4) != "WAVE") return false;
        while (!f.atEnd()) {
            QByteArray chunkHdr = f.read(8);
            if (chunkHdr.size() < 8) break;
            QByteArray id = chunkHdr.mid(0, 4);
            quint32 sz = readLE32(uchar(chunkHdr[4]), uchar(chunkHdr[5]),
                                  uchar(chunkHdr[6]), uchar(chunkHdr[7]));
            if (id == "ID3 " || id == "id3 ") {
                QByteArray tag = f.read(sz);
                return readId3v2(tag, out);
            }
            qint64 skip = sz + (sz & 1);
            if (!f.seek(f.pos() + skip)) break;
        }
        return true; // 无 ID3 chunk
    }
    case Format::Flac: return readFlac(path, out);
    case Format::Ogg: return readOgg(path, out);
    case Format::M4A: return readMp4(path, out);
    case Format::Unknown: return false;
    }
    return false;
}

// ============================================================
// ID3v2 读取（MP3 + WAV 共用）
// ============================================================
bool TagManager::readId3v2(const QByteArray &tag, TagFields &out)
{
    if (tag.size() < 10) return false;
    int versionMajor = uchar(tag[3]);
    uchar flags = uchar(tag[5]);
    quint32 headerSize = readSyncsafe(uchar(tag[6]), uchar(tag[7]), uchar(tag[8]), uchar(tag[9]));
    if (headerSize == 0) return true;

    QByteArray frames = tag.mid(10, qMin<qint64>(headerSize, tag.size() - 10));
    if (frames.size() < 6) return true;

    bool globalUnsynch = (versionMajor == 3) && (flags & 0x80);

    int pos = 0;
    if (flags & 0x40) { // 跳过扩展头
        if (versionMajor == 4 && frames.size() >= 4) {
            quint32 extSize = readSyncsafe(uchar(frames[0]), uchar(frames[1]),
                                            uchar(frames[2]), uchar(frames[3]));
            pos = qMin(qint64(extSize), qint64(frames.size()));
        } else if (versionMajor == 3 && frames.size() >= 4) {
            quint32 extSize = readBE32(uchar(frames[0]), uchar(frames[1]),
                                       uchar(frames[2]), uchar(frames[3]));
            pos = qMin(qint64(extSize + 4), qint64(frames.size()));
        }
    }

    const int minHeader = (versionMajor == 2) ? 6 : 10;
    QString txxxLyrics;

    while (pos + minHeader <= frames.size()) {
        QByteArray id;
        quint32 frameSize = 0;
        int headerLen = 10;
        bool frameUnsynch = false;

        if (versionMajor == 2) {
            id = frames.mid(pos, 3);
            if (!isFrameId(id)) break;
            frameSize = readBE24(uchar(frames[pos + 3]), uchar(frames[pos + 4]), uchar(frames[pos + 5]));
            headerLen = 6;
        } else {
            id = frames.mid(pos, 4);
            if (!isFrameId(id)) break;
            if (versionMajor == 4) {
                frameSize = readSyncsafe(uchar(frames[pos + 4]), uchar(frames[pos + 5]),
                                         uchar(frames[pos + 6]), uchar(frames[pos + 7]));
                frameUnsynch = (uchar(frames[pos + 8]) & 0x08);
            } else {
                frameSize = readBE32(uchar(frames[pos + 4]), uchar(frames[pos + 5]),
                                     uchar(frames[pos + 6]), uchar(frames[pos + 7]));
            }
        }

        if (frameSize == 0) { pos += headerLen; continue; }

        int bodyStart = pos + headerLen;
        int bodyEnd = qMin(qint64(bodyStart + frameSize), qint64(frames.size()));
        if (bodyStart >= frames.size()) break;
        QByteArray body = frames.mid(bodyStart, bodyEnd - bodyStart);
        if (globalUnsynch || frameUnsynch) body = deUnsynchronise(body);

        // 文本帧：body = [1B enc][text]
        auto textOf = [](const QByteArray &b) -> QString {
            if (b.size() < 1) return QString();
            quint8 enc = uchar(b[0]);
            return decodeText(enc, b.mid(1));
        };

        if (id == "TIT2" || id == "TT2") out.title = textOf(body);
        else if (id == "TPE1" || id == "TP1") out.artist = textOf(body);
        else if (id == "TALB" || id == "TAL") out.album = textOf(body);
        else if (id == "TDRC" || id == "TYER" || id == "TYE") {
            QString y = textOf(body);
            QRegularExpression re("(\\d{4})");
            auto m = re.match(y);
            if (m.hasMatch()) out.year = m.captured(1).toInt();
        }
        else if (id == "TRCK" || id == "TRK") {
            QString t = textOf(body);
            int slash = t.indexOf('/');
            out.track = (slash > 0 ? t.left(slash) : t).toInt();
        }
        else if (id == "TCON" || id == "TCO") out.genre = textOf(body);
        else if (id == "USLT" || id == "ULT") {
            if (body.size() >= 4) {
                quint8 enc = uchar(body[0]);
                int descEnd = skipDescriptor(body, 4, enc);
                if (descEnd < body.size()) {
                    out.lyrics = decodeText(enc, body.mid(descEnd));
                }
            }
        }
        else if (id == "TXXX") {
            // ffmpeg 等把歌词写入 TXXX 的回退
            if (body.size() >= 1) {
                quint8 enc = uchar(body[0]);
                int descEnd = skipDescriptor(body, 1, enc);
                int termLen = (enc == 1 || enc == 2) ? 2 : 1;
                int descLen = descEnd - 1 - termLen;
                if (descLen < 0) descLen = 0;
                QString desc = decodeText(enc, body.mid(1, descLen));
                QString d = desc.trimmed().toUpper();
                if (d.contains("LYRIC") || d == "USLT") {
                    if (descEnd < body.size()) {
                        txxxLyrics = decodeText(enc, body.mid(descEnd));
                    }
                }
            }
        }

        pos = bodyEnd;
    }

    if (out.lyrics.isEmpty() && !txxxLyrics.isEmpty()) out.lyrics = txxxLyrics;
    return true;
}

// ============================================================
// FLAC 读取
// ============================================================
bool TagManager::readFlac(const QString &path, TagFields &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray magic = f.read(4);
    if (magic.size() < 4 || magic[0] != 'f' || magic[1] != 'L' ||
        magic[2] != 'a' || magic[3] != 'C') return false;

    while (!f.atEnd()) {
        QByteArray hdr = f.read(4);
        if (hdr.size() < 4) break;
        uchar blockType = uchar(hdr[0]) & 0x7f;
        bool lastBlock = (uchar(hdr[0]) & 0x80) != 0;
        quint32 blockSize = readBE24(uchar(hdr[1]), uchar(hdr[2]), uchar(hdr[3]));
        if (blockType == 4) { // VORBIS_COMMENT
            QByteArray data = f.read(blockSize);
            QMap<QString, QString> fields = parseVorbisCommentFields(data);
            out.title = fields.value("TITLE");
            out.artist = fields.value("ARTIST");
            out.album = fields.value("ALBUM");
            QString y = fields.value("DATE", fields.value("YEAR"));
            QRegularExpression re("(\\d{4})");
            auto m = re.match(y);
            if (m.hasMatch()) out.year = m.captured(1).toInt();
            QString t = fields.value("TRACKNUMBER");
            int slash = t.indexOf('/');
            out.track = (slash > 0 ? t.left(slash) : t).toInt();
            out.genre = fields.value("GENRE");
            out.lyrics = fields.value("LYRICS", fields.value("UNSYNCEDLYRICS"));
            return true;
        }
        if (!f.seek(f.pos() + blockSize)) break;
        if (lastBlock) break;
    }
    return true;
}

// ============================================================
// OGG 读取（best-effort，扫描前 64KB）
// ============================================================
bool TagManager::readOgg(const QString &path, TagFields &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = f.read(64 * 1024);
    int vorbisIdx = data.indexOf("\x03vorbis", 0);
    int opusIdx = data.indexOf("OpusTags");
    if (opusIdx >= 0 && (vorbisIdx < 0 || opusIdx < vorbisIdx)) {
        QMap<QString, QString> fields = parseVorbisCommentFields(data.mid(opusIdx + 8));
        out.title = fields.value("TITLE");
        out.artist = fields.value("ARTIST");
        out.album = fields.value("ALBUM");
        out.lyrics = fields.value("LYRICS", fields.value("UNSYNCEDLYRICS"));
        out.genre = fields.value("GENRE");
        QRegularExpression re("(\\d{4})");
        auto m = re.match(fields.value("DATE"));
        if (m.hasMatch()) out.year = m.captured(1).toInt();
        out.track = fields.value("TRACKNUMBER").toInt();
        return true;
    }
    if (vorbisIdx >= 0) {
        QMap<QString, QString> fields = parseVorbisCommentFields(data.mid(vorbisIdx + 7));
        out.title = fields.value("TITLE");
        out.artist = fields.value("ARTIST");
        out.album = fields.value("ALBUM");
        out.lyrics = fields.value("LYRICS", fields.value("UNSYNCEDLYRICS"));
        out.genre = fields.value("GENRE");
        QRegularExpression re("(\\d{4})");
        auto m = re.match(fields.value("DATE"));
        if (m.hasMatch()) out.year = m.captured(1).toInt();
        QString t = fields.value("TRACKNUMBER");
        int slash = t.indexOf('/');
        out.track = (slash > 0 ? t.left(slash) : t).toInt();
        return true;
    }
    return true;
}

// ============================================================
// MP4 读取
// ============================================================
bool TagManager::readMp4(const QString &path, TagFields &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    qint64 fsize = f.size();

    // 顶层定位 moov
    qint64 moovStart, moovSize, moovHdrLen;
    if (!findChildAtom(f, 0, fsize, "moov", moovStart, moovSize, moovHdrLen)) return true;
    qint64 moovBodyStart = moovStart + moovHdrLen;
    qint64 moovBodyEnd = moovStart + moovSize;

    // moov → udta
    qint64 udtaStart, udtaSize, udtaHdrLen;
    if (!findChildAtom(f, moovBodyStart, moovBodyEnd, "udta", udtaStart, udtaSize, udtaHdrLen)) return true;

    // udta → meta（meta body 头 4B 是 version+flags，跳过）
    qint64 metaStart, metaSize, metaHdrLen;
    if (!findChildAtom(f, udtaStart + udtaHdrLen, udtaStart + udtaSize, "meta",
                      metaStart, metaSize, metaHdrLen)) return true;
    qint64 metaBodyStart = metaStart + metaHdrLen + 4; // 跳 4B 前缀
    qint64 metaBodyEnd = metaStart + metaSize;

    // meta → ilst
    qint64 ilstStart, ilstSize, ilstHdrLen;
    if (!findChildAtom(f, metaBodyStart, metaBodyEnd, "ilst",
                      ilstStart, ilstSize, ilstHdrLen)) return true;
    qint64 ilstBodyStart = ilstStart + ilstHdrLen;
    qint64 ilstBodyEnd = ilstStart + ilstSize;

    // 遍历 ilst 子 atom
    static const QByteArray kNam = QByteArray("\xA9nam", 4);
    static const QByteArray kArt = QByteArray("\xA9" "ART", 4);
    static const QByteArray kAlb = QByteArray("\xA9" "alb", 4);
    static const QByteArray kDay = QByteArray("\xA9" "day", 4);
    static const QByteArray kGen = QByteArray("\xA9gen", 4);
    static const QByteArray kLyr = QByteArray("\xA9lyr", 4);
    static const QByteArray kTrk = QByteArray("trkn", 4);

    qint64 pos = ilstBodyStart;
    while (pos + 8 <= ilstBodyEnd) {
        if (!f.seek(pos)) break;
        QByteArray hdr = f.read(8);
        if (hdr.size() < 8) break;
        quint32 sz32 = readBE32(uchar(hdr[0]), uchar(hdr[1]), uchar(hdr[2]), uchar(hdr[3]));
        QByteArray type = hdr.mid(4, 4);
        qint64 atomSize = qint64(sz32);
        qint64 headerLen = 8;
        if (sz32 == 1) {
            QByteArray ext = f.read(8);
            if (ext.size() < 8) break;
            quint64 hi = readBE32(uchar(ext[0]), uchar(ext[1]), uchar(ext[2]), uchar(ext[3]));
            quint64 lo = readBE32(uchar(ext[4]), uchar(ext[5]), uchar(ext[6]), uchar(ext[7]));
            atomSize = qint64((hi << 32) | lo);
            headerLen = 16;
        } else if (sz32 == 0) {
            atomSize = ilstBodyEnd - pos;
        }
        if (atomSize < headerLen) break;
        // 读整个子 atom 字节
        if (!f.seek(pos)) break;
        QByteArray atomBytes = f.read(atomSize);
        if (type == kNam) out.title = readMp4DataAtomValue(atomBytes);
        else if (type == kArt) out.artist = readMp4DataAtomValue(atomBytes);
        else if (type == kAlb) out.album = readMp4DataAtomValue(atomBytes);
        else if (type == kDay) {
            QString y = readMp4DataAtomValue(atomBytes);
            QRegularExpression re("(\\d{4})");
            auto m = re.match(y);
            if (m.hasMatch()) out.year = m.captured(1).toInt();
        }
        else if (type == kGen) out.genre = readMp4DataAtomValue(atomBytes);
        else if (type == kLyr) out.lyrics = readMp4DataAtomValue(atomBytes);
        else if (type == kTrk) out.track = readMp4Trkn(atomBytes);
        pos += atomSize;
    }
    return true;
}

// ============================================================
// 写入分发
// ============================================================
bool TagManager::write(const QString &path, const TagFields &in, QString *errMsg)
{
    Format fmt = detectFormat(path);
    switch (fmt) {
    case Format::Mp3: return writeMp3(path, in, errMsg);
    case Format::Wav: return writeWav(path, in, errMsg);
    case Format::Flac: return writeFlac(path, in, errMsg);
    case Format::Ogg: return writeOgg(path, in, errMsg);
    case Format::M4A: return writeMp4(path, in, errMsg);
    case Format::Unknown:
        if (errMsg) *errMsg = QStringLiteral("不支持的文件格式");
        return false;
    }
    return false;
}

bool TagManager::atomicWrite(const QString &path, const QByteArray &outData, QString *errMsg)
{
    QFileInfo info(path);
    QString tmp = info.absolutePath() + QDir::separator() +
                  QStringLiteral(".~mp_tmp_") + info.fileName();
    {
        QFile tf(tmp);
        if (!tf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (errMsg) *errMsg = QStringLiteral("无法创建临时文件: ") + tmp;
            return false;
        }
        if (tf.write(outData) != outData.size()) {
            if (errMsg) *errMsg = QStringLiteral("写入临时文件失败");
            tf.close();
            QFile::remove(tmp);
            return false;
        }
        tf.close();
    }
    if (!QFile::remove(path) || !QFile::rename(tmp, path)) {
        // rename 失败时尝试复制
        if (!QFile::copy(tmp, path)) {
            if (errMsg) *errMsg = QStringLiteral("替换原文件失败");
            QFile::remove(tmp);
            return false;
        }
        QFile::remove(tmp);
    }
    return true;
}

// ============================================================
// MP3 写入
// ============================================================
QByteArray TagManager::buildId3v24Tag(const TagFields &in, const QByteArray &preserveFrames)
{
    QByteArray frames;

    auto emitTextFrame = [&](const char *id, const QString &value) {
        if (value.isEmpty()) return;
        QByteArray body = encodeId3Text(value);
        frames.append(id, 4);
        writeSyncsafe(frames, quint32(body.size()));
        frames.append(char(0)); frames.append(char(0)); // flags
        frames.append(body);
    };

    emitTextFrame("TIT2", in.title);
    emitTextFrame("TPE1", in.artist);
    emitTextFrame("TALB", in.album);
    if (in.year > 0) emitTextFrame("TDRC", QString::number(in.year));
    if (in.track > 0) emitTextFrame("TRCK", QString::number(in.track));
    emitTextFrame("TCON", in.genre);

    // USLT
    if (!in.lyrics.isEmpty()) {
        QByteArray body;
        body.append(char(0x03)); // enc=3 UTF-8
        body.append("eng", 3);    // language
        body.append(char(0));    // descriptor terminator (empty)
        body.append(in.lyrics.toUtf8());
        frames.append("USLT", 4);
        writeSyncsafe(frames, quint32(body.size()));
        frames.append(char(0)); frames.append(char(0));
        frames.append(body);
    }

    // 追加保留的非目标帧
    frames.append(preserveFrames);

    // 组装 tag
    QByteArray tag;
    tag.append("ID3", 3);
    tag.append(char(0x04)); // version major 2.4
    tag.append(char(0x00)); // revision
    tag.append(char(0x00)); // flags: 无 unsynch / 无扩展头 / 无实验
    writeSyncsafe(tag, quint32(frames.size()));
    tag.append(frames);
    return tag;
}

bool TagManager::writeMp3(const QString &path, const TagFields &in, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开文件");
        return false;
    }
    QByteArray allData = f.readAll();
    f.close();

    QByteArray oldTag;
    qint64 audioStart = 0;
    if (allData.size() >= 10 && allData[0] == 'I' && allData[1] == 'D' && allData[2] == '3') {
        quint32 oldSize = readSyncsafe(uchar(allData[6]), uchar(allData[7]),
                                       uchar(allData[8]), uchar(allData[9]));
        oldTag = allData.mid(0, qMin<qint64>(10 + oldSize, allData.size()));
        audioStart = oldTag.size();
    }
    QByteArray audioData = allData.mid(audioStart);

    // 从旧 tag 提取需保留的非目标帧（已 de-unsynchronise，重写为 v2.4）
    QByteArray preserve;
    if (oldTag.size() >= 10) {
        int versionMajor = uchar(oldTag[3]);
        uchar flags = uchar(oldTag[5]);
        quint32 headerSize = readSyncsafe(uchar(oldTag[6]), uchar(oldTag[7]),
                                          uchar(oldTag[8]), uchar(oldTag[9]));
        QByteArray fr = oldTag.mid(10, qMin<qint64>(headerSize, oldTag.size() - 10));
        bool globalUnsynch = (versionMajor == 3) && (flags & 0x80);
        int pos = 0;
        if (flags & 0x40) {
            if (versionMajor == 4 && fr.size() >= 4) {
                pos = qMin(qint64(readSyncsafe(uchar(fr[0]), uchar(fr[1]),
                                                uchar(fr[2]), uchar(fr[3]))),
                           qint64(fr.size()));
            } else if (versionMajor == 3 && fr.size() >= 4) {
                pos = qMin(qint64(readBE32(uchar(fr[0]), uchar(fr[1]),
                                          uchar(fr[2]), uchar(fr[3])) + 4),
                           qint64(fr.size()));
            }
        }
        const int minHeader = (versionMajor == 2) ? 6 : 10;
        // 目标帧 ID 集合（v2.3/2.4 4B 与 v2.2 3B）
        static const QSet<QByteArray> targets = {
            "TIT2", "TPE1", "TALB", "TDRC", "TYER", "TRCK", "TCON", "USLT", "TXXX",
            "TT2", "TP1", "TAL", "TYE", "TRK", "TCO", "ULT" // v2.2
        };
        // v2.2 的 TXXX 等价等不直接保留（罕见格式，跳过以避免头转换复杂性）
        while (pos + minHeader <= fr.size()) {
            QByteArray id;
            quint32 frameSize = 0;
            int headerLen = 10;
            bool frameUnsynch = false;
            if (versionMajor == 2) {
                id = fr.mid(pos, 3);
                if (!isFrameId(id)) break;
                frameSize = readBE24(uchar(fr[pos + 3]), uchar(fr[pos + 4]), uchar(fr[pos + 5]));
                headerLen = 6;
            } else {
                id = fr.mid(pos, 4);
                if (!isFrameId(id)) break;
                if (versionMajor == 4) {
                    frameSize = readSyncsafe(uchar(fr[pos + 4]), uchar(fr[pos + 5]),
                                             uchar(fr[pos + 6]), uchar(fr[pos + 7]));
                    frameUnsynch = (uchar(fr[pos + 8]) & 0x08);
                } else {
                    frameSize = readBE32(uchar(fr[pos + 4]), uchar(fr[pos + 5]),
                                         uchar(fr[pos + 6]), uchar(fr[pos + 7]));
                }
            }
            if (frameSize == 0) { pos += headerLen; continue; }
            int bodyStart = pos + headerLen;
            int bodyEnd = qMin(qint64(bodyStart + frameSize), qint64(fr.size()));
            if (bodyStart >= fr.size()) break;
            QByteArray body = fr.mid(bodyStart, bodyEnd - bodyStart);
            if (globalUnsynch || frameUnsynch) body = deUnsynchronise(body);

            if (!targets.contains(id)) {
                // 仅保留 v2.3/v2.4 的 4B ID 帧；v2.2 的 3B ID 升级复杂，跳过
                if (id.size() == 4) {
                    preserve.append(id);
                    writeSyncsafe(preserve, quint32(body.size()));
                    preserve.append(char(0)); preserve.append(char(0));
                    preserve.append(body);
                }
            }
            pos = bodyEnd;
        }
    }

    QByteArray newTag = buildId3v24Tag(in, preserve);
    QByteArray outData = newTag + audioData;
    return atomicWrite(path, outData, errMsg);
}

// ============================================================
// Vorbis 字段构造
// ============================================================
QList<QPair<QString, QString>> TagManager::buildVorbisFields(
    const TagFields &in, const QMap<QString, QString> &preserve)
{
    // 工作副本：先放保留字段，再用目标 7 字段覆盖（含空串以清空）
    QMap<QString, QString> work = preserve;
    work.insert("TITLE", in.title);
    work.insert("ARTIST", in.artist);
    work.insert("ALBUM", in.album);
    work.insert("DATE", in.year > 0 ? QString::number(in.year) : QString());
    work.insert("TRACKNUMBER", in.track > 0 ? QString::number(in.track) : QString());
    work.insert("GENRE", in.genre);
    work.insert("LYRICS", in.lyrics);

    QList<QPair<QString, QString>> out;
    // 跳过值为空且原 preserve 中也无需保留的键；空值表示清空，直接不写出该字段
    for (auto it = work.constBegin(); it != work.constEnd(); ++it) {
        if (it.value().isEmpty()) continue;
        out.append(qMakePair(it.key(), it.value()));
    }
    return out;
}

// ============================================================
// FLAC 写入
// ============================================================
bool TagManager::writeFlac(const QString &path, const TagFields &in, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开文件");
        return false;
    }
    QByteArray allData = f.readAll();
    f.close();
    if (allData.size() < 4 || allData[0] != 'f' || allData[1] != 'L' ||
        allData[2] != 'a' || allData[3] != 'C') {
        if (errMsg) *errMsg = QStringLiteral("非 FLAC 文件");
        return false;
    }

    int pos = 4;
    QByteArray outData;
    outData.append("fLaC", 4);

    bool foundVorbis = false;
    QByteArray audioFrames;
    QList<QByteArray> blocksBefore; // STREAMINFO 等，原样
    bool lastSeen = false;

    while (pos < allData.size() && !lastSeen) {
        if (pos + 4 > allData.size()) break;
        uchar b0 = uchar(allData[pos]);
        uchar blockType = b0 & 0x7f;
        lastSeen = (b0 & 0x80) != 0;
        quint32 blockSize = readBE24(uchar(allData[pos + 1]), uchar(allData[pos + 2]),
                                      uchar(allData[pos + 3]));
        pos += 4;
        if (pos + blockSize > allData.size()) break;
        QByteArray blockBody = allData.mid(pos, blockSize);
        pos += blockSize;

        if (blockType == 4) { // VORBIS_COMMENT
            QMap<QString, QString> oldFields = parseVorbisCommentFields(blockBody);
            // 提取原 vendor：解析 vendorLen
            QString vendor = "musicplayer";
            if (blockBody.size() >= 4) {
                quint32 vlen = readLE32(uchar(blockBody[0]), uchar(blockBody[1]),
                                        uchar(blockBody[2]), uchar(blockBody[3]));
                if (vlen <= quint32(blockBody.size()) - 4) {
                    vendor = QString::fromUtf8(blockBody.mid(4, vlen));
                }
            }
            QList<QPair<QString, QString>> fields = buildVorbisFields(in, oldFields);
            QByteArray newBody = buildVorbisCommentBlock(vendor, fields);
            // 新块头：type=4，last 位后续统一设置
            QByteArray newBlock;
            newBlock.append(char(0x04)); // type 4，last 位暂 0
            writeBE24(newBlock, quint32(newBody.size()));
            newBlock.append(newBody);
            blocksBefore.append(newBlock);
            foundVorbis = true;
        } else {
            // 保留其它块
            QByteArray blk;
            blk.append(char(uchar(b0 & 0x7f))); // 清掉 last 位，稍后统一设置
            writeBE24(blk, quint32(blockBody.size()));
            blk.append(blockBody);
            blocksBefore.append(blk);
        }
    }

    if (!foundVorbis) {
        // 没有 VORBIS_COMMENT 块，新建一个并插入（在 STREAMINFO 之后）
        QList<QPair<QString, QString>> fields = buildVorbisFields(in, QMap<QString, QString>());
        QByteArray body = buildVorbisCommentBlock("musicplayer", fields);
        QByteArray blk;
        blk.append(char(0x04));
        writeBE24(blk, quint32(body.size()));
        blk.append(body);
        blocksBefore.append(blk);
    }

    // 设置 last 位：最后一块置 1
    for (int i = 0; i < blocksBefore.size(); ++i) {
        QByteArray &blk = blocksBefore[i];
        if (i == blocksBefore.size() - 1) blk[0] = uchar(blk[0] | 0x80);
        outData.append(blk);
    }
    // 追加剩余（音频帧）
    outData.append(allData.mid(pos));
    return atomicWrite(path, outData, errMsg);
}

// ============================================================
// OGG 写入（仅 Vorbis；Opus 标 TODO）
// ============================================================
bool TagManager::writeOgg(const QString &path, const TagFields &in, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开文件");
        return false;
    }
    QByteArray allData = f.readAll();
    f.close();

    // 定位包含 "\x03vorbis" 的 OGG page
    int commentMagicIdx = allData.indexOf("\x03vorbis", 0);
    if (commentMagicIdx < 0) {
        if (allData.indexOf("OpusTags") >= 0) {
            if (errMsg) *errMsg = QStringLiteral("Opus 标签写入暂不支持（TODO）");
            return false;
        }
        if (errMsg) *errMsg = QStringLiteral("未找到 Vorbis 注释包");
        return false;
    }

    // 向前查找该 magic 所在 page 的起点 "OggS"
    int pageStart = allData.lastIndexOf("OggS", commentMagicIdx);
    if (pageStart < 0) {
        if (errMsg) *errMsg = QStringLiteral("无法定位注释页起点");
        return false;
    }

    // 解析 page header
    if (pageStart + 27 > allData.size()) {
        if (errMsg) *errMsg = QStringLiteral("OGG page 头损坏");
        return false;
    }
    // OggS(4) version(1) headerType(1) granule(8) serial(4) pageSeq(4) crc(4) numSeg(1) = 27 字节
    uchar numSeg = uchar(allData[pageStart + 26]);
    int segTableStart = pageStart + 27;
    if (segTableStart + numSeg > allData.size()) {
        if (errMsg) *errMsg = QStringLiteral("segment table 越界");
        return false;
    }
    // 计算 page body 长度 = sum(segment table)
    int bodyLen = 0;
    for (int i = 0; i < numSeg; ++i) bodyLen += uchar(allData[segTableStart + i]);
    int bodyStart = segTableStart + numSeg;
    int pageEnd = bodyStart + bodyLen;

    if (pageEnd > allData.size()) {
        if (errMsg) *errMsg = QStringLiteral("page body 越界");
        return false;
    }

    // commentMagicIdx 必须位于本 page body 内
    if (commentMagicIdx < bodyStart || commentMagicIdx >= pageEnd) {
        if (errMsg) *errMsg = QStringLiteral("注释包跨页，暂不支持");
        return false;
    }

    // headerType：低位 0x01 = continuation page；0x02 = BOS；0x04 = EOS
    uchar headerType = uchar(allData[pageStart + 5]);
    if (headerType & 0x01) {
        if (errMsg) *errMsg = QStringLiteral("注释位于 continuation page，暂不支持");
        return false;
    }

    // 确定旧 comment packet 范围：从 commentMagicIdx 到 setup magic "\x05vorbis"（若同页）或 page body 末尾
    int setupMagicInPage = allData.indexOf("\x05vorbis", bodyStart);
    int oldCommentStart = commentMagicIdx; // 含 "\x03vorbis" 7B
    int oldCommentEnd;
    bool hasSetupInPage = (setupMagicInPage >= 0 && setupMagicInPage < pageEnd);
    if (hasSetupInPage) {
        oldCommentEnd = setupMagicInPage;
    } else {
        oldCommentEnd = pageEnd;
    }
    // 旧 comment packet 体（含 "\x03vorbis" 前缀）
    QByteArray oldCommentPacket = allData.mid(oldCommentStart, oldCommentEnd - oldCommentStart);
    // 旧 vendor 与字段
    QMap<QString, QString> oldFields = parseVorbisCommentFields(oldCommentPacket.mid(7));
    QString vendor = "musicplayer";
    if (oldCommentPacket.size() >= 11) {
        quint32 vlen = readLE32(uchar(oldCommentPacket[7]), uchar(oldCommentPacket[8]),
                                uchar(oldCommentPacket[9]), uchar(oldCommentPacket[10]));
        if (vlen <= quint32(oldCommentPacket.size()) - 11) {
            vendor = QString::fromUtf8(oldCommentPacket.mid(11, vlen));
        }
    }
    QList<QPair<QString, QString>> fields = buildVorbisFields(in, oldFields);
    QByteArray newCommentPacket;
    newCommentPacket.append("\x03vorbis", 7);
    newCommentPacket.append(buildVorbisCommentBlock(vendor, fields));

    // setup packet 在本页的剩余部分（若有）
    QByteArray setupInPageBytes;
    if (hasSetupInPage) {
        setupInPageBytes = allData.mid(oldCommentEnd, pageEnd - oldCommentEnd);
    }

    // 构造新 page body = newCommentPacket + setupInPageBytes（setup 字节不变）
    QByteArray newBody = newCommentPacket + setupInPageBytes;

    // 构造 segment table：必须保留 setup packet 的原 segment 边界。
    // 原始 segment table 已读在 allData[segTableStart .. segTableStart+numSeg]。
    // comment packet 是本页第一个 packet，其 segments 直到第一个 <255（含）。
    int commentSegCount = 0;
    bool commentEndsInPage = false;
    for (int i = 0; i < numSeg; ++i) {
        commentSegCount++;
        if (uchar(allData[segTableStart + i]) < 255) {
            commentEndsInPage = true;
            break;
        }
    }
    if (!commentEndsInPage) {
        // comment packet 在本页未终结（跨页），超出本实现的简化范围
        if (errMsg) *errMsg = QStringLiteral("注释包跨页，暂不支持");
        return false;
    }
    // setup packet 的 segments（本页内 comment 之后的所有段）原样保留——
    // 这保证 setup 跨页延续时的非终结 255 段不被破坏。
    QByteArray setupSegs = allData.mid(segTableStart + commentSegCount, numSeg - commentSegCount);

    // 为新 comment packet 生成 segments：255 填充 + 末段 <255 或 0 终结
    auto segsForPacket = [](int size) -> QByteArray {
        QByteArray s;
        int full = size / 255;
        int rem = size % 255;
        for (int i = 0; i < full; ++i) s.append(char(255));
        if (rem > 0) {
            s.append(char(rem));
        } else if (full > 0) {
            s.append(char(0)); // 整除：追加 0 长度段表示 packet 结束
        } else {
            s.append(char(0)); // 空 packet
        }
        return s;
    };
    QByteArray newCommentSegs = segsForPacket(newCommentPacket.size());
    QByteArray newSegTable = newCommentSegs + setupSegs;
    if (newSegTable.size() > 255) {
        // 单页段数上限 255；comment 过大或 setup 段过多时无法放入一页
        if (errMsg) *errMsg = QStringLiteral("新注释过大，无法放入单页");
        return false;
    }
    uchar newNumSeg = uchar(newSegTable.size());

    // 构造新 page header：保留原 page 的所有固定字段，仅重写 numSeg/segTable/CRC
    // 头 27 字节 = "OggS"+ver+headerType+granule(8)+serial(4)+pageSeq(4)+crc(4)+numSeg(1)
    QByteArray newHeader;
    newHeader.append(allData.mid(pageStart, 22)); // "OggS" 到 pageSeq 末（不含 CRC）
    int crcPos = newHeader.size();
    newHeader.append(char(0)); newHeader.append(char(0));
    newHeader.append(char(0)); newHeader.append(char(0)); // CRC 占位
    newHeader.append(char(newNumSeg));
    newHeader.append(newSegTable);

    // 计算 CRC：覆盖整个 page（header + segment table + body），CRC 字段位置先置 0
    QByteArray crcBuf = newHeader + newBody;
    quint32 crc = oggCrc32(crcBuf);
    // 填回 CRC（LE）
    newHeader[crcPos + 0] = char(uchar(crc & 0xFF));
    newHeader[crcPos + 1] = char(uchar((crc >> 8) & 0xFF));
    newHeader[crcPos + 2] = char(uchar((crc >> 16) & 0xFF));
    newHeader[crcPos + 3] = char(uchar((crc >> 24) & 0xFF));

    // 拼接新文件
    QByteArray outData;
    outData.append(allData.mid(0, pageStart));
    outData.append(newHeader);
    outData.append(newBody);
    outData.append(allData.mid(pageEnd));
    return atomicWrite(path, outData, errMsg);
}

// ============================================================
// WAV / RIFF ID3 写入
// ============================================================
bool TagManager::writeWav(const QString &path, const TagFields &in, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开文件");
        return false;
    }
    QByteArray allData = f.readAll();
    f.close();
    if (allData.size() < 12 || allData.mid(0, 4) != "RIFF" || allData.mid(8, 4) != "WAVE") {
        if (errMsg) *errMsg = QStringLiteral("非 WAV 文件");
        return false;
    }

    // 先读旧 ID3 chunk（若存在），保留非目标帧
    QByteArray preserve;
    int pos = 12;
    int id3ChunkStart = -1, id3ChunkEnd = -1;
    while (pos + 8 <= allData.size()) {
        QByteArray id = allData.mid(pos, 4);
        quint32 sz = readLE32(uchar(allData[pos + 4]), uchar(allData[pos + 5]),
                              uchar(allData[pos + 6]), uchar(allData[pos + 7]));
        int bodyStart = pos + 8;
        int bodyEnd = bodyStart + sz;
        if (id == "ID3 " || id == "id3 ") {
            id3ChunkStart = pos;
            id3ChunkEnd = bodyEnd;
            // 从旧 ID3 提取保留帧
            QByteArray oldTag = allData.mid(bodyStart, sz);
            if (oldTag.size() >= 10) {
                uchar versionMajor = uchar(oldTag[3]);
                quint32 headerSize = readSyncsafe(uchar(oldTag[6]), uchar(oldTag[7]),
                                                   uchar(oldTag[8]), uchar(oldTag[9]));
                QByteArray fr = oldTag.mid(10, qMin<qint64>(headerSize, oldTag.size() - 10));
                int p = 0;
                const int minHeader = (versionMajor == 2) ? 6 : 10;
                static const QSet<QByteArray> targets = {
                    "TIT2", "TPE1", "TALB", "TDRC", "TYER", "TRCK", "TCON", "USLT", "TXXX"
                };
                while (p + minHeader <= fr.size()) {
                    QByteArray fid;
                    quint32 fs = 0; int hl = 10;
                    if (versionMajor == 2) {
                        fid = fr.mid(p, 3);
                        if (!isFrameId(fid)) break;
                        fs = readBE24(uchar(fr[p + 3]), uchar(fr[p + 4]), uchar(fr[p + 5]));
                        hl = 6;
                    } else {
                        fid = fr.mid(p, 4);
                        if (!isFrameId(fid)) break;
                        if (versionMajor == 4)
                            fs = readSyncsafe(uchar(fr[p + 4]), uchar(fr[p + 5]),
                                              uchar(fr[p + 6]), uchar(fr[p + 7]));
                        else
                            fs = readBE32(uchar(fr[p + 4]), uchar(fr[p + 5]),
                                          uchar(fr[p + 6]), uchar(fr[p + 7]));
                    }
                    if (fs == 0) { p += hl; continue; }
                    int bs = p + hl, be = qMin(qint64(bs + fs), qint64(fr.size()));
                    if (bs >= fr.size()) break;
                    QByteArray fbody = fr.mid(bs, be - bs);
                    if (!targets.contains(fid) && fid.size() == 4) {
                        preserve.append(fid);
                        writeSyncsafe(preserve, quint32(fbody.size()));
                        preserve.append(char(0)); preserve.append(char(0));
                        preserve.append(fbody);
                    }
                    p = be;
                }
            }
            break;
        }
        pos = bodyEnd + (sz & 1); // 奇数补 1 字节对齐
    }

    QByteArray newTag = buildId3v24Tag(in, preserve);
    // 组装新 ID3 chunk
    QByteArray newId3Chunk;
    newId3Chunk.append("ID3 ", 4);
    writeLE32(newId3Chunk, quint32(newTag.size()));
    newId3Chunk.append(newTag);
    if (newTag.size() & 1) newId3Chunk.append(char(0)); // 对齐

    QByteArray outData;
    if (id3ChunkStart >= 0) {
        // 替换原 ID3 chunk
        outData.append(allData.mid(0, id3ChunkStart));
        outData.append(newId3Chunk);
        outData.append(allData.mid(id3ChunkEnd));
    } else {
        // 追加到末尾
        outData.append(allData);
        outData.append(newId3Chunk);
    }

    // 更新 RIFF size = 文件总大小 - 8
    quint32 riffSize = quint32(outData.size() - 8);
    writeLE32At(outData, 4, riffSize); // 用辅助函数
    return atomicWrite(path, outData, errMsg);
}

// ============================================================
// MP4 写入
// ============================================================
bool TagManager::writeMp4(const QString &path, const TagFields &in, QString *errMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errMsg) *errMsg = QStringLiteral("无法打开文件");
        return false;
    }
    QByteArray allData = f.readAll();
    f.close();
    qint64 fsize = allData.size();

    // 顶层扫描：定位 moov 与 mdat，判断 moov 是否在 mdat 之前
    qint64 moovStart = -1, moovSize = 0, moovHdrLen = 0;
    qint64 mdatStart = -1;
    qint64 pos = 0;
    while (pos + 8 <= fsize) {
        quint32 sz32 = readBE32(uchar(allData[pos]), uchar(allData[pos + 1]),
                                uchar(allData[pos + 2]), uchar(allData[pos + 3]));
        QByteArray type = allData.mid(pos + 4, 4);
        qint64 headerLen = 8;
        qint64 atomSize = qint64(sz32);
        if (sz32 == 1) {
            if (pos + 16 > fsize) break;
            quint64 hi = readBE32(uchar(allData[pos + 8]), uchar(allData[pos + 9]),
                                  uchar(allData[pos + 10]), uchar(allData[pos + 11]));
            quint64 lo = readBE32(uchar(allData[pos + 12]), uchar(allData[pos + 13]),
                                  uchar(allData[pos + 14]), uchar(allData[pos + 15]));
            atomSize = qint64((hi << 32) | lo);
            headerLen = 16;
        } else if (sz32 == 0) {
            atomSize = fsize - pos;
        }
        if (atomSize < headerLen) break;
        if (type == "moov") { moovStart = pos; moovSize = atomSize; moovHdrLen = headerLen; }
        else if (type == "mdat") { mdatStart = pos; }
        pos += atomSize;
    }

    if (moovStart < 0) {
        if (errMsg) *errMsg = QStringLiteral("未找到 moov atom");
        return false;
    }
    if (mdatStart >= 0 && moovStart < mdatStart) {
        if (errMsg) *errMsg = QStringLiteral("暂不支持 moov 在 mdat 之前的布局");
        return false;
    }

    qint64 moovBodyStart = moovStart + moovHdrLen;
    qint64 moovBodyEnd = moovStart + moovSize;

    // 在 moov 内查找 udta → meta → ilst
    auto findChild = [&](qint64 start, qint64 end, const QByteArray &type,
                         qint64 &cStart, qint64 &cSize, qint64 &cHdrLen) -> bool {
        qint64 p = start;
        while (p + 8 <= end) {
            quint32 sz32 = readBE32(uchar(allData[p]), uchar(allData[p + 1]),
                                    uchar(allData[p + 2]), uchar(allData[p + 3]));
            QByteArray t = allData.mid(p + 4, 4);
            qint64 hl = 8, asz = qint64(sz32);
            if (sz32 == 1) {
                quint64 hi = readBE32(uchar(allData[p + 8]), uchar(allData[p + 9]),
                                      uchar(allData[p + 10]), uchar(allData[p + 11]));
                quint64 lo = readBE32(uchar(allData[p + 12]), uchar(allData[p + 13]),
                                      uchar(allData[p + 14]), uchar(allData[p + 15]));
                asz = qint64((hi << 32) | lo); hl = 16;
            } else if (sz32 == 0) asz = end - p;
            if (asz < hl) return false;
            if (t == type) { cStart = p; cSize = asz; cHdrLen = hl; return true; }
            p += asz;
        }
        return false;
    };

    qint64 udtaStart, udtaSize, udtaHdrLen;
    if (!findChild(moovBodyStart, moovBodyEnd, "udta", udtaStart, udtaSize, udtaHdrLen)) {
        if (errMsg) *errMsg = QStringLiteral("未找到 udta atom");
        return false;
    }
    qint64 udtaBodyStart = udtaStart + udtaHdrLen;
    qint64 udtaBodyEnd = udtaStart + udtaSize;

    qint64 metaStart, metaSize, metaHdrLen;
    if (!findChild(udtaBodyStart, udtaBodyEnd, "meta", metaStart, metaSize, metaHdrLen)) {
        if (errMsg) *errMsg = QStringLiteral("未找到 meta atom");
        return false;
    }
    // meta body 头 4B version+flags
    qint64 metaVersionLen = 4;
    qint64 metaBodyChildrenStart = metaStart + metaHdrLen + metaVersionLen;
    qint64 metaBodyEnd = metaStart + metaSize;

    qint64 ilstStart, ilstSize, ilstHdrLen;
    if (!findChild(metaBodyChildrenStart, metaBodyEnd, "ilst", ilstStart, ilstSize, ilstHdrLen)) {
        if (errMsg) *errMsg = QStringLiteral("未找到 ilst atom");
        return false;
    }
    qint64 ilstBodyStart = ilstStart + ilstHdrLen;
    qint64 ilstBodyEnd = ilstStart + ilstSize;

    // 遍历 ilst 子 atom，重建
    static const QByteArray kNam = QByteArray("\xA9nam", 4);
    static const QByteArray kArt = QByteArray("\xA9" "ART", 4);
    static const QByteArray kAlb = QByteArray("\xA9" "alb", 4);
    static const QByteArray kDay = QByteArray("\xA9" "day", 4);
    static const QByteArray kGen = QByteArray("\xA9gen", 4);
    static const QByteArray kLyr = QByteArray("\xA9lyr", 4);
    static const QByteArray kTrk = QByteArray("trkn", 4);

    QByteArray newIlstBody;
    qint64 p = ilstBodyStart;
    while (p + 8 <= ilstBodyEnd) {
        quint32 sz32 = readBE32(uchar(allData[p]), uchar(allData[p + 1]),
                                uchar(allData[p + 2]), uchar(allData[p + 3]));
        QByteArray type = allData.mid(p + 4, 4);
        qint64 hl = 8, asz = qint64(sz32);
        if (sz32 == 1) {
            quint64 hi = readBE32(uchar(allData[p + 8]), uchar(allData[p + 9]),
                                  uchar(allData[p + 10]), uchar(allData[p + 11]));
            quint64 lo = readBE32(uchar(allData[p + 12]), uchar(allData[p + 13]),
                                  uchar(allData[p + 14]), uchar(allData[p + 15]));
            asz = qint64((hi << 32) | lo); hl = 16;
        } else if (sz32 == 0) asz = ilstBodyEnd - p;
        if (asz < hl) break;
        QByteArray atomBytes = allData.mid(p, asz);

        // 目标子 atom：替换；其它：原样保留
        if (type == kNam) newIlstBody.append(buildMp4TextAtom(kNam, in.title));
        else if (type == kArt) newIlstBody.append(buildMp4TextAtom(kArt, in.artist));
        else if (type == kAlb) newIlstBody.append(buildMp4TextAtom(kAlb, in.album));
        else if (type == kDay) {
            if (in.year > 0) newIlstBody.append(buildMp4TextAtom(kDay, QString::number(in.year)));
        }
        else if (type == kGen) newIlstBody.append(buildMp4TextAtom(kGen, in.genre));
        else if (type == kLyr) newIlstBody.append(buildMp4TextAtom(kLyr, in.lyrics));
        else if (type == kTrk) {
            if (in.track > 0) newIlstBody.append(buildMp4TrknAtom(in.track));
        }
        else newIlstBody.append(atomBytes); // 保留

        p += asz;
    }
    // 补加缺失的目标 atom（若原文件没有该字段，追加到 ilst 末尾）
    // 简化：只在原 ilst 中不存在时追加。这里靠上方"替换"逻辑若原无则未追加，
    // 下面统一补加未出现的字段（通过检查 newIlstBody 中是否已含 type）。
    auto containsType = [&](const QByteArray &type) -> bool {
        // newIlstBody 由若干 [4B size][4B type] atom 组成
        int q = 0;
        while (q + 8 <= newIlstBody.size()) {
            QByteArray t = newIlstBody.mid(q + 4, 4);
            if (t == type) return true;
            quint32 s = readBE32(uchar(newIlstBody[q]), uchar(newIlstBody[q + 1]),
                                 uchar(newIlstBody[q + 2]), uchar(newIlstBody[q + 3]));
            if (s < 8) break;
            q += s;
        }
        return false;
    };
    if (!in.title.isEmpty() && !containsType(kNam)) newIlstBody.append(buildMp4TextAtom(kNam, in.title));
    if (!in.artist.isEmpty() && !containsType(kArt)) newIlstBody.append(buildMp4TextAtom(kArt, in.artist));
    if (!in.album.isEmpty() && !containsType(kAlb)) newIlstBody.append(buildMp4TextAtom(kAlb, in.album));
    if (in.year > 0 && !containsType(kDay)) newIlstBody.append(buildMp4TextAtom(kDay, QString::number(in.year)));
    if (!in.genre.isEmpty() && !containsType(kGen)) newIlstBody.append(buildMp4TextAtom(kGen, in.genre));
    if (!in.lyrics.isEmpty() && !containsType(kLyr)) newIlstBody.append(buildMp4TextAtom(kLyr, in.lyrics));
    if (in.track > 0 && !containsType(kTrk)) newIlstBody.append(buildMp4TrknAtom(in.track));

    // 构造新 ilst
    QByteArray newIlst;
    writeBE32(newIlst, quint32(8 + newIlstBody.size()));
    newIlst.append("ilst");
    newIlst.append(newIlstBody);

    // 构造新 meta（保留 4B 版本前缀 + 其它子 atom + 新 ilst）
    // meta body = version(4) + 其它子 atom（除原 ilst 外）+ 新 ilst
    QByteArray metaVersionBytes = allData.mid(metaStart + metaHdrLen, metaVersionLen);
    QByteArray newMetaBody;
    newMetaBody.append(metaVersionBytes);
    // 拷贝 meta 内除 ilst 外的其它子 atom
    qint64 mp = metaBodyChildrenStart;
    while (mp + 8 <= metaBodyEnd) {
        quint32 sz32 = readBE32(uchar(allData[mp]), uchar(allData[mp + 1]),
                                uchar(allData[mp + 2]), uchar(allData[mp + 3]));
        QByteArray t = allData.mid(mp + 4, 4);
        qint64 hl = 8, asz = qint64(sz32);
        if (sz32 == 1) {
            quint64 hi = readBE32(uchar(allData[mp + 8]), uchar(allData[mp + 9]),
                                  uchar(allData[mp + 10]), uchar(allData[mp + 11]));
            quint64 lo = readBE32(uchar(allData[mp + 12]), uchar(allData[mp + 13]),
                                  uchar(allData[mp + 14]), uchar(allData[mp + 15]));
            asz = qint64((hi << 32) | lo); hl = 16;
        } else if (sz32 == 0) asz = metaBodyEnd - mp;
        if (asz < hl) break;
        if (t != "ilst") {
            newMetaBody.append(allData.mid(mp, asz));
        }
        mp += asz;
    }
    newMetaBody.append(newIlst);
    QByteArray newMeta;
    writeBE32(newMeta, quint32(8 + newMetaBody.size()));
    newMeta.append("meta");
    newMeta.append(newMetaBody);

    // 构造新 udta（保留其它子 atom + 新 meta）
    QByteArray newUdtaBody;
    qint64 up = udtaBodyStart;
    while (up + 8 <= udtaBodyEnd) {
        quint32 sz32 = readBE32(uchar(allData[up]), uchar(allData[up + 1]),
                                uchar(allData[up + 2]), uchar(allData[up + 3]));
        QByteArray t = allData.mid(up + 4, 4);
        qint64 hl = 8, asz = qint64(sz32);
        if (sz32 == 1) {
            quint64 hi = readBE32(uchar(allData[up + 8]), uchar(allData[up + 9]),
                                  uchar(allData[up + 10]), uchar(allData[up + 11]));
            quint64 lo = readBE32(uchar(allData[up + 12]), uchar(allData[up + 13]),
                                  uchar(allData[up + 14]), uchar(allData[up + 15]));
            asz = qint64((hi << 32) | lo); hl = 16;
        } else if (sz32 == 0) asz = udtaBodyEnd - up;
        if (asz < hl) break;
        if (t != "meta") {
            newUdtaBody.append(allData.mid(up, asz));
        }
        up += asz;
    }
    newUdtaBody.append(newMeta);
    QByteArray newUdta;
    writeBE32(newUdta, quint32(8 + newUdtaBody.size()));
    newUdta.append("udta");
    newUdta.append(newUdtaBody);

    // 构造新 moov（保留其它子 atom + 新 udta）
    QByteArray newMoovBody;
    qint64 mvp = moovBodyStart;
    while (mvp + 8 <= moovBodyEnd) {
        quint32 sz32 = readBE32(uchar(allData[mvp]), uchar(allData[mvp + 1]),
                                uchar(allData[mvp + 2]), uchar(allData[mvp + 3]));
        QByteArray t = allData.mid(mvp + 4, 4);
        qint64 hl = 8, asz = qint64(sz32);
        if (sz32 == 1) {
            quint64 hi = readBE32(uchar(allData[mvp + 8]), uchar(allData[mvp + 9]),
                                  uchar(allData[mvp + 10]), uchar(allData[mvp + 11]));
            quint64 lo = readBE32(uchar(allData[mvp + 12]), uchar(allData[mvp + 13]),
                                  uchar(allData[mvp + 14]), uchar(allData[mvp + 15]));
            asz = qint64((hi << 32) | lo); hl = 16;
        } else if (sz32 == 0) asz = moovBodyEnd - mvp;
        if (asz < hl) break;
        if (t != "udta") {
            newMoovBody.append(allData.mid(mvp, asz));
        }
        mvp += asz;
    }
    newMoovBody.append(newUdta);
    QByteArray newMoov;
    writeBE32(newMoov, quint32(8 + newMoovBody.size()));
    newMoov.append("moov");
    newMoov.append(newMoovBody);

    // 写出新文件：moov 之前 + 新 moov + moov 之后
    QByteArray outData;
    outData.append(allData.mid(0, moovStart));
    outData.append(newMoov);
    outData.append(allData.mid(moovStart + moovSize));
    return atomicWrite(path, outData, errMsg);
}
