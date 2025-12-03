# FYNiX: Framework for Yet to be Named eXperiences!

FYNiX is a lightweight, custom game engine built in C++ using OpenGL.  
Originally started as a 10-day challenge, it's now evolving into a long-term learning project and a powerful game development toolkit.

> ⚠️ Follow this repo at your own risk since it under active development, constant refactoring, and lots of trial-and-error.

---

## 🛠️ Tech Stack

- C++17
- OpenGL
- GLFW – windowing & input
- GLAD – OpenGL loader
- GLM – math library
- Assimp – model & animation loader
- stb_image – texture loading
- Dear ImGui – editor GUI
- Planned: Bullet physics
- Planned: miniaudio or OpenAL for 3D audio

---

## ✅ Core Features (Implemented)

- Load 3D models with materials (textures & color fallback)
- Load and play skeletal animations (basic skinning + playback)
- Scene graph and entity management system
- ImGui-based editor GUI with transform manipulation
- Scene saving and loading (.fynx)
- Basic lighting: Phong point lights
- Animation controls via GUI (switch, loop, etc.)
- Particle system (custom emitters)

---

## 🔜 In Progress / Upcoming

- Advanced lighting (spotlight, falloff, shadows)
- Material system (metallic, specular, roughness, alpha, etc.)
- Skybox + environment lighting
- Physics system (rigidbodies, basic collision)
- 3D audio system with spatial sound

---

## 🎮 Planned Showcase Game

To demonstrate the engine's capabilities, a small game will be built using FYNiX. Shortlisted ideas:

- 🏎️ **F1-style racing demo**  
  - Focus on VFX (particles, motion blur)
  - Ambient sounds, skybox, reflections

- 🚓 **Tactical FPS breach demo**  
  - Character animations, particles, physics, gunfire, and lighting

---

## 🎯 Why FYNiX Exists

This isn't about reinventing Unity or Unreal. It's about **learning how things work under the hood**:  
Rendering, animation, asset pipelines, and real-time systems — from scratch.  
FYNiX is built to understand, experiment, and create.

---

## 🚧 Status

**Actively being developed.**  
Check commits for progress logs, refactors, and subsystem overhauls.

---

## License

TBD — currently private/dev only.
