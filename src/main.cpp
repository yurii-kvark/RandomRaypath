#include "config/client_renderer.h"
#include "graphics/engine/graphical_loop.h"
#include "graphics/window/window.h"
#include "network/connection_test.h"

#include <filesystem>
#include <print>
#include "utils/ray_log.h"

void test_func() {
        ray::ray_error error = ray::network::test_tcp_connection("google.com", 80);

        if (error.has_value()) {
                ray::ray_log(ray::e_log_type::info, "connection failed: {}", error.value());
        }
        else {
                ray::ray_log(ray::e_log_type::info, "connection successful! ");
        }
}

int main() {

        test_func();

        return 0;

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