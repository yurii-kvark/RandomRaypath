#include <filesystem>
#include <print>

import ray.config;
import ray.graphics.window;

int main() {
        auto config_res = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer.toml"});

        if (!config_res) {
                std::println("Failed to load config file {}", config_res.error());
                return 1;
        }

        ray::graphics::window window(config_res->window);

        window.blocking_loop();

        return 0;

        //auto config1 = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer.toml"});
       // auto config2 = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer2.toml"});
       // auto config3 = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer3.toml"});

       // std::println("Config 1: \n{}\n---------", config1);
       // std::println("Config 2: \n{}\n---------", config2);
       // std::println("Config 3: \n{}\n---------", config3);

        //std::println("Hello!");

       // return 0;
}