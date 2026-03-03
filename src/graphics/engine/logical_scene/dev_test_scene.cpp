#include "dev_test_scene.h"


#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"
#include "graphics/rhi/pipeline/impl/glyph_pipeline.h"
#include "utils/ray_colors.h"
#include "utils/ray_time.h"

#include <thread>


using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

ray_error dev_test_scene::init(window& win, pipeline_manager& pipe) {
        last_time_ns = now_ticks_ns();
        last_delta_time_ns = 0;

        pipeline_handle<rainbow_rect_pipeline> rainbow_pipeline = pipe.create_pipeline<rainbow_rect_pipeline>(2);
        pipeline_handle<solid_rect_pipeline> rect_pipeline = pipe.create_pipeline<solid_rect_pipeline>(1);

        pipeline_handle<glyph_pipeline> text_pipeline = pipe.create_pipeline<glyph_pipeline>(3);

        if (!rainbow_pipeline.is_valid() || !rect_pipeline.is_valid() ){//|| !text_pipeline.is_valid()) {
                return "can't init some pipelines";
        }

        ray_error text_error = text_line_manager.init(pipe, 10);
        if (text_error.has_value()) {
                return text_error;
        }

        ray_error hud_error = hud_info.init(win, pipe);
        if (hud_error) {
                return hud_error;
        }

        //pipeline_handle<glyph_pipeline> text_pipeline;
        lifetime_pipelines.push_back(rainbow_pipeline);
        lifetime_pipelines.push_back(rect_pipeline);
        lifetime_pipelines.push_back(text_pipeline);

        //std::vector<pipeline_handle<object_2d_pipeline<>>> a = hud_info.get_pipelines();
        //world_processor.register_pipelines(a);
        world_processor.register_pipeline(rainbow_pipeline);
        world_processor.register_pipeline(rect_pipeline);
        const pipeline_handle<object_2d_pipeline<>>& text_2d_pipeline = text_line_manager.get_pipeline();
        world_processor.register_pipeline(text_2d_pipeline);
        world_processor.register_pipeline(text_pipeline);

        rainbow_a = rainbow_pipeline.create_draw_obj();
        rainbow_b = rainbow_pipeline.create_draw_obj();
        rect_1 = rect_pipeline.create_draw_obj();
        rect_2 = rect_pipeline.create_draw_obj();
        rect_3 = rect_pipeline.create_draw_obj();

        text_M_handle = text_pipeline.create_draw_obj();
        text_K_handle = text_pipeline.create_draw_obj();

        new_line_1 = text_line_manager.create_text_line( {
                        .content_text = "hell",
                        .static_capacity = 128,
                        .space_basis = e_space_type::world,
                        .transform = glm::vec4(-100, -100, 0, 12), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left) / 8
                        .z_order = 10,
                        .text_color = ray_colors::solid(ray_colors::green),
                        .outline_size_px = 0.3f,
                        .outline_color = ray_colors::solid(ray_colors::black),
                        .background_color = ray_colors::transparent
                });

        new_line_2 = text_line_manager.create_text_line( {
                .content_text = "Hello World 123 .;Hi':",
                .static_capacity = 64,
                .space_basis = e_space_type::world,
                .transform = glm::vec4(-0, -0, 0, 30), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
                .z_order = 10,
                .text_color = ray_colors::solid(ray_colors::green),
                .outline_size_px = 1.0f,
                .outline_color = ray_colors::solid(ray_colors::black),
                .background_color = ray_colors::transparent
        });

        // new_line_3 = text_line_manager.create_text_line( {
        //         .content_text = "Hello World 123 .;Hi':",
        //         .static_capacity = 64,
        //         .space_basis = e_space_type::world,
        //         .transform = glm::vec4(-100, -80, 0, 8), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
        //         .z_order = 10,
        //         .text_color = ray_colors::solid(ray_colors::green),
        //         .outline_size_px = 0.3f,
        //         .outline_color = ray_colors::solid(ray_colors::black),
        //         .background_color = ray_colors::transparent
        // });

        if (auto text_1_handle_data = text_M_handle.access_draw_obj_data()) {
                text_1_handle_data->content_glyph = 'h';
                text_1_handle_data->text_outline_size_px = 5.0f;
                text_1_handle_data->text_outline_color = ray_colors::red;
                text_1_handle_data->background_color = ray_colors::blue;
                text_1_handle_data->space_basis = e_space_type::world;
                text_1_handle_data->z_order = 10;
                text_1_handle_data->transform = glm::vec4(100, 100, 150, 150); // x_pos, y_pos, x_size, y_size
                text_1_handle_data->color = ray_colors::cyan;
        }

         if (auto text_K_handle_data = text_K_handle.access_draw_obj_data()) {
                 text_K_handle_data->content_glyph = 'h';
                 text_K_handle_data->text_outline_size_px = 3.0f;
                 text_K_handle_data->text_outline_color = ray_colors::solid(ray_colors::red);
                 text_K_handle_data->background_color = ray_colors::cyan;
                 text_K_handle_data->space_basis = e_space_type::screen;
                 text_K_handle_data->z_order = 2;
                 text_K_handle_data->transform = glm::vec4(180, 80, 40, 40); // x_pos, y_pos, x_size, y_size
                 text_K_handle_data->color = ray_colors::yellow;
         }

        if (auto rainbow_a_data = rainbow_a.access_draw_obj_data()) {
                rainbow_a_data->space_basis = e_space_type::screen;
                rainbow_a_data->z_order = 100;
                rainbow_a_data->transform = glm::vec4(0, 0, 10, 10);
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
                transform_dyn_1 = glm::vec4(210, 0, 300, 60);
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

        return {};
}


