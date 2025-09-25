#include "log_formatter.hpp"
#include "log_event.hpp"
#include "logger.hpp"
#include <functional>
#include <sstream>

namespace sylar
{
    LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<LogEvent> event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, event);
        }
        return ss.str();
    }
    LogFormatter::FormatItem::FormatItem(const std::string &fmt) {}
    LogFormatter::FormatItem::~FormatItem() {}
    bool LogFormatter::isError() const { return m_isError; }
    const std::string &LogFormatter::getPattern() const { return m_pattern; }
    void LogFormatter::init()
    {
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while (n < m_pattern.size())
            {
                if (!fmt_status && (!isalpha(m_pattern[n])) && m_pattern[n] != '{' && m_pattern[n] != '}')
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1;
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                }
                else if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if (n == m_pattern.size())
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_isError = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }

        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                               \
    {                                                                            \
        #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }
            XX(m, MessageFormatItem),    // %m -- 消息体
            XX(p, LevelFormatItem),      // %p -- level
            XX(r, ElapseFormatItem),     // %r -- 启动后的时间
            XX(c, NameFormatItem),       // %c -- 日志名称
            XX(t, ThreadIdFormatItem),   // %t -- 线程id
            XX(N, ThreadNameFormatItem), // %N -- 线程名称
            XX(n, NewLineFormatItem),    // %n -- 回车换行
            XX(d, DateTimeFormatItem),   // %d -- 时间
            XX(f, FileNameFormatItem),   // %f -- 文件名
            XX(l, LineFormatItem),       // %l -- 行号
            XX(T, TabFormatItem),        // %T -- Tab
            XX(F, FiberIdFormatItem),    // %F -- 协程号
#undef XX
        };

        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_isError = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }

            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }

        // std::cout << m_items.size() << std::endl;
    }

    MessageFormatItem::MessageFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void MessageFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getMessage();
    }
    LevelFormatItem::LevelFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void LevelFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << LogLevel::ToString(event->getLevel());
    }
    ElapseFormatItem::ElapseFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void ElapseFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getElapse();
    }
    NameFormatItem::NameFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void NameFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getLogger()->getName();
    }
    ThreadIdFormatItem::ThreadIdFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void ThreadIdFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getThreadId();
    }
    DateTimeFormatItem::DateTimeFormatItem(const std::string &fmt)
        : m_format(fmt)
    {
        if (m_format.empty())
        {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void DateTimeFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
    FileNameFormatItem::FileNameFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void FileNameFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getRelativeFileName();
    }
    LineFormatItem::LineFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void LineFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getLine();
    }
    NewLineFormatItem::NewLineFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void NewLineFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << std::endl;
    }
    TabFormatItem::TabFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void TabFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << "\t";
    }
    FiberIdFormatItem::FiberIdFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void FiberIdFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getFiberId();
    }
    StringFormatItem::StringFormatItem(const std::string &fmt)
        : m_string(fmt)
    {
    }
    void StringFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << m_string;
    }
    ThreadNameFormatItem::ThreadNameFormatItem(const std::string &fmt)
        : FormatItem(fmt)
    {
    }
    void ThreadNameFormatItem::format(std::ostream &os, std::shared_ptr<LogEvent> event)
    {
        os << event->getThreadName();
    }
}