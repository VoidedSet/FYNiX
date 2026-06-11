#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <intrin.h>

struct Job
{
    std::function<void()> work;
    std::atomic<int> *completionCounter = nullptr;
};

// dimitri ukovs lock free mp mc queue

class LockFreeMPMCQueue
{
private:
    struct Node
    {
        std::atomic<size_t> sequnce;
        Job job;
    };

    static constexpr size_t QUEUE_SIZE = 65536;
    static constexpr size_t BUFFER_MASK = QUEUE_SIZE - 1;

    alignas(64) Node buffer_[QUEUE_SIZE];
    alignas(64) std::atomic<size_t> enqueue_pos_{0};
    alignas(64) std::atomic<size_t> dequeue_pos_{0};

public:
    LockFreeMPMCQueue()
    {
        for (size_t i = 0; i < QUEUE_SIZE; ++i)
        {
            buffer_[i].sequnce.store(i, std::memory_order_relaxed);
        }
    }

    bool Push(const Job &job)
    {
        Node *node;
        size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        while (true)
        {
            node = &buffer_[pos & BUFFER_MASK];
            size_t seq = node->sequnce.load((std::memory_order_acquire));
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
        node->sequnce.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool Pop(Job &job)
    {
        Node *node;
        size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        while (true)
        {
            node = &buffer_[pos & BUFFER_MASK];
            size_t seq = node->sequnce.load(std::memory_order_acquire);
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
        node->sequnce.store(pos + BUFFER_MASK + 1, std::memory_order_release);
        return true;
    }

    size_t ApproximateDepth() const
    {
        size_t eq = enqueue_pos_.load(std::memory_order_relaxed);
        size_t dq = dequeue_pos_.load(std::memory_order_relaxed);
        return eq > dq ? (eq - dq) : 0;
    }
};

// normal mutex protected queue

class MutexJobQueue
{
private:
    std::vector<Job> queue_;
    mutable std::mutex mutex_;
    static constexpr size_t MAX_SIZE = 65536;

public:
    bool Push(const Job &job)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= MAX_SIZE)
            return false;

        queue_.push_back(job);
        return true;
    }

    bool Pop(Job &job)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
            return false;

        job = queue_.back();
        queue_.pop_back();
        return true;
    }

    size_t Depth() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

class JobSystem
{
private:
    std::vector<std::thread> workers_;
    LockFreeMPMCQueue lockFreeQueue_;
    MutexJobQueue mutexQueue_;

    std::atomic<bool> shutdown_{false}, useLockFree_{true};

    std::mutex wakeMutex_;
    std::condition_variable wakeCond_;

    std::atomic<size_t> totalJobsExecuted_{0};

    void WorkerLoop(size_t workerId)
    {
        Job job;

        while (!shutdown_.load(std::memory_order_relaxed))
        {
            bool foundJob = false;

            if (useLockFree_.load(std::memory_order_relaxed))
                foundJob = lockFreeQueue_.Pop(job);
            else
                foundJob = mutexQueue_.Pop(job);

            if (foundJob)
            {
                job.work();
                if (job.completionCounter)
                    job.completionCounter->fetch_sub(1, std::memory_order_release);

                totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                // spinnin
                for (int spin = 0; spin < 1000; ++spin)
                {
                    _mm_pause();

                    if (useLockFree_.load(std::memory_order_release))
                    {
                        if (lockFreeQueue_.Pop(job))
                        {
                            foundJob = true;
                            break;
                        }
                        else
                        {
                            if (mutexQueue_.Pop(job))
                            {
                                foundJob = true;
                                break;
                            }
                        }
                    }
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

                if (useLockFree_.load(std::memory_order_relaxed))
                    foundJob = lockFreeQueue_.Pop(job);
                else
                    foundJob = mutexQueue_.Pop(job);

                if (foundJob)
                {
                    job.work();
                    if (job.completionCounter)
                        job.completionCounter->fetch_sub(1, std::memory_order_release);
                    continue;
                }

                // sleep
                if (shutdown_.load(std::memory_order_relaxed))
                    break;
                std::unique_lock<std::mutex> lock(wakeMutex_);
                wakeCond_.wait_for(lock, std::chrono::milliseconds(2), [this, &job, &foundJob]()
                                   {
                    if (shutdown_.load(std::memory_order_relaxed)) return true;
                    if (useLockFree_.load(std::memory_order_relaxed)){
                        return lockFreeQueue_.Pop(job) ? foundJob = true : false;
                    } else {
                        return mutexQueue_.Pop(job) ? (foundJob = true) : false;
                    } });

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

public:
    static JobSystem &Get()
    {
        static JobSystem instance;
        return instance;
    }

    void Initialize()
    {
        size_t numCores = std::thread::hardware_concurrency();
        size_t numWorkers = numCores > 1 ? numCores - 1 : 1;
        shutdown_.store(false);

        for (size_t i = 0; i < numWorkers; ++i)
        {
            workers_.emplace_back(&JobSystem::WorkerLoop, this, i);
        }
    }

    void Shutdown()
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

    void Submit(const Job &job)
    {
        bool success = false;
        while (!success)
        {
            if (useLockFree_.load(std::memory_order_relaxed))
                success = lockFreeQueue_.Push(job);
            else
                success = mutexQueue_.Push(job);

            if (!success)
                // Queue full backpressure strategy -> spin to wait for free slots
                _mm_pause();
        }

        wakeCond_.notify_one();
    }

    // Help-Execution Wait Strategy: popping items locally instead of stalling
    void Wait(std::atomic<int> *counter)
    {
        if (!counter)
            return;

        Job helpJob;
        while (counter->load(std::memory_order_acquire) > 0)
        {
            // Actively pop work to prevent deadlocks and speed up execution
            bool executedLocal = false;
            if (useLockFree_.load(std::memory_order_relaxed))
                executedLocal = lockFreeQueue_.Pop(helpJob);
            else
                executedLocal = mutexQueue_.Pop(helpJob);

            if (executedLocal)
            {
                helpJob.work();
                if (helpJob.completionCounter)
                {
                    helpJob.completionCounter->fetch_sub(1, std::memory_order_release);
                }
                totalJobsExecuted_.fetch_add(1, std::memory_order_relaxed);
            }
            else
                // If there are no more jobs in the system, spin intensely until the counter drops
                _mm_pause();
        }
    }

    void ToggleQueueType(bool useLockFree)
    {
        useLockFree_.store(useLockFree);
    }

    bool IsUsingLockFree() const { return useLockFree_.load(); }
    size_t GetCurrentQueueDepth() const
    {
        return useLockFree_.load() ? lockFreeQueue_.ApproximateDepth() : mutexQueue_.Depth();
    }
    size_t
    GetTotalJobsExecuted() const { return totalJobsExecuted_.load(); }
};
