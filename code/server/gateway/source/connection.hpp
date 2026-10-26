#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "logger.hpp"

namespace bite_im {

// 定义 WebSocket 服务器的类型（基于 asio 网络库，无 TLS 加密）
typedef websocketpp::server<websocketpp::config::asio> server_t;
// 连接的类型为： server_t::connection_ptr

class Connection {
public:
    // 存储单个长连接绑定的客户端信息
    struct Client {
        Client(const std::string &u, const std::string &s) : uid(u), ssid(s) {}
        std::string uid;   // 用户ID
        std::string ssid;  // 会话ID（或设备ID，用于区分多端登录）
    };
    using ptr = std::shared_ptr<Connection>;

    Connection() {}
    ~Connection() {}

    /* ============================================================
     * 1. 新增长连接 (用户上线/连接建立)
     * ============================================================ */
    void insert(const server_t::connection_ptr &conn, 
                const std::string &uid, 
                const std::string &ssid) {
        // 加锁：保护哈希表，防止多线程并发修改导致崩溃
        std::unique_lock<std::mutex> lock(_mutex);
        
        // 建立正向映射：用户ID -> 物理连接
        _uid_connections.insert(std::make_pair(uid, conn));
        
        // 建立反向映射：物理连接 -> 客户端详细信息
        _conn_clients.insert(std::make_pair(conn, Client(uid, ssid)));
        
        LOG_DEBUG("新增长连接用户信息：{}-{}-{}", (size_t)conn.get(), uid, ssid);
    }

    /* ============================================================
     * 2. 查找连接 (下发消息时，通过 uid 查找)
     * ============================================================ */
    server_t::connection_ptr connection(const std::string &uid) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _uid_connections.find(uid);
        if (it == _uid_connections.end()) {
            // 找不到说明该用户当前不在线
            LOG_ERROR("未找到 {} 客户端的长连接！", uid);
            return server_t::connection_ptr(); // 返回空指针
        }
        LOG_DEBUG("找到 {} 客户端的长连接！", uid);
        return it->second;
    }

    /* ============================================================
     * 3. 反查用户信息 (收到消息时，通过 conn 查是谁发的)
     * ============================================================ */
    bool client(const server_t::connection_ptr &conn, 
                std::string &uid, 
                std::string &ssid) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _conn_clients.find(conn);
        if (it == _conn_clients.end()) {
            LOG_ERROR("获取-未找到长连接 {} 对应的客户端信息！", (size_t)conn.get());
            return false;
        }
        // 通过出参返回查到的用户信息
        uid = it->second.uid;
        ssid = it->second.ssid;
        LOG_DEBUG("获取长连接客户端信息成功！");
        return true;
    }

    /* ============================================================
     * 4. 删除连接 (用户下线/断网，清理内存)
     * ============================================================ */
    void remove(const server_t::connection_ptr &conn) {
        std::unique_lock<std::mutex> lock(_mutex);
        
        // 先从反向表中找到这条连接
        auto it = _conn_clients.find(conn);
        if (it == _conn_clients.end()) {
            LOG_ERROR("删除-未找到长连接 {} 对应的客户端信息！", (size_t)conn.get());
            return;
        }
        
        // 正向表和反向表必须同步删除，否则会导致内存泄漏
        _uid_connections.erase(it->second.uid); // 删正向：uid -> conn
        _conn_clients.erase(it);                // 删反向：conn -> Client
        
        LOG_DEBUG("删除长连接信息完毕！");
    }

private:
    std::mutex _mutex; // 互斥锁：保护下面两个哈希表
    
    // 双向映射表：
    // 1. 正向表：用于快速通过用户ID找到对应的WebSocket连接（发消息用）
    std::unordered_map<std::string, server_t::connection_ptr> _uid_connections;
    
    // 2. 反向表：用于通过WebSocket连接反查出是哪个用户（收消息、清理用）
    std::unordered_map<server_t::connection_ptr, Client> _conn_clients;
};

} // namespace bite_im