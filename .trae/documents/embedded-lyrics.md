# 从音乐文件读取并显示内嵌歌词

## Context（背景与目标）

当前 `LyricsManager::loadLyricsFromMedia()`（`lyricsmanager.cpp:67-73`）只会在音乐文件同目录下查找同名 `.lrc` 外挂歌词文件，**完全不读取音乐文件内部嵌入的歌词**。实际中 MP3/FLAC/OGG/M4A 等格式都可在文件元数据中携带歌词（ID3v2 的 USLT/SYLT、Vorbis 注释的 LYRICS、MP4 的 `©lyr`）。本任务在不引入任何新依赖的前提下（项目哲学：纯 Qt、`musicplayer.pro` 中 FFmpeg 已注释禁用），扩展 `LyricsManager` 使其能直接从音乐文件二进制中解析出内嵌歌词并交给既有显示逻辑渲染。

用户已确认采用 **纯 C++ 二进制解析**（不引 taglib、不链接/调用 ffmpeg）。

## 改动范围（极小）

只改两个文件，**不动 `.pro / UI / Player / MainWindow / 打包**：

- `lyricsmanager.h`
- `lyricsmanager.cpp`

原因：信号链已完整存在——`Player::setCurrentIndex`（`player.cpp:161`）切歌时已调用 `loadLyricsFromMedia`；`Player::onPositionChanged` 调 `getLyricsAtTime` 并 `emit lyricsChanged`；`MainWindow::onAllLyricsChanged/onLyricsChanged` 已能渲染与高亮。只需让 `loadLyricsFromMedia` 多一条"读取文件内部"的路径即可。

## 设计

### 1. 重构 LRC 解析为可复用入口

把现有 `loadLyrics(filePath)` 中的 regex 循环原样抽到新私有方法 `loadLyricsFromString(QString)`：

- 输入从 `file.readLine()` 改为 `content.split('\n', Qt::SkipEmptyParts)` 逐行 `.trimmed()`。
- **逻辑一字不改**：保留两遍 `globalMatch`（先取所有时间戳末位确定 lyricsText，再回填每个时间戳），保留"一行多时间戳共享同句"语义。这是零行为漂移的关键。
- `loadLyrics(filePath)` 改为：`QFile` 以非 Text 模式打开 → `readAll` → `QString::fromUtf8` → 剥离起始 UTF-8 BOM（`\uFEFF`）→ `return loadLyricsFromString(content)`。打开失败返回 false 且**不 emit**（保持既有行为，由 dispatcher 负责补 emit）。
- 这样"带 LRC 时间戳的内嵌歌词"可直接复用同一解析器。

### 2. 扩展 `loadLyricsFromMedia(mediaUrl)` 为 dispatcher

```
1) 显式 .lrc：mediaPath.left(lastIndexOf('.')) + ".lrc" → loadLyrics(path)。
   成功 → return true（外挂 .lrc 质量最高，优先级最高）。
2) 无 .lrc：调用新私有 extractEmbeddedLyrics(mediaPath) 按格式分派。
3) 返回串含 [mm:ss.xx]（用同一 regex 探测）→ loadLyricsFromString(content)。
4) 返回串非空但无时间戳（纯文本）→ m_lyricsMap.clear(); m_lyricsMap[0]=content;
   emit allLyricsChanged(getAllLyrics())。
5) 返回空 → 必须发 emit allLyricsChanged({}) 以触发 UI 占位符重置
   （否则会残留上一曲歌词）。
