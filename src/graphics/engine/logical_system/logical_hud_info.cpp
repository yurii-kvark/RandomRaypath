#include "logical_hud_info.h"

#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/pipeline_manager.h"
#include "graphics/rhi/pipeline/impl/solid_rect_pipeline.h"
#include "graphics/window/window.h"
#include "utils/ray_visual_config.h"
#include "utils/ray_error.h"
#include "utils/ray_time.h"

#if RAY_GRAPHICS_ENABLE

using namespace ray;
using namespace ray::graphics;

ray_error logical_hud_info::init(window& win, pipeline_manager& pipe, glm::vec4 text_color) {
        ray_error manager_error = text_line_manager.init(pipe, ray_pipeline_order::hud_info);
        if (manager_error.has_value()) {
                return manager_error;
        }

        background_rect_pipeline = pipe.create_pipeline<solid_rect_pipeline>(ray_pipeline_order::hud_info - 1);
        background_fps_obj = background_rect_pipeline.create_draw_obj();

        const glm::vec2 back_pivot_add_px = {4, 4};

        const glm::vec2 text_pivot_add_px = back_pivot_add_px + glm::vec2{4, 2};
        const float text_size_px = 10.f;
        const float line_height_em = 1.5f;
        const float line_height_px = text_size_px * line_height_em;

        logical_text_line_args text_args = logical_text_line_args {
                .content_text = "hell",
                .static_capacity = 128,
                .space_basis = e_space_type::screen,
                .transform = glm::vec4(text_pivot_add_px.x, text_pivot_add_px.y, 0, text_size_px), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left) / 8
                .z_order = 10,
                .text_color = text_color,
                .outline_size_px = text_size_px / 20.f,
                .outline_color = ray_colors::solid(ray_colors::black),
                .background_color = ray_colors::transparent,
                .pivot_offset_ndc = glm::vec4(-1.f, -1.f, 0, 0),
        };

        fps_text_line = text_line_manager.create_text_line(text_args);

        text_args.transform.y += line_height_px;
        mouse_text_line = text_line_manager.create_text_line(text_args);

        text_args.transform.y += line_height_px;
        cam_text_line = text_line_manager.create_text_line(text_args);

        if (auto back_data = background_fps_obj.access_draw_obj_data()) {
                back_data->space_basis = e_space_type::screen;
                back_data->z_order = 3;
                back_data->transform = glm::vec4(back_pivot_add_px.x, back_pivot_add_px.y, 380, text_args.transform.y + line_height_px * 0.8);
                back_data->color = ray_colors::alpha(ray_colors::black, 0.5);
                back_data->pivot_offset_ndc = glm::vec4(-1.f, -1.f, 0, 0);
        }

        return {};
}

// TODO: add frame counter
void logical_hud_info::tick(window& win, pipeline_manager& pipe) {
        glm::u64 curr_time_ns = now_ticks_ns();
        last_delta_time_ns = curr_time_ns - last_time_ns;
        last_time_ns = curr_time_ns;

        if (!!fps_text_line) {
                std::chrono::duration<glm::u64, std::nano> ns_duration {last_delta_time_ns};
                double sec_duration = std::chrono::duration_cast<std::chrono::duration<double>>(ns_duration).count();
                double fps = 1.f / sec_duration;

                const double smooth_coef = std::clamp(sec_duration / fps_smooth_sec, 0., 1.);
                smoothed_fps = smoothed_fps * (1 - smooth_coef) + fps * smooth_coef;

                fps_delay_reset_sec -= sec_duration;

                if (fps_delay_reset_sec <= 0) {
                        fps_delay_reset_sec = fps_maxmin_delay_sec;
                        max_fps = collecting_max_fps;
                        min_fps = collecting_min_fps;
                        collecting_max_fps = 0;
                        collecting_min_fps = 1000000000;
                }

                collecting_max_fps = std::max(collecting_max_fps, fps);
                collecting_min_fps = std::min(collecting_min_fps, fps);

                const std::string fps_str =std::format("frame: {:.2f} ms, avg_fps: {:.1f} | min {:.1f} | max {:.1f}", sec_duration * 1000, smoothed_fps, min_fps, max_fps);
                fps_text_line->update_content(fps_str);

                const glm::vec4 cam_vec = camera_transform ? *camera_transform : glm::vec4();

                float cam_zoom = cam_vec.z;
                bool sign_zoom_plus = false;
                if (cam_vec.z < 1.0) {
                        cam_zoom = 1/cam_zoom;
                        sign_zoom_plus = true;
                }

                const std::string cam_str =std::format("cam: ({:.1f}, {:.1f}) | 1:{}{:.1f}", cam_vec.x, cam_vec.y, sign_zoom_plus ? "+" : "-", cam_zoom);
                cam_text_line->update_content(cam_str);

                glm::vec2 viewport_px = (glm::vec2)pipe.get_target_resolution();
                glm::vec2 screen_mouse_pos = win.get_mouse_position();
                glm::vec2 world_mouse = (screen_mouse_pos - viewport_px * 0.5f) * (1.0f / cam_vec.z);

                const std::string mouse_str =std::format("mouse: screen ({:.1f}, {:.1f}) | world ({:.1f}, {:.1f})", screen_mouse_pos.x, screen_mouse_pos.y, world_mouse.x, world_mouse.y);
                mouse_text_line->update_content(mouse_str);
        }
}

void logical_hud_info::destroy(window& win, pipeline_manager& pipe) {
        text_line_manager.destroy(pipe);
        fps_text_line = nullptr;
        cam_text_line = nullptr;
        mouse_text_line = nullptr;

        pipe.destroy_pipeline(background_rect_pipeline);
}


void logical_hud_info::update_camera_transform_info(glm::vec4 new_cam) {
        camera_transform = new_cam;
}

#endif