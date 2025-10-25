#ifndef __CIM_WORKER_HPP__
#define __CIM_WORKER_HPP__

#include "lock.hpp"
#include "singleton.hpp"
#include "macro.hpp"
#include "iomanager.hpp"

namespace CIM
{
    class WorkerGroup : Noncopyable, public std::enable_shared_from_this<WorkerGroup>
    {
    public:
        typedef std::shared_ptr<WorkerGroup> ptr;
        static WorkerGroup::ptr Create(uint32_t batch_size, Scheduler *s = Scheduler::GetThis())
        {
            return std::make_shared<WorkerGroup>(batch_size, s);
        }

        WorkerGroup(uint32_t batch_size, Scheduler *s = Scheduler::GetThis());
        ~WorkerGroup();

        void schedule(std::function<void()> cb, int thread = -1);
        void waitAll();

    private:
        void doWork(std::function<void()> cb);

    private:
        uint32_t m_batchSize;
        bool m_finish;
        Scheduler *m_scheduler;
        CoroutineSemaphore m_sem;
    };

    class WorkerManager
    {
    public:
        WorkerManager();
        void add(Scheduler::ptr s);
        Scheduler::ptr get(const std::string &name);
        IOManager::ptr getAsIOManager(const std::string &name);

        template <class FiberOrCb>
        void schedule(const std::string &name, FiberOrCb fc, int thread = -1)
        {
            auto s = get(name);
            if (s)
            {
                s->schedule(fc, thread);
            }
            else
            {
                static Logger::ptr s_logger = CIM_LOG_NAME("system");
                CIM_LOG_ERROR(s_logger) << "schedule name=" << name
                                          << " not exists";
            }
        }

        template <class Iter>
        void schedule(const std::string &name, Iter begin, Iter end)
        {
            auto s = get(name);
            if (s)
            {
                s->schedule(begin, end);
            }
            else
            {
                static Logger::ptr s_logger = CIM_LOG_NAME("system");
                CIM_LOG_ERROR(s_logger) << "schedule name=" << name
                                          << " not exists";
            }
        }

        bool init();
        bool init(const std::map<std::string, std::map<std::string, std::string>> &v);
        void stop();

        bool isStoped() const { return m_stop; }
        std::ostream &dump(std::ostream &os);

        uint32_t getCount();

    private:
        std::map<std::string, std::vector<Scheduler::ptr>> m_datas;
        bool m_stop;
    };

    typedef Singleton<WorkerManager> WorkerMgr;

}

#endif