#include "mysql.hpp"
#include "message.hxx"
#include "message-odb.hxx"

namespace bite_im {

class MessageTable {
public:
    using ptr = std::shared_ptr<MessageTable>;

    // 构造函数：注入 ODB 数据库连接池对象
    MessageTable(const std::shared_ptr<odb::core::database> &db) : _db(db) {}
    ~MessageTable() {}

    /* ============================================================
     * 1. 新增消息 (INSERT)
     * ============================================================ */
    bool insert(Message &msg) {
        try {
            // 开启事务
            odb::transaction trans(_db->begin());
            
            // 将 msg 对象持久化到数据库（ODB 会自动生成 INSERT INTO message (...) VALUES (...)）
            // 注意：执行后，msg 对象内部的主键 _id 会被更新为数据库自增生成的 ID
            _db->persist(msg);
            
            // 提交事务
            trans.commit();
        } catch (std::exception &e) {
            // 发生异常（如主键冲突、连接断开），记录日志，返回 false
            LOG_ERROR("新增消息失败 {}:{}！", msg.message_id(), e.what());
            return false;
        }
        return true;
    }

    /* ============================================================
     * 2. 删除会话的所有消息 (DELETE)
     * ============================================================ */
    bool remove(const std::string &ssid) {
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<Message> query;
            
            // 相当于执行：DELETE FROM message WHERE session_id = 'ssid'
            // erase_query 支持条件批量删除，比先查出来再逐个 delete 效率高得多
            _db->erase_query<Message>(query::session_id == ssid);
            
            trans.commit();
        } catch (std::exception &e) {
            LOG_ERROR("删除会话所有消息失败 {}:{}！", ssid, e.what());
            return false;
        }
        return true;
    }

    /* ============================================================
     * 3. 获取最近 N 条消息 (SELECT ... ORDER BY ... LIMIT)
     * ============================================================ */
    std::vector<Message> recent(const std::string &ssid, int count) {
        std::vector<Message> res;
        try {
            odb::transaction trans(_db->begin());
            
            // 拼接 SQL 查询条件
            // 我们要的是：WHERE session_id = 'ssid' ORDER BY create_time DESC LIMIT count
            std::stringstream cond;
            cond << "session_id='" << ssid << "' ";
            // 按照时间倒序取最新的 count 条
            cond << "order by create_time desc limit " << count;
            
            // 执行原生条件查询
            odb::result<Message> r(_db->query<Message>(cond.str()));
            
            // 遍历结果集，放入 vector
            for (odb::result<Message>::iterator i(r.begin()); i != r.end(); ++i) {
                res.push_back(*i);
            }
            
            // 因为上面是 "时间倒序(新->旧)"，前端展示通常需要 "正序(旧->新)"
            // 所以这里将 vector 翻转一下
            std::reverse(res.begin(), res.end());
            
            trans.commit();
        } catch (std::exception &e) {
            LOG_ERROR("获取最近消息失败:{}-{}-{}！", ssid, count, e.what());
        }
        return res;
    }

    /* ============================================================
     * 4. 获取指定时间段内的消息 (SELECT ... WHERE time BETWEEN ...)
     * ============================================================ */
    std::vector<Message> range(const std::string &ssid, 
                               boost::posix_time::ptime &stime, 
                               boost::posix_time::ptime &etime) {
        std::vector<Message> res;
        try {
            odb::transaction trans(_db->begin());
            typedef odb::query<Message> query;
            
            // 使用 ODB 的面向对象查询语法（类型安全，防 SQL 注入）
            // 相当于：WHERE session_id = ? AND create_time >= ? AND create_time <= ?
            odb::result<Message> r(_db->query<Message>(
                query::session_id == ssid && 
                query::create_time >= stime &&
                query::create_time <= etime
            ));
            
            for (odb::result<Message>::iterator i(r.begin()); i != r.end(); ++i) {
                res.push_back(*i);
            }
            
            // 注意：这里没有 order by，也没有 reverse，依赖数据库默认返回顺序
            trans.commit();
        } catch (std::exception &e) {
            LOG_ERROR("获取区间消息失败:{}-{}:{}-{}！", ssid, 
                boost::posix_time::to_simple_string(stime), 
                boost::posix_time::to_simple_string(etime), e.what());
        }
        return res;
    }

private:
    // ODB 数据库连接池句柄，所有操作共享此连接池
    std::shared_ptr<odb::core::database> _db; 
};

} // namespace bite_im