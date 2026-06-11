# FYNiX Engine — Audit & Fix Status

This document tracks all bugs, caveats, and improvements identified in the codebase audit and their current implementation status.

| # | System & Issue Title | Severity / Type | Status |
|---|----------------------|-----------------|--------|
| **1** | **Scene Graph & Transform Hierarchy** | | |
| 1.1 | Parent-child transform propagation is completely absent | 🔴 Bug | **Done** |
| 1.2 | Node ID lookup is O(n) linear search across a flat vector | 🔴 Bug | **Done** |
| 1.3 | `find_node` has a dead guard that never fires | 🔴 Bug | **Done** |
| 1.4 | `deleteNode` reparents children but doesn't delete their components | 🟠 Caveat | **Done** |
| 1.5 | Deleting a `Particles` node accesses children after clearing them (leak) | 🟠 Caveat | **Done** |
| 1.6 | `root` node is allocated in header at class definition time | 🟠 Caveat | **Done** |
| 1.7 | No concept of "world transform" vs "local transform" on nodes | 🟡 Improvement | **Done** |
| 1.8 | `nextID` is not reused after deletions (ID fragmentation) | 🟡 Improvement | Left to fix |
| **2** | **Physics System** | | |
| 2.1 | RigidBodies are completely disconnected from Mesh/Light/render nodes | 🔴 Bug | Left to fix |
| 2.2 | Only `CUBE` shape is implemented (SPHERE/CAPSULE fail silently) | 🔴 Bug | Left to fix |
| 2.3 | "Add Node" modal always creates CUBE rigid bodies | 🔴 Bug | Left to fix |
| 2.4 | Physics background simulation runs concurrently with main-thread GUI read (data race) | 🔴 Bug | Left to fix |
| 2.5 | `createBoxRigidBody` always spawns at hardcoded (1,1,1) size & pos | 🟠 Caveat | Left to fix |
| 2.6 | Scale applied via `setLocalScaling` doesn't recompute inertia | 🟠 Caveat | Left to fix |
| 2.7 | `setGravity(int)` truncates fractional float gravity values | 🟠 Caveat | Left to fix |
| 2.8 | `RigidBodyShape::SPHERE` and `CAPSULE` need shapes/debug drawing | 🟡 Improvement | Left to fix |
| 2.9 | No static ground plane / infinite floor | 🟡 Improvement | Left to fix |
| 2.10| Physics `update` timestep is raw `deltaTime` without fixed-step clamping | 🟡 Improvement | Left to fix |
| **3** | **Rendering & Shaders** | | |
| 3.1 | Projection matrix computed with integer division on initial setup | 🔴 Bug | Left to fix |
| 3.2 | Normal matrix computed per fragment in shader instead of CPU precomputed uniform | 🔴 Bug | Left to fix |
| 3.3 | Normal matrix calculation does not support scaling | 🟠 Caveat | Left to fix |
| 3.4 | Fragment shader ignores light type (treats all as point lights with no attenuation) | 🟠 Caveat | Left to fix |
| 3.5 | Max light cap (16) is hardcoded in GLSL but not checked/enforced in C++ | 🟠 Caveat | Left to fix |
| 3.6 | Specular map is loaded/bound but unused in fragment shader | 🟠 Caveat | Left to fix |
| 3.7 | `glDepthMask(GL_FALSE)` not set before transparent particles (sorting issues) | 🟠 Caveat | Left to fix |
| 3.8 | Blend state left enabled if `activeParticles > 0` and error thrown | 🟠 Caveat | Left to fix |
| 3.9 | Light shader uses `uLightPos` as uniform but never sets/uses it | 🟠 Caveat | Left to fix |
| 3.10| Bone transforms array uploaded every frame even for non-animated models | 🟡 Improvement | Left to fix |
| 3.11| Viewport Y-offset is hardcoded to 250px (doesn't follow console panel height constant) | 🟡 Improvement | Left to fix |
| 3.12| Camera speed is hardcoded with no UI adjustment or sprint key | 🟡 Improvement | Left to fix |
| 3.13| Cube mesh utilizes non-indexed buffer but also instantiates unused EBO | 🟡 Improvement | Left to fix |
| **4** | **OpenGL State Management** | | |
| 4.1 | `VertexArray::UnBind()` unbinds `GL_ARRAY_BUFFER` instead of VAO | 🔴 Bug | Left to fix |
| 4.2 | `glEnable(GL_MULTISAMPLE)` is never called (MSAA requested but inactive) | 🔴 Bug | Left to fix |
| 4.3 | Texture bind/unbind slot mismatch during async asset loads | 🟠 Caveat | Left to fix |
| 4.4 | Uniform locations are re-queried via `glGetUniformLocation` on every uniform set | 🟡 Improvement | Left to fix |
| 4.5 | `std::string` heap allocations occur inside hot render path loop | 🟡 Improvement | Left to fix |
| **5** | **Scene Save / Load** | | |
| 5.1 | Particle emitter shader name saved as `"shdaerName"` and loaded as `"shaderName"` | 🔴 Bug | **Done** |
| 5.2 | Scene load assigns IDs based on insertion order instead of saved JSON IDs | 🔴 Bug | **Done** |
| 5.3 | Loaded light type is hardcoded to `DIRECTIONAL` | 🔴 Bug | Left to fix |
| 5.4 | Absolute path saved for models (`it->directory`) instead of relative path | 🟠 Caveat | Left to fix |
| 5.5 | `LoadScene` does not clear `particleEmitters` before loading | 🟠 Caveat | Left to fix |
| 5.6 | `LoadScene` does not reset GUI `selectedNodeID` | 🟠 Caveat | Left to fix |
| 5.7 | No "Load Scene" button in GUI | 🟡 Improvement | Left to fix |
| 5.8 | `saveScene` writes directly to target file with no backup | 🟡 Improvement | Left to fix |
| 5.9 | `ShaderManager::deleteShader()` is declared but not implemented | 🔴 Bug | Left to fix |
| 5.10| `ShaderManager` uses ordered `std::map` instead of `std::unordered_map` | 🟠 Caveat | Left to fix |
| **6** | **Animation System** | | |
| 6.1 | `readSkeleton` discards sibling bone chains in parallel hierarchies | 🟠 Caveat | Left to fix |
| 6.2 | `boneCount` overwritten by each sub-mesh but `finalBoneMatrices` sized from first | 🟠 Caveat | Left to fix |
| 6.3 | Bone weight normalization is not validated or enforced | 🟠 Caveat | Left to fix |
| 6.4 | Animation time wraps with `fmod` which can cause pose glitches at loop boundary | 🟠 Caveat | Left to fix |
| 6.5 | Animation seek slider in GUI conflicts with background animation job | 🟠 Caveat | Left to fix |
| 6.6 | No animation blend or crossfade (switching is a hard cut) | 🟡 Improvement | Left to fix |
| 6.7 | `animationNames` and `animations` sync is manually managed and brittle | 🟡 Improvement | Left to fix |
| **7** | **Job System & Threading** | | |
| 7.1 | `ToggleQueueType` mid-frame is thread-unsafe and can drop jobs | 🔴 Bug | Left to fix |
| 7.2 | `Submit` busy-spins forever if queues are full (hard freeze risk) | 🟠 Caveat | Left to fix |
| 7.3 | mutex queue uses LIFO order while lock-free queue uses FIFO | 🟠 Caveat | Left to fix |
| 7.4 | `ParallelFor` counter pre-incremented before job submission | 🟠 Caveat | Left to fix |
| 7.5 | Animation jobs capture model by reference while vector might reallocate | 🟠 Caveat | Left to fix |
| 7.6 | Worker threads are not assigned debug names | 🟡 Improvement | Left to fix |
| 7.7 | Benchmark blocks main thread, freezing the screen | 🟡 Improvement | Left to fix |
| 7.8 | `seek()` time unit mismatch (seconds vs ticks) | 🟠 Caveat | Left to fix |
| 7.9 | Hardcoded 200-bone limit in shader with no CPU boundary validation | 🟡 Improvement | Left to fix |
| **8** | **Particle System** | | |
| 8.1 | `ParticleEmitter::Update` and `SpawnParticle` data race potential | 🔴 Bug | Left to fix |
| 8.2 | Spawn rate is hardcoded to 10 particles/frame (no configurable rate) | 🟠 Caveat | Left to fix |
| 8.3 | Particles spawn at exact origin (no volume/spread shape) | 🟠 Caveat | Left to fix |
| 8.4 | Quad geometry VBO leaked on emitter initialization | 🟠 Caveat | Left to fix |
| 8.5 | `ParticleEmitter` lacks destructor (VAO and VBO leaked on deletion) | 🟠 Caveat | Left to fix |
| 8.6 | Particles spawn dead due to `0.0f` initial life pool initialization | 🟡 Improvement | Left to fix |
| 8.7 | Particle size is hardcoded to `0.05f` at spawn | 🟡 Improvement | Left to fix |
| **9** | **Memory Management** | | |
| 9.1 | `SceneManager::nodes` stores raw pointers and leaks them at shutdown | 🔴 Bug | Left to fix |
| 9.2 | `SceneManager` has no destructor | 🔴 Bug | **Done** |
| 9.3 | `Model` copies in `std::vector` deep-copy raw GL handles, potential double free | 🟠 Caveat | Left to fix |
| 9.4 | `Texture` copy-constructor duplicates GL handles, potential double free | 🟠 Caveat | Left to fix |
| 9.5 | `Model` default position/rotation are `(1,1,1)` instead of identity | 🟠 Caveat | **Done** |
| **10**| **GUI / Editor** | | |
| 10.1| "Add Node" modal lacks shape selector for RigidBodies | 🟠 Caveat | Left to fix |
| 10.2| `parentNodeId` not validated before modal submission | 🟠 Caveat | Left to fix |
| 10.3| Node Inspector for Light has no rotation/type/attenuation control | 🟠 Caveat | Left to fix |
| 10.4| `inputHandler` registers GLFW callbacks every single frame | 🟠 Caveat | Left to fix |
| 10.5| Console panel lock held during full ImGui render loop pass | 🟠 Caveat | Left to fix |
| 10.6| Clicking hierarchy node prints debug text to console | 🟡 Improvement | Left to fix |
| 10.7| Hierarchy panel has fixed 250px height with no resize handle | 🟡 Improvement | Left to fix |
| 10.8| No drag-and-drop reparenting in the hierarchy | 🟡 Improvement | Left to fix |
| 10.9| "Add Node" modal fields do not reset between invocations | 🟡 Improvement | Left to fix |
| 10.10| `ImGui::PopStyleVar()` called after `ImGui::End()` in `DrawSidePanel` | 🟠 Caveat | Left to fix |
| **11**| **Architecture & Design** | | |
| 11.1| Component vectors are stored flat and looked up with O(n) scans | 🟠 Caveat | Left to fix |
| 11.2| `ShaderManager::findShader` returns `Shader` by value (struct copy) | 🟠 Caveat | Left to fix |
| 11.3| Heavy transitively chained includes inside `Light.h` and `Mesh.h` | 🟠 Caveat | Left to fix |
| 11.4| No `Camera` node type in scene graph (cannot be serialized) | 🟠 Caveat | Left to fix |
| 11.5| HardcodedFullscreen on primary monitor with no windowed mode option | 🟡 Improvement | Left to fix |
| 11.6| No material system (all assets use hardcoded Phong parameters) | 🟡 Improvement | Left to fix |
| 11.7| `Model::physicsEnabled` is a dead/unused field | 🟡 Improvement | Left to fix |
| 11.8| No undo/redo system in editor | 🟡 Improvement | Left to fix |
| 11.9| No scene "dirty" flag to prevent redundant saves | 🟡 Improvement | Left to fix |
| 11.10| `Shader::checkCompileErrors` compares pointers on literal strings | 🟠 Caveat | Left to fix |
