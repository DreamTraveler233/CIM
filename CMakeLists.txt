cmake_minimum_required(VERSION 3.10)
project(CIM)

# 查找 Ragel 状态机编译器
find_program(RAGEL_EXECUTABLE ragel)
if(NOT RAGEL_EXECUTABLE)
    message(FATAL_ERROR "Ragel not found. Please install ragel to compile this project.")
endif()

# 查找 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 查找 zlib 库
find_package(ZLIB REQUIRED)

# 根据构建类型设置不同的编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wall -Wno-deprecated -Werror -Wno-unused-function")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wall")
endif()

# 添加通用编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -std=c++11")

# 处理 .rl 文件，生成对应的 .cpp 文件
# 查找所有 .rl 文件
file(GLOB_RECURSE RL_SOURCES "src/*.rl")
foreach(RL_FILE ${RL_SOURCES})
    # 获取不带扩展名的文件名
    string(REGEX REPLACE "\\.rl$" ".rl.cpp" CPP_FILE ${RL_FILE})
    
    # 添加自定义命令，使用 ragel 生成 cpp 文件
    add_custom_command(
        OUTPUT ${CPP_FILE}
        COMMAND ${RAGEL_EXECUTABLE} -G2 -o ${CPP_FILE} ${RL_FILE}
        DEPENDS ${RL_FILE}
        COMMENT "Generating ${CPP_FILE} from ${RL_FILE}"
    )
    
    # 将生成的 cpp 文件添加到生成文件列表
    list(APPEND GENERATED_SOURCES ${CPP_FILE})
endforeach()

# 收集其他源文件（排除 .rl 文件）
file(GLOB_RECURSE OTHER_SOURCES "src/*.cpp")
list(FILTER OTHER_SOURCES EXCLUDE REGEX ".*\\.rl\\.cpp$")

# 合并所有源文件
set(LIB_SRC ${OTHER_SOURCES} ${GENERATED_SOURCES})

# 生成动态库（.so）
add_library(CIM SHARED ${LIB_SRC})

# 指定 GENERATED_SOURCES 为由 CIM 目标生成的文件
set_source_files_properties(${GENERATED_SOURCES} PROPERTIES GENERATED TRUE)

# 链接 OpenSSL 和 zlib 库到 CIM 库
target_link_libraries(CIM ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})

# 头文件搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/base
    ${PROJECT_SOURCE_DIR}/include/config
    ${PROJECT_SOURCE_DIR}/include/http
    ${PROJECT_SOURCE_DIR}/include/log
    ${PROJECT_SOURCE_DIR}/include/net
    ${PROJECT_SOURCE_DIR}/include/other
    ${PROJECT_SOURCE_DIR}/include/util
    ${PROJECT_SOURCE_DIR}/include/io
)

# 定义测试可执行文件列表
set(TEST_LIST
    test_log_basic
    test_log_performance
    test_log_rotate
    test_config
    test_util
    test_coroutine
    test_scheduler
    test_iomanager
    test_hook
    test_hook2
    test_address
    test_socket
    test_bytearray
    test_http
    test_http_parser
    test_tcp_server
    test_http_server
    test_uri
    test_http_connection
    test_daemon
)

set(EXAMPLES_LIST
    echo_server
)

# 循环创建测试可执行文件
foreach(TEST ${TEST_LIST})
    add_executable(${TEST} tests/${TEST}.cpp)    # 生成可执行文件
    add_dependencies(${TEST} CIM)                # 指定test依赖 CIM库，先编译库再编译可执行文件
    target_link_libraries(${TEST} CIM yaml-cpp)  # 链接库到可执行文件
endforeach()

foreach(EXAMPLE ${EXAMPLES_LIST})
    add_executable(${EXAMPLE} examples/${EXAMPLE}.cpp)
    add_dependencies(${EXAMPLE} CIM)
    target_link_libraries(${EXAMPLE} CIM yaml-cpp)
endforeach()

# 指定可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 指定库文件输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)