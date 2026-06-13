# FYNiX Engine — Full Audit Report
> Deep review of every source file in the engine. Organized by system, graded by severity: 🔴 Bug | 🟠 Caveat | 🟡 Improvement | 🟢 Polish

---

## 1. Scene Graph & Transform Hierarchy

> **Note:** Items in this section cross-cut with the "No parent propagation" root cause described under rendering/architecture.


### 🔴 Parent-child transform propagation is completely absent
The `Node` struct stores a scene tree (parent/child pointers), but **no system ever walks that tree to propagate transforms**. Moving a parent node (Model, Light, RigidBody) does **not** move any of its children. Each object (`Model`, `Light`, `ParticleEmitter`, `btRigidBody`) stores its own position independently and `getModelMatrix()` is computed purely from the Model's own `position/rotation/scale` fields with no awareness of its parent.

**Root cause:** `RenderModels()` just calls `model.getModelMatrix()` for every model flat — it never looks at what node that model belongs to, nor does it accumulate a parent transform chain.

**Fix direction:** Each frame, walk the node tree top-down accumulating a `parentWorldMatrix`, and multiply each object's local matrix against it before sending it to the GPU.

---

### 🔴 Node ID lookup is O(n) linear search across a flat vector
`find_node(id)` iterates the entire `nodes` vector every call. It's called dozens of times per frame in the GUI inspector and during load. A `std::unordered_map<unsigned int, Node*>` would make this O(1).

---

### 🔴 `find_node` has a dead guard that never fires
```cpp
if (id < 0 || id >= nextID)  // id is unsigned int — "id < 0" is always false
```
Negative IDs can never be caught. The guard is useless for catching invalid IDs passed as large unsigned values.

---

### 🟠 `deleteNode` reparents children to the deleted node's parent but doesn't delete them
When a parent is deleted the children are re-linked upward — correct — but if those children have associated objects (models, lights, etc.) their data continues to be rendered. There is no recursive delete option.

---

### 🟠 Deleting a `Particles` node accesses `nodeToDelete->children[0]` after already clearing children
In `deleteNode`, the code first moves children to the parent (including calling `nodeToDelete->children.clear()`), then later checks `if (!nodeToDelete->children.empty())` to delete the particle emitter's light child. By then children are empty so the sub-light never gets deleted — it stays in the `lights` array forever (memory/render leak).

---

### 🟠 `root` node is allocated in the header with `new Node(...)` at class definition time
`Node *root = new Node({...})` is inline in the class definition. If `SceneManager` is ever copy-constructed or moved (e.g., when stored by value), the raw pointer will alias or leak. `LoadScene` also does `delete node` for all old nodes, which would `delete` the original `root`, but then reassigns `root` to a `new Node` — fine now, but fragile.

---

### 🟡 No concept of "world transform" vs "local transform" on nodes
Models, Lights, and RigidBodies all store their position in world space directly. There is no local-space vs world-space distinction. This makes parent-child relationships essentially decorative at this point.

---

### 🟡 `nextID` is not reused after deletions
Deleting nodes leaves gaps in the ID space. Over a long editing session `nextID` grows without bound. More practically, the save/load logic re-uses serialized IDs, so after a scene reload `nextID` is correctly set via `std::max`, but this is fragile if future code relies on IDs being dense.

---

## 2. Physics System

### 🔴 RigidBodies are completely disconnected from Mesh/Light/any other node
A `RigidBody` node is a standalone Bullet physics object. It is **never linked to any Model or Light**. Dropping a physics box on a mesh does nothing — the mesh won't move with the physics simulation, and the physics box visual (the debug wireframe) sits independently. There is no way to "attach" a physics body to a renderable.

**Fix direction:** When creating a RigidBody node that is a sibling/child of a Model node, store the pairing and write back the Bullet transform to the Model's `position/rotation` every frame inside `RenderPhysics` or a dedicated sync step.

---

### 🔴 Only `CUBE` shape is implemented despite `SPHERE` and `CAPSULE` being in the enum
```cpp
// PhysicsEngine.h
enum class RigidBodyShape { CUBE, CAPSULE, SPHERE };
```
In `addToParent` for RigidBody:
```cpp
if (shape == RigidBodyShape::CUBE)
    body = physics->createBoxRigidBody(...);
// No else — SPHERE and CAPSULE just create no body at all!
```
Selecting SPHERE or CAPSULE in the UI results in a `nullptr` body silently stored in `rigidBodies`, which later causes `getRigidBodyByID` to dereference null in `InspectRigidBodyNode`.

---

