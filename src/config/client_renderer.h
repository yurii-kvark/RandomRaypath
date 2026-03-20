#pragma once
#include "glm/glm.hpp"


#include <filesystem>
#include <string>
#include <expected>
#include <format>

namespace ray::config {

struct visual_style {
        glm::vec4 color_background;
        glm::vec4 color_hud_info;
        glm::vec4 color_grid;

        static const visual_style default_style;
};

enum class e_window_mode : glm::i8 {
        none = 0,
        windowed = none,
        fullscreen = 1,
        count
};

struct window_config {
        bool graphics_window_enabled;
        e_window_mode window_mode; // 0 - windowed, 1 - fullscreen
        glm::i32vec2 window_position;
        glm::i32vec2 window_size;
        glm::f32 zoom_speed;
};

class client_renderer {
public:
        bool enable_computational_server; // if true, it needs computational_server.toml
        window_config window;
        visual_style style;
        std::string logical_scene;

public:
        static const client_renderer default_renderer;
        static std::expected<client_renderer, std::string> load(std::filesystem::path target);

        [[nodiscard]]
        std::string to_string() const;
};
};


template<>
struct std::formatter<ray::config::client_renderer, char> : std::formatter<std::string, char> {
        auto format(ray::config::client_renderer const& v, std::format_context& ctx) const {
                return std::formatter<std::string, char>::format( v.to_string (), ctx);
        }
};


template<>
struct std::formatter<std::expected<ray::config::client_renderer, std::string>, char> : std::formatter<std::string, char> {
        auto format(std::expected<ray::config::client_renderer, std::string> const& v, std::format_context& ctx) const {
                if (v) {
                        return std::format_to(ctx.out(), "{}", *v);
                }
                return std::format_to(ctx.out(), "<error: {}>", v.error());
        }
};