6) return !m_lyricsMap.isEmpty()。
```

> 纯文本 QMap 只能存一条（无每行时间戳），单条目 @0 使整段可被 `getLyricsAtTime` 返回并被高亮。多行换行已通过 `<p style="white-space:pre-wrap">` 保留（详见文末"已知限制"修复说明）。

### 3. 格式解析器（私有，返回原始歌词 QString 或空）

`extractEmbeddedLyrics` 按扩展名 + magic 分派到下列解析器。**读取用 `QFile`+`seek`+有界 `read(n)`，禁止对大文件 `readAll()`**。

| 格式 | 解析器 | 要点 |
|------|--------|------|
| MP3 / WAV 的 `ID3 ` chunk | `parseId3v2Lyrics(QByteArray)` | 见下 ID3v2 要点 |
| FLAC | `parseFlacLyrics(path)` | `fLaC`+元数据块，type 4 取 Vorbis 注释 |
| OGG (Vorbis/Opus) | `parseOggLyrics(path)` | best-effort：扫描 `\x03vorbis`/`OpusTags` 标记后按 Vorbis 注释结构解析 |
| M4A/MP4 | `parseMp4Lyrics(path)` | 遍历 atom 树取 `©lyr` |
| WAV | `parseRiffId3Lyrics(path)` | RIFF 子 chunk 找 `ID3 `（注意带尾空格）→ body 喂 `parseId3v2Lyrics` |

### ID3v2 解析要点（最易错）

- **header size**：偏移 6-9 共 4 字节，v2.3 与 v2.4 **均为 syncsafe**（`b0<<21 \| b1<<14 \| b2<<7 \| b3`）。
- **frame size 区别（关键）**：v2.3 = 普通 32 位 BE；v2.4 = **syncsafe**。最易写反。
- **v2.2（可选 best-effort）**：帧头 6B = 3B ID + 3B 普通 BE size，无 flags；歌词帧 ID 为 `ULT`（非 `USLT`）。
- **unsynchronisation**：v2.3 = header 全局 flag（偏移 5 的 bit7=0x80），作用于所有 frame；v2.4 = per-frame format-flags 字节（bit3=0x08）。去同步算法：把 `0xFF 0x00` 还原为 `0xFF`，**在解码文本前应用**。
- **USLT 结构**：`[1B enc][3B lang][descriptor 按 enc 终结][lyrics 余下]`。终结符：enc 0(ISO-8859-1)/3(UTF-8) 用 1B `0x00`；enc 1(UTF-16 w/BOM)/2(UTF-16BE) 用 2B `0x00 0x00`。终结符字节数搞错会错切 descriptor/lyrics。
- **编码解码（不用 QTextCodec，用 Qt6 Core 的 `QStringDecoder`）**：enc 0 `QString::fromLatin1`；enc 3 `fromUtf8`；enc 1 先判 BOM（`FF FE`=LE/`FE FF`=BE）用 `QStringDecoder(Utf16LE/Utf16BE)`；enc 2 `QStringDecoder(Utf16BE)`。
- **padding/结束**：4B 帧首不是 `[A-Za-z0-9]{4}`（出现 0x00 等）即视为 padding，停止遍历，防越界。
- **SYLT 回退**：结构 `[1B enc][3B lang][1B 时间格式][1B 内容类型][descriptor 终结][循环: 文本+终结(按enc)+4B 普通 BE ms 时间戳]`，时间格式 2=毫秒。**4B 时间戳是普通 BE 非 syncsafe**。实现策略：把每片 `[ms, text]` 合成为 `[mm:ss.xx]text\n` 的 LRC 文本串返回，交 `loadLyricsFromString` 复用——保持 dispatcher 纯 QString 出入。

### FLAC 解析要点

- `fLaC` 后元数据块：1B 头（bit7=last-block、bit0-6=type）+ **3B 普通 BE** 长度。
- **last-block 置位即停止遍历**（即便没遇 type 4）。
- type 4 = VORBIS_COMMENT：vendor 长度 **4B LE** + vendor + count **4B LE** + count×`[4B LE 长度 + "KEY=value"(UTF-8)]`。**FLAC 流用 BE 但 Vorbis 注释字段长度全 LE，最易写反**。
- key 大小写不敏感匹配 `LYRICS / UNSYNCEDLYRICS / SYNCEDLYRICS / LYRIC`，取首个非空。

### OGG 解析要点（best-effort）

- 不做完整 OGG 页重组。在文件前若干 MB 扫描 `\x03vorbis`（Vorbis）或 `OpusTags`（Opus），从标记后按与 FLAC 相同的 Vorbis 注释结构解析。误命中概率极低，可接受。

### MP4/M4A 解析要点

- atom 头：4B 普通 BE size + 4B type。**size==1** → 后跟 8B 普通 BE 扩展 size（头共 16B）；**size==0** → atom 延伸到文件末尾。两者都须处理，否则死循环/截断。
- 容器 atom 递归：`moov/trak/mdia/minf/stbl/udta/dinf/edts`。仅取 `moov→udta→©lyr` 与 `moov→trak→mdia→udta→©lyr` 两条路径。
- `©lyr`（字节 `A9 6C 79 72`，源码写作 `"\xA9lyr"`）body 判定：前 4B == `"data"` → iTunes 风格，data 子 atom = `[4B size][4B "data"][4B type-indicator][4B locale][value...]`，跳过 data 头后 8B 取 value；否则老 QuickTime 风格，body 整体即 UTF-8 文本。

## 方法签名（lyricsmanager.h）

公共 API 不变；新增私有声明。辅助小工具（syncsafe/BE32/LE32 读取、`QStringDecoder` 封装）作为 `.cpp` 内匿名命名空间 `file-static` 函数，不进头文件。

```cpp
private:
    bool loadLyricsFromString(const QString &content);           // 复用 LRC 解析
    QString extractEmbeddedLyrics(const QString &mediaPath);    // 分派
    QString parseId3v2Lyrics(const QByteArray &tagBytes);        // MP3 + WAV ID3 chunk
    QString parseFlacLyrics(const QString &path);
    QString parseOggLyrics(const QString &path);
    QString parseMp4Lyrics(const QString &path);
    QString parseRiffId3Lyrics(const QString &path);             // WAV
    // QMap<qint64,QString> m_lyricsMap; 不变