### 🔴 The "Add Node" modal always creates CUBE rigid bodies regardless of UI selection
The GUI `DrawAddNodeModal` always passes `RigidBodyShape::CUBE` when creating a RigidBody — the shape dropdown doesn't exist in the UI at all. There is no way to create a sphere or capsule body from the editor.

---

### 🔴 Physics simulation runs on a background thread while the main thread reads body transforms
`RenderModels` submits `physics->update(deltaTime)` as a job to the `JobSystem`, then proceeds to render. `RenderPhysics` calls `JobSystem::Get().Wait(&physicsCounter)` before drawing debug wireframes — but `RenderModels` (which renders actual scene objects and reads nothing from physics) runs in between, so that's fine. However, the GUI inspector in the same frame (`InspectRigidBodyNode`) reads `body->getMotionState()->getWorldTransform()` **without** waiting for the physics job to finish. This is a **data race** on Bullet's internal state.

---

### 🟠 `createBoxRigidBody` always spawns the box at `position = (1,1,1)` ignoring the UI
In `addToParent(name, type, parentID, shape, mass)`, the call is:
```cpp
body = physics->createBoxRigidBody(glm::vec3(1.f), glm::vec3(1.f), mass);
```
The first argument is the spawn position — hardcoded to `(1,1,1)`. The node has no position at that point so there is no source of a sensible default, but the size is also hardcoded. The user has no way to set spawn position or size from the "Add Node" dialog.

---

### 🟠 Scale applied via `setLocalScaling` doesn't recompute inertia
In `InspectRigidBodyNode`, when the user drags the Scale slider:
```cpp
body->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
scene->physics->getDynamicsWorld()->updateSingleAabb(body);
```
`calculateLocalInertia` is not called again after scaling, so the physics body's rotational inertia becomes incorrect for its new visual size.

---

### 🟠 `setGravity(int gravity)` takes an `int` but gravity is a float concept
The `PhysicsEngine::setGravity(int gravity)` signature is typed `int` — any fractional gravity value is truncated. There is no GUI exposure of this setter anyway, so it's dead code.

---

### 🟡 `RigidBodyShape::SPHERE` needs `btSphereShape`, `CAPSULE` needs `btCapsuleShape`
These shapes have fundamentally different debug meshes too — the debug renderer unconditionally draws a box wireframe for all shapes.

---

### 🟡 No static ground plane / infinite floor
Without a ground body, all physics objects fall forever. There should be an automatic (or easily-addable) static plane at `y = 0`.

---

### 🟡 Physics `update` timestep is raw `deltaTime` with no fixed-step clamping
If the game frame takes a long time (e.g., during an asset load spike), a huge deltaTime is fed into `stepSimulation`. Bullet's `maxSubSteps = 10` provides some safety, but large spikes can still cause tunnel-through and instability. Standard practice is to cap deltaTime to ~33ms before passing it to physics.

---

## 3. Rendering & Shaders

### 🔴 Projection matrix computed with integer division — always 45° FOV with broken aspect
```cpp
// main.cpp, setup phase (before main loop)
projection = glm::perspective(glm::radians(45.f),
    (float)(windowManager.mode->width / windowManager.mode->height), 0.1f, 100.f);
```
`windowManager.mode->width / windowManager.mode->height` is **integer division** — always `1` for any landscape window (e.g. 1920/1080 = 1). This sets a 45° square FOV. The correct version is computed inside the main loop with `(float)renderWidth / (float)renderHeight` — but the initial setup path is wrong and is applied before the loop, which is then fortunately overwritten each frame.

---

### 🔴 Normal matrix recomputed in the fragment shader every frame, on every fragment
```glsl
// model/vertex.glsl
Normal = mat3(transpose(inverse(model))) * mat3(skinningTransform) * aNormal;
```
`transpose(inverse(model))` is an extremely expensive matrix inverse computed **per vertex in the GPU**. This should be precomputed on the CPU and passed as a `uniform mat3 normalMatrix`.

---

### 🟠 The fragment shader ignores light type entirely
The `LightType` enum has `DIRECTIONAL`, `POINTLIGHT`, `SPOT`, `SUN` — but the shader treats every light as a point light with no attenuation. Directional lights should not compute `lightPositions[i] - FragPos`, they should use the position vector as a direction. There is no concept of attenuation, inner/outer cone for spots, or sun direction.

---

### 🟠 The max light cap (16) is hardcoded in GLSL but not enforced in C++
```glsl
uniform vec3 lightPositions[16];
```
If more than 16 lights are in the scene the C++ code will write out-of-bounds uniform data. There's no check in `RenderModels` capping `lightCount` to 16.

---

