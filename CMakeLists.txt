cmake_minimum_required(VERSION 3.10)
project(sylar)

# 让 make 输出详细的编译命令，便于调试
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    src/log/log_level.cpp
    src/log/log_event.cpp
    src/log/log_formatter.cpp
    src/log/log_appender.cpp
    src/log/logger.cpp
    src/log/logger_manager.cpp
    src/log/log_config.cpp
    src/config/config_var_base.cpp
    src/config/config.cpp
    src/thread/thread.cpp
    src/thread/semaphore.cpp
    src/thread/lock.cpp
    src/util.cpp)

# 生成动态库（.so）
add_library(sylar SHARED ${LIB_SRC})

# 头文件搜索目录
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/log)
include_directories(${PROJECT_SOURCE_DIR}/include/config)
include_directories(${PROJECT_SOURCE_DIR}/include/thread)

add_executable(test_log tests/test_log.cpp)     # 生成可执行文件test
add_dependencies(test_log sylar)                # 指定test依赖 sylar库，先编译库再编译可执行文件
target_link_libraries(test_log sylar yaml-cpp)  # 链接sylar库到test可执行文件

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
target_link_libraries(test_config sylar yaml-cpp)

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread sylar yaml-cpp)

# 指定可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 指定库文件输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)