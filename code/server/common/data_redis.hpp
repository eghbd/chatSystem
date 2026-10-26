#include <sw/redis++/redis.h>
#include <iostream>
// ┌─────────────────────────────────────────────┐
// │  业务层（网关/消息服务/登录服务）              │
// │  ── 调用 Session / Status / Codes ──          │
// └──────────────────┬──────────────────────────┘
//                    │
// ┌──────────────────▼──────────────────────────┐
// │  存储抽象层（本文件的三个类）                  │
// │  Session  ·  Status  ·  Codes                 │
// └──────────────────┬──────────────────────────┘
//                    │ 共享同一个 shared_ptr<Redis>
// ┌──────────────────▼──────────────────────────┐
// │  RedisClientFactory → sw::redis::Redis        │
// │  （连接池，全局唯一，线程安全）                │
// └──────────────────┬──────────────────────────┘
//                    │
//                 Redis 服务器

//shared_ptr 让 Session、Status、Codes 共享同一个 Redis 客户端实例（同一个连接池），
// 避免每个类各自建连造成连接爆炸。
namespace bite_im {

/* ============================================================
 * 1. RedisClientFactory —— Redis 客户端工厂（基础设施层）
 *    职责：封装 ConnectionOptions，统一创建 Redis 客户端
 *    返回 shared_ptr，供 Session / Status / Codes 共享同一实例
 * ============================================================ */
class RedisClientFactory {
public:
    // 静态工厂方法：创建并返回 Redis 客户端智能指针
    static std::shared_ptr<sw::redis::Redis> create(
        const std::string &host,   // Redis 服务器地址，如 "127.0.0.1"
        int port,                  // 端口，默认 6379
        int db,                    // 逻辑数据库编号，0~15
        bool keep_alive) {         // 是否保持长连接（TCP keep-alive）

        // 1. 构造连接选项
        sw::redis::ConnectionOptions opts;
        opts.host       = host;
        opts.port       = port;
        opts.db         = db;
        opts.keep_alive = keep_alive;

        // 2. 用连接选项创建 Redis 客户端对象
        //    ⚠️ 注意：这里用的是 Redis(opts) —— 单连接模式，非线程安全
        //    多线程共享时应改用 Redis(opts, pool_opts) 连接池模式
        auto res = std::make_shared<sw::redis::Redis>(opts);

        // 3. 返回 shared_ptr，调用方共享所有权，最后一个引用销毁时自动析构
        return res;
    }
};


/* ============================================================
 * 2. Session —— 会话存储（ssid → uid 映射）
 *    职责：登录态的集中化存储，多实例网关共享
 *    Redis 结构：SET ssid uid / GET ssid / DEL ssid
 * ============================================================ */
class Session {
public:
    using ptr = std::shared_ptr<Session>;

    // 构造：注入 Redis 客户端（依赖注入，便于测试/共享连接）
    Session(const std::shared_ptr<sw::redis::Redis> &redis_client)
        : _redis_client(redis_client) {}

    // 新增会话：登录成功时调用，建立 ssid → uid 映射
    void append(const std::string &ssid, const std::string &uid) {
        // ⚠️ 未设置过期时间，会话永久有效
        //    建议加 EX（如 7 天），或用 expire 续期
        _redis_client->set(ssid, uid);
    }

    // 删除会话：登出时调用
    void remove(const std::string &ssid) {
        _redis_client->del(ssid);
    }

    // 查询会话：网关根据 ssid 查 uid，判断请求者身份
    sw::redis::OptionalString uid(const std::string &ssid) {
        // 返回 OptionalString，不存在时为空，调用方需判断
        return _redis_client->get(ssid);
    }

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;  // 共享的 Redis 客户端
};


/* ============================================================
 * 3. Status —— 在线状态存储（uid 是否存在即在线）
 *    职责：记录用户在线状态，发消息前判断对方是否在线
 *    Redis 结构：SET uid "" / GET uid / DEL uid（只用 key 存在性）
 * ============================================================ */
class Status {
public:
    using ptr = std::shared_ptr<Status>;

    Status(const std::shared_ptr<sw::redis::Redis> &redis_client)
        : _redis_client(redis_client) {}

    // 上线：用户建立长连接时调用，value 存空串占位
    void append(const std::string &uid) {
        //  设置 TTL，异常断线（未走 remove）会永久在线
        //  设短 TTL（如 60s）+ 心跳续期
        _redis_client->set(uid, "", std::chrono::seconds(60));
    }

    // 下线：用户断开连接时调用
    void remove(const std::string &uid) {
        _redis_client->del(uid);
    }

    // 判断是否在线：发消息前查对方状态
    bool exists(const std::string &uid) {
        // 用 GET 判断 key 是否存在：有值（哪怕空串）即在线
        //  更直接的写法：_redis_client->exists(uid)
        auto res = _redis_client->get(uid);
        if (res) return true;
        return false;
    }

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};


/* ============================================================
 * 4. Codes —— 验证码存储（cid → code，带自动过期）
 *    职责：存储验证码/临时凭证，5 分钟自动失效
 *    Redis 结构：SET cid code PX 300000 / GET cid / DEL cid
 * ============================================================ */
class Codes {
public:
    using ptr = std::shared_ptr<Codes>;

    Codes(const std::shared_ptr<sw::redis::Redis> &redis_client)
        : _redis_client(redis_client) {}

    // 写入验证码：默认 5 分钟（300000ms）后自动过期
    void append(const std::string &cid, const std::string &code,
                const std::chrono::milliseconds &t = std::chrono::milliseconds(300000)) {
        // set(key, value, ttl) 第三个参数即过期时间
        _redis_client->set(cid, code, t);
    }

    // 删除验证码：验证成功后调用（一次性使用，防重放）
    void remove(const std::string &cid) {
        _redis_client->del(cid);
    }

    // 读取验证码：用户提交验证码时取出比对
    sw::redis::OptionalString code(const std::string &cid) {
        return _redis_client->get(cid);
    }

private:
    std::shared_ptr<sw::redis::Redis> _redis_client;
};

} // namespace bite_im