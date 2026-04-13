#include "config.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <expected>
#include <toml++/toml.hpp>

using namespace ray;
using namespace ray::config;

const visual_style_config visual_style_config::default_style = {
        .color_background = glm::vec4(0.005f, 0.005f, 0.005f, 1.f),
        .color_hud_info = glm::vec4(0.f, 1.f, 0.f, 1.f),
        .color_grid = glm::vec4(0.03f, 0.02f, 0.04f, 1.f)
};

const app_config app_config::default_renderer = {
        .server_config = {
                .enable = true,
                .graphics_window_enabled = true,
                .network = {
                        .listening_port = 47039,
                        .enable_remote_control = false,
                        .server_control_addr = "127.0.0.1:57039",
                },
                .window = {
                        .window_mode = e_window_mode::windowed,
                        .window_position = glm::i32vec2(800, 400),
                        .window_size = glm::i32vec2(900, 500),
                },
                .scene = {
                        .tickless_mode = false,
                        .zoom_speed = 0.1f,
                        .logical_scene = "dev_test",
                },
                .style = visual_style_config::default_style,
        },
        .compute_config = {
                .enable = false,
                .server_addr = "127.0.0.1:47039",
                .thread_limit = 4,
        },
};

std::expected<app_config, std::string> app_config::load_file(std::filesystem::path target) {
        toml::parse_result toml_result = toml::parse_file(target.string());

        if (toml_result.failed()) {
                const auto error_source = toml_result.error().source();
                const std::string fail_text = std::format("[start line:char {}:{}] {}",
                        error_source.begin.line, error_source.begin.column, toml_result.error().description());
                return std::unexpected(fail_text);
        }

        const toml::table& table = toml_result.table();

        app_config cnf = default_renderer;

        auto read_vec2i = [](const toml::table* tbl, std::string_view section, std::string_view key, glm::i32vec2& out) -> std::optional<std::string> {
                if (tbl->find(key) == tbl->end()) {
                        return std::nullopt;
                }

                const auto* arr = tbl->get_as<toml::array>(key);
                if (!arr) {
                        return std::format("cant parce array. {}.{}", section, key);
                }

                if (arr->size() != 2u) {
                        return std::format("cant parce array. {}.{} must be array of 2 integers. Got: {}", section, key, arr->size());
                }

                const auto* x = (*arr)[0].as_integer();
                const auto* y = (*arr)[1].as_integer();

                if (!x || !y) {
                        return std::format("cant parce integers. {}.{} must be array of 2 integers", section, key);
                }

                out.x = static_cast<glm::i32>(x->get());
                out.y = static_cast<glm::i32>(y->get());

                return std::nullopt;
        };

        auto read_color = [](const toml::table* tbl, std::string_view section, std::string_view key, glm::vec4& out) -> std::optional<std::string> {
                if (tbl->find(key) == tbl->end()) {
                        return std::nullopt;
                }

                const auto* color_arr = tbl->get_as<toml::array>(key);
                if (!color_arr) {
                        return std::format("cant parce color. {}.{}", section, key);
                }

                if (color_arr->size() != 4u) {
                        return std::format("cant parce color. {}.{} must be array of 4 floats. got: {}", section, key, color_arr->size());
                }

                const auto* r = (*color_arr)[0].as_floating_point();
                const auto* g = (*color_arr)[1].as_floating_point();
                const auto* b = (*color_arr)[2].as_floating_point();
                const auto* a = (*color_arr)[3].as_floating_point();

                if (!r || !g || !b || !a) {
                        return std::format("{}.{}: r({}), g({}), b({}), a({})", section, key, !!r, !!g, !!b, !!a);
                }

                out = glm::vec4(r->get(), g->get(), b->get(), a->get());

                return std::nullopt;
        };

        if (const auto* toml_rs = table.get_as<toml::table>("render_server")) {
                if (const auto p = toml_rs->get_as<bool>("enable")) {
                        cnf.server_config.enable = p->get();
                }
                if (const auto p = toml_rs->get_as<bool>("graphics_window_enabled")) {
                        cnf.server_config.graphics_window_enabled = p->get();
                }
                if (const auto p = toml_rs->get_as<bool>("log_in_file")) {
                        cnf.server_config.log_in_file = p->get();
                }
                
                if (const auto* toml_net = toml_rs->get_as<toml::table>("network")) {
                        if (const auto p = toml_net->get_as<int64_t>("listening_port")) {
                                cnf.server_config.network.listening_port = static_cast<int>(p->get());
                        }
                        if (const auto p = toml_net->get_as<bool>("enable_remote_control")) {
                                cnf.server_config.network.enable_remote_control = p->get();
                        }
                        if (const auto p = toml_net->get_as<std::string>("server_control_addr")) {
                                cnf.server_config.network.server_control_addr = p->get();
                        }
                }

                if (const auto* toml_win = toml_rs->get_as<toml::table>("window")) {
                        if (const auto p = toml_win->get_as<int64_t>("window_mode")) {
                                const int64_t in_mode = p->get();
                                cnf.server_config.window.window_mode = in_mode >= static_cast<int64_t>(e_window_mode::count) ? e_window_mode::none : static_cast<e_window_mode>(in_mode);
                        }
                        if (auto err = read_vec2i(toml_win, "render_server.window", "window_position", cnf.server_config.window.window_position)) {
                                return std::unexpected(*err);
                        }
                        if (auto err = read_vec2i(toml_win, "render_server.window", "window_size", cnf.server_config.window.window_size)) {
                                return std::unexpected(*err);
                        }
                }

                if (const auto* toml_scene = toml_rs->get_as<toml::table>("scene")) {
                        if (const auto p = toml_scene->get_as<bool>("tickless_mode")) {
                                cnf.server_config.scene.tickless_mode = p->get();
                        }
                        if (const auto p = toml_scene->get_as<double>("fixed_delta_time_ms")) {
                                cnf.server_config.scene.fixed_delta_time_ms = static_cast<glm::f32>(p->get());
                        }
                        if (const auto p = toml_scene->get_as<double>("zoom_speed")) {
                                cnf.server_config.scene.zoom_speed = static_cast<glm::f32>(p->get());
                        }
                        if (const auto p = toml_scene->get_as<bool>("show_hud_info")) {
                                cnf.server_config.scene.show_hud_info = p->get();
                        }
                        if (const auto p = toml_scene->get_as<double>("size_text_hud_info")) {
                                cnf.server_config.scene.size_text_hud_info = static_cast<glm::f32>(p->get());
                        }
                        if (const auto p = toml_scene->get_as<std::string>("logical_scene")) {
                                cnf.server_config.scene.logical_scene = p->get();
                        } else {
                                return std::unexpected("Can't parse render_server.scene.logical_scene");
                        }

                        bool enable_hud_info = false;
                        float size_text_hud_info = 10.0;
                }

                if (const auto* toml_style = toml_rs->get_as<toml::table>("visual_style")) {
                        if (auto err = read_color(toml_style, "render_server.visual_style", "color_background", cnf.server_config.style.color_background)) {
                                return std::unexpected(*err);
                        }
                        if (auto err = read_color(toml_style, "render_server.visual_style", "color_hud_info", cnf.server_config.style.color_hud_info)) {
                                return std::unexpected(*err);
                        }
                        if (auto err = read_color(toml_style, "render_server.visual_style", "color_grid", cnf.server_config.style.color_grid)) {
                                return std::unexpected(*err);
                        }
                }
        }

        if (const auto* toml_cc = table.get_as<toml::table>("compute_client")) {
                if (const auto p = toml_cc->get_as<bool>("enable")) {
                        cnf.compute_config.enable = p->get();
                }
                if (const auto p = toml_cc->get_as<std::string>("server_addr")) {
                        cnf.compute_config.server_addr = p->get();
                }
                if (const auto p = toml_cc->get_as<int64_t>("thread_limit")) {
                        cnf.compute_config.thread_limit = static_cast<int>(p->get());
                }
        }

        return cnf;
}

