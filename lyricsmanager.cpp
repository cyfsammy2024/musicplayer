#include "lyricsmanager.h"
#include "tagutils.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QByteArray>
#include <QSet>
#include <QStringConverter>
#include <QIODevice>
#include <QVector>

// 编码转换的平台选择：Windows CRT 不含 iconv，改用系统 API；
// 其余平台（Linux/macOS）使用 iconv。
#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#else
#include <iconv.h>
#endif

// ============================================================
// 匿名命名空间：仅保留 lyrics 专用解析函数。
// 通用二进制读写辅助（readBE32 / decodeText / readId3v2Tag 等）已迁移至
// TagUtils 命名空间（tagutils.h/.cpp），读写两侧共用。
// ============================================================
namespace {

using namespace TagUtils;

// 严格字节级 UTF-8 校验（用于判断 .lrc 文件编码）。
// 注意：Qt6 的 QStringDecoder(Utf8) 默认是 lenient 模式，会把非法字节替换为
// U+FFFD 而不报 hasError()，无法用来判编码，因此必须按字节逐位校验。
bool isValidUtf8(const QByteArray &data) {
    const int n = data.size();
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    for (int i = 0; i < n; ) {
        uchar c = p[i];
        if (c <= 0x7F) { ++i; continue; } // ASCII

        int need = 0;
        if ((c & 0xE0) == 0xC0) need = 1;      // 2 字节：U+0080..U+07FF
        else if ((c & 0xF0) == 0xE0) need = 2; // 3 字节：U+0800..U+FFFF
        else if ((c & 0xF8) == 0xF0) need = 3; // 4 字节：U+10000..U+10FFFF
        else return false; // 非法首字节（含 0x80-0xBF 的 continuation 字节，GBK 高位字节正是此类）

        if (i + 1 + need > n) return false; // 截断
        for (int j = 1; j <= need; ++j) {
            if ((p[i + j] & 0xC0) != 0x80) return false; // continuation 必须是 10xxxxxx
        }
        // 过长编码 / 越界检查
        if (need == 1 && (c & 0x1E) == 0) return false;                       // C0/C1 过长（< U+0080）
        if (need == 2 && c == 0xE0 && (p[i + 1] & 0x20) == 0) return false;    // E0 过长（< U+0800）
        if (need == 2 && c == 0xED && (p[i + 1] & 0x20) != 0) return false;    // 代理区 U+D800..U+DFFF
        if (need == 3 && c == 0xF0 && (p[i + 1] & 0x30) == 0) return false;   // F0 过长（< U+10000）
        if (need == 3 && c == 0xF4 && (p[i + 1] & 0x30) != 0) return false;    // > U+10FFFF
        i += 1 + need;
    }
    return true;
}

// 将 GBK（兼容 GB2312）字节流转为 UTF-8 字节流；失败返回空。
// 各平台策略：Linux 由 glibc 内置 iconv（无需额外链接）；macOS 需链接 libiconv；
// Windows 走系统 API（见下方 convertViaWinCodePage）。修复 GBK 编码 .lrc 既不处理的问题。

#if defined(Q_OS_WIN)

// Windows 实现：经 UTF-16 中转完成 指定代码页 -> UTF-8 转换。
// MB_ERR_INVALID_CHARS 使非法输入字节直接失败，与 iconv 的严格失败语义一致。
// codePage：54936 = GB18030，936 = GBK。
QByteArray convertViaWinCodePage(const char *data, int size, UINT codePage) {
    if (!data || size <= 0) return QByteArray();

    // 第一步：源字节流解码为 UTF-16（两次调用：先求长度，再实际转换）
    int wideLen = ::MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
                                        data, size, nullptr, 0);
    if (wideLen <= 0) return QByteArray();
    std::wstring wide(static_cast<size_t>(wideLen), L'\0');
    wideLen = ::MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
                                    data, size, &wide[0], wideLen);
    if (wideLen <= 0) return QByteArray();

    // 第二步：UTF-16 编码为 UTF-8 输出
    const int utf8Len = ::WideCharToMultiByte(CP_UTF8, 0,
                                              wide.data(), wideLen,
                                              nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0) return QByteArray();
    QByteArray out(utf8Len, Qt::Uninitialized);
    if (::WideCharToMultiByte(CP_UTF8, 0,
                              wide.data(), wideLen,
                              out.data(), utf8Len, nullptr, nullptr) <= 0) {
        return QByteArray();
    }
    return out;
}

