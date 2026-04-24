# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Random Raypath is a C++23 real-time Vulkan renderer for visualizing multi-ray paths through heterogeneous environments (branched flow of light). The goal is maximum ray count on screen with low overhead and stable frame pacing. Ray tracing runs on multi-threaded CPU (planned: distributed network workers). No persistent storage — RAM and VRAM only.

## General

Everything will be visible from public repository, 
so don't leave personal or sensitive information in .md files.

Key constraints: C++23, `-fno-exceptions -fno-rtti`, use `glm::` types not `std` integer types.

User defined instruction are located in `.claude/instructions`.
`mcp_raypath` folder contains mcp_raypath source code.
Project has Obsidian folder for human notes and management `raypath_vault`, do not edit it.

## Available MCP usage

Do not compile or run the application directly, use mcp_raypath for application build and runtime test.

Git MCP is granted, Use Git only to get history.

Enable Human-In-The-Loop inside the CLI if you need clarification, questions or asked from prompt.

Enable WebSearch if you need updated documentation.