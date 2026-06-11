# Core Concurrency Concepts: FYNiX Job System

This document outlines the high-performance C++ concurrency patterns and algorithms implemented in the FYNiX engine's task scheduler.

---

## 1. Dmitry Vyukov's Lock-Free MPMC Ring Buffer Queue

A Multi-Producer, Multi-Consumer (MPMC) queue allows multiple threads to push and pop items concurrently. Vyukov's algorithm uses a fixed-size array where each slot (Node) has a sequence number (`std::atomic<size_t>`).

### How It Works:
* **The Ring Buffer**: The buffer size $N$ must be a power of two. This allows map-to-index using a fast bitwise AND: `pos & (N - 1)`.
* **The Sequence Field**: Each slot starts with its sequence number set to its array index `i`.
* **Enqueue (`Push`)**:
  1. A thread reads the current `enqueue_pos` and gets the node at `enqueue_pos & (N - 1)`.
  2. It reads the node's `sequence`.
  3. If `sequence == enqueue_pos`, the slot is empty. The thread tries to reserve the slot by atomically incrementing `enqueue_pos` (using `compare_exchange_weak`).
  4. If successful, it writes the job data and updates the node's `sequence` to `enqueue_pos + 1` (releasing ownership so consumers know data is ready).
  5. If `sequence < enqueue_pos`, the queue is full (backpressure).
* **Dequeue (`Pop`)**:
  1. A thread reads the current `dequeue_pos` and gets the node.
  2. If `sequence == dequeue_pos + 1`, the slot has data. The thread attempts to increment `dequeue_pos`.
  3. If successful, it copies the job and sets the node's `sequence` to `dequeue_pos + N` (marking the slot as empty for future producers).
  4. If `sequence < dequeue_pos + 1`, the queue is empty.

### Why It Is Fast (Lock-Free):
No thread blocks or locks an OS mutex to modify the queue. Instead, threads cooperatively claim slots using atomic Compare-And-Swap (CAS) instructions.

---

## 2. False Sharing and Cache Line Alignment

When multiple processors modify independent variables that reside on the same cache line, the CPU cache controller is forced to refresh the entire cache line across cores. This is called **False Sharing** and ruins multithreaded scalability.

### Mitigation:
We use `alignas(64)` on the critical components of the lock-free queue:
```cpp
alignas(64) Node buffer_[QUEUE_SIZE];
alignas(64) std::atomic<size_t> enqueue_pos_{0};
alignas(64) std::atomic<size_t> dequeue_pos_{0};
```
This ensures the `enqueue_pos` and `dequeue_pos` variables reside on separate L1 cache lines (typically 64 bytes on modern x86/ARM CPUs), preventing cache invalidation ping-pong.

---

## 3. Hybrid Spin-Yield-Sleep Coordination

OS context switches (putting a thread to sleep and waking it up) take 2–5 microseconds. For micro-jobs, this context switch is slower than the job itself.

The scheduler uses a **three-tier fallback strategy**:
1. **Spinning**: Active loop checking the queue for new jobs using `_mm_pause()` (tells the CPU it is in a spin-wait loop, saving power and avoiding pipeline flushes).
2. **Yielding**: If no job is found after spinning, call `std::this_thread::yield()` to let other threads on the system run.
3. **Sleeping**: Only when inactive for a longer period does the thread block on a condition variable (`std::condition_variable::wait_for`).

---

## 4. Main-Thread Helping Execution (Cooperative Scheduling)

If the main thread spawns jobs and block-waits on them, a CPU core sits completely idle. Furthermore, if all worker threads are busy or blocked, this can cause deadlocks.

To solve this, `JobSystem::Wait()` implements **help-execution**:
```cpp
void JobSystem::Wait(std::atomic<int> *counter) {
    while (counter->load() > 0) {
        Job helpJob;
        if (lockFreeQueue_.Pop(helpJob)) {
            helpJob.work(); // Do work while waiting!
            ...
        }
    }
}
```
While waiting, the main thread actively pops jobs from the queue and executes them, acting as an additional worker thread.

---

## 5. Zero-Heap-Allocation Job Dependencies

Typical job systems track dependencies using reference-counted pointers like `std::shared_ptr<std::atomic<int>>`. This forces a heap allocation (`new`) and atomic increments/decrements for the reference counts.

In a high-performance system, we allocate the atomic counter **on the stack** of the thread that spawns the jobs:
```cpp
void DispatchWork() {
    std::atomic<int> counter{10}; // Allocated on the stack!
    for (int i = 0; i < 10; ++i) {
        Job job;
        job.completionCounter = &counter; // Pass raw pointer
        job.work = [](){ ... };
        JobSystem::Get().Submit(job);
    }
    JobSystem::Get().Wait(&counter); // Wait block prevents stack frame from popping
}
```
Because `Wait()` blocks until the counter reaches zero, the stack frame is guaranteed to remain valid, eliminating dangling pointer risks and avoiding all heap allocations.