### 🟠 The specular channel (`texture_specular0`) is sampled and bound but never used in the fragment shader
`loadMaterialTextures` loads specular maps, they are uploaded to the GPU, and `Mesh::Draw` binds them — but the fragment shader never reads `texture_specular0`. The specular term is a hardcoded `0.5` multiplied only by the light color, not the texture.

---

### 🟠 `glDepthMask(GL_FALSE)` is not set before rendering transparent particles
Additive-blended particles should write to the color buffer but not the depth buffer, otherwise particles sort incorrectly against each other. Currently they write depth, causing near particles to occlude distant ones.

---

### 🟠 Blend state is left enabled if `activeParticles > 0` and then something else throws
`glEnable(GL_BLEND)` / `glDisable(GL_BLEND)` wraps only the instanced draw call, but if an exception or early return happens between them the blend state leaks into subsequent draw calls. RAII blend state management would be safer.

---

### 🟠 Light shader uses `uLightPos` as a uniform but never sets it
The light vertex shader declares `uniform vec3 uLightPos;` but this uniform is never set anywhere in C++ code. It's also not actually used in `gl_Position` calculation — the model matrix already encodes position. Dead uniform.

---

### 🟡 The `model/vertex.glsl` skinning path: when `isAnimated = false`, `skinningTransform = mat4(1.0)` is correct, but the bone transforms array (200 mat4s = 12.8KB) is still uploaded every frame even for non-animated models
The uniform block upload in `Model::Draw` iterates `finalBoneMatrices` only when `hasAnimation` is true — so the data isn't uploaded. But the uniform location is still queried, which is fine. However, the shader declares `uniform mat4 bone_transforms[200]` statically regardless, which consumes uniform buffer space even for models that will never animate.

---

### ~~🟡 `glViewport(0, 250, renderWidth, renderHeight)` — the Y-origin is hardcoded to 250px~~
> ⏭️ **Skipped** — low risk as long as `CONSOLE_HEIGHT` constant doesn't move. Fix if you ever resize the panel.

The viewport is offset by 250px from the bottom to account for the console panel, but this value is hardcoded and doesn't respond to the `CONSOLE_HEIGHT` constant (which is actually 180px in `DrawConsolePanel`). If the console height changes this breaks.

---

### ~~🟡 Camera speed is hardcoded to `20.0f * deltaTime` — no UI control, no sprint key~~
> ⏭️ **Skipped** — not a bug, just a missing UX nicety for an internal tool. Add when the editor becomes more user-facing.

Movement speed cannot be adjusted in the editor or at runtime. There's no shift-to-sprint.

---

### ~~🟡 Cube `Mesh(MeshType::CUBE)` uses a non-indexed vertex buffer with 36 raw vertices~~
> ⏭️ **Skipped** — pedantic. Wastes ~144 bytes of VRAM; not worth touching unless you're overhauling the primitive mesh path.

The cube mesh used for lights and physics debug has `cubeVert[]` with 36 raw vertices (no EBO) but also has an EBO initialized with `cubeIndices[36]`. The `Draw` function checks `if (indices.size() > 1)` — since only `indices.push_back(0)` is called in the `MeshType` constructor, `indices.size() == 1` and it always falls into `glDrawArrays(GL_TRIANGLES, 0, 36)`. The EBO is wasted. Either use indexed drawing properly or remove the EBO from the primitive mesh path.

---

## 3b. OpenGL State Bugs

### 🔴 `VertexArray::UnBind()` unbinds the array buffer, not the VAO
```cpp
void VertexArray::UnBind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // WRONG — should be glBindVertexArray(0)
}
```
After calling `VAO.UnBind()` the VAO is **still bound**. Every mesh draw call that calls `VAO.UnBind()` followed by `VBO.UnBind()` / `EBO.UnBind()` leaves the VAO active, potentially corrupting subsequent draw state.

---

### 🔴 `glEnable(GL_MULTISAMPLE)` is never called
`glfwWindowHint(GLFW_SAMPLES, 4)` is set in Window.cpp, requesting 4x MSAA. But `glEnable(GL_MULTISAMPLE)` is never called anywhere. MSAA is requested from GLFW but never activated in OpenGL — all rendering is effectively unanti-aliased.

---

### 🟠 Texture bind/unbind slot mismatch
In `Mesh::Draw()`, textures are bound to unit `i` (loop index) via `textures[i].Bind(i)`, but `SetUniform` sends `textureUnit` — the value stored at texture construction time. After async model loading the construction-time unit and the draw-time loop index may diverge.

---

