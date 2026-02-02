module;
#include <filesystem>
#include <string>
#include <expected>
#include "toml++/toml.hpp"
module ray.config;

using namespace ray;
using namespace ray::config;
using namespace ray::graphics;

const visual_style visual_style::default_style = {
        .color_nothing = glm::dvec4(0.f, 0.f, 0.f, 1.f),
        .color_density_low = glm::dvec4(0.f, 1.f, 0.f, 1.f),
        .color_density_high = glm::dvec4(0.f, 0.f, 1.f, 1.f)
};

const client_renderer client_renderer::default_renderer = {
        .enable_computational_server = true,
        .window = {
                .graphics_window_enabled = true,
                .window_mode = graphics::e_window_mode::windowed,
                .window_position = glm::i32vec2(300, 200),
                .window_size = glm::i32vec2(800, 500)
        },
        .style = visual_style::default_style
};

std::expected<client_renderer, std::string> client_renderer::load(std::filesystem::path target) {
        toml::parse_result toml_result = toml::parse_file(target.string());

        if (toml_result.failed()) {
                const auto error_source = toml_result.error().source();
                const std::string fail_text = std::format("[start line:char {}:{}] {}",
                        error_source.begin.line, error_source.begin.column, toml_result.error().description());
                return std::unexpected(fail_text);
        }

        const toml::table& table = toml_result.table();

        client_renderer cnf_cr = default_renderer;

        const auto* toml_cnf_cr = table.get_as<toml::table>("client_renderer");

        if (!toml_cnf_cr) {
                return cnf_cr;
        }

        if (const auto ptr_ecs = toml_cnf_cr->get_as<bool>("enable_computational_server")) {
                cnf_cr.enable_computational_server = ptr_ecs->get();
        }

        if (const auto ptr_gw = toml_cnf_cr->get_as<bool>("graphics_window_enabled")) {
                cnf_cr.window.graphics_window_enabled = ptr_gw->get();
        }

        if (const auto ptr_wm_i = toml_cnf_cr->get_as<int64_t>("window_mode")) {
                const int64_t in_mode = ptr_wm_i->get();
                cnf_cr.window.window_mode = in_mode >= static_cast<int64_t>(e_window_mode::count) ? e_window_mode::none : static_cast<e_window_mode>(in_mode);
        }

        auto read_vec2i = [&](std::string_view key, glm::i32vec2& out) -> std::optional<std::string> {
                if (toml_cnf_cr->find(key) == toml_cnf_cr->end()) { // no key - no error
                        return std::nullopt;
                }

                const auto* arr = toml_cnf_cr->get_as<toml::array>(key);
                if (!arr) {
                        return std::format("cant parce array.", key);
                }

                if (arr->size() != 2u) {
                        return std::format("cant parce array. client_renderer.{} must be array of 2 integers. Got: {}", key, arr->size());
                }

                const auto* x = (*arr)[0].as_integer();
                const auto* y = (*arr)[1].as_integer();

                if (!x || !y) {
                        return std::format("cant parce integers. client_renderer.{} must be array of 2 integers", key);
                }

                out.x = static_cast<std::int32_t>(x->get());
                out.y = static_cast<std::int32_t>(y->get());

                return std::nullopt;
        };

        if (auto err = read_vec2i("window_position", cnf_cr.window.window_position)) {
                return std::unexpected(*err);
        }
        if (auto err = read_vec2i("window_size", cnf_cr.window.window_size)) {
                return std::unexpected(*err);
        }

        if (const auto* toml_visual_style = toml_cnf_cr->get_as<toml::table>("visual_style")) {
                auto read_color = [&](std::string_view key, glm::dvec4& out) -> std::optional<std::string> {
                        if (toml_visual_style->find(key) == toml_visual_style->end()) { // no key - no error
                                return std::nullopt;
                        }

                        const auto* color_arr = toml_visual_style->get_as<toml::array>(key);
                        if (!color_arr) {
                                return std::format("cant parce color.", key);
                        }

                        if (color_arr->size() != 4u) {
                                return std::format("cant parce color. client_renderer.visual_style.{} must be array of 4 floats. got: {}", key, color_arr->size());
                        }

                        const auto* r = (*color_arr)[0].as_floating_point();
                        const auto* g = (*color_arr)[1].as_floating_point();
                        const auto* b = (*color_arr)[2].as_floating_point();
                        const auto* a = (*color_arr)[3].as_floating_point();

                        if (!r || !g || !b || !a) {
                                return std::format("client_renderer.visual_style.{}: r({}), g({}), b({}), a({})", key, !!r, !!g, !!b, !!a);
                        }

                        out = glm::dvec4(r->get(), g->get(), b->get(), a->get());

                        return std::nullopt;
                };

                if (auto err = read_color("color_nothing", cnf_cr.style.color_nothing)) {
                        return std::unexpected(*err);
                }

                if (auto err = read_color("color_density_low", cnf_cr.style.color_density_low)) {
                        return std::unexpected(*err);
                }

                if (auto err = read_color("color_density_high", cnf_cr.style.color_density_high)) {
                        return std::unexpected(*err);
                }
        }

        return cnf_cr;
}


std::string client_renderer::to_string() const {
        auto color_to_toml_arr = [](const glm::dvec4& in) -> std::string {
                return std::format("[{}, {}, {}, {}]", in.x, in.y, in.z, in.w);
        };

        return std::format(
        "[client_renderer]\n"
        "enable_computational_server = {}\n"
        "graphics_window_enabled = {}\n"
        "window_mode = {}\n"
        "window_position = [{}, {}]\n"
        "window_size = [{}, {}]\n\n"
        "[client_renderer.visual_style]\n"
        "color_nothing = \"{}\"\n"
        "color_density_low = \"{}\"\n"
        "color_density_high = \"{}\"\n",
        enable_computational_server ? "true" : "false",
        window.graphics_window_enabled ? "true" : "false",
        static_cast<int>(window.window_mode),
        window.window_position.x, window.window_position.y,
        window.window_size.x, window.window_size.y,
        color_to_toml_arr(style.color_nothing),
        color_to_toml_arr(style.color_density_low),
        color_to_toml_arr(style.color_density_high));
}