#include "config/client_renderer.h"
#include "graphics/engine/graphical_loop.h"
#include "graphics/window/window.h"

#include <filesystem>
#include <print>
#include "utils/ray_log.h"


int main() {
        auto config_res = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer.toml"});

        if (!config_res) {
                ray::ray_log(ray::e_log_type::fatal, "Failed to load config file {}", config_res.error());
                return 1;
        }

#if RAY_GRAPHICS_ENABLE
        ray::graphics::async_graphical_loop async_loop(*config_res);
        async_loop.wait_blocking();
#endif

        return 0;
}