#include "JobSystem.h"
#include <intrin.h> // for windowss
#include <chrono>

// dimitri ukov's lock free MPMC ring buffer q

LockFreeMPMCQueue::LockFreeMPMCQueue()
{
    for (size_t i = 0; i < QUEUE_SIZE; ++i)
    {
        buffer_[i].sequence.store(i, std::memory_order_relaxed);
    }
}

bool LockFreeMPMCQueue::Push(const Job &job)
{
    Node *node;
    size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
    while (true)
    {
        node = &buffer_[pos & BUFFER_MASK];
        size_t seq = node->sequence.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

        if (diff == 0)
        {
            if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                break;
        }
        else if (diff < 0)
            return false;
        else
            pos = enqueue_pos_.load(std::memory_order_relaxed);
    }

    node->job = job;
    node->sequence.store(pos + 1, std::memory_order_release);
    return true;
}

bool LockFreeMPMCQueue::Pop(Job &job)
{
    Node *node;
    size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
    while (true)
    {
        node = &buffer_[pos & BUFFER_MASK];
        size_t seq = node->sequence.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

        if (diff == 0)
        {
            if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                break;
        }
        else if (diff < 0)
            return false;
        else
            pos = dequeue_pos_.load(std::memory_order_relaxed);
    }

    job = node->job;
    node->sequence.store(pos + BUFFER_MASK + 1, std::memory_order_release);
    return true;
}

size_t LockFreeMPMCQueue::ApproximateDepth() const
{
    size_t eq = enqueue_pos_.load(std::memory_order_relaxed);
    size_t dq = dequeue_pos_.load(std::memory_order_relaxed);
    return eq > dq ? (eq - dq) : 0;
}

// normal mutex protected q

bool MutexJobQueue::Push(const Job &job)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= MAX_SIZE)
        return false;
    queue_.push_back(job);
    return true;
}

bool MutexJobQueue::Pop(Job &job)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
        return false;
    job = queue_.back();
    queue_.pop_back();
    return true;
}

size_t MutexJobQueue::Depth() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

// main job system / task scheduler

JobSystem &JobSystem::Get()
{
    static JobSystem instance;
    return instance;
}

void JobSystem::Initialize()
{
    size_t numCores = std::thread::hardware_concurrency();
    size_t numWorkers = numCores > 1 ? numCores - 1 : 1;
    shutdown_.store(false);

    for (size_t i = 0; i < numWorkers; ++i)
    {
        workers_.emplace_back(&JobSystem::WorkerLoop, this, i);
    }
}

void JobSystem::Shutdown()
{
    shutdown_.store(true);
    wakeCond_.notify_all();
    for (auto &thread : workers_)
    {
        if (thread.joinable())
            thread.join();
    }
    workers_.clear();
}

void JobSystem::WorkerLoop(size_t workerId)
{
    Job job;
    while (!shutdown_.load(std::memory_order_relaxed))
    {
        bool foundJob = useLockFree_.load(std::memory_order_relaxed) ? lockFreeQueue_.Pop(job) : mutexQueue_.Pop(job);

        if (foundJob)
        {
            job.work();
            if (job.completionCounter)
                job.completionCounter->fetch_sub(1, std::memory_order_release);
            totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            // spinin
            for (int spin = 0; spin < 1000; ++spin)
            {
                _mm_pause();
                foundJob = useLockFree_.load(std::memory_order_relaxed) ? lockFreeQueue_.Pop(job) : mutexQueue_.Pop(job);
                if (foundJob)
                    break;
            }

            if (foundJob)
            {
                job.work();
                if (job.completionCounter)
                    job.completionCounter->fetch_sub(1, std::memory_order_release);
                totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            // yieldin
            std::this_thread::yield();
            foundJob = useLockFree_.load(std::memory_order_relaxed) ? lockFreeQueue_.Pop(job) : mutexQueue_.Pop(job);

            if (foundJob)
            {
                job.work();
                if (job.completionCounter)
                    job.completionCounter->fetch_sub(1, std::memory_order_release);
                totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            // sleepin
            if (shutdown_.load(std::memory_order_relaxed))
                break;
            std::unique_lock<std::mutex> lock(wakeMutex_);
            wakeCond_.wait_for(lock, std::chrono::milliseconds(2), [this, &job, &foundJob]()
                               {
                                   if (shutdown_.load(std::memory_order_relaxed)) return true;
                                   foundJob = useLockFree_.load(std::memory_order_relaxed) ? lockFreeQueue_.Pop(job) : mutexQueue_.Pop(job);
                                   return foundJob;
                               });

            if (foundJob)
            {
                job.work();
                if (job.completionCounter)
                    job.completionCounter->fetch_sub(1, std::memory_order_release);
                totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
}

void JobSystem::Submit(const Job &job)
{
    bool success = false;
    while (!success)
    {
        success = useLockFree_.load(std::memory_order_relaxed) ? lockFreeQueue_.Push(job) : mutexQueue_.Push(job);
        if (!success)
            _mm_pause();
    }
    wakeCond_.notify_one();
}

void JobSystem::Wait(std::atomic<int> *counter)
{
    if (!counter)
        return;
    Job helpJob;
    while (counter->load(std::memory_order_acquire) > 0)
    {
        bool executedLocal = useLockFree_.load(std::memory_order_relaxed) ? lockFreeQueue_.Pop(helpJob) : mutexQueue_.Pop(helpJob);
        if (executedLocal)
        {
            helpJob.work();
            if (helpJob.completionCounter)
                helpJob.completionCounter->fetch_sub(1, std::memory_order_release);
            totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            _mm_pause();
        }
    }
}

void JobSystem::ToggleQueueType(bool useLockFree) { useLockFree_.store(useLockFree); }
bool JobSystem::IsUsingLockFree() const { return useLockFree_.load(); }
size_t JobSystem::GetCurrentQueueDepth() const { return useLockFree_.load() ? lockFreeQueue_.ApproximateDepth() : mutexQueue_.Depth(); }
size_t JobSystem::GetTotalJobsExecuted() const { return totalJobsExecuted_.load(); }