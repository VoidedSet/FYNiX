#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

struct Job
{
    std::function<void()> work;
    std::atomic<int> *completionCounter = nullptr;
};

// dimitri ukov's lock free MPMC ring buffer q
class LockFreeMPMCQueue
{
private:
    struct Node
    {
        std::atomic<size_t> sequence;
        Job job;
    };

    static constexpr size_t QUEUE_SIZE = 65536; // must always be raised to 2
    static constexpr size_t BUFFER_MASK = QUEUE_SIZE - 1;

    alignas(64) Node buffer_[QUEUE_SIZE];
    alignas(64) std::atomic<size_t> enqueue_pos_{0};
    alignas(64) std::atomic<size_t> dequeue_pos_{0};

public:
    LockFreeMPMCQueue();
    bool Push(const Job &job);
    bool Pop(Job &job);
    size_t ApproximateDepth() const;
};

// normal mutex protected q
class MutexJobQueue
{
private:
    std::vector<Job> queue_;
    mutable std::mutex mutex_;
    static constexpr size_t MAX_SIZE = 65536;

public:
    bool Push(const Job &job);
    bool Pop(Job &job);
    size_t Depth() const;
};

class JobSystem
{
private:
    std::vector<std::thread> workers_;
    LockFreeMPMCQueue lockFreeQueue_;
    MutexJobQueue mutexQueue_;

    std::atomic<bool> shutdown_{false};
    std::atomic<bool> useLockFree_{true};

    std::mutex wakeMutex_;
    std::condition_variable wakeCond_;

    std::atomic<size_t> totalJobsExecuted_{0};

    void WorkerLoop(size_t workerId);
    JobSystem() = default;

public:
    static JobSystem &Get();

    JobSystem(const JobSystem &) = delete;
    JobSystem &operator=(const JobSystem &) = delete;

    void Initialize();
    void Shutdown();
    void Submit(const Job &job);
    void Wait(std::atomic<int> *counter);
    void ToggleQueueType(bool useLockFree);

    template <typename Index, typename Callable>
    void ParallelFor(Index start, Index end, size_t batchSize, const Callable &func, std::atomic<int> *counter)
    {
        if (start >= end)
            return;

        size_t totalElements = static_cast<size_t>(end - start);
        size_t numBatches = (totalElements + batchSize - 1) / batchSize;

        if (counter)
        {
            counter->fetch_add(static_cast<int>(numBatches), std::memory_order_relaxed);
        }

        for (size_t i = 0; i < numBatches; ++i)
        {
            Index batchStart = start + static_cast<Index>(i * batchSize);
            Index batchEnd = (start + static_cast<Index>((i + 1) * batchSize) < end) ? (start + static_cast<Index>((i + 1) * batchSize)) : end;

            Job job;
            job.completionCounter = counter;
            job.work = [batchStart, batchEnd, func]()
            {
                for (Index idx = batchStart; idx < batchEnd; ++idx)
                {
                    func(idx);
                }
            };
            Submit(job);
        }
    }

    bool IsUsingLockFree() const;
    size_t GetCurrentQueueDepth() const;
    size_t GetTotalJobsExecuted() const;
};