# Build Instructions

Prerequisites: MinGW64 (posix, v13+), CMake 4.1+, Vulkan SDK (for `glslangValidator`).

```bash
# First time: fetch submodule dependencies
git submodule update --init --recursive

# Configure + build (debug)
cmake -B cmake-build-debug -G Ninja -DRAY_DEBUG_NO_OPT=1 -DGLSLANG_VALIDATOR="{VulkanSDK}/Bin/glslangValidator.exe"
cmake --build cmake-build-debug

# Configure + build (release package)
cmake -B cmake-build-package -G Ninja -DRAY_DEBUG_NO_OPT=0 -DRAY_ENABLE_PACKAGING=1 -DGLSLANG_VALIDATOR="{VulkanSDK}/Bin/glslangValidator.exe"
cmake --build cmake-build-package
cpack --config cmake-build-package/CPackConfig.cmake
```

The executable is `RayClientRenderer`. It loads config from `../config/client_renderer.toml` relative to the working directory.

## CMake Options

| Option | Default | Purpose |
|---|---|---|
| `RAY_DEBUG_NO_OPT` | OFF | Debug build (`-O0 -g3`), enables extra validation code via `#if RAY_DEBUG_NO_OPT` |
| `RAY_GRAPHICS_ENABLE` | ON | Build Vulkan+GLFW graphics. Set OFF to compile without graphics |
| `RAY_ENABLE_PACKAGING` | OFF | Enable CPack distribution packaging |
| `RAY_CPU_PROFILE` / `RAY_GPU_PROFILE` | OFF | Enable Tracy profiling |

There are no tests or linter configured in this project.
