#include "config/config.h"
#include "engine/logical_loop.h"

#include <chrono>
#include <filesystem>
#include <print>
#include "utils/ray_log.h"



int main(int argc, char** argv) {

        //auto config_res = ray::config::app_config::load_file(std::filesystem::path {"../config/config.toml"});
        auto config_res = ray::config::app_config::load_file_from_args(argc, argv);

        if (!config_res) {
                ray::ray_log(ray::e_log_type::fatal, "Failed to load config file {}", config_res.error());
                return 1;
        }

        config_res->upgrade_with_args(argc, argv);

        auto now_time = std::chrono::system_clock::now();
        const std::string log_file_name = std::format("log_{:%F-%H-%M-%S}.log", now_time);
        ray::ray_log_init(config_res->server_config.log_in_file);
        // ray::ray_log_rename(log_file_name);

        if (config_res->server_config.log_in_file) {
                ray::ray_log(ray::e_log_type::info, "Log file: {}\n", log_file_name);
        }

        ray::ray_log(ray::e_log_type::info, "Composed config: \n{}\n------ end cfg -------\n", *config_res);

        ray::logical::async_logical_loop async_loop(*config_res);
        async_loop.wait_blocking();

        return 0;
}