#include "config/client_renderer.h"
#include "engine/logical_loop.h"
#include "graphics/window/window.h"
#include "network/connection_test.h"

#include <filesystem>
#include <print>
#include "utils/ray_log.h"

#include <glaze/glaze.hpp>
#include <string>

void test_func() {
        ray::ray_error error = ray::network::test_tcp_connection("google.com", 80);

        if (error.has_value()) {
                ray::ray_log(ray::e_log_type::info, "connection failed: {}", error.value());
        }
        else {
                ray::ray_log(ray::e_log_type::info, "connection successful! ");
        }
}

struct test_message {
        std::string type;
        int32_t id;
        double value;
};

void test_glaze() {
        // Serialize
        test_message msg{.type = "ray_batch", .id = 42, .value = 3.14};
        std::string json = glz::write_json(msg).value_or("error");
        ray::ray_log(ray::e_log_type::info, "serialized: {}", json);

        // Deserialize
        test_message parsed{};
        auto err = glz::read_json(parsed, json);
        if (err) {
                ray::ray_log(ray::e_log_type::info, "glaze parse error: {}", glz::format_error(err, json));
        }
        else {
                ray::ray_log(ray::e_log_type::info, "parsed: type={} id={} value={}", parsed.type, parsed.id, parsed.value);
        }
}

int main() {

        //test_func();
        test_glaze();

        return 0;

        auto config_res = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer.toml"});

        if (!config_res) {
                ray::ray_log(ray::e_log_type::fatal, "Failed to load config file {}", config_res.error());
                return 1;
        }

#if RAY_GRAPHICS_ENABLE
        ray::logical::async_logical_loop async_loop(*config_res);
        async_loop.wait_blocking();
#endif

        return 0;
}