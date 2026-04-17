# Architecture

## Startup Flow

`main.cpp` loads TOML config -> creates `async_graphical_loop` -> which spawns a `std::jthread` running the render loop. The render thread creates `window`, `renderer`, and a `logical_scene` (selected by config name string).

## Layered Structure

- **Config** (`src/config/`): TOML-based configuration using tomlplusplus. `client_renderer` holds window, visual style, and scene selection.
- **Graphics RHI** (`src/graphics/rhi/`): Low-level Vulkan abstraction.
  - `g_app_driver` — singleton Vulkan instance/device/queues (thread-safe init).
  - `renderer` — swapchain, command buffers, frame synchronization (double-buffered via `k_frames_in_flight = 2`).
  - `pipeline_manager` — owns all pipeline instances, sorted by render priority, dispatches draw calls.
  - `object_2d_pipeline` — template-heavy base for 2D draw pipelines with GPU buffer management, z-ordering, and camera transforms.
- **Pipeline Implementations** (`src/graphics/rhi/pipeline/impl/`): Concrete pipelines — `solid_rect`, `rainbow_rect`, `glyph` (MTSDF text), `visual_grid`. Each pairs with GLSL shaders in `shaders/`.
- **Logical Scenes** (`src/graphics/engine/logical_scene/`): Implement `i_logical_scene` interface (`init`/`tick`/`cleanup`). Scene selected at startup by name: `"main"`, `"dev_test"`, `"minecraft"`. Adding a new scene requires registering it in `make_scene_by_name()` in `graphical_loop.cpp`.
- **Logical Systems** (`src/graphics/engine/logical_system/`): Reusable systems composed within scenes — `logical_grid`, `logical_hud_info`, `logical_2d_world_view`, `logical_text_line`, `logical_crate_sim`.
- **Utils** (`src/utils/`): `ray_log` (logging), `ray_error` (just `std::optional<std::string>`), `ray_profile` (Tracy wrappers), `ray_glyph` (font atlas), `ray_time`, `ray_index` (index pool), `ray_visual_config` (color constants).
- **Shaders** (`shaders/`): GLSL sources compiled to SPIR-V in `shaders/bins/` by `CompileRayShaders.cmake` using `glslangValidator`.
