# MusicPlayer

一个基于 Qt（C++）开发的桌面音乐播放器，面向 Linux 系统。

---

## 功能特性

| 功能 | 说明 |
|------|------|
| **播放控制** | 播放、暂停、停止、上一曲、下一曲；各模式手动上/下一曲均切歌，单曲循环自动播完重播当前 |
| **音量控制** | 滑动条调节，显示百分比（0–100%） |
| **进度条** | 可点击跳转，显示当前时间 / 总时长 |
| **播放模式** | 顺序播放、随机播放、单曲循环、列表循环（一键切换） |
| **播放列表** | 导入单文件/整个文件夹；导入/导出 `.txt` 播放列表；**右键删除选中项（Ctrl/Shift 多选）**；清空列表 |
| **支持格式** | MP3、WAV、FLAC、OGG、M4A |
| **歌词显示** | **内嵌歌词解析**（MP3 ID3v2 USLT/SYLT、FLAC/OGG Vorbis、M4A `©lyr`、WAV ID3）+ 外部 `.lrc`；支持 UTF-8/GBK/BOM；独立歌词窗口、高亮当前歌词、上下各 2 行；纯文本保留多行换行；按时间戳匹配避免重复行错位 |
| **菜单栏** | 文件（导入/导出）、插件（均衡器、歌词）、关于（检查更新、关于） |
| **均衡器** | 10 频段图形均衡器（31Hz–16kHz），内置预设（流行、摇滚、古典、爵士、电子） |
| **状态持久化** | 自动保存/恢复播放列表、当前曲目、播放模式、音量等设置 |

---

## 技术栈

- **语言**: C++17
- **框架**: Qt（Core、GUI、Widgets、Multimedia）
- **构建系统**: qmake6

---

## 项目结构

```
musicplayer/
├── main.cpp              # 程序入口
├── mainwindow.h/.cpp     # 主窗口 UI 与业务逻辑
├── mainwindow.ui         # Qt Designer 主窗口布局
├── player.h/.cpp         # 音频播放器封装（QMediaPlayer + QAudioOutput）
├── playlistmodel.h/.cpp  # 播放列表模型（QAbstractTableModel）
├── lyricsmanager.h/.cpp  # 歌词解析（外部 .lrc + 内嵌 ID3v2/Vorbis/MP4/WAV）
├── lyricswindow.h/.cpp   # 独立歌词显示窗口
├── equalizerwindow.h/.cpp # 均衡器对话框
├── musicplayer.pro       # qmake 项目文件
├── build_musicplayer.sh  # 一键构建脚本（qmake + make）
├── VERSION               # 版本号文件（编译时注入 APP_VERSION 宏）
├── run_musicplayer.sh    # 一键启动脚本
├── app.dir/              # Debian 打包源目录
│   ├── DEBIAN/control
│   └── usr/share/applications/musicplayer.desktop
```

---

## 架构设计

采用 **Model-View-Controller** 模式：

```
MainWindow（视图 / 控制器）
  ├── Player（媒体播放逻辑）
  │     ├── QMediaPlayer + QAudioOutput
  │     └── LyricsManager（外部 .lrc 解析 + 内嵌歌词解析）
  └── PlaylistModel（QAbstractTableModel → QTableView）

EqualizerWindow（QDialog — 设置对话框）
LyricsWindow（QDialog — 独立歌词窗口）
```

信号驱动的数据流：`Player` → `MainWindow`，监听播放位置、时长、播放状态、歌词更新、曲目切换等事件。

---

## 歌词解析

支持两种歌词来源，按优先级加载：

1. **内嵌歌词**：直接从音频文件标签读取，纯 C++ 解析（无第三方依赖）
   - MP3：ID3v2 USLT/SYLT 帧（处理 UTF-16、同步安全整数、去同步）
   - FLAC/OGG：Vorbis 注释 `LYRICS` / `UNSYNCEDLYRICS`
   - M4A：MP4 `©lyr` atom
   - WAV：RIFF `ID3 ` 子块
2. **外部 `.lrc`**：与音频同目录同名；支持 UTF-8（含 BOM）、GBK/GB18030 编码

显示特性：高亮当前歌词、上下各 2 行、纯文本保留多行换行、按时间戳匹配避免重复行错位。

详见 [.trae/documents/embedded-lyrics.md](.trae/documents/embedded-lyrics.md)。

---

## 构建与运行

### 编译

```bash
./build_musicplayer.sh
```

脚本每次都会重新执行 qmake 生成 Makefile；检测到 `VERSION` 变更时自动清除目标文件强制全量重编；随后并行编译（`make -j$(nproc)`），任一步失败即中止，完成时打印当前构建版本。也可手动执行：

```bash
qmake6 musicplayer.pro
make
```

> 手动方式注意：若只改了 `VERSION` 而未改任何源码，make 会因目标文件较新而跳过重编，需先 `make clean` 再编译（构建脚本已自动处理该情况）。

> 提示：脚本与手动命令均使用 PATH 中的 `qmake6`。若同时安装了本地 Qt 工具链（如 `~/Qt/<版本>`），先用 `which qmake6` 确认实际使用的工具链，需要固定路径时可把完整路径写进构建脚本。

### 清理构建产物

```bash
make clean        # 删除 .o 及 moc/uic/rcc 生成文件
make distclean    # 清理更彻底：连 Makefile 与 .qmake.stash 一并删除
```

清理后重新运行编译命令即可完全重建。

### 运行

```bash
./run_musicplayer.sh
# 或直接
./musicplayer
```

### 打包为 .deb

```bash
install -m755 musicplayer app.dir/usr/bin/        # 将最新二进制放入打包目录
dpkg-deb --build --root-owner-group app.dir musicplayer.deb
sudo dpkg -i musicplayer.deb
```

> 注：`Depends` 按本机实际包名书写（`libqt6core6t64` 为 t64 命名、其余为常规命名的混合状态）；发布到其他发行版前请核对其 Qt6 库包名。

### 版本号管理

[VERSION](VERSION) 文件是唯一版本来源：qmake 构建时注入为 `APP_VERSION` 宏，关于对话框显示该值。

升级版本的完整步骤：

1. 编辑 `VERSION`（如 `1.0.0` → `1.0.1`）；
2. 同步修改 [app.dir/DEBIAN/control](app.dir/DEBIAN/control) 的 `Version:` 字段——必须高于已安装版本，否则 dpkg 视为降级且拒绝完成配置；
3. 运行 `./build_musicplayer.sh`（自动检测版本变更并强制重编；手动方式需先 `make clean`）；
4. 按"打包为 .deb"重新拷贝二进制、打包并安装。

---

## 依赖

### 构建依赖

- C++17 编译器与 make（Debian/Ubuntu：`build-essential`）
- qmake6 与 Qt 6.10+ 开发库（`qt6-base-dev`, `qt6-multimedia-dev`）

```bash
sudo apt install build-essential qmake6 qt6-base-dev qt6-multimedia-dev
```

### 运行依赖

通过 `.deb` 安装时由 [control](app.dir/DEBIAN/control) 的 `Depends` 字段自动解析（libqt6core6t64 等运行库）；直接执行二进制则要求系统已安装对应 Qt6 运行库。

---

## 作者

**cyfsammy** — [cyfsammy@163.com](mailto:cyfsammy@163.com)
