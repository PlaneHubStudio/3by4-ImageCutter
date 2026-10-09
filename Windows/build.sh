#!/bin/zsh
set -eu
cd "${0:A:h}"
compiler="${ZIG:-zig}"
mkdir -p ../dist
export ZIG_GLOBAL_CACHE_DIR="${TMPDIR:-/tmp}/rednote-zig-cache"
export ZIG_LOCAL_CACHE_DIR="${TMPDIR:-/tmp}/rednote-zig-local"
"$compiler" cc main.c app.rc -target x86_64-windows-gnu -O2 -s -municode -Wl,--subsystem,windows -lgdi32 -luser32 -lgdiplus -lcomdlg32 -lcomctl32 -lshell32 -lole32 -luuid -o '../dist/3比4图片快切.exe'