QByteArray convertGbkToUtf8(const QByteArray &src) {
    if (src.isEmpty()) return QByteArray();
    // 先按更宽的 GB18030（代码页 54936）尝试，覆盖 GBK 及其扩展字符；
    // 失败再按纯 GBK（代码页 936）解码。
    QByteArray out = convertViaWinCodePage(src.constData(), src.size(), 54936);
    if (out.isEmpty()) {
        out = convertViaWinCodePage(src.constData(), src.size(), 936);
    }
    return out;
}

#else

QByteArray convertGbkToUtf8(const QByteArray &src) {
    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        cd = iconv_open("UTF-8", "GB18030"); // 回退到更宽的超集编码
    }
    if (cd == reinterpret_cast<iconv_t>(-1)) return QByteArray();

    QByteArray out;
    out.resize(src.size() * 4 + 4); // GBK 单字 ≤2B→UTF-8 ≤4B，留余量
    char *inBuf = const_cast<char *>(src.constData());
    size_t inLeft = static_cast<size_t>(src.size());
    char *outBuf = out.data();
    size_t outLeft = static_cast<size_t>(out.size());
    size_t done = iconv(cd, &inBuf, &inLeft, &outBuf, &outLeft);
    iconv_close(cd);
    if (done == static_cast<size_t>(-1)) return QByteArray();
    out.resize(out.size() - static_cast<int>(outLeft));
    return out;
}

#endif

// LRC 时间戳探测/解析用的 regex（与既有实现保持一致）
const QRegularExpression &lrcTimeRegex() {
    static const QRegularExpression re(QStringLiteral("\\[(\\d+):(\\d+)(\\.\\d+)?\\]"));
    return re;
}

// 解析 USLT 帧体
QString parseUsltBody(const QByteArray &body) {
    // [1B enc][3B lang][descriptor 终结][lyrics 余下全部]
    if (body.size() < 4) return QString();
    quint8 enc = uchar(body[0]);
    int descEnd = skipDescriptor(body, 4, enc);
    if (descEnd >= body.size()) return QString();
    return decodeText(enc, body.mid(descEnd));
}

// 解析 TXXX 帧体 -> (description, value)，用于 ffmpeg 等工具把歌词写入 TXXX 的情形
void parseTxxxBody(const QByteArray &body, QString &desc, QString &value) {
    desc.clear();
    value.clear();
    if (body.size() < 1) return;
    quint8 enc = uchar(body[0]);
    const int descStart = 1; // TXXX: enc(1) + desc + value（无 lang 字段）
    int descEnd = skipDescriptor(body, descStart, enc);
    int termLen = (enc == 1 || enc == 2) ? 2 : 1;
    int descLen = descEnd - descStart - termLen;
    if (descLen < 0) descLen = 0;
    desc = decodeText(enc, body.mid(descStart, descLen));
    if (descEnd < body.size()) {
        value = decodeText(enc, body.mid(descEnd));
    }
}

// 判断 TXXX 描述符是否为歌词类（ffmpeg 的非标准 desc="USLT"，或常见 "lyrics" 等）
bool isLyricsDescription(const QString &desc) {
    if (desc.isEmpty()) return false;
    const QString d = desc.trimmed().toUpper();
    static const QVector<QString> keys = {
        "USLT", "LYRICS", "LYRIC", "UNSYNCEDLYRICS",
        "UNSYNCED LYRICS", "SYNCEDLYRICS", "SYNCED LYRICS"
    };
    for (const QString &k : keys) {
        if (d == k) return true;
    }
    return d.contains("LYRIC") || d.contains("USLT");
}

