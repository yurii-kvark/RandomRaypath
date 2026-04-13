#include "logical_cursor_visualize.h"

#include "graphics/rhi/pipeline/pipeline_manager.h"
#include "graphics/window/window.h"
#include "utils/ray_visual_config.h"

using namespace ray;
using namespace ray::logical;
using namespace ray::graphics;

ray_error logical_cursor_visualize::init(window& win, pipeline_manager& pipe, config::render_server_config in_cfg) {
        cfg = std::move(in_cfg);

        if (!cfg.scene.show_cursor) {
                return {};
        }

        cursor_pipeline = pipe.create_pipeline<solid_rect_pipeline>(ray_pipeline_order::cursor);
        border_rect = cursor_pipeline.create_draw_obj();
        inner_rect = cursor_pipeline.create_draw_obj();

        if (auto data = border_rect.access_draw_obj_data()) {
                data->space_basis = e_space_type::screen;
                data->z_order = 10;
                data->transform = glm::vec4(0.f, 0.f, cfg.scene.cursor_size_px.x, cfg.scene.cursor_size_px.y);
                data->color = cfg.style.cursor_border_color;
                data->pivot_offset_ndc = glm::vec4(-1.f, -1.f, 0, 0);
        }

        if (auto data = inner_rect.access_draw_obj_data()) {
                data->space_basis = e_space_type::screen;
                data->z_order = 11;
                data->transform = glm::vec4(0.f, 0.f, cfg.scene.cursor_size_px.x - 2.f * cfg.scene.cursor_border_size_px, cfg.scene.cursor_size_px.y - 2.f * cfg.scene.cursor_border_size_px);
                data->color = cfg.style.cursor_idle_color;
                data->pivot_offset_ndc = glm::vec4(-1.f, -1.f, 0, 0);
        }

        return {};
}

void logical_cursor_visualize::tick(window& win, pipeline_manager& pipe) {
        if (!cfg.scene.show_cursor) {
                return;
        }

        const glm::vec2 pos = win.get_mouse_position();
        const glm::f32 border = cfg.scene.cursor_border_size_px;
        const glm::vec2 size = cfg.scene.cursor_size_px;

        if (auto data = border_rect.access_draw_obj_data()) {
                data->transform.x = pos.x;
                data->transform.y = pos.y;
        }

        // Determine inner color based on mouse button state
        const bool left = win.get_mouse_button_left();
        const bool right = win.get_mouse_button_right();

        glm::vec4 inner_color = cfg.style.cursor_idle_color;
        if (left && right) {
                inner_color = cfg.style.cursor_both_pressed_color;
        } else if (left) {
                inner_color = cfg.style.cursor_left_pressed_color;
        } else if (right) {
                inner_color = cfg.style.cursor_right_pressed_color;
        }

        if (auto data = inner_rect.access_draw_obj_data()) {
                data->transform.x = pos.x + border;
                data->transform.y = pos.y + border;
                data->color = inner_color;
        }
}

void logical_cursor_visualize::destroy(window& win, pipeline_manager& pipe) {
        if (!cfg.scene.show_cursor) {
                return;
        }

        pipe.destroy_pipeline(cursor_pipeline);
}