### 🟡 Uniform locations are re-queried every `setUniforms()` call
`glGetUniformLocation()` is called inside every `Shader::setUniforms()` invocation — once per bone (up to 200), once per light, once for model matrix, etc. This is a driver round-trip per call. Caching locations in a `std::unordered_map<std::string, int>` after first query would eliminate the overhead.

---

### 🟡 `std::string` heap allocations inside the hot render path
Bone uniform names (`"bone_transforms[0]"` … `"bone_transforms[199]"`) and light uniform names (`"lightPositions[0]"` etc.) are constructed with `+` string concatenation every frame. Pre-computing these strings or using `snprintf` into stack buffers would eliminate per-frame allocations.

---

## 4. Scene Save / Load

### 🔴 Particle emitter shader name is saved with a typo and loaded with the correct key — always fails
```cpp
// saveScene:
j["shdaerName"] = it->shader.Name;  // typo: "shdaerName"

// LoadScene:
if (j.contains("shaderName"))       // reads "shaderName" — never matches!
    shaderName = j["shaderName"];
```
Every particle emitter saves its shader name under `"shdaerName"` but tries to load it from `"shaderName"`. On load, `shaderName` is always empty string, so the particle emitter is recreated without a valid shader.

---

### 🔴 Scene load assigns IDs based on insertion order, not the saved IDs
`addToParent` increments `nextID` and assigns it, regardless of what `id` was in the JSON. This means loaded nodes get fresh sequential IDs rather than their original ones. The recursive `buildNodeRecursive` then calls `find_node(id)` — the original saved `id` — to get the parent for children, which will find the wrong node or fail entirely if the scene has non-trivial structure.

---

### 🔴 Loaded `Light` type is always hardcoded to `DIRECTIONAL` regardless of saved type
```cpp
addToParent(name, type, parent->ID, LightType::DIRECTIONAL);
```
Light type is not saved to JSON, and on load it's always created as directional. Point lights and spot lights become directional after a save-load cycle.

---

### 🟠 `saveScene` saves the model path as `it->directory` — which is the full absolute path
If the project is moved to a different machine or a different folder, all model paths break. Paths should be stored relative to the project file.

---

### 🟠 `LoadScene` does not clear `particleEmitters` before loading
```cpp
// LoadScene cleanup:
models.clear();
lights.clear();
// particleEmitters is NOT cleared!
```
Doing `LoadScene` on a scene that has particle emitters loaded will append new emitters to the existing ones.

---

### 🟠 `LoadScene` does not reset `selectedNodeID` in the GUI
After loading a new scene, the GUI may still have a stale `selectedNodeID` pointing at a node that no longer exists. The next frame `find_node` returns null, but the inspector just shows "Selected node not found" rather than cleanly resetting.

---

### 🟡 There is no "Load Scene" button in the GUI
`saveScene()` is exposed via a "Save Scene" button. Loading is only done at startup. There is no way to reload or open a different scene from within the running engine.

---

### ~~🟡 `saveScene` writes directly to the project file with no backup~~
> ⏭️ **Skipped** — standard game-dev practice to skip until near-shipping. Add atomic rename when the project file format stabilises.

If the process crashes mid-write, the project file is corrupted. Writing to a `.tmp` file and renaming atomically is safer.

---

### 🔴 No `deleteShader()` implementation exists
`ShaderManager::deleteShader()` is declared in the header but has no implementation anywhere in the codebase.

---

### 🟠 `ShaderManager` uses `std::map` (ordered) instead of `std::unordered_map`
Shader lookup is O(log n) when O(1) is trivially achievable.

---

## 5. Animation System

### 🟠 `readSkeleton` only finds the first bone chain — silently discards bones in parallel hierarchies
The function recurses into children but returns `true` as soon as one bone is found, and stops recursing siblings. For skeletons with multiple root bones (e.g., a character with separate root bones for left/right hand IK targets), this drops all but the first chain.

---

### 🟠 `boneCount` is set per-mesh but `finalBoneMatrices` is sized from the first mesh's boneCount
If a model has multiple meshes with different numbers of bones, `skeleton.boneCount` gets overwritten by each `processMesh` call. The final count may be wrong for earlier meshes.

---

### 🟠 Bone weight normalization is not enforced
In `processMesh`, bone weights are copied directly from Assimp. If the sum of 4 weights isn't 1.0, the skinned mesh will deform incorrectly. Assimp guarantees normalized weights for standard formats, but this assumption is implicit and not checked.

---

### 🟠 Animation time wraps with `fmod` which can produce 0.0 exactly at loop boundary
```cpp
if (currentTime > anim.duration)
    currentTime = fmod(currentTime, anim.duration);
```
If `currentTime == anim.duration` exactly (from a seek operation), this correctly stays at `duration`. But `fmod` can produce floating-point artifacts near zero that cause a one-frame pose glitch at loop point. A `currentTime = 0.0f` explicit reset is cleaner.

