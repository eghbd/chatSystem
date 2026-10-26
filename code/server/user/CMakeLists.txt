# 1. 添加cmake版本说明
cmake_minimum_required(VERSION 3.5)
# 2. 声明工程名称
project(user_server)

set(target "user_server")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 3. 生成 Protobuf 框架代码
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
set(proto_files base.proto user.proto file.proto)
set(proto_srcs "")
foreach(proto_file ${proto_files})
    string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
            COMMAND protoc
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} --experimental_allow_proto3_optional ${proto_path}/${proto_file}
            DEPENDS ${proto_path}/${proto_file}
            COMMENT "生成Protobuf框架代码文件: ${proto_cc}"
        )
    endif()
    list(APPEND proto_srcs ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
endforeach()

# 4. 生成 ODB 框架代码
set(odb_path ${CMAKE_CURRENT_SOURCE_DIR}/../odb)
set(odb_files user.hxx)
set(odb_srcs "")
foreach(odb_file ${odb_files})
    string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
            COMMAND odb
            ARGS -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time ${odb_path}/${odb_file}
            DEPENDS ${odb_path}/${odb_file}
            COMMENT "生成ODB框架代码文件: ${odb_cxx}"
        )
    endif()
    list(APPEND odb_srcs ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})
endforeach()

# 5. 收集源码，声明目标
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
add_executable(${target} ${src_files} ${proto_srcs} ${odb_srcs})

# 6. 链接库
target_link_libraries(${target} -lgflags
    -lspdlog -lfmt -lbrpc -lssl -lcrypto
    -lprotobuf -lleveldb -letcd-cpp-api
    -lcpprest -lcurl -lodb-mysql -lodb -lodb-boost
    -lelasticlient -ljsoncpp -lcpr
    -lredis++ -lhiredis)


# 7. 头文件搜索路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)
include_directories(/usr/include/jsoncpp)

# 8. 安装路径
INSTALL(TARGETS ${target} RUNTIME DESTINATION bin)
