#!/bin/bash
# 一键启动 bite_im 全部七个服务（按依赖顺序）
# 用法: ./start_all.sh          启动
#       ./start_all.sh stop     停止
S=~/chatsystem/code/server

if [ "$1" = "stop" ]; then
  pkill -f "_server --run_mode=false" 2>/dev/null
  echo "已停止全部服务"
  exit 0
fi

pkill -f "_server --run_mode=false" 2>/dev/null
sleep 1

start() {
  name=$1; dir=$2; shift 2
  ( cd "$S/$dir/build" && setsid ./${name}_server --run_mode=false "$@" \
      > /tmp/${name}_server.log 2>&1 < /dev/null & )
  sleep 2
  printf "  %-10s -> %s\n" "$name" "/tmp/${name}_server.log"
}

echo "启动中（依赖顺序：file -> speech -> user -> message -> transmite -> friend -> gateway）"
start file      file
start speech    speech
start user      user      --mysql_cset=utf8mb4
start message   message   --mysql_cset=utf8mb4 --mq_user=guest --mq_pswd=guest
start transmite transmite --mysql_cset=utf8mb4 --mq_user=guest --mq_pswd=guest
start friend    friend    --mysql_cset=utf8mb4
start gateway   gateway

echo ""
echo "=== etcd 注册情况 ==="
ETCDCTL_API=3 etcdctl get --prefix /service --keys-only 2>/dev/null | grep -v '^$'
