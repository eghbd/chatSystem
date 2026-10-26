#pragma once
#include <string>
#include <cstddef> 
#include <odb/nullable.hxx>
#include <odb/core.hxx>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace bite_im {

// 【ODB 宏指令】：告诉 ODB 编译器，把这个类映射成数据库表，表名为 "message"
#pragma db object table("message")
class Message {
public:
    // 默认构造函数（ODB 反序列化查库结果时必须要有）
    Message(){}

    // 全参构造函数：用于业务层快速组装一条消息
    Message(const std::string &mid,                    // 消息ID
            const std::string &ssid,                   // 会话ID
            const std::string &uid,                    // 发送者ID
            const unsigned char mtype,                 // 消息类型
            const boost::posix_time::ptime &ctime):    // 创建时间
            _message_id(mid), _session_id(ssid),
            _user_id(uid), _message_type(mtype),
            _create_time(ctime){}
        
    // ================== Getter / Setter ==================
    // 1. 基础字段（必填）
    std::string message_id() const { return _message_id; }
    void message_id(const std::string &val) { _message_id = val; }

    std::string session_id() const { return _session_id; }
    void session_id(const std::string &val) { _session_id = val; }
    
    std::string user_id() const { return _user_id; }
    void user_id(const std::string &val) { _user_id = val; }
    
    unsigned char message_type() const { return _message_type; }
    void message_type(unsigned char val) { _message_type = val; }
    
    boost::posix_time::ptime create_time() const { return _create_time; }
    void create_time(const boost::posix_time::ptime &val) { _create_time = val; }

    // 2. 可选字段（针对不同类型的消息）
    // 文本消息内容
    std::string content() const { 
        if (!_content) return std::string(); // 判空：如果是 nullptr/nullopt，返回空串防崩溃
        return *_content; 
    }
    void content(const std::string &val) { _content = val; }

    // 文件ID（图片、文件、语音消息用）
    std::string file_id() const { 
        if (!_file_id) return std::string();
        return *_file_id; 
    }
    void file_id(const std::string &val) { _file_id = val; }
    
    // 文件名
    std::string file_name() const { 
        if (!_file_name) return std::string();
        return *_file_name; 
    }
    void file_name(const std::string &val) { _file_name = val; }
    
    // 文件大小
    unsigned int file_size() const { 
        if (!_file_size) return 0; // 判空：无值返回 0
        return *_file_size; 
    }
    void file_size(unsigned int val) { _file_size = val; }

private:
    // 【ODB 宏指令】：声明 odb::access 为友元，允许 ODB 访问 private 成员进行序列化/反序列化
    friend class odb::access;

    // ================== 数据库字段映射 ==================

    // 数据库自增主键（不映射到业务字段，仅在库内部使用）
    #pragma db id auto
    unsigned long _id;

    // 消息唯一ID（业务主键）：varchar(64)，建立唯一索引（防重复插入）
    #pragma db type("varchar(64)") index unique
    std::string _message_id;

    // 所属会话ID：varchar(64)，建立普通索引（方便按会话拉取历史消息）
    #pragma db type("varchar(64)") index
    std::string _session_id;

    // 发送者用户ID：varchar(64)
    #pragma db type("varchar(64)")
    std::string _user_id;

    // 消息类型：0-文本；1-图片；2-文件；3-语音
    unsigned char _message_type;

    // 消息产生时间：MySQL 的 TIMESTAMP 类型
    #pragma db type("TIMESTAMP")
    boost::posix_time::ptime _create_time;

    // ------------------ 可选字段（对应 nullable） ------------------
    // ODB 的 nullable 包装器：对应 MySQL 中允许为 NULL 的列。
    
    // 文本消息内容，非文本消息该列为 NULL
    odb::nullable<std::string> _content;

    // 文件ID，文本消息为 NULL
    #pragma db type("varchar(64)")
    odb::nullable<std::string> _file_id;

    // 文件名，文本消息为 NULL
    #pragma db type("varchar(128)")
    odb::nullable<std::string> _file_name;

    // 文件大小，文本消息为 NULL
    odb::nullable<unsigned int> _file_size;
};

// 注释里附带的手工生成 ODB 代码的命令（实际由 CMake 自动执行）
// odb -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time person.hxx

} // namespace bite_im