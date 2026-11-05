# chatSystem

C++ 微服务架构的 IM（即时通讯）系统。服务端拆成 7 个独立微服务，通过 brpc 通信、etcd 做服务注册与发现；客户端是 Qt6 桌面程序。

## 目录结构

```
code/
├── server/              服务端
│   ├── CMakeLists.txt   顶层聚合构建，add_subdirectory 七个服务
│   ├── common/          公共封装（rpc 通道、etcd、mysql、redis、es、mq、日志等）
│   ├── proto/           所有 proto 接口定义
│   ├── odb/             ODB 数据实体定义（.hxx）
│   ├── sql/             建表脚本
│   ├── conf/            配置模板（*.conf.example）
│   ├── third/include/   第三方头文件（httplib.h、百度 AI SDK）
│   ├── gateway/         HTTP + WebSocket 网关
│   ├── user/            用户服务
│   ├── friend/          好友服务
│   ├── message/         消息服务
│   ├── file/            文件服务
│   ├── speech/          语音识别服务
│   └── transmite/       消息转发服务
└── client/              Qt6 客户端
```

每个服务目录下都是同一套结构：`CMakeLists.txt`、`source/`（服务本体）、`test/`（对应的测试客户端）、`dockerfile`。

## 服务与端口

| 服务 | 目录 | 默认端口 | 职责 |
|---|---|---|---|
| gateway | `gateway/` | 9000 (HTTP) / 9001 (WebSocket) | 统一入口，对外暴露 HTTP 与 WS |
| speech | `speech/` | 10001 | 语音转文字 |
| file | `file/` | 10002 | 文件上传下载 |
| user | `user/` | 10003 | 注册、登录、验证码、用户信息 |
| transmite | `transmite/` | 10004 | 消息转发 |
| message | `message/` | 10005 | 消息存储与历史检索 |
| friend | `friend/` | 10006 | 好友、会话、好友申请 |

## 技术栈

- RPC / 服务发现：brpc、etcd-cpp-api
- 存储：MySQL（ODB ORM 映射）、Redis、Elasticsearch
- 消息队列：RabbitMQ（amqpcpp）
- 序列化：protobuf
- HTTP / WebSocket：cpprest、httplib、websocketpp、cpr
- 其他：gflags、spdlog、gtest

## 构建服务端

需要先准备好 `cmake`、`g++`、`protoc`（protobuf 编译器）和 `odb` 编译器，以及下列库：

```
gflags spdlog fmt brpc protobuf leveldb etcd-cpp-api
odb odb-mysql odb-boost hiredis redis++ cpprest curl
jsoncpp elasticlient cpr amqpcpp ev openssl
```

然后：

```bash
cd code/server
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

产物为各服务目录下的 `build/<service>_server`，以及 `test/` 里对应的 `<service>_client`。`CMakeLists.txt` 会在构建前自动调用 `protoc` 和 `odb` 生成框架代码，因此这两个工具必须在 `PATH` 里。

## 配置

`conf/` 下每个服务一个配置文件，入库的只有脱敏模板 `*.conf.example`：

```bash
cd code/server/conf
cp user_server.conf.example user_server.conf
# 然后把 mysql_pswd、api_key、secret_key、dms_key_id 等换成自己的
```

真实配置文件被 `.gitignore` 排除，不要提交。配置里需要填的项：MySQL 地址与密码、etcd 注册中心地址、Elasticsearch / Redis / RabbitMQ 地址，以及百度语音识别和火山 DMS 的密钥。

## 数据库

`sql/` 下是六张表的建表脚本，按需导入：

```bash
mysql -uroot -p bite_im < code/server/sql/user.sql
```

## 部署

`Dockerfile.im` 是通用镜像，通过 `SERVICE` 构建参数选择要打进镜像的二进制；`docker-compose.yml` 编排七个服务加 etcd / MySQL / Redis / RabbitMQ / Elasticsearch。

注意两点：

1. 镜像里 `COPY depends/`，这个目录由 `collect_depends.sh` 在**编译机**上从二进制 `ldd` 结果收集而来，不入库。所以必须先编译、再收依赖、再 build 镜像，顺序不能反。
2. `docker-compose.yml` 里各服务挂载 `./conf-docker/<service>_server.conf`，这个目录需要自行准备（容器内要用服务名当主机名，不能直接拿 `conf/*.example` 里的 IP 用）。

`start_all.sh` 是不走 Docker、直接在宿主机批量拉起七个服务的脚本。

## 客户端

Qt6 项目，CMake 构建：

```bash
cd code/client
cmake -B build .
cmake --build build -j
```

## 说明

- 仓库只保存源码、proto、SQL 和配置模板；编译产物、运行时依赖库、真实配置均不入库。
- 早期学习用的 `example/` 示例代码未包含在仓库中。
