#!/bin/bash
# 从宿主机收集七个二进制的动态库依赖，放进 depends/
set -e
cd "$(dirname "$0")"
rm -rf depends && mkdir -p depends

for s in file speech user message transmite friend gateway; do
  b=$s/build/${s}_server
  [ -f "$b" ] || { echo "跳过 $s（无二进制）"; continue; }
  ldd "$b" 2>/dev/null | awk '/=> \//{print $3}'
done | sort -u \
  | grep -vE 'libc\.so|ld-linux|libm\.so|libpthread|libdl\.so|librt\.so|libnss|libresolv|libutil\.so' \
  > /tmp/im_libs.txt

n=0
while read -r lib; do
  [ -f "$lib" ] && cp -L "$lib" depends/ && n=$((n+1))
done < /tmp/im_libs.txt

echo "已收集 $n 个依赖库到 depends/（$(du -sh depends | cut -f1)）"
