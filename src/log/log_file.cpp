#include "log_file.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <cstdio>

namespace sylar
{

    LogFile::LogFile(const std::string &filePath)
        : m_fd(-1),
          m_filePath(filePath),
          m_rotateType(RotateType::None)
    {
    }

    LogFile::~LogFile()
    {
        if (m_fd != -1)
        {
            ::close(m_fd);
        }
    }

    bool LogFile::openFile()
    {
        /*
        O_CREAT：如果文件不存在则创建新文件。
        O_APPEND：每次写入都追加到文件末尾。
        O_WRONLY：以只写方式打开文件。
        DEFFILEMODE：新建文件时的默认权限（通常定义为 0666，即所有用户可读写）。
        */
        int fd = ::open(m_filePath.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
        if (fd < 0)
        {
            std::cout << "open file log error.path!" << m_filePath << std::endl;
            return false;
        }

        if (m_fd != -1)
        {
            ::close(m_fd);
        }
        m_fd = fd;
        return true;
    }

    size_t LogFile::writeLog(const std::string &logMsg)
    {
        int fd = m_fd == -1 ? 1 : m_fd; // 如果未打开文件，则写到标准输出
        return ::write(fd, logMsg.data(), logMsg.size());
    }

    void LogFile::rotate(const std::string &newFilePath)
    {
        // 如果旧文件未打开，则直接返回
        if (m_filePath.empty())
        {
            return;
        }

        // 关闭当前文件
        if (m_fd != -1)
        {
            ::close(m_fd);
            m_fd = -1;
        }

        // 将文件重命名为归档文件
        if (::rename(m_filePath.c_str(), newFilePath.c_str()) != 0)
        {
            std::cerr << "rename failed. old:" << m_filePath << " new:" << newFilePath << std::endl;
            // 即使重命名失败也要重新打开原文件
        }

        // 重新创建原文件
        openFile();
    }

    void LogFile::setRotateType(RotateType type)
    {
        m_rotateType = type;
    }

    RotateType LogFile::getRotateType() const
    {
        return m_rotateType;
    }

    RotateType LogFile::rotateTypeFromString(const std::string &str)
    {
#define XX(rotateType, name) \
    if (str == #name)        \
        return RotateType::rotateType;

        XX(Minute, minute);
        XX(Hour, hour);
        XX(Day, day);
#undef XX
        return RotateType::None;
    }

    std::string LogFile::rotateTypeToString(RotateType rotateType)
    {
        switch (rotateType)
        {
#define XX(name)           \
    case RotateType::name: \
        return #name;

            XX(Minute);
            XX(Hour);
            XX(Day);
#undef XX
        default:
            return "None";
        }
    }

    int64_t LogFile::getFileSize() const
    {
        if (m_fd == -1)
        {
            return 0;
        }
        return ::lseek64(m_fd, 0, SEEK_END);
    }

    const std::string &LogFile::getFilePath() const
    {
        return m_filePath;
    }
}