---

### 🟠 Animation `seek` slider in the GUI conflicts with `updateAnimation` running on a background thread
The GUI directly writes `animator.currentTime` via `seek()` while the JobSystem may be concurrently calling `model.UpdateAnimation(deltaTime)` which also modifies `currentTime`. This is a data race. Animation updates should be fenced or the seek should flag for next-frame application.

---

### 🟡 No animation blend/crossfade — switching animations is a hard cut
`setAnimation(index)` immediately resets `currentTime` to 0 and switches. Blending from the current pose to the new animation over N frames would be much smoother.

---

### ~~🟡 `animationNames` and `animations` vectors are kept in sync manually — brittle~~
> ⏭️ **Skipped** — low actual risk; they're always pushed together at load time. Worth a struct wrap eventually, but nothing is broken.

`setAnimation(index)` immediately resets `currentTime` to 0 and switches. Blending from the current pose to the new animation over N frames would be much smoother.

---

## 6. Job System & Threading

### 🔴 `ToggleQueueType` mid-frame is unsafe — jobs can be lost
When `ToggleQueueType` is called while workers are running and both queues have jobs, swapping the active queue means workers switch from consuming one queue to the other. Jobs already in the old queue are abandoned until the queue type is toggled back. The benchmark deliberately toggles back, but an accidental mid-frame toggle (e.g., from the UI) would silently drop pending work.

---

### 🟠 `Submit` busy-spins forever if both queues are full
```cpp
while (!success) {
    success = ... Push(job);
    if (!success) _mm_pause();
}
```
The lock-free queue's `Push` returns `false` when full (65536 slots). `Submit` then **busy-spins indefinitely** on the main thread. For the ring buffer at full capacity this is a hard hang.

---

### 🟠 `MutexJobQueue::Pop` uses `pop_back` (LIFO order) while `LockFreeMPMCQueue` is FIFO
The two queues have different scheduling orders. When switching queue types, job execution order changes, which can affect animation update timing (animations submitted as jobs would now execute in reverse submission order under the mutex queue).

---

### 🟠 `ParallelFor` counter is pre-incremented before jobs are submitted
```cpp
counter->fetch_add(numBatches);  // added first
// then jobs are submitted
Submit(job);
```
If the main thread calls `Wait(counter)` between the `fetch_add` and the first `Submit`, the counter is non-zero but no jobs exist yet, so `Wait` spins but finds nothing to execute. This corrects itself as jobs arrive but burns CPU unnecessarily. The counter should be set before calling `Wait` but the jobs should be submitted first, or the counter shouldn't be touched until all jobs are queued.

---

### 🟠 Animation jobs capture `model` by reference while the `models` vector may reallocate
In `RenderModels`:
```cpp
job.work = [&model, deltaTime]() { model.UpdateAnimation(deltaTime); };
```
`model` is a reference to an element of `scene.models` (a `std::vector<Model>`). If any code triggers a `models.push_back` on the main thread **while the jobs are running** (e.g., via `UpdateAsyncLoads`), the vector may reallocate, dangling the reference. `UpdateAsyncLoads` is called earlier in the same frame, so this particular sequence is currently safe — but it's a fragile coupling.

---

### ~~🟡 No thread names assigned to workers~~
> ⏭️ **Skipped** — only matters when profiling. Add when you're doing a threading pass.

Worker threads have no names, making them indistinguishable in profilers/debuggers.

---

### 🟡 Benchmark runs synchronously on the main thread, blocking the frame for ~200ms+
The "Run Micro-Benchmark" button executes `ExecuteMicroBenchmark` twice sequentially while the engine is rendering — the entire editor freezes for the duration.

---

### 🟠 `seek()` time unit mismatch
`currentTime` accumulates in **ticks** (`+= deltaTime * ticksPerSecond`), but the GUI seek slider is labeled `0.0f` to `currentAnim->duration` which is also in ticks (raw Assimp `mDuration`). The slider label implies seconds but both are in ticks — inconsistent expectations could confuse future devs or break if units are ever corrected.

---

### ~~🟡 Hardcoded 200-bone limit in shader with no CPU-side validation~~
> ⏭️ **Skipped** — not firing on current assets. Add a `assert(bones <= 200)` guard if you ever load high-bone-count rigs.

```glsl
uniform mat4 bone_transforms[200];
```
If a model has > 200 bones, the CPU happily uploads past the array, causing GPU undefined behavior. No check exists.

---

## 7. Particle System

