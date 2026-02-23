#include "scene_logic.h"
#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"
#include "utils/ray_time.h"

#include <thread>


using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE


class ray_colors final {
public:
        static constexpr float alpha_value = 0.8f;
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

        if (!rainbow_pipeline.is_valid() || !rect_pipeline.is_valid()) {
                return;
        }

        pipeline_handle<base_pipeline<>> casted_pipeline = rainbow_pipeline;

        all_pipelines.push_back(rainbow_pipeline);
        all_pipelines.push_back(rect_pipeline);

        world_pipelines.push_back(rainbow_pipeline);
        world_pipelines.push_back(rect_pipeline);

        rainbow_1_screen = rend.pipe.create_draw_obj<rainbow_rect_pipeline>(rainbow_pipeline);
        rainbow_2_screen = rend.pipe.create_draw_obj<rainbow_rect_pipeline>(rainbow_pipeline);
        rect_3_dyn_world = rend.pipe.create_draw_obj<solid_rect_pipeline>(rect_pipeline);
        rect_4_dyn_world = rend.pipe.create_draw_obj<solid_rect_pipeline>(rect_pipeline);
        rect_5_world = rend.pipe.create_draw_obj<solid_rect_pipeline>(rect_pipeline);

        if (auto rainbow_1_screen_data = rend.pipe.access_draw_obj_data(rainbow_1_screen)) {
                rainbow_1_screen_data->space_basis = e_space_type::screen;
                rainbow_1_screen_data->z_order = 100;
                rainbow_1_screen_data->transform = glm::vec4(40, 150, 100, 120);
                rainbow_1_screen_data->color = ray_colors::cyan;
        }

        if (auto rainbow_2_world_data = rend.pipe.access_draw_obj_data(rainbow_2_screen)) {
                rainbow_2_world_data->space_basis = e_space_type::world;
                rainbow_2_world_data->z_order = 200;
                rainbow_2_world_data->transform = glm::vec4(0, -10, 80, 80);
                rainbow_2_world_data->color = ray_colors::navy;
        }

        if (auto rect_3_dyn_screen_data = rend.pipe.access_draw_obj_data(rect_3_dyn_world)) {
                rect_3_dyn_screen_data->space_basis = e_space_type::world;
                rect_3_dyn_screen_data->z_order = 3;
                transform_dyn_3 = glm::vec4(200, 100, 200, 100);
                rect_3_dyn_screen_data->transform = transform_dyn_3;
                rect_3_dyn_screen_data->color = ray_colors::lime;
        }

        if (auto rect_4_dyn_world_data = rend.pipe.access_draw_obj_data(rect_4_dyn_world)) {
                rect_4_dyn_world_data->space_basis = e_space_type::screen;
                rect_4_dyn_world_data->z_order = 4;
                transform_dyn_4 = glm::vec4(470, 120, 150, 150);
                rect_4_dyn_world_data->transform = transform_dyn_4;
                rect_4_dyn_world_data->color = ray_colors::purple;
        }

        if (auto rect_5_world_data = rend.pipe.access_draw_obj_data(rect_5_world)) {
                rect_5_world_data->space_basis = e_space_type::world;
                rect_5_world_data->z_order = 5;
                rect_5_world_data->transform = glm::vec4(100, 250, 350, 210);
                rect_5_world_data->color = ray_colors::red;
        }
}


bool scene_logic::tick(window& win, renderer& rend) {
        {
                glm::u64 curr_time_ns = now_ticks_ns();
                last_delta_time_ns = curr_time_ns - last_time_ns;
                last_time_ns = curr_time_ns;

                //std::this_thread::sleep_for(std::chrono::milliseconds(50));
                // if ((last_time_ns % 1'000'000) == 0) {
                //         std::chrono::duration<glm::u64, std::nano> ns_duration {last_delta_time_ns};
                //         double sec_duration = std::chrono::duration_cast<std::chrono::duration<double>>(ns_duration).count();
                //         double fps = 1.f / sec_duration;
                //         std::println("delta ns: {}, fps: {}", last_delta_time_ns, fps);
                // }
        }

        tick_camera_movement(win, rend);

        for (auto& world_pipeline : world_pipelines) {
                auto world_data = rend.pipe.access_pipeline_data(world_pipeline);
                if (!world_data) {
                        return false;
                }

                glm::u64 last_time_ms = last_time_ns / 1'000'000;
                world_data->time_ms = static_cast<glm::u32>(last_time_ms);

                world_data->camera_transform = camera_transform;
        }

        if (auto rect_3_dyn_world_data = rend.pipe.access_draw_obj_data(rect_3_dyn_world)) {
                rect_3_dyn_world_data->transform = transform_dyn_3 * std::sin((float)last_time_ns / 1'000'000'000 );
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
        for (auto p : all_pipelines) {
                rend.pipe.destroy_pipeline(p);
        }
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