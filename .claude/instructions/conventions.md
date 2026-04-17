# Code Conventions

- **by default work inside 'dev' brauch or don't touch branching**: no checkout or changes of non 'dev' branch.
- **No exceptions, no RTTI**: compiled with `-fno-exceptions -fno-rtti`. Error handling uses `std::expected` (config loading) or `ray_error` (`std::optional<std::string>` — has value means error).
- **Conditional compilation**: most graphics code is wrapped in `#if RAY_GRAPHICS_ENABLE`. Debug-only code uses `#if RAY_DEBUG_NO_OPT`.
- **Namespace**: everything lives under `ray::` with `ray::graphics::` for rendering, `ray::config::` for configuration.
- **GLM as primitive types**: the codebase uses `glm::u32`, `glm::u8`, `glm::vec4`, etc. everywhere instead of standard integer types.
- **Data-oriented pipeline model**: each pipeline type defines a `*_data_model` struct containing nested `pipeline` and `draw_obj` structs with GPU-layout UBOs (`std140` alignment).

## Third-Party Dependencies (all as git submodules in `third_party/`)

glm, glfw, Vulkan-Headers, volk (Vulkan loader), tracy (profiler), tomlplusplus (header-only TOML parser).