### 🔴 `ParticleEmitter::Update` uses `ParallelFor` to mutate particles, but `Draw` reads particles on the main thread immediately after — potential data race
```cpp
// SceneManager::RenderParticles
emitter.Update(dt);  // dispatches jobs, waits — OK
emitter.Draw();      // reads particles on main thread — OK because Wait() was called
```
Actually `Wait` is called inside `Update` so this specific call is safe. But `SpawnParticle` is called on the main thread **before** `Update`, during the same frame:
```cpp
for (int i = 0; i < 10; i++) emitter.SpawnParticle(...);  // main thread
emitter.Update(dt);                                         // submits jobs
```
If `Update`'s batch jobs read `particles[idx]` while `SpawnParticle` writes to `particles[firstUnusedParticle()]`... they could alias to the same index simultaneously. The pool index is returned from a linear scan without any synchronization.

---

### 🟠 10 particles are spawned every single frame regardless of emitter position or state
```cpp
for (int i = 0; i < 10; i++) { ... emitter.SpawnParticle(newParticle); }
```
The spawn rate is hardcoded. There's no concept of emission rate (particles/second), burst mode, or emitter enabled/disabled state.

---

### 🟠 Particle `Position` is always `(0, 0, 0)` relative to emitter with no shape/volume spread
```cpp
newParticle.Position = glm::vec3(0.0f, 0.0f, 0.0f);
```
All particles spawn at the exact emitter origin. There's no sphere/box spawn volume, no randomized spawn offset.

---

### 🟠 Particle `VBO` is leaked — `unsigned int VBO` in `init()` is a local that goes out of scope
```cpp
unsigned int VBO;
glGenBuffers(1, &VBO);
// ... VBO goes out of scope. Never stored. Never freed.
```
The quad geometry VBO is allocated on the GPU but the handle is lost. This is a permanent GPU memory leak per emitter.

---

### 🟠 `ParticleEmitter` has no destructor — `VAO`, `instanceVBO` are never freed
When a `ParticleEmitter` is destroyed (e.g., via `deleteNode`), `glDeleteBuffers` and `glDeleteVertexArrays` are never called. Every destroyed emitter leaks GPU resources.

---

### ~~🟡 Particle `Life` initial value in `Particle()` constructor is `0.0f`~~
> ⏭️ **Skipped** — the pool works correctly as-is. `0.0f` doubles as the "unused" sentinel and the pool scan finds it immediately. Only change if the pool logic is refactored.

```cpp
Particle() : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) {}
```
All pool particles start with `Life = 0`, meaning `firstUnusedParticle()` will always immediately find slot 0 as "unused." This is intentional for the pool, but it means a freshly constructed particle is already dead — `Life(0.0f)` should arguably be `-1.0f` or a sentinel to distinguish "never spawned" from "just expired."

---

### 🟡 Particle size is hardcoded to `0.05f` in `RenderParticles`
The `Particle.Size` field exists and is uploaded to the shader, but the spawn code always sets `Size = 0.05f`. There's no GUI control for particle size range.

---

## 7b. Rendering — Fragment Shader Correctness

### 🔴 Meshes with zero lights render completely black — no ambient floor
```glsl
vec3 result = vec3(0.0);
for (int i = 0; i < numLights; ++i) { ... }
FragColor = vec4(result * texColor.rgb, texColor.a);
```
If `numLights == 0`, `result` is `vec3(0)` and the output is solid black regardless of texture. Any model visible before adding a light is invisible. A minimum global ambient (`vec3(0.05)` for example) should exist outside the loop.

---

## 8. Memory Management

### 🔴 `SceneManager::nodes` stores raw pointers — the `root` node is pushed into `nodes` in the constructor but `root` itself is also a separate member pointer
```cpp
// constructor
nodes.push_back(root);
```
`root` is a raw `Node*` member and also stored in `nodes`. `LoadScene` calls `delete node` for every node in `nodes`, which deletes `root`, then sets `root = new Node{...}`. But if `LoadScene` is called multiple times (it could be), the old `root` inside `nodes` would be deleted, then `nodes` is cleared — safe. However `SceneManager` has no destructor, so when it goes out of scope, none of the `Node*` in `nodes` are freed.

---

### 🔴 `SceneManager` has no destructor — all heap-allocated `Node*` objects leak at shutdown
There is no `~SceneManager()`. At program exit, all `new Node(...)` allocations are implicitly freed by the OS but this is bad practice and will show as leaks in Valgrind/sanitizers.

---

