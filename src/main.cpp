#include "config/config.h"
#include "engine/logical_loop.h"

#include <filesystem>
#include <print>
#include "utils/ray_log.h"



int main() {

        auto config_res = ray::config::app_config::load(std::filesystem::path {"../config/config.toml"});

        if (!config_res) {
                ray::ray_log(ray::e_log_type::fatal, "Failed to load config file {}", config_res.error());
                return 1;
        }

        config_res->upgrade_with_args(); // pass args here

        ray::logical::async_logical_loop async_loop(*config_res);
        async_loop.wait_blocking();

        return 0;
}