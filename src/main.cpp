#include "config/client_renderer.h"
#include "graphics/window/window.h"

#include <filesystem>
#include <print>


int main() {
        auto config_res = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer.toml"});

        if (!config_res) {
                std::println("Failed to load config file {}", config_res.error());
                return 1;
        }

#if RAY_GRAPHICS_ENABLE
        ray::graphics::window window(config_res->window);

        window.blocking_loop();
#endif

        return 0;
}