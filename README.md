Random Raypath Project  
  
Random Raypath is a high-performance C++23 project built around an optimized Vulkan renderer (Linux + Windows)   
for interactive visualizing multi-ray paths through a heterogeneous-environment simulation.   
Think of a light ray bending and splitting inside a soap bubble.   
  
The core goal is to push the maximum number of rays on screen in real time, with rapid draw-target switching   
and a data-oriented pipeline designed for low overhead and stable frame pacing.   
  
Ray tracing/propagation is scaled to multi-threaded CPU computation,  
and ultimately distributed network workers, batching rays, aggregating area segments, and streaming results into the renderer.  
No persistent result of computation storage, only RAM and VRAM usage at the maximum limit.  
  
Current status:  
build pipeline - 100%   
optimized graphics - 30%  
simulation and interaction - 10%  
network distribution - 0%  
multithread computation - 0%.  
  
Simulation reference:  
https://www.youtube.com/watch?v=UNCNp1tBqKY  
https://www.youtube.com/watch?v=7Cc08CGDKNY  
