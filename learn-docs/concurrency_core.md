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

---

## 6. ParallelFor Data Parallelism and Batch Chunking

When simulating systems with thousands of items (like particles), creating one job per item introduces massive scheduler overhead. Instead, we chunk the workload into contiguous batches.

### Implementation:
`JobSystem::ParallelFor` divides a range `[start, end)` into chunks of size `batchSize`:
```cpp
template <typename Index, typename Callable>
void ParallelFor(Index start, Index end, size_t batchSize, const Callable &func, std::atomic<int> *counter)
{
    size_t numBatches = (totalElements + batchSize - 1) / batchSize;
    counter->fetch_add(numBatches); // Increment completion counter by number of batches

    for (size_t i = 0; i < numBatches; ++i) {
        // Submit one Job per batch, executing the lambda over that index range
        Job job;
        job.work = [batchStart, batchEnd, func]() {
            for (Index idx = batchStart; idx < batchEnd; ++idx) {
                func(idx);
            }
        };
        Submit(job);
    }
}
```

### Why Batch Chunking is Critical for High-Frequency Systems:
* **Saves CPU Cycles**: By grouping e.g. 1024 particle updates into a single job, we make only 1 queue push/pop operation instead of 1024.
* **Cache Friendliness**: Processing consecutive elements in memory on the same thread maximizes L1/L2 data cache hits.

---

## 7. Decoupling Thread-Safe Calculations from Single-Threaded API Contexts

Many rendering/graphics APIs (like OpenGL) require that all draw calls, texture generations, and buffer creations occur on the thread that created the OpenGL context (usually the main thread).

### The Parallel Update / Serial Render Pattern:
To parallelize systems that end up calling OpenGL commands, we must split the update process into two distinct phases:
1. **The Parallel Update Phase**:
   - Worker threads perform pure CPU-bound mathematical operations (e.g. skinning matrix calculations, skeletal animation bone traversals) concurrently.
   - The main thread coordinates this phase by dispatching the update jobs and waiting for them to complete using cooperative help-execution.
2. **The Serial Render Phase**:
   - Once all calculations are complete and synchronized, the main thread sequentially issues OpenGL draw calls and uploads buffers.

This decoupling allows us to maximize multi-core CPU utilization for game loop updates while fully respecting the single-threaded rendering constraint of OpenGL.

---

## 8. Asynchronous Asset Loading & Resource Deferral

Loading heavy assets (3D meshes, hi-res textures) synchronously halts the main loop, causing the application/game to freeze. Asynchronous asset loading solves this, but presents synchronization challenges.

### 1. The Two-Stage Loading Pattern:
We refactored `Texture`, `Mesh`, and `Model` to separate CPU disk/memory work from GPU allocation:
* **Stage 1 (Async CPU Load)**:
  - Disk operations (reading files, Assimp glTF/OBJ parsing).
  - CPU image decompression (`stbi_load`).
  - Storing vertices, indices, and raw pixel pointers in RAM.
  - This phase runs on a background worker thread.
* **Stage 2 (Sync GPU Upload)**:
  - Generating and binding VAO, VBO, EBO.
  - Uploading pixel buffers to OpenGL (`glTexImage2D`, `glGenerateMipmap`).
  - Freeing CPU RAM (`stbi_image_free`).
  - This phase runs on the main thread inside the render loop.

### 2. Preventing Texture Duplication:
When meshes share the same textures (e.g. standard materials), copying texture objects can lead to duplicate uploads if we call `UploadToGPU` on each copy.
We resolved this by:
1. Maintaining a list of unique `textures_loaded` inside the loaded model.
2. Uploading unique textures to the GPU first.
3. Updating all mesh texture references to use the newly generated OpenGL texture IDs.
4. Uploading mesh geometry (VBO/EBO) afterward.



