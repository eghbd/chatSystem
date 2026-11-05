# Docker 部署说明

## 一、整体结构

```
                    ┌──────────── im-net（自定义 bridge 网络）────────────┐
                    │                                                    │
  客户端 ──10007──▶ │  gateway ──┬──▶ file      (10002)                   │
  客户端 ──10008──▶ │            ├──▶ speech    (10001)                   │
                    │            ├──▶ user      (10003) ──┐               │
                    │            ├──▶ message   (10005) ──┼──▶ mysql      │
                    │            ├──▶ transmite (10004) ──┤    redis      │
                    │            └──▶ friend    (10006)   └──▶ rabbitmq   │
                    │                                          elasticsearch│
                    │  服务注册与发现：etcd (2379)                            │
                    └────────────────────────────────────────────────────┘
```

- **中间件 5 个**：etcd、MySQL、Redis、RabbitMQ、Elasticsearch
- **业务服务 7 个**：file、speech、user、message、transmite、friend、gateway
- 只有 gateway 对外暴露端口（10007 WebSocket / 10008 HTTP），其余都在内网

## 二、文件清单

| 文件 | 作用 |
|---|---|
| `docker-compose.yml` | 全部 12 个容器的编排 |
| `Dockerfile.im` | 通用服务镜像（通过 `SERVICE` 参数选择二进制） |
| `collect_depends.sh` | 从宿主机收集动态库到 `depends/` |
| `depends/` | 运行期依赖库（由脚本生成，不提交 git） |

> 原有的 `user/dockerfile`、`friend/dockerfile` 等**不能用**：基础镜像是 `ubuntu:22.04`，
> 而二进制是在 Ubuntu 26.04（glibc 2.43）上编译的，版本不兼容会直接 `GLIBC_X.XX not found`。
> 另外它们依赖 `depends/` 和 `conf/` 目录，这两个目录项目里根本不存在。

## 三、启动步骤

```bash
cd ~/chatsystem/code/server

# 1. 收集依赖库（只需执行一次，重新编译后要再跑）
./collect_depends.sh

# 2. 构建并启动
docker compose up -d --build

# 3. 查看状态
docker compose ps
docker compose logs -f gateway
```

停止 / 清理：

```bash
docker compose down          # 停止并删除容器，保留数据卷
docker compose down -v       # 连数据卷一起删（数据库会被清空）
```

## 四、关键设计点（为什么这么写）

### 1. `access_host` 必须写容器名，不能写 127.0.0.1

这是**最关键**的一处。宿主机上直接跑时，每个服务向 etcd 注册的是：

```
/service/user_service/instance  →  127.0.0.1:10003
```

在容器里，`127.0.0.1` 指向**容器自己**。如果照抄这个值，gateway 拿到注册信息后
会去连自己容器的 10003，必然失败。所以 compose 里全部改成：

```
--access_host=user:10003      # 用 compose 的服务名，Docker 内建 DNS 会解析
```

etcd 里存的就是 `user:10003`，任何容器都能解析到这个地址。

### 2. RabbitMQ 不能用 guest

RabbitMQ 的 `guest` 账号默认**只允许 localhost 登录**。容器之间互相访问时，
guest 会被拒绝，报错形如：

```
NOT_ALLOWED - access to vhost '/' refused for user 'guest'
```

所以 compose 里用环境变量建了专用账号：

```yaml
RABBITMQ_DEFAULT_USER: im
RABBITMQ_DEFAULT_PASS: "123456"
```

message 和 transmite 的 `--mq_user` / `--mq_pswd` 必须与之对应。

### 3. 依赖顺序用 healthcheck，不是只看启动

`depends_on` 只保证**容器进程启动**，不保证服务可用。MySQL 启动后要几十秒才能接受连接。
所以关键中间件都配了 `healthcheck`，业务服务用 `condition: service_healthy` 等它真正就绪。

### 4. Elasticsearch 的内存限制

ES 默认堆内存较大，之前直接在宿主机跑时被 OOM killer 杀过。compose 里限死：

```yaml
ES_JAVA_OPTS=-Xms512m -Xmx512m
```

宿主机仍需满足（一般 Ubuntu 默认满足）：

```bash
sysctl -w vm.max_map_count=262144
```

### 5. MySQL 初始化

```yaml
- ./sql:/docker-entrypoint-initdb.d:ro
```

`sql/` 下的 6 个 `.sql` 会在 MySQL **首次创建数据卷时**自动执行。注意：

- 每个文件都带 `CREATE DATABASE IF NOT EXISTS bite_im`，不需要额外初始化脚本
- 只在数据卷为空时执行。如果改了表结构，要 `docker compose down -v` 重来
- 每个文件带 `DROP TABLE IF EXISTS`，重复执行会清空数据

### 6. 服务间仍然走 etcd 做服务发现

容器化没有改变架构：服务之间仍然通过 etcd 注册/发现，RPC 走 brpc，
异步消息走 RabbitMQ。变化只是「地址从 127.0.0.1 变成了容器名」。

## 五、与直接跑二进制的差异

| 项目 | 直接跑 | Docker |
|---|---|---|
| 服务地址 | 127.0.0.1:1000x | 容器名:1000x |
| MySQL | 宿主机 | 容器 mysql |
| 启动方式 | `./start_all.sh` | `docker compose up -d` |
| 依赖库 | 系统已安装 | `depends/` 拷入镜像 |
| 数据 | 宿主机磁盘 | 命名卷 |

## 六、常见问题

**Q: 构建时报 `COPY failed: file not found`**
`depends/` 或某个 `build/xxx_server` 不存在。先编译服务、再跑 `collect_depends.sh`。

**Q: 容器起来但 etcd 里看不到服务**
看该服务日志：`docker compose logs user`。常见原因是连不上 MySQL/ES，
或者 `access_host` 写错。

**Q: gateway 能连上但发消息失败**
检查 message 服务是否正常、RabbitMQ 账号对不对：
`docker compose logs -f message`

**Q: 端口冲突**
10007/10008 被宿主机上直接跑的服务占用。先 `pkill -f _server` 或 `./start_all.sh stop`。

## 七、国内网络加速

拉镜像慢可以改 `/etc/docker/daemon.json`：

```json
{
  "registry-mirrors": ["https://docker.m.daocloud.io"]
}
```

改完 `sudo systemctl restart docker`。
