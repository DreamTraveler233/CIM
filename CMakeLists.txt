cmake_minimum_required(VERSION 3.10)
project(sylar)

# 查找 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 查找 zlib 库
find_package(ZLIB REQUIRED)

# 根据构建类型设置不同的编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wall -Wno-deprecated -Werror -Wno-unused-function")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -DNDEBUG -Wall -Wno-deprecated -Werror -Wno-unused-function")
endif()

# 添加通用编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -std=c++11")

# 收集源文件
file(GLOB_RECURSE LIB_SRC "src/*.cpp")

# 生成动态库（.so）
add_library(sylar SHARED ${LIB_SRC})

# 链接 OpenSSL 和 zlib 库到 sylar 库
target_link_libraries(sylar ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})

# 头文件搜索目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/config
    ${PROJECT_SOURCE_DIR}/include/core
    ${PROJECT_SOURCE_DIR}/include/log
    ${PROJECT_SOURCE_DIR}/include/net
    ${PROJECT_SOURCE_DIR}/include/runtime
    ${PROJECT_SOURCE_DIR}/include/system
    ${PROJECT_SOURCE_DIR}/include/http
    ${PROJECT_SOURCE_DIR}/include/stream
    ${PROJECT_SOURCE_DIR}/include/util
)

# 定义测试可执行文件列表
set(TEST_EXECUTABLES
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

set(EXAMPLES_EXECUTABLES
    echo_server
)

# 循环创建测试可执行文件
foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
    add_executable(${TEST_EXECUTABLE} tests/${TEST_EXECUTABLE}.cpp) # 生成可执行文件
    add_dependencies(${TEST_EXECUTABLE} sylar)                      # 指定test依赖 sylar库，先编译库再编译可执行文件
    target_link_libraries(${TEST_EXECUTABLE} sylar yaml-cpp)        # 链接库到可执行文件
endforeach()

foreach(EXAMPLES_EXECUTABLES ${EXAMPLES_EXECUTABLES})
    add_executable(${EXAMPLES_EXECUTABLES} examples/${EXAMPLES_EXECUTABLES}.cpp)
    add_dependencies(${TEST_EXECUTABLE} sylar)
    target_link_libraries(${EXAMPLES_EXECUTABLES} sylar yaml-cpp)
endforeach()

# 指定可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 指定库文件输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)