### 🟠 `Model` copies in `std::vector<Model>` are expensive — each copy deep-copies meshes, textures, animator
`scene.models` is a `std::vector<Model>`. When the vector grows (via `push_back`), all existing `Model` objects are **move-constructed**, but `Model` has no explicit move constructor defined. The compiler-generated one may move the vectors inside, but the VBOs/VAOs (handled by `VertexBuffer`/`VertexArray` wrappers) also have no explicit move constructors — if they hold raw GLuints, the moved-from object will call `glDeleteBuffers` on the same handle, corrupting GPU state.

---

### 🟠 `Texture` is copied freely but holds GPU handles — double-free potential
`textures_loaded` stores `Texture` by value. `Texture` objects are copied into `mesh.textures`. If `Texture` has a destructor that calls `glDeleteTextures`, every copy-destruction frees the same GPU handle. This is a classic GPU handle dangling/double-free pattern.

---

### 🟠 `Model` default position/rotation are `vec3(1,1,1)` not identity
```cpp
glm::vec3 position = glm::vec3(1.f),
          rotation = glm::vec3(1.f),
          scale    = glm::vec3(1.f);
```
Every new model spawns at `(1, 1, 1)` instead of `(0, 0, 0)`, and with `rotation = (1, 1, 1)` meaning 1 radian (~57°) on all axes. Models load visually offset and tilted until the user manually resets the transform. Should be `position = vec3(0)`, `rotation = vec3(0)`, `scale = vec3(1)`.

---

## 9. GUI / Editor

### 🟠 "Add Node" modal has no shape selector for RigidBody — always creates CUBE
As noted in Physics section. The `DrawAddNodeModal` has a `rigidBodyMass` field but no shape selector — there is a `RigidBodyShape` enum but it's never surfaced in UI.

---

### 🟠 The `parentNodeId` in "Add Node" modal defaults to 0 and is never validated before submission
If the user enters a parent ID that doesn't exist, `addToParent` logs an error and returns — but the modal closes and the user gets no visual feedback that the node wasn't created. The UI should prevent submission if the parent ID is invalid.

---

### 🟠 The "Node Inspector" for Light has no rotation control, no type display, and no attenuation
`InspectLightNode` only exposes `Position` and `Color`. Light type is not shown, cannot be changed after creation. No range/intensity/falloff controls.

---

### 🟠 `inputHandler` re-registers GLFW callbacks every frame
```cpp
void inputHandler(GLFWwindow *window, float deltaTime, Camera &camera)
{
    glfwSetCursorPosCallback(window, mouse_callback);     // every frame!
    glfwSetMouseButtonCallback(...);
    ...
}
```
These `glfwSet*Callback` calls happen every frame. They should be set once at startup. This is harmless (GLFW just overwrites the same pointer) but wasteful and confusing.

---

### 🟠 The console panel reads from `g_consoleBuffer.lines` under a lock, but `sync()` is called by `std::cout` from background threads holding the same lock
The `ImGuiConsoleBuffer::sync()` acquires `mutex`, as does `DrawConsolePanel`. This is correct. However, `g_originalCoutBuf` and `g_originalCerrBuf` are set once at startup — if `RedirectOutputToConsoleBuffer()` is called before a background thread starts writing, the thread will also write to the custom buffer (correct). But if threads write after `RestoreOutput()` (e.g., during `Shutdown`), they write to the restored cout which may cause a race with the original streambuf. Low risk at exit but technically UB.

---

### 🟡 Clicking a node in the hierarchy prints to console every single click
```cpp
std::cout << "Selected Node ID: " << selectedNodeID << std::endl;
```
Debug print left in production flow. Noisy in the console.

---

### 🟡 The hierarchy panel has a fixed height of 250px with no resize handle
The inspector sub-regions (`Hierarchy` child = 250px, `InspectorChild` fills remainder) are fixed. A large scene with many nodes forces scrolling in a tiny box.

---

### 🟡 No drag-and-drop reparenting in the hierarchy
Nodes can only be parented at creation time. Changing a node's parent after the fact requires deleting and recreating it.

---

### 🟡 The "Add Node" modal doesn't reset its fields between invocations
If you add a model at path "foo.gltf", close, and reopen the modal, the path field still shows "foo.gltf". The `nodeNameInput`, `modelPathInput` etc. are static arrays that persist.

---

### 🟠 `ImGui::PopStyleVar()` called after `ImGui::End()` in `DrawSidePanel`
```cpp
ImGui::End();
ImGui::PopStyleVar();  // should be BEFORE End()
```
This is incorrect ImGui usage. `PopStyleVar` must be paired within the Begin/End scope. While current ImGui versions may tolerate this, it's technically wrong and could break in future versions.

---

