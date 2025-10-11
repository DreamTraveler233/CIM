cmake_minimum_required(VERSION 3.10)
project(sylar)

# 让 make 输出详细的编译命令，便于调试
set(CMAKE_VERBOSE_MAKEFILE OFF)

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

# 头文件搜索目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/config
    ${PROJECT_SOURCE_DIR}/include/core
    ${PROJECT_SOURCE_DIR}/include/log
    ${PROJECT_SOURCE_DIR}/include/net
    ${PROJECT_SOURCE_DIR}/include/runtime
    ${PROJECT_SOURCE_DIR}/include/system
)

# 定义测试可执行文件列表
set(TEST_EXECUTABLES
    test_log_basic
    test_log_thread_safe
    test_log_performance
    test_config
    test_config_thread_sefa
    test_util
    test_coroutine
    test_scheduler
    test_iomanager
    test_hook
    test_hook2
    test_address
    test_socket
)

# 创建一个函数来确定测试源文件路径
function(GET_TEST_SOURCE TEST_NAME RESULT)
    # 根据测试名称确定源文件路径
    if(${TEST_NAME} STREQUAL "test_util")
        set(TEST_SOURCE "tests/core/${TEST_NAME}.cpp" PARENT_SCOPE)
    elseif(${TEST_NAME} STREQUAL "test_coroutine" OR ${TEST_NAME} STREQUAL "test_scheduler" OR ${TEST_NAME} STREQUAL "test_iomanager")
        set(TEST_SOURCE "tests/runtime/${TEST_NAME}.cpp" PARENT_SCOPE)
    elseif(${TEST_NAME} STREQUAL "test_hook" OR ${TEST_NAME} STREQUAL "test_hook2")
        set(TEST_SOURCE "tests/system/${TEST_NAME}.cpp" PARENT_SCOPE)
    elseif(${TEST_NAME} STREQUAL "test_address" OR ${TEST_NAME} STREQUAL "test_socket")
        set(TEST_SOURCE "tests/net/${TEST_NAME}.cpp" PARENT_SCOPE)
    elseif(${TEST_NAME} MATCHES "test_log_.*")
        set(TEST_SOURCE "tests/log/${TEST_NAME}.cpp" PARENT_SCOPE)
    elseif(${TEST_NAME} MATCHES "test_config.*")
        set(TEST_SOURCE "tests/config/${TEST_NAME}.cpp" PARENT_SCOPE)
    else()
        set(TEST_SOURCE "tests/${TEST_NAME}.cpp" PARENT_SCOPE)
    endif()
endfunction()

# 循环创建测试可执行文件
foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
    # 获取测试源文件路径
    GET_TEST_SOURCE(${TEST_EXECUTABLE} TEST_SOURCE)
    
    add_executable(${TEST_EXECUTABLE} ${TEST_SOURCE})
    target_link_libraries(${TEST_EXECUTABLE} sylar yaml-cpp)
endforeach()

# 指定可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 指定库文件输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)