// 解析 SYLT 帧体，合成为 [mm:ss.xx]text\n 的 LRC 串
QString parseSyltBody(const QByteArray &body) {
    // [1B enc][3B lang][1B timefmt][1B contentType][descriptor 终结]
    //  循环: [text 终结(按enc)] [4B 普通 BE ms 时间戳]
    if (body.size() < 6) return QString();
    quint8 enc = uchar(body[0]);
    quint8 timeFmt = uchar(body[4]);
    int pos = skipDescriptor(body, 6, enc);
    const int n = body.size();

    QString lrc;
    while (pos < n) {
        // 读文本到终结符
        int textEnd = pos;
        if (enc == 1 || enc == 2) {
            while (textEnd + 1 < n &&
                   !(uchar(body[textEnd]) == 0x00 && uchar(body[textEnd + 1]) == 0x00)) {
                textEnd += 2;
            }
        } else {
            while (textEnd < n && uchar(body[textEnd]) != 0x00) {
                ++textEnd;
            }
        }
        QByteArray textBytes = body.mid(pos, textEnd - pos);
        int tsStart = (enc == 1 || enc == 2) ? textEnd + 2 : textEnd + 1;
        if (tsStart + 4 > n) break;
        quint32 ts = readBE32(uchar(body[tsStart]), uchar(body[tsStart + 1]),
                              uchar(body[tsStart + 2]), uchar(body[tsStart + 3]));

        QString text = decodeText(enc, textBytes);
        if (timeFmt == 2) { // 毫秒
            qint64 ms = ts;
            int totalSec = int(ms / 1000);
            int mm = totalSec / 60;
            int ss = totalSec % 60;
            int cs = int(ms % 1000) / 10; // 厘秒
            lrc += QStringLiteral("[%1:%2.%3]")
                       .arg(mm, 2, 10, QChar('0'))
                       .arg(ss, 2, 10, QChar('0'))
                       .arg(cs, 2, 10, QChar('0'));
        }
        lrc += text + "\n";
        pos = tsStart + 4;
    }
    return lrc;
}

// 解析 Vorbis 注释块（FLAC 与 OGG 共用），返回歌词文本。
// 通用字段解析已迁移至 TagUtils::parseVorbisCommentFields；此处按歌词键优先级挑选。
QString parseVorbisComments(const QByteArray &data) {
    const QMap<QString, QString> fields = parseVorbisCommentFields(data);
    if (fields.isEmpty()) return QString();
    // 带时间戳优先
    if (fields.contains("SYNCEDLYRICS") && !fields.value("SYNCEDLYRICS").isEmpty()) {
        return fields.value("SYNCEDLYRICS");
    }
    static const QVector<QString> keys = {"LYRICS", "UNSYNCEDLYRICS", "LYRIC"};
    for (const QString &k : keys) {
        if (fields.contains(k) && !fields.value(k).isEmpty()) {
            return fields.value(k);
        }
    }
    return QString();
}

// 递归遍历 MP4 atom 树寻找 ©lyr
QString findMp4Lyrics(QIODevice &dev, qint64 start, qint64 end) {
    static const QByteArray kAtomLyr = QByteArray("\xA9lyr", 4);
    // 容器 atom（需递归）
    static const QSet<QByteArray> kContainers = {
        "moov", "trak", "mdia", "minf", "stbl",
        "udta", "dinf", "edts", "meta", "ilst"
    };
    qint64 pos = start;
    while (pos + 8 <= end) {
        if (!dev.seek(pos)) return QString();
        QByteArray hdr = dev.read(8);
        if (hdr.size() < 8) return QString();
        quint32 size32 = readBE32(uchar(hdr[0]), uchar(hdr[1]), uchar(hdr[2]), uchar(hdr[3]));
        QByteArray type = hdr.mid(4, 4);
        qint64 headerLen = 8;
        qint64 atomSize = qint64(size32);
        if (size32 == 1) {
            // 64 位扩展 size
            QByteArray ext = dev.read(8);
            if (ext.size() < 8) return QString();
            quint64 hi = readBE32(uchar(ext[0]), uchar(ext[1]), uchar(ext[2]), uchar(ext[3]));
            quint64 lo = readBE32(uchar(ext[4]), uchar(ext[5]), uchar(ext[6]), uchar(ext[7]));
            atomSize = qint64((hi << 32) | lo);
            headerLen = 16;
        } else if (size32 == 0) {
            atomSize = end - pos; // 延伸到末尾
        }
        if (atomSize < headerLen) break; // 异常，避免死循环
        qint64 bodyStart = pos + headerLen;
        qint64 bodyEnd = pos + atomSize;
        if (bodyEnd > end) bodyEnd = end;
        if (bodyEnd < bodyStart) break; // 仅在异常时中断；空 body atom（如 free）应跳过

        if (kContainers.contains(type)) {
            // meta atom 子项前有 4 字节 version+flags 前缀，需跳过
            qint64 childStart = (type == "meta") ? bodyStart + 4 : bodyStart;
            if (childStart > bodyEnd) childStart = bodyEnd;
            QString res = findMp4Lyrics(dev, childStart, bodyEnd);
            if (!res.isEmpty()) return res;
        } else if (type == kAtomLyr) {
            if (!dev.seek(bodyStart)) return QString();
            QByteArray body = dev.read(bodyEnd - bodyStart);
            // iTunes 风格：body 以 data 子 atom 开头
            if (body.size() >= 8 && body.mid(4, 4) == "data") {
                // data atom: [4B size][4B "data"][4B type-indicator][4B locale][value...]
                const int valueStart = 16;
                if (body.size() >= valueStart) {
                    return QString::fromUtf8(body.mid(valueStart));
                }
                return QString();
            }
            // 老 QuickTime 风格：body 整体即 UTF-8 文本
            return QString::fromUtf8(body);
        }
        pos = bodyEnd;
    }
    return QString();
}

} // namespace