bool dev_test_scene::tick(window& win, pipeline_manager& pipe) {
        // {
        //         glm::u64 curr_time_ns = now_ticks_ns();
        //         last_delta_time_ns = curr_time_ns - last_time_ns;
        //         last_time_ns = curr_time_ns;
        //
        //         if (!!new_line_1) {
        //                 std::chrono::duration<glm::u64, std::nano> ns_duration {last_delta_time_ns};
        //                 double sec_duration = std::chrono::duration_cast<std::chrono::duration<double>>(ns_duration).count();
        //                 double fps = 1.f / sec_duration;
        //                 //std::println("delta ns: {}, fps: {}", last_delta_time_ns, fps)
        //                 glm::vec4 camera_transform = world_processor.get_camera_transform();
        //                 const std::string fps_ms_str = std::format("print('Hello Google!') {}:{}:{} | delta ms: {:.3f}, fps: {:.1f}", camera_transform.x, camera_transform.y, camera_transform.z, (float)sec_duration * 1000.f, (float)fps);
        //                 new_line_1->update_content(fps_ms_str);
        //         }
        // }

        world_processor.tick(win, pipe);

        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        hud_info.update_camera_transform(new_cam_transform);

        hud_info.tick(win, pipe);

        // tick_camera_movement(win, pipe);

        // for (auto& world_pipeline : all_pipelines) {
        //         auto world_data = world_pipeline.access_pipeline_data();
        //         if (!world_data) {
        //                 return false;
        //         }
        //
        //         glm::u64 last_time_ms = last_time_ns / 1'000'000;
        //         world_data->time_ms = static_cast<glm::u32>(last_time_ms);
        //
        //         world_data->camera_transform = camera_transform;
        // }

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


void dev_test_scene::cleanup(window& win, pipeline_manager& pipe) {
        text_line_manager.destroy(pipe);
        hud_info.destroy(win, pipe);

        for (auto p : lifetime_pipelines) {
                pipe.destroy_pipeline(p);
        }

        new_line_1.reset();
}


// void dev_test_scene::tick_camera_movement(window& win, const pipeline_manager& pipe) {
//         glm::vec2 curr_mouse_pos = win.get_mouse_position();
//         glm::f32 delta_zoom = win.get_mouse_wheel_delta();
//
//         if (std::abs(delta_zoom) > 0.0001) {
//                 glm::vec2 viewport_px = (glm::vec2)pipe.get_target_resolution();
//                 glm::vec2 world_mouse_before = (curr_mouse_pos - viewport_px * 0.5f) * (1.0f / camera_transform.z);
//
//                 delta_zoom = std::exp(delta_zoom);
//                 camera_transform.z *= delta_zoom;
//                 camera_transform.z = std::clamp(camera_transform.z, 0.004f, 120.f);
//
//                 glm::vec2 world_mouse_after = (curr_mouse_pos - viewport_px * 0.5f) * (1.0f / camera_transform.z);
//
//                 glm::vec2 delta_zoom_move = world_mouse_before - world_mouse_after;
//
//                 camera_transform.x += delta_zoom_move.x;
//                 camera_transform.y += delta_zoom_move.y;
//         }
//
//         glm::vec2 delta_move = glm::vec2(0, 0);
//         if (win.get_mouse_button_right()) {
//                 if (base_move_position) {
//                         delta_move = *base_move_position - curr_mouse_pos;
//                 }
//                 base_move_position = curr_mouse_pos;
//         } else {
//                 base_move_position = std::nullopt;
//         }
//
//         delta_move /= camera_transform.z;
//         delta_move *= 1;
//
//         camera_transform.x += delta_move.x;
//         camera_transform.y += delta_move.y;
// }

#endif