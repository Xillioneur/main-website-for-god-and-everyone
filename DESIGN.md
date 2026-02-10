# The Divine Code - Design Document

### 1. Project Vision & Core Purpose

**One-liner**  
A sacred playground where developers can upload, compile, and run **C/C++ games** (built with **Raylib** and/or **SDL2**) directly in the browser — powered by **WebAssembly** — with zero installation, zero servers, and maximum creative velocity.

**Spiritual & philosophical framing**  
- “The Divine Code” — code as a manifestation of universal order  
- “All things were made through Him, and without Him was not anything made that was made.” (John 1:3)  
- Technology as a way to explore atomic / divine structure of creation  
- Tone: reverent yet hacker-spirited, mystical yet technical

### 2. Current Technical Stack
- **Frontend**: Next.js / React (Vite)
- **Backend**: Node.js / Express (Vercel Functions)
- **Compiler**: Emscripten (emcc)
- **Graphics**: Raylib / SDL2 via WebAssembly

### 3. Roadmap – Phased Development

**Phase 0 – Foundation (COMPLETE)**
- [x] Stabilize single-file upload & compile
- [x] Show clear compile errors & warnings
- [x] Fix canvas resize / DPI bugs
- [x] Basic theme toggle

**Phase 1 – Usable Playground (COMPLETE)**
- [x] Monaco Editor integration (for Raylib syntax help)
- [x] File tree view for multi-file projects
- [x] Drag & drop folder / multiple files support
- [x] Basic asset support (.png, .wav, .ttf) mapped to `/resources/`
- [x] "New project from template" (5-8 templates)
- [x] "Share" button for project snapshots
- [x] Basic settings: window size, target FPS, optimization level

**Phase 2 – Power User Features (COMPLETE)**
- [x] Version history / snapshots (Sacred Snapshots)
- [x] Live reload (debounced auto-compile)
- [x] Export options (WASM bundle)
- [ ] Gamepad + touch input mapper UI

**Phase 3 – Ecosystem & Sacred Layer (IN PROGRESS)**
- [ ] User accounts (GitHub/Google)
- [ ] Public gallery / discover page
- [ ] Sacred geometry shader library
- [ ] Collaborative editing (multiplayer cursors)

**Phase 4 – Long-term Vision**
- [ ] Native package manager for Raylib extras
- [ ] WebGPU backend experiments
- [ ] Offline-first support (PWA)
- [ ] AI code assistant integration
