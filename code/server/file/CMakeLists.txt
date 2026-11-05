# ============================================================
# 文件子服务 file_server 的 CMakeLists.txt
# ============================================================

# 1. CMake 版本说明（3.5 及以上，兼容 CMake 4.x）
cmake_minimum_required(VERSION 3.5)

# 2. 声明工程名称
project(file_server)

# 目标可执行文件名
set(target "file_server")

# 编译选项：带调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# ============================================================
# 3. 检测并生成 Protobuf 框架代码
# ============================================================
# 3.1 proto 目录与所需的 proto 文件列表
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
# 注意：文件服务需要 file.proto，base.proto 通常被 import，保留
set(proto_files base.proto file.proto)

# 3.2 检测是否已生成，未生成则预定义生成指令
set(proto_srcs "")
foreach(proto_file ${proto_files})
    string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
        add_custom_command(
            PRE_BUILD
            COMMAND protoc
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                 -I ${proto_path}
                 --experimental_allow_proto3_optional
                 ${proto_path}/${proto_file}
            DEPENDS ${proto_path}/${proto_file}
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
            COMMENT "生成Protobuf框架代码文件: ${proto_cc}"
        )
    endif()
    list(APPEND proto_srcs ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
endforeach()

# ============================================================
# 4. 获取源码目录下的所有源码文件
# ============================================================
set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)

# ============================================================
# 5. 头文件默认搜索路径
# ============================================================
include_directories(${CMAKE_CURRENT_BINARY_DIR})              # 生成的 pb.h 所在目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)    # 公共组件（logger/etcd 等）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)  # 第三方头文件
# 注意：语音服务不操作数据库，不需要 ../odb 目录

# ============================================================
# 6. 声明服务端目标及依赖
# ============================================================
add_executable(${target} ${src_files} ${proto_srcs})

# 7. 链接库
#    按语音服务实际依赖精简：
#    - RPC/序列化: brpc, protobuf, ssl, crypto
#    - 服务发现:   etcd-cpp-api, cpprest, curl
#    - 日志:       spdlog, fmt
#    - 参数解析:   gflags
#    - JSON:       jsoncpp（ASR 返回解析）
#    - 百度 ASR SDK 相关（按 asr.hpp 实际依赖保留/增删）
target_link_libraries(${target}
    -lgflags
    -lspdlog -lfmt
    -lbrpc -lleveldb -lssl -lcrypto -lprotobuf
    -letcd-cpp-api -lcpprest -lcurl
    -ljsoncpp
)

# ============================================================
# 8. 测试客户端（可选）
#    test/ 目录下若没有源码，可整段注释掉，避免构建失败
# ============================================================
set(test_client "file_client")
set(test_files "")
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test)
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_files)
endif()

if (test_files)
    add_executable(${test_client} ${test_files} ${proto_srcs})
    target_link_libraries(${test_client}
        -pthread -lgtest
        -lgflags -lspdlog -lfmt
        -lbrpc -lleveldb -lssl -lcrypto -lprotobuf
        -letcd-cpp-api -lcpprest -lcurl
        -ljsoncpp
    )
endif()

# ============================================================
# 9. 安装路径
# ============================================================
if (test_files)
    INSTALL(TARGETS ${target} ${test_client} RUNTIME DESTINATION bin)
else()
    INSTALL(TARGETS ${target} RUNTIME DESTINATION bin)
endif()