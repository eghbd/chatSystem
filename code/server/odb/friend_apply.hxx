// 好友申请事件映射
#pragma once
#include <string>
#include <cstddef>
#include <odb/core.hxx>

namespace bite_im {

#pragma db object table("friend_apply")
class FriendApply {
    public:
        FriendApply(){}
        FriendApply(const std::string &eid,
                    const std::string &uid,
                    const std::string &pid):
            _event_id(eid), _user_id(uid), _peer_id(pid) {}

        std::string event_id() const { return _event_id; }
        void event_id(const std::string &val) { _event_id = val; }

        std::string user_id() const { return _user_id; }
        void user_id(const std::string &val) { _user_id = val; }

        std::string peer_id() const { return _peer_id; }
        void peer_id(const std::string &val) { _peer_id = val; }

    private:
        friend class odb::access;

        #pragma db id auto
        unsigned long _id;

        #pragma db type("varchar(64)") index unique
        std::string _event_id;

        #pragma db type("varchar(64)") index
        std::string _user_id;

        #pragma db type("varchar(64)") index
        std::string _peer_id;
};
}
