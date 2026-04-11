#pragma once
#include "glm/glm.hpp"


#include <filesystem>
#include <string>
#include <expected>
#include <format>

namespace ray::config {

struct visual_style_config {
        glm::vec4 color_background = glm::vec4(0);
        glm::vec4 color_hud_info = glm::vec4(0);
        glm::vec4 color_grid = glm::vec4(0);

        static const visual_style_config default_style;
};

enum class e_window_mode : glm::i8 {
        none = 0,
        windowed = none,
        fullscreen = 1,
        count
};

struct render_server_config {
        bool enable = true;
        bool graphics_window_enabled = true; // -headless flag will force it to false

        struct {
                int listening_port = 00000;
                bool enable_remote_control = false; // -remote_control will force it to true
                std::string server_control_addr = "127.0.0.1:00000"; // address of control server
        } network;

        struct {
                e_window_mode window_mode = e_window_mode::windowed; // 0 - windowed, 1 - fullscreen
                glm::i32vec2 window_position = {800, 400}; // in pixels [pos_x, pos_y] size_x, size_y]
                glm::i32vec2 window_size = {900, 500};// in pixels [pos_x, pos_y] size_x, size_y]
        } window;

        struct {
                bool tickless_mode = false; // -tickless will force it to true
                float fixed_delta_time_ms = -1;
                float zoom_speed = 0.1;
                std::string logical_scene = "dev_test"; // main / dev_test / minecraft
        } scene;

        visual_style_config style;
};

struct compute_client_config {
        bool enable = false;
        std::string server_addr = "127.0.0.1:00000";
        int thread_limit = 4; // -1 to enable optimal full
};

class app_config {
public:
        render_server_config server_config;
        compute_client_config compute_config;

public:
        static const app_config default_renderer;
        static std::expected<app_config, std::string> load(std::filesystem::path target);

        void upgrade_with_args(int argc, char** argv);

        [[nodiscard]]
        std::string to_string() const;
};
};


template<>
struct std::formatter<ray::config::app_config, char> : std::formatter<std::string, char> {
        auto format(ray::config::app_config const& v, std::format_context& ctx) const {
                return std::formatter<std::string, char>::format( v.to_string (), ctx);
        }
};


template<>
struct std::formatter<std::expected<ray::config::app_config, std::string>, char> : std::formatter<std::string, char> {
        auto format(std::expected<ray::config::app_config, std::string> const& v, std::format_context& ctx) const {
                if (v) {
                        return std::format_to(ctx.out(), "{}", *v);
                }
                return std::format_to(ctx.out(), "<error: {}>", v.error());
        }
};