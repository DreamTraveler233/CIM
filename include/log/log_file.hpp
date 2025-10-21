#pragma once

#include <string>
#include <memory>

namespace sylar
{
    enum class RotateType
    {
        None,   // 不轮转
        Minute, // 按分钟轮转
        Hour,   // 按小时轮转
        Day     // 按天轮转
    };

    /**
     * @brief 文件日志类，封装单个日志文件的打开、写入、轮转、获取大小等操作
     */
    class LogFile
    {
    public:
        using ptr = std::shared_ptr<LogFile>;

        /**
         * @brief 构造函数
         * @param filePath 日志文件路径
         */
        LogFile(const std::string &filePath);

        /**
         * @brief 析构函数，关闭文件描述符
         */
        ~LogFile();

        /**
         * @brief 打开日志文件。
         *
         * 以追加写入方式打开指定路径的日志文件，如果文件不存在则创建。
         *
         * @return true 打开成功
         * @return false 打开失败
         */
        bool openFile();

        /**
         * @brief 写日志内容到文件。
         *
         * 将日志内容写入已打开的日志文件，如果文件未打开则写到标准输出。
         *
         * @param logMsg 日志内容字符串
         * @return size_t 实际写入的字节数
         */
        size_t writeLog(const std::string &logMsg);

        /**
         * @brief 日志文件轮转（切换）。
         *
         * 将当前日志文件重命名为新文件名，并打开一个新的日志文件，实现日志文件的无缝切换。
         *
         * @param newFilePath 新日志文件路径
         */
        void rotate(const std::string &newFilePath);

        /**
         * @brief 获取当前日志文件的大小（字节数）。
         *
         * 使用 lseek64 将文件指针移动到文件末尾，并返回文件总字节数。
         *
         * @return int64_t 文件大小（字节数）
         */
        int64_t getFileSize() const;

        /**
         * @brief 获取当前日志文件路径。
         *
         * @return std::string 日志文件路径
         */
        const std::string &getFilePath() const;

        /**
         * @brief 设置日志轮转类型。
         *
         * @param type 日志轮转类型
         */
        void setRotateType(RotateType type);

        /**
         * @brief 获取日志轮转类型。
         *
         * @return RotateType 当前日志轮转类型
         */
        RotateType getRotateType() const;

        RotateType rotateTypeFromString(const std::string &rotateType);
        std::string rotateTypeToString(RotateType rotateType);

    private:
        int m_fd;                //!< 文件描述符
        std::string m_filePath;  //!< 日志文件名
        RotateType m_rotateType; //!< 日志轮转类型
    };
}