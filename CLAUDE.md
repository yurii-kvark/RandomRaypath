# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Random Raypath is a C++23 real-time Vulkan renderer for visualizing multi-ray paths through heterogeneous environments (branched flow of light). The goal is maximum ray count on screen with low overhead and stable frame pacing. Ray tracing runs on multi-threaded CPU (planned: distributed network workers). No persistent storage — RAM and VRAM only.

## Autocode Documentation

Everything will be visible from public repository, 
so don't leave personal or sensitive information in .md files.

All AI-assistant context is organized under `.claude/autocode_md/`:

- **[Build](autocode_md/instructions/build.md)** — build commands, CMake options, prerequisites
- **[Architecture](autocode_md/instructions/architecture.md)** — startup flow, layered structure, module responsibilities
- **[Conventions](autocode_md/instructions/conventions.md)** — coding style, error handling, compile flags, dependencies
- **[Memory](autocode_md/memory/MEMORY.md)** — persistent cross-session memory index
- **[Settings](autocode_md/.claude)** — project-level Claude Code settings

## Quick Reference

Do not compile or run the application directly. MCP servers will be granted for application test.

Use Git only to get history.

```bash
git submodule update --init --recursive
cmake -B cmake-build-debug -G Ninja -DRAY_DEBUG_NO_OPT=1
cmake --build cmake-build-debug
```

Key constraints: C++23, `-fno-exceptions -fno-rtti`, use `glm::` types not `std` integer types, wrap graphics code in `#if RAY_GRAPHICS_ENABLE`.