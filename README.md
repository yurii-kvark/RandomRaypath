## Random Raypath Project  
  
Random Raypath is a high-performance C++23 project built around an optimized Vulkan renderer (Linux + Windows)   
for interactive visualizing multi-ray paths through a heterogeneous-environment simulation.   
Think of a light ray bending and splitting inside a flat soap bubble medium (Branched flow of light phenomenon).    
  
The core goal is to push the maximum number of rays on screen in real time, with rapid draw-target switching   
and a data-oriented pipeline designed for low overhead and stable frame pacing.   
  
Ray tracing/propagation is scaled to multi-threaded CPU computation,  
and ultimately distributed network workers, batching rays, aggregating area segments, and streaming results into the renderer.  
No persistent result of computation storage, only RAM and VRAM usage at the maximum limit.
  
Simulation reference:  
https://www.youtube.com/watch?v=6aZ45RNHa6U  
https://www.youtube.com/watch?v=UNCNp1tBqKY  
https://www.youtube.com/watch?v=7Cc08CGDKNY  


## Build:

Get submodules libs:
- git submodule update --init --recursive

CMake options:
* RAY_ENABLE_PACKAGING=1/(0) - Enable distributable package generation via CPack.
* RAY_DEBUG_NO_OPT=1/(0) - Enable debug build options. Also enables additional checking or stat code.
* RAY_GRAPHICS_ENABLE=(1)/0 - Build graphics (Vulkan + GLFW), enabling graphics namespace classes.
* RAY_CPU_PROFILE=1/(0) - Enable profile build options.
* RAY_GPU_PROFILE=1/(0) - Enable profile build options.
* RAY_CPU_PROFILE_IMPL=("tracy") - CPU profiler implementation
* RAY_GPU_PROFILE_IMPL=("tracy") - GPU profiler implementation

Debug build:
cmake --build cmake-build-debug -G Ninja -DRAY_DEBUG_NO_OPT=1 -DGLSLANG_VALIDATOR="{VulkanSDK_Path}/Bin/glslangValidator.exe"

Release Package build:
cmake --build cmake-build-package -G Ninja -DRAY_DEBUG_NO_OPT=0 -DRAY_ENABLE_PACKAGING=1 -DGLSLANG_VALIDATOR="{VulkanSDK_Path}/Bin/glslangValidator.exe"
cpack --build cmake-build-package

## Notes:
Design:
* Graphical engine did not designed to handle huge amount of object, need to optimize object_2d_pipeline::update_object_memory.
* Graphical engine should strictly hold above 200 FPS in true sync, some spikes are allowed, but not logic processing related.

Known perf hits:
* [renderer::draw_frame] Memory fence for graphical buffer waiting up to 1.4 ms sometimes, it's ok since this is graphical data update wait.
* [logical_text_line::update_content] Trigger every glyph to update if text changed, up to 1 ms per frame for hud_info draw.
* [object_2d_pipeline::update_object_memory] Pipeline iterates through every draw object to check if update need. Not critical for the engine purposes, but will make issues for a bigger quantity of draw objects.
