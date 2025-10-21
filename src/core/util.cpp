#include "util.hpp"
#include "macro.hpp"
#include "coroutine.hpp"
#include <execinfo.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

namespace sylar
{
    static auto g_logger = SYLAR_LOG_NAME("system");

    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint64_t GetCoroutineId()
    {
        return Coroutine::GetCoroutineId();
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        // 分配存储堆栈地址的数组空间
        void **array = (void **)malloc(sizeof(void *) * size);
        // 获取函数调用堆栈地址
        size_t s = ::backtrace(array, size);

        // 将地址转换为可读的符号字符串
        char **strings = backtrace_symbols(array, s);
        if (NULL == strings)
        {
            SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
            free(array);
            return;
        }

        // 将堆栈符号信息存储到输出向量中
        for (size_t i = skip; i < s; ++i)
        {
            bt.push_back(strings[i]);
        }

        // 释放分配的内存
        free(array);
        free(strings);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i)
        {
            ss << prefix << bt[i] << std::endl;
        }

        return ss.str();
    }

    uint64_t TTime::NowToMS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }

    uint64_t TTime::NowToUS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    }

    uint64_t TTime::NowToS()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr); // 获取当前时间
        return tv.tv_sec;           // 返回秒数
    }

    uint64_t TTime::Now(int& year, int& month, int& day, int& hour, int& minute, int& second)
    {
        struct tm tm;
        time_t t = time(nullptr); // 获取当前时间戳（Unix 时间戳）
        localtime_r(&t, &tm);     // 线程安全地将时间戳转换为本地时间

        year = tm.tm_year + 1900;
        month = tm.tm_mon + 1;
        day = tm.tm_mday;
        hour = tm.tm_hour;
        minute = tm.tm_min;
        second = tm.tm_sec;
        return t; // 返回当前时间戳（秒）
    }

    std::string TTime::NowToString()
    {
        struct tm tm;             // 用于存储分解的时间值
        time_t t = time(nullptr); // 获取当前时间戳（Unix 时间戳）
        localtime_r(&t, &tm);     // 线程安全地将时间戳转换为本地时间

        char buf[128] = {0};
        size_t n = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        return std::string(buf, n); // 使用buf的前n个字符构造string对象
    }

    bool StringUtils::StartsWith(const std::string &str, const std::string &sub)
    {
        // 如果字串为空，直接返回 true
        if (sub.empty())
        {
            return true;
        }
        // 如果字符串为空或字符串比子串短，直接返回 false
        auto strLen = str.size();
        auto subLen = sub.size();
        if (str.empty() || strLen < subLen)
        {
            return false;
        }
        // 用 str 从下标 0 开始、长度为 subLen 的子串，与 sub 进行比较
        return str.compare(0, subLen, sub) == 0;
    }

    bool StringUtils::EndsWith(const std::string &str, const std::string &sub)
    {
        // 如果字串为空，直接返回 true
        if (sub.empty())
        {
            return true;
        }
        // 如果字符串为空或字符串比子串短，直接返回 false
        auto strLen = str.size();
        auto subLen = sub.size();
        if (str.empty() || strLen < subLen)
        {
            return false;
        }
        // 用 str 从下标 0 开始、长度为 subLen 的子串，与 sub 进行比较
        return str.compare(strLen - subLen, subLen, sub) == 0;
    }

    std::string StringUtils::FilePath(const std::string &path)
    {
        auto pos = path.find_last_of("/\\"); // 查找最后一个'/'或'\'的位置（兼容Linux和Windows路径）
        if (pos != std::string::npos)        // 如果找到了分隔符
        {
            return path.substr(0, pos + 1); // 返回从开头到分隔符前的所有字符（即目录部分）
        }
        else // 如果没有找到分隔符
        {
            return "./"; // 返回当前目录
        }
    }

    std::string StringUtils::FileNameExt(const std::string &path)
    {
        auto pos = path.find_last_of("/\\"); // 查找路径中最后一个'/'或'\'的位置（兼容Linux和Windows路径）
        if (pos != std::string::npos)        // 如果找到了分隔符
        {
            if (pos + 1 < path.size()) // 并且分隔符后还有内容
            {
                return path.substr(pos + 1); // 返回分隔符后的所有内容（即文件名+扩展名）
            }
        }
        return path; // 如果没有分隔符，直接返回原字符串（说明本身就是文件名）
    }

    std::string StringUtils::FileName(const std::string &path)
    {
        std::string file_name = FileNameExt(path); // 先获取文件名（含扩展名）
        auto pos = file_name.find_last_of(".");    // 查找最后一个'.'的位置（即扩展名前的点）
        if (pos != std::string::npos)              // 如果找到了'.'
        {
            if (pos != 0) // 并且'.'不是第一个字符（防止隐藏文件如 .bashrc）
            {
                return file_name.substr(0, pos); // 返回从头到'.'前的内容（即文件名，不含扩展名）
            }
        }
        return file_name; // 如果没有'.'，或'.'在第一个字符，直接返回整个文件名
    }

    std::string StringUtils::Extension(const std::string &path)
    {
        std::string file_name = FileNameExt(path); // 先获取文件名（含扩展名）
        auto pos = file_name.find_last_of(".");    // 查找最后一个'.'的位置
        if (pos != std::string::npos)              // 如果找到了'.'
        {
            if (pos != 0 && pos + 1 < path.size()) // '.'不是第一个字符，且后面还有内容
            {
                return file_name.substr(pos); // 返回从'.'开始到结尾的内容（即扩展名，包含点）
            }
        }
        return std::string(); // 没有扩展名则返回空字符串
    }

    std::vector<std::string> StringUtils::SplitString(const std::string &str, const std::string &delimiter)
    {
        std::vector<std::string> result; // 存放分割后的子串
        // 如果分隔符为空，直接返回空vector
        if (delimiter.empty())
        {
            return result;
        }
        size_t last = 0; // 上一次分割结束的位置
        size_t next = 0; // 下一个分隔符出现的位置
        // 循环查找分隔符并分割
        while ((next = str.find(delimiter, last)) != std::string::npos)
        {
            if (next > last) // 分隔符前有内容
            {
                // 截取从last到next之间的子串，加入结果
                result.emplace_back(str.substr(last, next - last));
            }
            // 更新last到下一个分隔符后面
            last = next + delimiter.size();
        }
        // 处理最后一段（如果last还没到末尾）
        if (last < str.size())
        {
            result.emplace_back(str.substr(last));
        }
        return result; // 返回所有分割结果
    }

    void FSUtil::ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix)
    {
        if (access(path.c_str(), 0) != 0)
        {
            return;
        }
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
        {
            return;
        }
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)) != nullptr)
        {
            if (dp->d_type == DT_DIR)
            {
                if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
                {
                    continue;
                }
                ListAllFile(files, path + "/" + dp->d_name, subfix);
            }
            else if (dp->d_type == DT_REG)
            {
                std::string filename(dp->d_name);
                if (subfix.empty())
                {
                    files.push_back(path + "/" + filename);
                }
                else
                {
                    if (filename.size() < subfix.size())
                    {
                        continue;
                    }
                    if (filename.substr(filename.length() - subfix.size()) == subfix)
                    {
                        files.push_back(path + "/" + filename);
                    }
                }
            }
        }
        closedir(dir);
    }

    static int __lstat(const char *file, struct stat *st = nullptr)
    {
        struct stat lst;
        int ret = lstat(file, &lst);
        if (st)
        {
            *st = lst;
        }
        return ret;
    }

    static int __mkdir(const char *dirname)
    {
        if (access(dirname, F_OK) == 0)
        {
            return 0;
        }
        return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    bool FSUtil::Mkdir(const std::string &dirname)
    {
        if (__lstat(dirname.c_str()) == 0)
        {
            return true;
        }
        char *path = strdup(dirname.c_str());
        char *ptr = strchr(path + 1, '/');
        do
        {
            for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/'))
            {
                *ptr = '\0';
                if (__mkdir(path) != 0)
                {
                    break;
                }
            }
            if (ptr != nullptr)
            {
                break;
            }
            else if (__mkdir(path) != 0)
            {
                break;
            }
            free(path);
            return true;
        } while (0);
        free(path);
        return false;
    }

    bool FSUtil::IsRunningPidfile(const std::string &pidfile)
    {
        if (__lstat(pidfile.c_str()) != 0)
        {
            return false;
        }
        std::ifstream ifs(pidfile);
        std::string line;
        if (!ifs || !std::getline(ifs, line))
        {
            return false;
        }
        if (line.empty())
        {
            return false;
        }
        pid_t pid = atoi(line.c_str());
        if (pid <= 1)
        {
            return false;
        }
        if (kill(pid, 0) != 0)
        {
            return false;
        }
        return true;
    }

    bool FSUtil::Unlink(const std::string &filename, bool exist)
    {
        if (!exist && __lstat(filename.c_str()))
        {
            return true;
        }
        return ::unlink(filename.c_str()) == 0;
    }

    bool FSUtil::Rm(const std::string &path)
    {
        struct stat st;
        if (lstat(path.c_str(), &st))
        {
            return true;
        }
        if (!(st.st_mode & S_IFDIR))
        {
            return Unlink(path);
        }

        DIR *dir = opendir(path.c_str());
        if (!dir)
        {
            return false;
        }

        bool ret = true;
        struct dirent *dp = nullptr;
        while ((dp = readdir(dir)))
        {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            {
                continue;
            }
            std::string dirname = path + "/" + dp->d_name;
            ret = Rm(dirname);
        }
        closedir(dir);
        if (::rmdir(path.c_str()))
        {
            ret = false;
        }
        return ret;
    }

    bool FSUtil::Mv(const std::string &from, const std::string &to)
    {
        if (!Rm(to))
        {
            return false;
        }
        return rename(from.c_str(), to.c_str()) == 0;
    }

    bool FSUtil::Realpath(const std::string &path, std::string &rpath)
    {
        if (__lstat(path.c_str()))
        {
            return false;
        }
        char *ptr = ::realpath(path.c_str(), nullptr);
        if (nullptr == ptr)
        {
            return false;
        }
        std::string(ptr).swap(rpath);
        free(ptr);
        return true;
    }

    bool FSUtil::Symlink(const std::string &from, const std::string &to)
    {
        if (!Rm(to))
        {
            return false;
        }
        return ::symlink(from.c_str(), to.c_str()) == 0;
    }

    std::string FSUtil::Dirname(const std::string &filename)
    {
        if (filename.empty())
        {
            return ".";
        }
        auto pos = filename.rfind('/');
        if (pos == 0)
        {
            return "/";
        }
        else if (pos == std::string::npos)
        {
            return ".";
        }
        else
        {
            return filename.substr(0, pos);
        }
    }

    std::string FSUtil::Basename(const std::string &filename)
    {
        if (filename.empty())
        {
            return filename;
        }
        auto pos = filename.rfind('/');
        if (pos == std::string::npos)
        {
            return filename;
        }
        else
        {
            return filename.substr(pos + 1);
        }
    }

    bool FSUtil::OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode)
    {
        ifs.open(filename.c_str(), mode);
        return ifs.is_open();
    }

    bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode)
    {
        ofs.open(filename.c_str(), mode);
        if (!ofs.is_open())
        {
            std::string dir = Dirname(filename);
            Mkdir(dir);
            ofs.open(filename.c_str(), mode);
        }
        return ofs.is_open();
    }
}