### 🟠 Console buffer lock held during full ImGui render pass
`DrawConsolePanel` acquires `g_consoleBuffer.mutex` then loops over all lines calling `ImGui::TextWrapped`. Background threads that try to log anything during this time will block on the same mutex for the entire duration of the console draw. Should copy lines into a local vector first, release the lock, then render.

---

## 10. Architecture & Design Caveats

### 🟠 All objects (Model, Light, ParticleEmitter, btRigidBody) are stored in separate flat vectors, looked up by ID — O(n) per lookup
The scene's data storage is fully decoupled from the scene graph. Every inspector render calls `getModelByID`, `getLightByID`, etc., each of which is O(n). Replacing with hash maps keyed on node ID would be straightforward.

---

### 🟠 `ShaderManager::findShader` returns a `Shader` by value — copies the entire struct every call
`findShader` returns `Shader` by value. If `Shader` holds any GPU handles, this creates aliasing issues as noted in the texture section. Should return `Shader*` or `Shader&`.

---

### 🟠 `Light.h` includes `Mesh.h`, `Mesh.h` includes everything — heavy include chain in a header
Every file that includes `Light.h` (which is included from `SceneManager.h`, which is included almost everywhere) transitively pulls in all of OpenGL, GLFW, GLM, and the buffer objects. This slows compilation significantly and creates tight coupling.

---

### 🟠 No `Camera` node type in the scene graph
The camera is a free-floating object in `main.cpp` with a global pointer. It can't be serialized, can't be attached to a node, and can't be part of the scene hierarchy. There's no way to have a scripted camera path or a node-attached camera.

---

### 🟡 `Window.cpp` uses a hardcoded `glfwGetPrimaryMonitor()` — forces fullscreen on primary monitor
The window is created in fullscreen mode using the primary monitor's current video mode with no option for windowed mode, resolution selection, or multi-monitor support.

---

### 🟡 No material system — every model uses the same Phong shading with diffuse+specular
There is no `Material` concept. There are no PBR inputs (metallic, roughness, AO, normal maps). All models render identically except for their diffuse texture. The specular map is loaded but unused in the shader.

---

### 🟡 `Model::physicsEnabled` field exists but is never read or written after declaration
```cpp
bool physicsEnabled = false;  // Model.h
```
Dead field — likely a placeholder for linking rigidbodies to models.

---

### 🟡 No undo/redo system in the editor
Any destructive operation (delete node, move, rename) is irreversible without reloading the saved scene.

---

### 🟡 No scene "dirty" flag — "Save Scene" is always available even if nothing changed
There's no tracking of whether the scene has been modified since last save. The save button is always active and clicking it overwrites the file even with no changes.

---

### 🟠 `Shader::checkCompileErrors` uses pointer comparison on `const char*`
```cpp
if (type == "Shader")    // compares pointers, not string content
if (type == "Program")
```
This works only because string literals from the same translation unit are typically interned by the compiler. It's technically non-portable UB. Should use `strcmp`.

---

## 11. Quick-Fix Summary Table

| # | Issue | Severity | Estimated Effort |
|---|-------|----------|-----------------|
| 1 | Parent-child transform propagation | 🔴 Bug | Medium |
| 2 | Physics ↔ Mesh disconnection | 🔴 Bug | Medium |
| 3 | Sphere/Capsule shapes unimplemented | 🔴 Bug | Low |
| 4 | Particle shader name save/load typo | 🔴 Bug | Trivial |
| 5 | Scene load ID mismatch | 🔴 Bug | Medium |
| 6 | Light type not saved/loaded | 🔴 Bug | Low |
| 7 | Normal matrix GPU inverse | 🔴 Perf | Low |
| 8 | GUI data race on physics body | 🔴 Bug | Low |
| 9 | `particleEmitters` not cleared on load | 🔴 Bug | Trivial |
| 10 | Particle quad VBO leaked | 🔴 Leak | Trivial |
| 11 | `ParticleEmitter` no destructor | 🔴 Leak | Low |
| 12 | `SceneManager` no destructor | 🔴 Leak | Low |
| 13 | `inputHandler` sets callbacks every frame | 🟠 Waste | Trivial |
| 14 | Debug print on node select | 🟡 Polish | Trivial |
| 15 | `find_node` O(n) linear search | 🟡 Perf | Low |
| 16 | No `Load Scene` button in GUI | 🟡 Feature | Low |
| 17 | No ground plane | 🟡 UX | Low |
| 18 | Viewport Y-offset hardcoded | 🟠 Bug | Trivial |
| 19 | `setGravity` takes int | 🟠 API | Trivial |
| 20 | Physics inertia not updated on scale | 🟠 Bug | Trivial |
