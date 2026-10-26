// 建立映射关系
// 用于和mysql进行交互
#pragma once // 头文件防重包含

#include <string>
#include <cstddef> 
#include <odb/nullable.hxx> // ODB 提供的一个包装类，用于处理数据库中的 NULL 值
#include <odb/core.hxx>    // ODB 核心头文件，必须包含，里面定义了 odb::access 等

namespace bite_im {
#pragma db object table("user") // 告诉 ODB：这是一个需要映射到数据库的对象，对应的表名是 "user"
class User {
    public:
        // 默认构造函数。ODB 在从数据库读取数据重建对象时，必须调用默认构造函数，所以不能省
        User(){} 

        // 构造函数1：通过 用户ID、昵称、密码 创建用户（新增用户）
        User(const std::string &uid, const std::string &nickname, const std::string &password):
            _user_id(uid), _nickname(nickname), _password(password){}
        
        // 构造函数2：通过 用户ID、手机号 创建用户（手机号注册场景，默认将ID作为昵称）
        User(const std::string &uid, const std::string &phone):
            _user_id(uid), _nickname(uid), _phone(phone){}
        
        // ---- 以下是一系列的 Getter 和 Setter 方法 ----

        void user_id(const std::string &val) { _user_id = val; }
        std::string user_id() { return _user_id; }

        // 下面这些在返回前都做了判空处理。因为数据库里取出来可能是 NULL (odb::nullable)，直接解引用会导致程序崩溃
        std::string nickname() { 
            if (_nickname) return *_nickname; // 如果非空，提取裸值
            return std::string();             // 如果为空，返回空字符串
        }
        void nickname(const std::string &val) { _nickname = val; }

        std::string description() { 
            if (!_description) return std::string();
            return *_description; 
        }
        void description(const std::string &val) { _description = val; }
        
        std::string password() { 
            if (!_password) return std::string();
            return *_password; 
        }
        void password(const std::string &val) { _password = val; }

        std::string phone() { 
            if (!_phone) return std::string();
            return *_phone; 
        }
        void phone(const std::string &val) { _phone = val; }

        std::string avatar_id() { 
            if (!_avatar_id) return std::string();
            return *_avatar_id; 
        }
        void avatar_id(const std::string &val) { _avatar_id = val; }

    private:
        // 友元声明：这是 ODB 的核心机制！
        // 因为 ODB 需要访问 private 的成员变量来映射，为了不破坏类的封装性，必须加上这行
        friend class odb::access; 

        // 主键 ID。
        // auto 表示自增（对应 MySQL 的 AUTO_INCREMENT），插入时不用管，数据库会自动分配
        #pragma db id auto
        unsigned long _id;

        // 用户ID：强制指定为 varchar(64) 类型。index unique 表示建立一个唯一索引（查得快且不允许重复）
        #pragma db type("varchar(64)") index unique 
        std::string _user_id;

        // 用户昵称：varchar(64)，也设为唯一索引（防止重名）
        #pragma db type("varchar(64)") index unique
        odb::nullable<std::string> _nickname; // 允许为空

        // 用户签名：没加 type 限制，ODB 默认会生成一个足够长（比如 varchar(255) 或 text）的字段
        odb::nullable<std::string> _description; 

        // 用户密码：varchar(64)。注意：密码通常不应该做唯一索引，因为不同用户可能碰巧密码一样
        #pragma db type("varchar(64)")
        odb::nullable<std::string> _password; 

        // 用户手机号：varchar(64)，唯一索引（一个手机号只能注册一个账号）
        #pragma db type("varchar(64)") index unique
        odb::nullable<std::string> _phone; 

        // 用户头像ID：varchar(64)
        #pragma db type("varchar(64)")
        odb::nullable<std::string> _avatar_id; 
};
}

// 最底下这行是备忘：在实际使用 ODB 编译器时，你应该运行类似下面的命令
// odb -d mysql --std c++11 --generate-query --generate-schema user.hxx

