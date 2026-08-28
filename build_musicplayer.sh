#!/usr/bin/env bash
# 一键构建脚本：封装 qmake + make。
# 每次都会重新生成 Makefile；并自动检测 VERSION 变更后强制重编
# （qmake 注入的 -DAPP_VERSION 变化不会被 make 的依赖跟踪感知）。
set -euo pipefail

cd "$(dirname "$0")"

snapshot_version() {
    local raw
    raw=$(grep -m1 -o 'APP_VERSION=[^ ]*' Makefile 2>/dev/null || true)
    raw=${raw#*=}        # 去掉 "APP_VERSION=" 前缀，得到形如 \"1.0\" 的值
    raw=${raw//\\/}      # 去掉转义反斜杠
    raw=${raw//\"/}      # 去掉引号
    printf '%s' "$raw"
}

old_ver=$(snapshot_version)

qmake6 musicplayer.pro

new_ver=$(snapshot_version)
if [ -n "$old_ver" ] && [ "$old_ver" != "$new_ver" ]; then
    echo "检测到版本号变更: ${old_ver} -> ${new_ver}，清除目标文件强制重编..."
    rm -f -- ./*.o
fi

make -j"$(nproc)"

echo
echo "构建完成: $(pwd)/musicplayer (v${new_ver:-unknown})"
