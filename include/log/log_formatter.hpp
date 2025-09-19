#pragma once

#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <map>

namespace sylar
{
    class LogEvent;

    class LogFormatter
    {
    public:
        using ptr = std::shared_ptr<LogFormatter>;

        LogFormatter(const std::string &pattern);
        std::string format(std::shared_ptr<LogEvent> event);

    public:
        class FormatItem
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;
            FormatItem(const std::string &fmt = "");
            virtual ~FormatItem();
            virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) = 0;
        };

        void init();
        bool isError() const;
        const std::string &getPattern() const;

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_isError = false;
    };

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &fmt = "%Y-%m-%d %H:%M:%S");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;

    private:
        std::string m_format;
    };

    class FileNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FileNameFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &fmt = "");
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &fmt);
        virtual void format(std::ostream &os, std::shared_ptr<LogEvent> event) override;

    private:
        std::string m_string;
    };
}