std::expected<app_config, std::string> app_config::load_file_from_args(int argc, char **argv) {
        constexpr std::string_view prefix = "-config=";
        std::string path = "default.toml";

        for (int i = 1; i < argc; ++i) {
                const std::string_view arg{argv[i]};
                if (arg.starts_with(prefix)) {
                        auto value = arg.substr(prefix.size());
                        // Strip surrounding quotes if present
                        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                                value = value.substr(1, value.size() - 2);
                        }
                        path = std::string{value};
                        break;
                }
        }

        path = std::format("../config/{}", path);

        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || ec) {
                return std::unexpected("Config file not found: " + path + " -> " + ec.message());
        }

        return load_file(path);
}

void app_config::upgrade_with_args(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
                const std::string_view a{argv[i]};
                if (a == "-headless") {
                        server_config.graphics_window_enabled = false;
                } else if (a == "-remote_control") {
                        server_config.network.enable_remote_control = true;
                } else if (a == "-tickless") {
                        server_config.scene.tickless_mode = true;
                }
        }
}

std::string app_config::to_string() const {
        auto color_to_toml_arr = [](const glm::vec4& in) -> std::string {
                return std::format("[{}, {}, {}, {}]", in.x, in.y, in.z, in.w);
        };

        return std::format(
        "[render_server]\n"
        "enable = {}\n"
        "graphics_window_enabled = {}\n\n"
        "log_in_file = {}\n"
        "[render_server.network]\n"
        "listening_port = {}\n"
        "enable_remote_control = {}\n"
        "server_control_addr = \"{}\"\n\n"
        "[render_server.window]\n"
        "window_mode = {}\n"
        "window_position = [{}, {}]\n"
        "window_size = [{}, {}]\n\n"
        "[render_server.scene]\n"
        "tickless_mode = {}\n"
        "fixed_delta_time_ms = {}\n"
        "show_hud_info = {}\n"
        "size_text_hud_info = {}\n"
        "zoom_speed = {}\n"
        "logical_scene = \"{}\"\n\n"
        "[render_server.visual_style]\n"
        "color_background = {}\n"
        "color_hud_info = {}\n"
        "color_grid = {}\n\n"
        "[compute_client]\n"
        "enable = {}\n"
        "server_addr = \"{}\"\n"
        "thread_limit = {}\n",
        server_config.enable ? "true" : "false",
        server_config.graphics_window_enabled ? "true" : "false",
        server_config.log_in_file ? "true" : "false",
        server_config.network.listening_port,
        server_config.network.enable_remote_control ? "true" : "false",
        server_config.network.server_control_addr,
        static_cast<int>(server_config.window.window_mode),
        server_config.window.window_position.x, server_config.window.window_position.y,
        server_config.window.window_size.x, server_config.window.window_size.y,
        server_config.scene.tickless_mode ? "true" : "false",
        server_config.scene.fixed_delta_time_ms,
        server_config.scene.zoom_speed,
        server_config.scene.show_hud_info ? "true" : "false",
        server_config.scene.size_text_hud_info,
        server_config.scene.logical_scene,
        color_to_toml_arr(server_config.style.color_background),
        color_to_toml_arr(server_config.style.color_hud_info),
        color_to_toml_arr(server_config.style.color_grid),
        compute_config.enable ? "true" : "false",
        compute_config.server_addr,
        compute_config.thread_limit);
}