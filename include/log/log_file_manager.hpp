#pragma once

#include "log_file.hpp"
#include "singleton.hpp"
#include "lock.hpp"
#include "timer.hpp"
#include "iomanager.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace sylar
{
    /**
     * @brief 日志文件管理类，负责日志文件对象的创建、轮转和管理
     *        支持按分钟、小时、天自动轮转日志文件
     */
    class LogFileManager : public Singleton<LogFileManager>
    {
    public:
        using ptr = std::shared_ptr<LogFileManager>;
        using MutexType = Mutex;

        LogFileManager();
        ~LogFileManager();

        void init();

        /**
         * @brief 检查日志文件是否需要轮转（按分钟、小时、天），并执行轮转操作。
         *
         * 该函数会根据当前时间与上次记录的时间进行比较，判断是否发生了天、小时或分钟的变化。
         * 如果发生变化，则对相应类型的日志文件进行轮转。
         */
        void onCheck();

        /**
         * @brief 获取指定文件名的日志文件对象，如果不存在则创建。
         *
         * @param fileName 日志文件名（含路径）
         * @param rotateType 轮转类型
         * @return LogFilePtr 日志文件智能指针，打开失败时返回空指针
         */
        LogFile::ptr getLogFile(const std::string &fileName);

        /**
         * @brief 移除指定的日志文件对象。
         *
         * @param log 日志文件智能指针
         */
        void removeLogFile(const LogFile::ptr &log);

        /**
         * @brief 对按分钟轮转的日志文件进行轮转。
         *
         * @param file 日志文件智能指针
         */
        void rotateMinute(const LogFile::ptr &file);

        /**
         * @brief 对按小时轮转的日志文件进行轮转。
         *
         * @param file 日志文件智能指针
         */
        void rotateHours(const LogFile::ptr &file);

        /**
         * @brief 对按天轮转的日志文件进行轮转。
         *
         * @param file 日志文件智能指针
         */
        void rotateDays(const LogFile::ptr &file);

    private:
        std::unordered_map<std::string, LogFile::ptr> m_logs; //!< 文件路径+文件名->日志文件对象的映射
        MutexType m_mutex;                                    //!< 线程安全锁
        int m_lastYear;                                       // 年
        int m_lastMonth;                                      // 月
        int m_lastDay;                                        // 日
        int m_lastHour;                                       // 小时
        int m_lastMinute;                                     // 分钟
        Timer::ptr m_timer;                                   // 定时器
        bool m_isInit;                                        // 已初始化
    };
}