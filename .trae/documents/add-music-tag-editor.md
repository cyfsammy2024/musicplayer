# 添加音乐标签编辑功能

## Context

当前 musicplayer 已能读取 5 种格式（MP3/FLAC/OGG/M4A/WAV）的内嵌歌词（[lyricsmanager.cpp](file:///home/sammy/Program/trae/musicplayer/lyricsmanager.cpp)），但没有任何编辑元数据的能力。用户无法在程序内修改标题、艺术家、专辑等标签，只能借助外部工具。

本任务新增「标签编辑器」：一个独立的 TagManager 负责读写 5 种格式的 7 个标准字段，一个 TagEditorWindow（QDialog）呈现表单，插件菜单和播放列表右键菜单各提供一个入口。

读写逻辑直接镜像现有 lyricsmanager 的二进制解析模式，复用其匿名命名空间辅助函数（提取到共享 `tagutils.h/.cpp`）。

## 用户已确认的范围

- **写入格式**：全部 5 种（MP3/ID3v2.4、FLAC、OGG/Vorbis、M4A/MP4、WAV/RIFF ID3）。
- **选曲入口**：① 插件菜单「标签编辑」加载当前播放/选中曲目；② 播放列表右键「编辑标签」作用于右键行。
- **字段**：title、artist、album、year、track、genre、lyrics（7 个标准字段）。

## 字段映射表

| 字段 | MP3 ID3v2.4 | Vorbis (FLAC/OGG) | MP4 atom |
|---|---|---|---|
| title | TIT2 | TITLE | ©nam |
| artist | TPE1 | ARTIST | ©ART |
| album | TALB | ALBUM | ©alb |
| year | TDRC | DATE (回退 YEAR) | ©day |
| track | TRCK (`n/N`) | TRACKNUMBER | trkn |
| genre | TCON | GENRE | ©gen |
| lyrics | USLT (回退 TXXX:LYRICS) | LYRICS (回退 UNSYNCEDLYRICS) | ©lyr |

## 架构

新增两个类，沿用项目现有 Model-View 分层：

```
MainWindow（控制器）
  └── TagManager（5 格式读写，纯二进制，无第三方依赖）
  └── TagEditorWindow（QDialog — 7 字段表单，保存按钮）
```

入口与信号流：插件菜单 actionEditTags → `on_actionEditTags_triggered()` → 加载当前播放曲目 URL → 弹出 TagEditorWindow。右键菜单「编辑标签」→ 以右键行 URL 调用同一窗口。保存成功后，若编辑的是当前播放曲目，刷新 LyricsWindow 以反映歌词变更。

## 实现步骤

### 1. 提取共享二进制辅助 `tagutils.h/.cpp`

把 [lyricsmanager.cpp](file:///home/sammy/Program/trae/musicplayer/lyricsmanager.cpp) 第 31-436 行匿名命名空间中双向语义的辅助函数迁移到 `tagutils.h/.cpp`，并在 lyricsmanager.cpp 改 `#include "tagutils.h"` + 删除重复定义：

迁移（共享）：
- `readBE32`/`readLE32`/`readBE24`/`readSyncsafe` → 同时加 `writeBE32`/`writeLE32`/`writeBE24`/`writeSyncsafe`
- `deUnsynchronise`、`decodeText`、`skipDescriptor`、`isFrameId`、`readId3v2Tag`、`parseVorbisComments`、`findMp4Lyrics`
- 新增写出辅助：`encodeText(enc, str)`（编码到 ID3v2 文本帧 body）、`packVorbisComment`、`oggCrc32`（OGG 多项式 `0x04C11DB7`，查表）

保留在 lyricsmanager.cpp（仅读侧需要）：`isValidUtf8`、`convertGbkToUtf8`、`convertViaWinCodePage`、`parseUsltBody`/`parseTxxxBody`/`parseSyltBody`/`parseId3v2Lyrics` 等高层语义解析（这些只读不写，留原处）。

### 2. TagManager 类 `tagmanager.h/.cpp`

```cpp
struct TagFields {
    QString title, artist, album, genre, lyrics;
    int year = 0, track = 0;
};
class TagManager {
public:
    bool read(const QString &path, TagFields &out);   // 复用 tagutils 读路径 + 扩展 TIT2/TPE1/...解析
    bool write(const QString &path, const TagFields &in); // 仅写非空字段，保留其它帧/注释/atom
};
```

按后缀分发到 5 个格式读写器（参照 lyricsmanager 的 `extractEmbeddedLyrics` 后缀分发）。

### 3. 各格式写入策略（字节级）

**MP3 / ID3v2.4**：读现有 ID3v2 块，逐帧保留非目标帧（APIC/COMM/…）按原字节拼接；目标 7 帧丢弃重写。统一写 v2.4（flags=0，无 unsynch）。文本帧 body=`[1B enc=3 UTF-8][UTF-8 bytes]`；USLT body=`[1B enc=3][3B lang="eng"][1B 0x00 descriptor 终结][UTF-8 lyrics]`。新 tag 块=`[ID3\x04\x00\x00\x00][4B syncsafe size][frames][padding 至 1024 对齐]`，后接原音频数据（旧 tag 之后）。写 tmp 文件 → rename 原子替换。

**FLAC**：解析 `fLaC` + 元数据块链。重建 VORBIS_COMMENT 块（type=4）：`[4B LE vendorLen][原 vendor][4B LE count][逐条 4B LE len + "KEY=UTF-8VAL"]`，保留非目标 Vorbis 字段，目标 7 字段覆盖或追加。写回 `fLaC` + 块链（STREAMINFO 原样、VORBIS_COMMENT 替换、其它块原样，最后块 last-bit=1）+ 后续音频帧。

**OGG/Vorbis**（最难，有风险标记）：解析 page1（identification）、page2（comment）。仅替换 page2 的 comment 包体。**风险缓解**：仅实现「新 comment 包体长度 ≤ 原 comment 页容量（含余量）」路径——原地重写 page2，重算 segment 表（每段 ≤255，末段 <255 表示 packet 结束）+ CRC32（OGG 多项式）。若新包超出原页容量，`write()` 返回 false，UI 提示「OGG 标签扩展暂不支持（超出原容量）」。后续页 sequence 号与 granule 不变，无需重写。Opus (.opus) 标 TODO。

**M4A/MP4**：递归扫描定位 `moov/udta/meta/ilst`（复用 `findMp4Lyrics` 模式）。重建 ilst：目标 7 个 sub-atom 重写为 iTunes 风格 `[4B size][4B type][data atom: 4B size=16+valuelen][4B "data"][4B type-indicator=1][4B locale=0][UTF-8 value]`；非目标 atom 原样拷贝。`trkn` 特殊：type-indicator=0，body=`[8B: 2B track, 2B total, 4B 0]`。注意 `meta` body 头 4 字节是 version+flags 前缀，保留。向上更新 ilst→meta(+4 prefix)→udta→moov 的 size。**风险缓解**：先检测 `moov` 是否在 `mdat` 之前——若是（streaming 布局，stco 需加 delta），`write()` 返回 false 提示「暂不支持 moov 在 mdat 之前的布局」；仅支持 iTunes 常见的 `mdat` 在前布局（stco 偏移不受 moov 大小影响）。写 tmp → 拷贝 moov 前部分 → 写新 moov → 拷贝 mdat+后续 → rename。

**WAV/RIFF ID3**：12B header `RIFF`+size+`WAVE`，逐 chunk `[4B id][4B LE size][body]`（奇数补 1B 对齐）。找到 `ID3 `/`id3 ` chunk 替换 body 为新 ID3v2 块；不存在则在末尾追加 `[ID3 ][LE size][tag]`（奇数补 0）。重写末尾 chunk 即可（WAV 通常 ID3 在末尾），更新 RIFF size = 文件总大小 - 8。

### 4. TagEditorWindow `tageditorwindow.h/.cpp`

QDialog（仿 [equalizerwindow.h](file:///home/sammy/Program/trae/musicplayer/equalizerwindow.h) 结构）：
- 7 个 QFormLayout 行：title/artist/album（QLineEdit），year/track（QSpinBox），genre（QLineEdit 或 QComboBox 带补全），lyrics（QPlainTextEdit，多行）
- 顶部 QLabel 显示当前文件名
- 底部按钮：保存（QDialogButtonBox::Save）、取消
- 构造接收 `QString filePath`，构造时 `TagManager::read()` 填充表单
- 保存按钮 → 收集字段 → `TagManager::write()` → 成功则 `QMessageBox::information` + `accept()`，失败则 `QMessageBox::warning` 显示原因
- 失败原因（OGG 超容量、MP4 moov 前置）直接显示在 warning

### 5. 菜单与右键接入

修改 [mainwindow.ui](file:///home/sammy/Program/trae/musicplayer/mainwindow.ui)：
- `menuPlugins`（第 186-192 行）追加 `<addaction name="actionEditTags"/>`
- 新增 `<action name="actionEditTags"><property name="text"><string>标签编辑</string></property></action>`

修改 [mainwindow.h](file:///home/sammy/Program/trae/musicplayer/mainwindow.h)：
- 私有成员加 `TagEditorWindow *m_tagEditorWindow = nullptr;`
- 私有 slots 加 `void on_actionEditTags_triggered();`
- include `"tageditorwindow.h"`

修改 [mainwindow.cpp](file:///home/sammy/Program/trae/musicplayer/mainwindow.cpp)：
- 新增 `on_actionEditTags_triggered()`：取当前播放曲目 URL（`m_player->currentIndex()` + `m_playlistModel->mediaList()`）；空列表时 `QMessageBox::warning` 提示；否则 `new TagEditorWindow(url, this)` → `exec()` → 若 Accepted 且 url == 当前曲目，触发歌词重载（`m_player->lyricsManager()->loadLyricsFromMedia(url)` 或重设 source 触发现有 onCurrentMediaChanged 路径）
- `onPlaylistContextMenu`（第 447 行）右键菜单追加 `QAction *editAct = menu.addAction("编辑标签")`；选中后取该行 URL 弹同一窗口

### 6. 构建配置

[musicplayer.pro](file:///home/sammy/Program/trae/musicplayer/musicplayer.pro) 的 SOURCES 加 `tagutils.cpp tagmanager.cpp tageditorwindow.cpp`，HEADERS 加 `tagutils.h tagmanager.h tageditorwindow.h`。

## 风险与已知限制

- **OGG**：超出原 comment 页容量的扩展写入不支持（返回失败 + UI 提示）。Opus 格式标记 TODO。
- **MP4**：`moov` 在 `mdat` 之前的 streaming 布局不支持（需 stco delta 调整），仅支持 iTunes 常见布局。
- 写入前不主动备份原文件；写 tmp → rename 保证原子性，失败时原文件不受影响。

## 验证

构建：`cd /home/sammy/Program/trae/musicplayer && ./build_musicplayer.sh`（qmake6 重生成 Makefile + make -j）。

验证工具：`ffprobe`（已在 `/usr/bin/ffprobe`，能读全部 5 种格式的标签）。

测试矩阵（每种格式备 1 个真实文件，先 cp 备份）：
1. **MP3**：填 7 字段保存 → `ffprobe -show_entries format_tags=title,artist,album,date,track,genre -of default=nw=1 file.mp3` 显示新值；`strings file.mp3 | grep -i USLT` 确认 USLT 帧存在；播放器加载确认歌词显示。
2. **FLAC**：写入 → `ffprobe` 显示 Vorbis 注释 → 播放器加载歌词正常。
3. **OGG**：写入 → `ffprobe -v error` 无 CRC/segmentation 报错 → 播放器解码前几秒无杂音。
4. **M4A**：写入 → `ffprobe` 显示 ©atom 值 → `ffprobe -v error` 解码测试通过。
5. **WAV**：写入 → `ffprobe` → `xxd file.wav | head -3` 确认 RIFF size 更新。

回归：编辑当前播放曲目后，LyricsWindow 应刷新显示新歌词（验证 on_actionEditTags_triggered 的重载路径）。

## 关键文件

新建：
- [tagutils.h](file:///home/sammy/Program/trae/musicplayer/tagutils.h) / [tagutils.cpp](file:///home/sammy/Program/trae/musicplayer/tagutils.cpp)
- [tagmanager.h](file:///home/sammy/Program/trae/musicplayer/tagmanager.h) / [tagmanager.cpp](file:///home/sammy/Program/trae/musicplayer/tagmanager.cpp)
- [tageditorwindow.h](file:///home/sammy/Program/trae/musicplayer/tageditorwindow.h) / [tageditorwindow.cpp](file:///home/sammy/Program/trae/musicplayer/tageditorwindow.cpp)

修改：
- [lyricsmanager.cpp](file:///home/sammy/Program/trae/musicplayer/lyricsmanager.cpp)（include tagutils.h，删重复辅助）
- [mainwindow.h](file:///home/sammy/Program/trae/musicplayer/mainwindow.h) / [mainwindow.cpp](file:///home/sammy/Program/trae/musicplayer/mainwindow.cpp)
- [mainwindow.ui](file:///home/sammy/Program/trae/musicplayer/mainwindow.ui)
- [musicplayer.pro](file:///home/sammy/Program/trae/musicplayer/musicplayer.pro)
