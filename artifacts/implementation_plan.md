# Implementation Plan - FYNiX Job System & Engine Scheduler

This plan introduces a reusable, high-performance task scheduler (Job System) to the FYNiX engine. It replaces sequential updates for independent tasks (animations, particles) with parallel execution, supports asynchronous asset loading, and provides a runtime benchmarking tool to compare mutex-based vs. lock-free MPMC queue designs.

---

## User Review Required

> [!NOTE]
> **Queue Type Implementation Details**
> We will implement two queue types:
> 1. A standard `MutexQueue` using `std::mutex` and `std::condition_variable`.
> 2. A lock-free `LockFreeMPMCQueue` utilizing a fixed-size ring buffer with atomic sequence indices (based on Dmitri Vyukov's algorithm).
> A runtime toggle in ImGui will allow you to switch queues on the fly to compare latency and CPU overhead.
>
> **OpenGL Context Constraints**
> Since OpenGL calls (like creating textures, VAOs, VBOs) must be performed on the main thread, the asynchronous asset loader will process model files (Assimp imports) and parse textures (using `stbi_load`) on worker threads, then hand the data to the main thread to instantiate GPU resources.

---

## Proposed Changes

We will create and modify the following files:

### 1. Job System Core
#### [NEW] [JobSystem.h](file:///d:/Projects/fynx/include/JobSystem.h)
* Define the `Job` struct (functor + atomic completion counter).
* Implement the lock-free MPMC ring-buffer queue (`LockFreeMPMCQueue`) and the standard `MutexQueue`.
* Define the `JobSystem` interface, including `Initialize`, `Shutdown`, `Execute`, `Wait`, and `GetStats`.

#### [NEW] [JobSystem.cpp](file:///d:/Projects/fynx/src/JobSystem.cpp)
* Manage the thread pool of worker threads.
* Implement worker thread loops supporting both blocking (condition variable) and spinning/yielding wake modes.
* Implement stats tracking (jobs completed, queue depth, throughput, active threads).

---

### 2. Engine Integration
#### [MODIFY] [main.cpp](file:///d:/Projects/fynx/src/main.cpp)
* Initialize `JobSystem` at engine startup and shut it down on close.

#### [MODIFY] [Model.h](file:///d:/Projects/fynx/include/Model.h)
* Define CPU-side structures (`CPUModelData`, `CPUMeshData`, `CPUTextureData`) to support asynchronous asset parsing.
* Add a `Model` constructor that accepts `CPUModelData&&` for main-thread GPU instantiations.

#### [MODIFY] [Model.cpp](file:///d:/Projects/fynx/src/Model.cpp)
* Implement the GPU instantiation logic from `CPUModelData`.
* Expose a helper function/static method to parse file content into `CPUModelData` from worker threads.

#### [MODIFY] [Texture.h](file:///d:/Projects/fynx/include/Texture.h) & [Texture.cpp](file:///d:/Projects/fynx/src/Texture.cpp)
* Add a `Texture` constructor that takes `CPUTextureData&` to initialize OpenGL textures on the main thread.

#### [MODIFY] [SceneManager.h](file:///d:/Projects/fynx/include/SceneManager.h) & [SceneManager.cpp](file:///d:/Projects/fynx/src/SceneManager.cpp)
* Parallelize model animation updates (`UpdateAnimation`) using `JobSystem::Execute` and `JobSystem::Wait`.
* Parallelize particle system updates (`SpawnParticle` + `Update`) in the same manner.
* Add an asynchronous asset loading interface (`addToParentAsync`) and a queue of completed assets to spawn onto the GPU during the main loop.

---

### 3. Editor & Profiling GUI
#### [MODIFY] [GUI.h](file:///d:/Projects/fynx/include/GUI.h) & [GUIManager.cpp](file:///d:/Projects/fynx/src/GUIManager.cpp)
* Add a "Job System Profiler" window showing:
  * Worker thread utilization and active thread count.
  * Runtime toggle for queue type (Mutex vs. Lock-Free) and wake mode.
  * Real-time metrics (Queue depth, throughput, total jobs completed).
  * A "Run Benchmark" button to dispatch 20,000 dummy workloads and compare queue latency.
* Add a "Load Asynchronously" checkbox to the "Add New Node" modal.

---

## Verification Plan

### Automated/Build Verification
* Compile the engine via the VS Code "CMake Build" task (`cmake --build build`).
* Validate that no compiler errors exist in C++17.

### Manual Verification
1. **Parallel Execution**: Verify that multiple animations and particles update correctly without graphical glitches or crashes.
2. **Benchmark Tool**: Trigger the ImGui micro-benchmark and verify that the lock-free queue demonstrates lower execution times under high contention compared to the mutex queue.
3. **Async Loading**: Add a node (e.g., `assets/dancer.gltf`) with "Load Asynchronously" checked. Verify that the editor remains responsive (no freezing/stuttering) and the model renders once loading completes.
