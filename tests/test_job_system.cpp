#include "JobSystem.h"
#include <iostream>
#include <atomic>
#include <cassert>
#include <chrono>
#include <vector>

void TestSingleJob()
{
    std::cout << "[TEST] Running Single Job Test..." << std::endl;
    std::atomic<int> counter{1};
    
    Job job;
    job.completionCounter = &counter;
    job.work = []() {
        std::cout << "  Job executed on thread: " << std::this_thread::get_id() << std::endl;
    };
    
    JobSystem::Get().Submit(job);
    JobSystem::Get().Wait(&counter);
    
    assert(counter.load() == 0 && "Counter should be 0 after job execution!");
    std::cout << "[PASS] Single Job Test passed!" << std::endl;
}

void TestStressCounter()
{
    std::cout << "[TEST] Running Stress Counter Test (100k jobs)..." << std::endl;
    constexpr int NUM_JOBS = 100000;
    std::atomic<int> completionCounter{NUM_JOBS};
    std::atomic<int> sharedCounter{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_JOBS; ++i)
    {
        Job job;
        job.completionCounter = &completionCounter;
        job.work = [&sharedCounter]() {
            sharedCounter.fetch_add(1, std::memory_order_relaxed);
        };
        JobSystem::Get().Submit(job);
    }
    
    // Main thread helps execute tasks while waiting
    JobSystem::Get().Wait(&completionCounter);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    assert(completionCounter.load() == 0 && "Completion counter must be 0!");
    assert(sharedCounter.load() == NUM_JOBS && "Shared counter value mismatch!");
    
    std::cout << "[PASS] Stress Counter Test passed!" << std::endl;
    std::cout << "  Processed " << NUM_JOBS << " jobs in " << elapsed.count() << " ms." << std::endl;
}

void TestParallelFor()
{
    std::cout << "[TEST] Running ParallelFor Test..." << std::endl;
    constexpr int NUM_ELEMENTS = 10000;
    std::vector<int> data(NUM_ELEMENTS, 0);
    std::atomic<int> counter{0};
    
    JobSystem::Get().ParallelFor(0, NUM_ELEMENTS, 1000, [&data](int idx) {
        data[idx] = idx * 2;
    }, &counter);
    
    JobSystem::Get().Wait(&counter);
    
    for (int i = 0; i < NUM_ELEMENTS; ++i)
    {
        assert(data[i] == i * 2 && "ParallelFor data index value mismatch!");
    }
    
    std::cout << "[PASS] ParallelFor Test passed!" << std::endl;
}

int main()
{
    std::cout << "=== Job System Standalone Tests ===" << std::endl;
    
    JobSystem::Get().Initialize();
    
    // 1. Run under lock-free MPMC queue
    std::cout << "\n--- Testing with Lock-Free Queue ---" << std::endl;
    JobSystem::Get().ToggleQueueType(true);
    TestSingleJob();
    TestStressCounter();
    TestParallelFor();
    
    // 2. Run under Mutex-protected queue
    std::cout << "\n--- Testing with Mutex Queue ---" << std::endl;
    JobSystem::Get().ToggleQueueType(false);
    TestSingleJob();
    TestStressCounter();
    TestParallelFor();
    
    JobSystem::Get().Shutdown();
    
    std::cout << "\n=== All Tests Passed Successfully! ===" << std::endl;
    return 0;
}
