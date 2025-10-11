#pragma once

#include "logger.hpp"
#include "logger_manager.hpp"
#include "util.hpp"
#include "thread.hpp"
#include <string.h>
#include <assert.h>

/**
 * @brief 分支预测优化宏定义
 * @details 这些宏用于帮助编译器进行分支预测优化，提高程序执行效率
 * 在支持的编译器(GCC/LLVM)上使用__builtin_expect内建函数进行优化，
 * 在不支持的编译器上退化为普通表达式
 */
#if defined __GNUC__ || defined __llvm__
#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif

#define SYLAR_LOG(logger, level)                                               \
    if (level >= logger->getLevel())                                           \
    sylar::LogEventWrap(                                                       \
        sylar::LogEvent::ptr(                                                  \
            new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0,          \
                                sylar::GetThreadId(), sylar::GetCoroutineId(), \
                                time(0), sylar::Thread::GetName())))           \
        .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG(logger, sylar::LogLevel::Level::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG(logger, sylar::LogLevel::Level::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG(logger, sylar::LogLevel::Level::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG(logger, sylar::LogLevel::Level::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG(logger, sylar::LogLevel::Level::FATAL)

#define SYLAR_LOG_FMT(logger, level, fmt, ...)                                 \
    if (level >= logger->getLevel())                                           \
    sylar::LogEventWrap(                                                       \
        sylar::LogEvent::ptr(                                                  \
            new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0,          \
                                sylar::GetThreadId(), sylar::GetCoroutineId(), \
                                time(0), sylar::Thread::GetName())))           \
        .getEvent()                                                            \
        ->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT(logger, sylar::LogLevel::Level::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT(logger, sylar::LogLevel::Level::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT(logger, sylar::LogLevel::Level::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT(logger, sylar::LogLevel::Level::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT(logger, sylar::LogLevel::Level::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::loggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::loggerMgr::GetInstance()->getLogger(name)

#define SYLAR_ASSERT(X)                                                                \
    if (SYLAR_UNLIKELY(!(X)))                                                          \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #X                          \
                                          << "\nbacktrace:\n"                          \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(X);                                                                     \
    }

#define SYLAR_ASSERT2(X, W)                                                            \
    if (SYLAR_UNLIKELY(!(X)))                                                          \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #X                          \
                                          << "\n"                                      \
                                          << W                                         \
                                          << "\nbacktrace:\n"                          \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(X);                                                                     \
    }