```

## 编辑区域（lyricsmanager.cpp）

- 顶部 include 增补 `<QByteArray>`、`<QStringConverter>`、`<QStringDecoder>`、`<QIODevice>`（按需），保留 `<QFile>`/`<QRegularExpression>`/`<QUrl>`。
- 替换 `loadLyrics`（10-56 行）：改为 readAll→fromUtf8→剥 BOM→`loadLyricsFromString`。
- 新增 `loadLyricsFromString`：原 regex 循环原样搬入（两遍 globalMatch 保持）。
- 替换 `loadLyricsFromMedia`（67-73 行）：实现上述 dispatcher，重点确保 Step 5 发空列表信号。
- 文件末尾追加：`extractEmbeddedLyrics` + 五个 `parse*` + 匿名命名空间辅助函数。

代码风格沿用现有：中文注释、`QObject`、`Q_OBJECT` 已在头文件、命名小驼峰方法 + `m_` 成员。

## 关键文件

- [lyricsmanager.cpp](file:///home/sammy/Program/trae/musicplayer/lyricsmanager.cpp)
- [lyricsmanager.h](file:///home/sammy/Program/trae/musicplayer/lyricsmanager.h)
- 调用点参考（不改）：[player.cpp:161](file:///home/sammy/Program/trae/musicplayer/player.cpp#L161)、[mainwindow.cpp:32-33](file:///home/sammy/Program/trae/musicplayer/mainwindow.cpp#L32)

## 验证

### 构建
```
cd /home/sammy/Program/trae/musicplayer
qmake6 musicplayer.pro && make -j$(nproc)
./musicplayer
```
头加私有方法后 `make` 会自动重跑 moc；若旧 Makefile 未识别，先 `make clean` 再 `qmake6 && make`。

### 测试夹具（外部目录，不污染仓库）
仓库内无现成音频/歌词样本。用 ffmpeg 注入（`-c:a copy` 不重编码）：
- MP3 USLT：`ffmpeg -i base.mp3 -c:a copy -metadata 'lyrics=[00:01.00]第一行\n[00:03.50]第二行\n[00:06.00]第三行' out.mp3`
- FLAC：`-metadata 'LYRICS=[00:01.00]...' out.flac`
- M4A：`-metadata "lyr=[00:01.00]..." out.m4a`（©lyr 由 ffmpeg 处理）
- 纯文本：去掉 `[mm:ss.xx]` 前缀同样注入，验证单条目 @0 路径。

### 检视 ground truth
`ffprobe -v quiet -print_format json -show_format <file>` 看 `format.tags.lyrics / unsynced_lyrics / LYRICS / lyr`，与界面显示对照。

### 手工验证步骤
1. 导入上述 out.mp3/flac/m4a 依次播放，歌词面板显示内嵌歌词；带时间戳者在对应秒数高亮当前行（2em 红字、前后各 2 行）。
2. 纯文本夹具：整段以单红字块显示（接受折叠限制）。
3. 删除同名 `.lrc`，确认 embedded 仍生效；再放同名 `.lrc`，确认外挂优先覆盖。
4. 播放无歌词文件：面板回到占位符"歌词将显示在这里"（验证 Step 5 空列表信号）。
5. 切歌来回切换，无崩溃、无残留上一曲歌词、无 ASan 报错。

## 已知限制（已修复）

以下三项原本列为"不在本次范围"的限制，现已修复：

- **纯文本歌词多行折叠**：纯文本（无时间戳）歌词多行会被 HTML `<p>` 空白折叠为一段红字 → 已在 `MainWindow::onLyricsChanged` 的 `<p>` style 加 `white-space: pre-wrap`，多行换行得以保留。
- **重复行高亮卡首处**：`onLyricsChanged` 按精确文本匹配索引，LRC 重复行时高亮卡首处；SYLT 合成 LRC 继承此行为 → 已改为按当前播放时间（`m_currentPosition`，在 `onPositionChanged` 中更新）在时间升序的 `m_lyricsList` 中定位索引，重复行命中当前播放段落对应的正确索引。改动文件：[mainwindow.h](file:///home/sammy/Program/trae/musicplayer/mainwindow.h)（新增 `m_currentPosition` 成员）、[mainwindow.cpp](file:///home/sammy/Program/trae/musicplayer/mainwindow.cpp)（`onPositionChanged` 记录位置、`onLyricsChanged` 按时间定位）。
- **GBK 编码 `.lrc` 不处理**：GBK 编码 `.lrc` 既不处理 → 已在 `LyricsManager::loadLyrics` 中先字节级严格校验 UTF-8，含非法序列则用 iconv 从 GBK/GB18030 转码为 UTF-8 再解析。**坑**：不能用 Qt6 的 `QStringDecoder(Utf8)` 判编码——其默认 lenient，会把非法字节替换为 U+FFFD 而不报 `hasError()`，导致 GBK 被误判为合法 UTF-8 走 lenient `fromUtf8` 产生乱码；`isValidUtf8` 已改为字节逐位校验（含过长/代理区/越界检查）。glibc 内置 iconv（符号在 libc），无需改 `musicplayer.pro` 链接。改动文件：[lyricsmanager.cpp](file:///home/sammy/Program/trae/musicplayer/lyricsmanager.cpp)（匿名命名空间新增 `isValidUtf8`/`convertGbkToUtf8`、`loadLyrics` 增加 GBK 回退分支）。