// ============================================================
// LyricsManager 实现
// ============================================================
LyricsManager::LyricsManager(QObject *parent) : QObject(parent)
{
}

bool LyricsManager::loadLyrics(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false; // 打开失败不 emit（保持既有行为，由 dispatcher 负责补 emit）
    }
    QByteArray data = file.readAll();
    file.close();

    // 剥离起始 UTF-8 BOM（GBK/GB2312 无 BOM，仅影响 UTF-8 文件）
    if (data.startsWith("\xEF\xBB\xBF")) {
        data = data.mid(3);
    }

    // 先按 UTF-8 严格校验；若含非法序列，按 GBK（兼容 GB2312）解码。
    // 修复此前 GBK 编码 .lrc 既不处理的问题。
    QString content;
    if (isValidUtf8(data)) {
        content = QString::fromUtf8(data);
    } else {
        QByteArray utf8 = convertGbkToUtf8(data);
        content = QString::fromUtf8(!utf8.isEmpty() ? utf8 : data);
    }
    // 兜底剥离可能的 BOM
    if (content.startsWith(QChar(0xFEFF))) {
        content.remove(0, 1);
    }
    return loadLyricsFromString(content);
}

bool LyricsManager::loadLyricsFromString(const QString &content)
{
    m_lyricsMap.clear();

    const QRegularExpression regex = lrcTimeRegex();
    const QStringList lines = content.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        QRegularExpressionMatchIterator it = regex.globalMatch(line);
        QString lyricsText;

        // 第一遍：确定所有时间标签之后的歌词文本
        int lastMatchEnd = 0;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            lastMatchEnd = match.capturedEnd();
        }
        if (lastMatchEnd < line.length()) {
            lyricsText = line.mid(lastMatchEnd).trimmed();
        }

        // 第二遍：为每个时间标签回填同一句歌词（保留"一行多时间戳共享同句"语义）
        it = regex.globalMatch(line);
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

    emit allLyricsChanged(getAllLyrics());

    return !m_lyricsMap.isEmpty();
}

bool LyricsManager::loadLyricsFromMedia(const QUrl &mediaUrl)
{
    m_lyricsMap.clear();

    QString mediaPath = mediaUrl.toLocalFile();
    if (mediaPath.isEmpty()) {
        emit allLyricsChanged(getAllLyrics());
        return false;
    }

    // 1) 优先外挂同名 .lrc（质量最高）
    QString lyricsPath = mediaPath.left(mediaPath.lastIndexOf('.')) + ".lrc";
    if (QFile::exists(lyricsPath) && loadLyrics(lyricsPath)) {
        return true; // loadLyrics 内部已 emit allLyricsChanged
    }

    // 外挂失败：清空残留状态后读取音乐文件内嵌歌词
    m_lyricsMap.clear();

    // 2) 读取音乐文件内嵌歌词
    QString embedded = extractEmbeddedLyrics(mediaPath);

    // 3) 含 LRC 时间戳：复用解析器
    if (!embedded.isEmpty() && lrcTimeRegex().match(embedded).hasMatch()) {
        return loadLyricsFromString(embedded);
    }

    // 4) 纯文本（无时间戳）：单条目 @0，整段可被 getLyricsAtTime 返回并高亮
    if (!embedded.isEmpty()) {
        m_lyricsMap.clear();
        m_lyricsMap[0] = embedded;
        emit allLyricsChanged(getAllLyrics());
        return true;
    }

    // 5) 无歌词：发空列表重置 UI（避免残留上一曲歌词）
    emit allLyricsChanged(getAllLyrics());
    return false;
}

