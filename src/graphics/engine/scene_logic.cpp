#include "scene_logic.h"
#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"
#include "graphics/rhi/pipeline/impl/glyph_pipeline.h"
#include "utils/ray_time.h"

#include <thread>


using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE


class ray_colors final {
public:
        static constexpr float alpha_value = 0.8f;

        static constexpr glm::vec4 transparent {0.0f, 0.0f, 0.0f, 0.0f};

        static constexpr glm::vec4 black {0.0f, 0.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 white {1.0f, 1.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 gray {0.5f, 0.5f, 0.5f, alpha_value};

        static constexpr glm::vec4 red {1.0f, 0.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 green {0.0f, 1.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 blue {0.0f, 0.0f, 1.0f, alpha_value};

        static constexpr glm::vec4 yellow {1.0f, 1.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 cyan {0.0f, 1.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 magenta {1.0f, 0.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 orange {1.0f, 0.5f, 0.0f, alpha_value};
        static constexpr glm::vec4 purple {0.5f, 0.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 pink {1.0f, 0.25f, 0.5f, alpha_value};
        static constexpr glm::vec4 brown {0.55f, 0.27f, 0.07f, alpha_value};
        static constexpr glm::vec4 lime {0.50f, 1.00f, 0.00f, alpha_value};
        static constexpr glm::vec4 teal {0.00f, 0.50f, 0.50f, alpha_value};
        static constexpr glm::vec4 navy {0.00f, 0.00f, 0.50f, alpha_value};
        static constexpr glm::vec4 maroon {0.50f, 0.00f, 0.00f, alpha_value};
        static constexpr glm::vec4 olive {0.50f, 0.50f, 0.00f, alpha_value};
};


scene_logic::scene_logic(window& win, renderer& rend) {
        last_time_ns = now_ticks_ns();
        last_delta_time_ns = 0;

        pipeline_handle<rainbow_rect_pipeline> rainbow_pipeline = rend.pipe.create_pipeline<rainbow_rect_pipeline>(2);
        pipeline_handle<solid_rect_pipeline> rect_pipeline = rend.pipe.create_pipeline<solid_rect_pipeline>(1);

        pipeline_handle<glyph_pipeline> text_pipeline = rend.pipe.create_pipeline<glyph_pipeline>(3);

        if (!rainbow_pipeline.is_valid() || !rect_pipeline.is_valid() ){//|| !text_pipeline.is_valid()) {
                return;
        }

        text_line_manager.init(rend.pipe, 10);

        //pipeline_handle<glyph_pipeline> text_pipeline;
        all_pipelines.push_back(rainbow_pipeline);
        all_pipelines.push_back(rect_pipeline);
        all_pipelines.push_back(text_line_manager.get_pipeline());
        all_pipelines.push_back(text_pipeline);

        rainbow_a = rainbow_pipeline.create_draw_obj();
        rainbow_b = rainbow_pipeline.create_draw_obj();
        rect_1 = rect_pipeline.create_draw_obj();
        rect_2 = rect_pipeline.create_draw_obj();
        rect_3 = rect_pipeline.create_draw_obj();

        text_M_handle = text_pipeline.create_draw_obj();
       // text_2_handle = text_pipeline.create_draw_obj();

        // TODO: fix position slows *2 and text size
        new_line_1 = text_line_manager.create_text_line( {
                        .content_text = "hell",
                        .static_capacity = 64,
                        .space_basis = e_space_type::world,
                        .transform = glm::vec4(100, 100, 0, 60), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
                        .z_order = 10,
                        .text_color = ray_colors::lime,
                        .outline_size_ndc = 0.15f,
                        .outline_color = ray_colors::magenta,
                        .background_color = glm::vec4(0.,0.,0.,0.2f)
                });

        if (auto text_1_handle_data = text_M_handle.access_draw_obj_data()) {
                text_1_handle_data->content_glyph = 'h';
                text_1_handle_data->text_outline_size_ndc = 0.1f;
                text_1_handle_data->text_outline_color = ray_colors::red;
                text_1_handle_data->background_color = ray_colors::blue;
                text_1_handle_data->space_basis = e_space_type::world;
                text_1_handle_data->z_order = 10;
                text_1_handle_data->transform = glm::vec4(100, 100, 100, 100); // x_pos, y_pos, x_size, y_size
                text_1_handle_data->color = ray_colors::cyan;
        }
        //
        // if (auto text_2_handle_data = text_2_handle.access_draw_obj_data()) {
        //         text_2_handle_data->content_glyph = 'Q';
        //         text_2_handle_data->text_outline_size_ndc = 0.2f;
        //         text_2_handle_data->color = glm::vec4(1);
        //         text_2_handle_data->text_outline_color = ray_colors::lime;
        //         text_2_handle_data->background_color = ray_colors::transparent;
        //         text_2_handle_data->space_basis = e_space_type::world;
        //         text_2_handle_data->z_order = 11;
        //         text_2_handle_data->transform = glm::vec4(-200, -200, 150, 200); // x_pos, y_pos, x_size, y_size
        // }

        if (auto rainbow_a_data = rainbow_a.access_draw_obj_data()) {
                rainbow_a_data->space_basis = e_space_type::screen;
                rainbow_a_data->z_order = 100;
                rainbow_a_data->transform = glm::vec4(0, 0, 8, 8);
                rainbow_a_data->color = ray_colors::red;
        }

        if (auto rainbow_b_data = rainbow_b.access_draw_obj_data()) {
                rainbow_b_data->space_basis = e_space_type::world;
                rainbow_b_data->z_order = 200;
                rainbow_b_data->transform = glm::vec4(0, 0, 200, 200);
                rainbow_b_data->color = ray_colors::transparent;
        }

        if (auto rect_1_data = rect_1.access_draw_obj_data()) {
                rect_1_data->space_basis = e_space_type::world;
                rect_1_data->z_order = 3;
                transform_dyn_1 = glm::vec4(205, 0, 300, 60);
                rect_1_data->transform = transform_dyn_1;
                rect_1_data->color = ray_colors::blue;
        }

        if (auto rect_2_data = rect_2.access_draw_obj_data()) {
                rect_2_data->space_basis = e_space_type::world;
                rect_2_data->z_order = 4;
                transform_dyn_2 = glm::vec4(270, 0, 200, 30);
                rect_2_data->transform = transform_dyn_2;
                rect_2_data->color = ray_colors::cyan;
        }

        if (auto rect_3_data = rect_3.access_draw_obj_data()) {
                rect_3_data->space_basis = e_space_type::world;
                rect_3_data->z_order = 5;
                rect_3_data->transform = glm::vec4(100, 250, 350, 210);
                rect_3_data->color = ray_colors::transparent;
        }
}


bool scene_logic::tick(window& win, renderer& rend) {
        {
                glm::u64 curr_time_ns = now_ticks_ns();
                last_delta_time_ns = curr_time_ns - last_time_ns;
                last_time_ns = curr_time_ns;

                if (!!new_line_1) {
                        std::chrono::duration<glm::u64, std::nano> ns_duration {last_delta_time_ns};
                        double sec_duration = std::chrono::duration_cast<std::chrono::duration<double>>(ns_duration).count();
                        double fps = 1.f / sec_duration;
                        //std::println("delta ns: {}, fps: {}", last_delta_time_ns, fps);

                        const std::string fps_ms_str = std::format("{}:{} | delta ms: {:.3f}, fps: {:.1f}", camera_transform.x, camera_transform.y, (float)sec_duration * 1000.f, (float)fps);
                        new_line_1->update_content(fps_ms_str);
                }
        }

        tick_camera_movement(win, rend);

        for (auto& world_pipeline : all_pipelines) {
                auto world_data = world_pipeline.access_pipeline_data();
                if (!world_data) {
                        return false;
                }

                glm::u64 last_time_ms = last_time_ns / 1'000'000;
                world_data->time_ms = static_cast<glm::u32>(last_time_ms);

                world_data->camera_transform = camera_transform;
        }

        if (auto rect_1_data = rect_1.access_draw_obj_data()) {
                //rect_3_dyn_world_data->transform = transform_dyn_3 * std::sin((float)last_time_ns / 1'000'000'000 );
        } else {
                return false;
        }

        // if (auto rect_4_dyn_world_data = rend.pipe.access_draw_obj_data(rect_4_dyn_world)) {
        //         rect_4_dyn_world_data->transform = transform_dyn_4;
        // } else {
        //         return false;
        // }

        return true;
}


void scene_logic::cleanup(window& win, renderer& rend) {
        text_line_manager.destroy(rend.pipe);

        for (auto p : all_pipelines) {
                rend.pipe.destroy_pipeline(p);
        }

        new_line_1.reset();
}


void scene_logic::tick_camera_movement(window& win, renderer& rend) {
        glm::vec2 curr_mouse_pos = win.get_mouse_position();
        glm::f32 delta_zoom = win.get_mouse_wheel_delta();

        if (std::abs(delta_zoom) > 0.0001) {
                glm::vec2 viewport_px = (glm::vec2)rend.pipe.get_target_resolution();
                glm::vec2 world_mouse_before = (curr_mouse_pos - viewport_px * 0.5f) * (1.0f / camera_transform.z);

                delta_zoom = std::exp(delta_zoom);
                camera_transform.z *= delta_zoom;
                camera_transform.z = std::clamp(camera_transform.z, 0.004f, 120.f);

                glm::vec2 world_mouse_after = (curr_mouse_pos - viewport_px * 0.5f) * (1.0f / camera_transform.z);

                glm::vec2 delta_zoom_move = world_mouse_before - world_mouse_after;

                camera_transform.x += delta_zoom_move.x;
                camera_transform.y += delta_zoom_move.y;
        }

        glm::vec2 delta_move = glm::vec2(0, 0);
        if (win.get_mouse_button_right()) {
                if (base_move_position) {
                        delta_move = *base_move_position - curr_mouse_pos;
                }
                base_move_position = curr_mouse_pos;
        } else {
                base_move_position = std::nullopt;
        }

        delta_move /= camera_transform.z;
        delta_move *= 1;

        camera_transform.x += delta_move.x;
        camera_transform.y += delta_move.y;
}


scene_logic::~scene_logic() {
}

#endif