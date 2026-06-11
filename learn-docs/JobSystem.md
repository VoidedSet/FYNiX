1. Dmitri Vyukov's Lock-Free MPMC Ring Buffer
A standard thread pool uses a single queue guarded by a std::mutex. When 8 or 16 CPU cores try to push or pop jobs simultaneously, they slam into that mutex. This creates lock contention. Threads are forced into an OS-level sleep state, costing thousands of clock cycles to wake up via context switches.

Vyukov's bounded Multi-Producer Multi-Consumer (MPMC) queue avoids this by using a fixed-size array of slots, where each slot contains an atomic sequence number (std::atomic<size_t>) along with the data payload.

Enqueueing: A producer atomics-increments the enqueue_pos_. It then checks if the slot at enqueue_pos_ % buffer_mask has a sequence number equal to enqueue_pos_. If it matches, the producer claims the slot, updates the payload, and stamps the sequence to enqueue_pos_ + 1. If the slot's sequence is less, it means the queue is full, causing the producer to hit backpressure and spin-wait.

Dequeueing: Consumers increment a matching dequeue_pos_ and validate that the sequence number equals dequeue_pos_ + 1.

This layout ensures memory modifications are highly localized and synchronized purely through hardware-level cache-coherency protocols (like MESI) without invoking kernel transitions.

2. Cache Line Padding & False Sharing
In multi-core processing, if two separate variables (e.g., enqueue_pos_ and dequeue_pos_) reside on the same 64-byte chunk of memory (a cache line), a core modifying enqueue_pos_ will invalidate the entire cache line for other cores reading dequeue_pos_. This performance killer is called False Sharing. We eliminate it entirely using alignas(64) or std::hardware_destructive_interference_size to force variables onto completely separate cache lines.

3. Stack-Allocated Atomics for Cache Locality
Instead of tracking a group of parallel tasks via a heap-allocated std::shared_ptr<std::atomic<int>> (which forces pointer indirection, cache misses, and reference-counting atomic thrashing), our system implements a structural dependency layout using raw pointers to stack-allocated counters.
The dispatching thread declares an atomic counter right on its local stack frame, passes its address to the workers, and reads it directly from its L1/L2 cache lines while waiting.

4. Spin-Yield-Sleep Backoff Strategy
An idle worker thread spinning continuously at 100% CPU utilization burns power and blocks hyper-threaded companion cores. Sleeping instantly on a condition variable induces an unacceptably high wake-up latency penalty. We use a hybrid backoff strategy to achieve the best of both worlds:

Spin Phase: Loop for ~1000 iterations executing the _mm_pause() intrinsic. This hints to the CPU that it's in a spin-loop, optimizing pipeline flushes and drastically saving power.

Yield Phase: If the queue is still empty, execute std::this_thread::yield(), giving up the current OS time-slice to other ready threads.

Sleep Phase: Fall back to a low-frequency condition variable sleep to stop wasting cycles if the engine is idle.