QString LyricsManager::extractEmbeddedLyrics(const QString &mediaPath)
{
    QFileInfo info(mediaPath);
    QString suffix = info.suffix().toLower();

    if (suffix == "mp3") {
        QFile f(mediaPath);
        if (!f.open(QIODevice::ReadOnly)) return QString();
        QByteArray tag = readId3v2Tag(f);
        if (tag.isEmpty()) return QString();
        return parseId3v2Lyrics(tag);
    }
    if (suffix == "flac") {
        return parseFlacLyrics(mediaPath);
    }
    if (suffix == "ogg" || suffix == "oga" || suffix == "opus") {
        return parseOggLyrics(mediaPath);
    }
    if (suffix == "m4a" || suffix == "mp4" || suffix == "m4p" || suffix == "m4b" || suffix == "alac") {
        return parseMp4Lyrics(mediaPath);
    }
    if (suffix == "wav") {
        return parseRiffId3Lyrics(mediaPath);
    }
    return QString();
}

QString LyricsManager::parseId3v2Lyrics(const QByteArray &tag)
{
    // tag 以 "ID3" + ver(2) + flags(1) + size(4 syncsafe) + frames...
    if (tag.size() < 10) return QString();
    int versionMajor = uchar(tag[3]); // 2 / 3 / 4
    uchar flags = uchar(tag[5]);
    quint32 headerSize = readSyncsafe(uchar(tag[6]), uchar(tag[7]), uchar(tag[8]), uchar(tag[9]));
    if (headerSize == 0) return QString();

    QByteArray frames = tag.mid(10, qMin<qint64>(headerSize, tag.size() - 10));
    if (frames.size() < 6) return QString();

    bool globalUnsynch = (versionMajor == 3) && (flags & 0x80); // v2.3 全局去同步标志

    int pos = 0;
    // 跳过扩展头（best-effort，含扩展头的文件较少）
    if (flags & 0x40) {
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
    QString usltResult; // USLT 提取的文本（优先）
    QString syltLrc;    // SYLT 合成的 LRC 串
    QString txxxResult; // TXXX 歌词类描述符的值（ffmpeg 等非标准存储的回退）

    while (pos + minHeader <= frames.size()) {
        QByteArray id;
        quint32 frameSize = 0;
        int headerLen = 10;
        bool frameUnsynch = false;

        if (versionMajor == 2) {
            // v2.2: 3B id + 3B BE size，无 flags
            id = frames.mid(pos, 3);
            if (!isFrameId(id)) break; // padding
            frameSize = readBE24(uchar(frames[pos + 3]), uchar(frames[pos + 4]), uchar(frames[pos + 5]));
            headerLen = 6;
        } else {
            // v2.3 / v2.4: 4B id + 4B size + 2B flags
            id = frames.mid(pos, 4);
            if (!isFrameId(id)) break; // padding
            if (versionMajor == 4) {
                frameSize = readSyncsafe(uchar(frames[pos + 4]), uchar(frames[pos + 5]),
                                         uchar(frames[pos + 6]), uchar(frames[pos + 7]));
                frameUnsynch = (uchar(frames[pos + 8]) & 0x08); // per-frame unsynch
            } else { // v2.3
                frameSize = readBE32(uchar(frames[pos + 4]), uchar(frames[pos + 5]),
                                     uchar(frames[pos + 6]), uchar(frames[pos + 7]));
            }
        }

        if (frameSize == 0) {
            pos += headerLen;
            continue;
        }

        int bodyStart = pos + headerLen;
        int bodyEnd = qMin(qint64(bodyStart + frameSize), qint64(frames.size()));
        if (bodyStart >= frames.size()) break;
        QByteArray body = frames.mid(bodyStart, bodyEnd - bodyStart);

        // 去同步（v2.3 全局 或 v2.4 帧级），在解码文本前应用
        if (globalUnsynch || frameUnsynch) {
            body = deUnsynchronise(body);
        }

        // USLT（v2.2 为 ULT）
        if (id == "USLT" || id == "ULT") {
            QString lyrics = parseUsltBody(body);
            if (!lyrics.isEmpty()) {
                usltResult = lyrics;
                break; // USLT 优先，找到即停
            }
        } else if (id == "SYLT" || id == "SLT") {
            // SYLT 回退：记录，若无 USLT 则用之
            QString lrc = parseSyltBody(body);
            if (!lrc.isEmpty() && syltLrc.isEmpty()) {
                syltLrc = lrc;
            }
        } else if (id == "TXXX") {
            // ffmpeg 等工具把歌词写入 TXXX（desc="USLT"/"lyrics" 等）的回退
            QString desc, value;
            parseTxxxBody(body, desc, value);
            if (!value.isEmpty() && isLyricsDescription(desc) && txxxResult.isEmpty()) {
                txxxResult = value;
            }
        }

        pos = bodyEnd;
    }

    if (!usltResult.isEmpty()) return usltResult;
    if (!syltLrc.isEmpty()) return syltLrc;
    return txxxResult;
}

QString LyricsManager::parseFlacLyrics(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    QByteArray magic = f.read(4);
    if (magic.size() < 4 || magic[0] != 'f' || magic[1] != 'L' ||
        magic[2] != 'a' || magic[3] != 'C') {
        return QString();
    }
    // 遍历元数据块
    while (!f.atEnd()) {
        QByteArray hdr = f.read(4);
        if (hdr.size() < 4) break;
        uchar blockType = uchar(hdr[0]) & 0x7f;
        bool lastBlock = (uchar(hdr[0]) & 0x80) != 0;
        quint32 blockSize = readBE24(uchar(hdr[1]), uchar(hdr[2]), uchar(hdr[3]));
        if (blockType == 4) { // VORBIS_COMMENT
            QByteArray data = f.read(blockSize);
            return parseVorbisComments(data);
        }
        // 跳过此块
        if (!f.seek(f.pos() + blockSize)) break;
        if (lastBlock) break;
    }
    return QString();
}

QString LyricsManager::parseOggLyrics(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    // best-effort：扫描前 64KB 内的 Vorbis/Opus 注释包标记
    const QByteArray data = f.read(64 * 1024);
    int opusIdx = data.indexOf("OpusTags");
    int vorbisIdx = data.indexOf("\x03vorbis", 0);
    if (opusIdx >= 0 && (vorbisIdx < 0 || opusIdx < vorbisIdx)) {
        // Opus: "OpusTags"(8) 之后直接是 Vorbis 注释结构
        return parseVorbisComments(data.mid(opusIdx + 8));
    }
    if (vorbisIdx >= 0) {
        // Vorbis: "\x03vorbis"(7) 之后是 vendor 长度(LE)...
        return parseVorbisComments(data.mid(vorbisIdx + 7));
    }
    return QString();
}

QString LyricsManager::parseMp4Lyrics(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return findMp4Lyrics(f, 0, f.size());
}

QString LyricsManager::parseRiffId3Lyrics(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    QByteArray hdr = f.read(12);
    if (hdr.size() < 12 || hdr.mid(0, 4) != "RIFF" || hdr.mid(8, 4) != "WAVE") {
        return QString();
    }
    // 遍历 WAVE 子 chunk（RIFF 为小端格式，chunk size 为 LE，2 字节对齐）
    while (!f.atEnd()) {
        QByteArray chunkHdr = f.read(8);
        if (chunkHdr.size() < 8) break;
        QByteArray chunkId = chunkHdr.mid(0, 4);
        quint32 chunkSize = readLE32(uchar(chunkHdr[4]), uchar(chunkHdr[5]),
                                    uchar(chunkHdr[6]), uchar(chunkHdr[7]));
        if (chunkId == "ID3 " || chunkId == "id3 ") {
            QByteArray tag = f.read(chunkSize);
            return parseId3v2Lyrics(tag);
        }
        // 跳过 chunk body（奇数长度需补 1 字节对齐）
        qint64 skip = chunkSize + (chunkSize & 1);
        if (!f.seek(f.pos() + skip)) break;
    }
    return QString();
}

QList<QPair<qint64, QString>> LyricsManager::getAllLyrics() const
{
    QList<QPair<qint64, QString>> lyricsList;
    for (auto it = m_lyricsMap.constBegin(); it != m_lyricsMap.constEnd(); ++it) {
        lyricsList.append(qMakePair(it.key(), it.value()));
    }
    return lyricsList;
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
