#include "timer.hpp"
#include "time_utils.hpp"

namespace sylar
{
    /**
     * @brief 构造一个定时器对象
     * @param ms 定时器的间隔时间，单位为毫秒
     * @param cb 定时器回调函数
     * @param recurring 是否为重复定时器，true表示重复触发，false表示只触发一次
     * @param manager 管理该定时器的定时器管理器
     */
    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager)
        : m_recurring(recurring),
          m_ms(ms),
          m_next(TimeUtils::NowToMS() + m_ms),
          m_cb(cb),
          m_manager(manager) {}

    /**
     * @brief 构造一个定时器对象
     * @param next 定时器下次触发的时间戳
     */
    Timer::Timer(uint64_t next)
        : m_next(next)
    {
    }

    /**
     * @brief 取消定时器
     * @return 如果成功取消定时器则返回true，否则返回false
     *
     * 取消定时器的操作包括：
     * 1. 清空回调函数指针
     * 2. 从定时器管理器的定时器集合中移除该定时器
     */
    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (m_cb)
        {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief 刷新定时器，将定时器的下次执行时间重置为当前时间加上间隔时间
     * @return 如果成功刷新定时器则返回true，否则返回false
     *
     * 刷新定时器的操作包括：
     * 1. 检查定时器是否有效（回调函数是否存在）
     * 2. 从定时器管理器中移除该定时器
     * 3. 更新下次执行时间为当前时间加上间隔时间
     * 4. 将定时器重新插入到定时器管理器中
     */
    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        m_manager->m_timers.erase(it);
        m_next = TimeUtils::NowToMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    /**
     * @brief 重置定时器
     * @param ms 新的定时器间隔时间，单位为毫秒
     * @param from_now 是否从当前时间开始计算，true表示从现在开始，false表示从原定时间点开始计算
     * @return 重置成功返回true，失败返回false
     *
     * 重置定时器的操作包括：
     * 1. 检查新参数是否与当前参数相同，如果相同且不需要重新计算时间则直接返回
     * 2. 检查定时器是否有效（回调函数是否存在）
     * 3. 从定时器管理器中移除该定时器
     * 4. 根据from_now参数确定新的起始时间
     * 5. 更新定时器的间隔时间和下次执行时间
     * 6. 将定时器重新插入到定时器管理器中
     */
    bool Timer::reset(uint64_t ms, bool from_now)
    {
        // 如果新的间隔时间与当前间隔时间相同，并且不需要从现在开始计算，则无需重置
        if (ms == m_ms && !from_now)
        {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        // 检查定时器回调函数是否存在
        if (!m_cb)
        {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        // 查找定时器是否在管理器中存在
        if (it == m_manager->m_timers.end())
        {
            return false;
        }
        // 从定时器管理器中移除该定时器
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if (from_now)
        {
            // 从当前时间开始计算
            start = TimeUtils::NowToMS();
        }
        else
        {
            // 从原定时间点开始计算
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + ms;
        // 将更新后的定时器重新添加到管理器中
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    /**
     * @brief 比较两个定时器智能指针的大小
     * @param lhs 左侧定时器智能指针
     * @param rhs 右侧定时器智能指针
     * @return 如果左侧定时器小于右侧定时器则返回true，否则返回false
     *
     * 比较规则：
     * 1. 首先比较m_next成员变量（下次执行时间）
     * 2. 如果m_next相等，则比较智能指针本身的地址
     * 3. 空指针被认为是最小的
     */
    bool Timer::Comparator::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const
    {
        // 处理空指针情况
        if (!lhs && !rhs)
        {
            return false;
        }
        if (!lhs)
        {
            return true;
        }
        if (!rhs)
        {
            return false;
        }

        // 按照下次执行时间进行比较
        if (lhs->m_next < rhs->m_next)
        {
            return true;
        }
        if (lhs->m_next > rhs->m_next)
        {
            return false;
        }

        // 如果下次执行时间相同，则按照对象地址比较
        return lhs.get() < rhs.get();
    }

    TimerManager::TimerManager()
    {
        m_previouseTime = TimeUtils::NowToMS();
    }

    TimerManager::~TimerManager()
    {
    }

    /**
     * @brief 添加定时器
     * @param ms 定时器超时时间，单位毫秒
     * @param cb 定时器回调函数
     * @param recurring 是否为周期性定时器
     * @return 返回创建的定时器对象指针
     */
    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutex::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }

    /**
     * @brief 定时器回调函数，检查条件对象是否仍然有效，如果有效则执行回调
     * @param weak_cond 条件对象的弱引用，用于检查对象是否仍然存在
     * @param cb 需要执行的回调函数
     */
    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp)
        {
            cb();
        }
    }

    /**
     * @brief 添加一个条件定时器
     * @param ms 定时器触发的时间间隔，单位为毫秒
     * @param cb 定时器触发时执行的回调函数
     * @param weak_cond 条件弱引用指针，只有当该弱引用可以lock成功时才会执行回调函数
     * @param recurring 是否为重复触发定时器，true表示重复执行，false表示只执行一次
     * @return 返回创建的定时器对象指针
     *
     * 该函数创建一个条件定时器，只有当weak_cond可以成功lock为shared_ptr时，才会执行回调函数。
     * 这通常用于确保回调执行时相关对象仍然存在，避免悬空指针访问。
     */
    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                               std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    /**
     * @brief 获取距离下一个定时器触发的时间间隔
     * @return 距离下一个定时器触发的毫秒数，如果没有定时器则返回UINT64_MAX
     *
     * 该函数用于获取距离下一个定时器触发的时间间隔。
     * 如果有定时器已经超时，则返回0；如果没有定时器，则返回UINT64_MAX。
     */
    uint64_t TimerManager::getNextTimer()
    {
        RWMutex::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty())
        {
            return ~0ull;
        }
        const Timer::ptr &next = *m_timers.begin();
        uint64_t now = TimeUtils::NowToMS();
        if (now >= next->m_next)
        {
            return 0;
        }
        else
        {
            return next->m_next - now;
        }
    }

    /**
     * @brief 获取已到期的定时器回调函数列表
     * @param cbs 用于存储已到期定时器回调函数的向量引用
     *
     * 该函数会找出所有已到期的定时器，并执行以下操作：
     * 1. 收集所有已到期的定时器回调函数
     * 2. 对于重复类型的定时器，重新设置下次执行时间并重新加入定时器队列
     * 3. 对于一次性定时器，清空其回调函数
     */
    void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
    {
        uint64_t now_ms = TimeUtils::NowToMS();
        std::vector<Timer::ptr> expired;

        {
            RWMutex::ReadLock lock(m_mutex);
            if (m_timers.empty())
            {
                return;
            }
        }
        // 获取写锁以修改定时器集合
        RWMutex::WriteLock lock(m_mutex);
        if (m_timers.empty())
        {
            return;
        }

        // 检查是否有系统时钟回退或是否有到期的定时器
        bool rollover = detectClockRollover(now_ms);
        if (!rollover && ((*m_timers.begin())->m_next > now_ms))
        {
            return;
        }

        // 查找所有已到期的定时器
        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while (it != m_timers.end() && (*it)->m_next == now_ms)
        {
            ++it;
        }
        // 将已到期的定时器移到expired向量中
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        // 处理所有已到期的定时器
        for (auto &timer : expired)
        {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring)
            {
                // 对于重复执行的定时器，设置下次执行时间并重新插入队列
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else
            {
                // 对于一次性定时器，清空回调函数
                timer->m_cb = nullptr;
            }
        }
    }

    bool TimerManager::hasTimer()
    {
        RWMutex::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }

    /**
     * @brief 添加定时器到定时器管理器中
     * @param val 要添加的定时器智能指针
     * @param lock 写锁对象引用，用于同步访问
     *
     * 该函数负责将新的定时器插入到定时器集合中，并根据插入位置判断是否需要触发前置通知。
     * 如果新插入的定时器位于集合的最前端且之前未触发过tick，则标记为已触发并调用前置插入通知。
     */
    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock)
    {
        auto it = m_timers.insert(val).first;
        bool at_front = (it == m_timers.begin() && !m_tickled);
        if (at_front)
        {
            m_tickled = true;
        }
        lock.unlock();
        if (at_front)
        {
            onTimerInsertedAtFront();
        }
    }

    /**
     * @brief 检测系统时钟回退
     * @param now_ms 当前时间戳（毫秒）
     * @return 如果检测到时钟回退则返回true，否则返回false
     *
     * 该函数用于检测系统时钟是否发生了显著回退。当时钟回退超过1小时时，
     * 认为是一次有效的时钟回退事件，需要特殊处理。
     */
    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if (now_ms < m_previouseTime &&
            now_ms < (m_previouseTime - 60 * 60 * 1000))
        {
            rollover = true;
        }
        m_previouseTime = now_ms;
        return rollover;
    }
}