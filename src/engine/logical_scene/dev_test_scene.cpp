#include "dev_test_scene.h"


#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"
#include "graphics/rhi/pipeline/impl/glyph_pipeline.h"
#include "graphics/rhi/pipeline/impl/visual_grid_pipeline.h"
#include "utils/ray_visual_config.h"
#include "utils/ray_time.h"

#include <thread>


using namespace ray;
using namespace ray::graphics;
using namespace ray::logical;


ray_error dev_test_scene::init(window& win, pipeline_manager& pipe) {
        ray_error init_error = base_scene::init(win, pipe);
        if (init_error) {
                return init_error;
        }

        pipeline_handle<rainbow_rect_pipeline> rainbow_pipeline = pipe.create_pipeline<rainbow_rect_pipeline>(ray_pipeline_order::world_obj + 1);
        pipeline_handle<solid_rect_pipeline> rect_pipeline = pipe.create_pipeline<solid_rect_pipeline>(ray_pipeline_order::world_obj + 4);
        pipeline_handle<glyph_pipeline> text_pipeline = pipe.create_pipeline<glyph_pipeline>(ray_pipeline_order::world_obj + 3);
        pipeline_handle<visual_grid_pipeline> grid_pipeline = pipe.create_pipeline<visual_grid_pipeline>(ray_pipeline_order::world_obj + 20);

        if (!rainbow_pipeline.is_valid() || !rect_pipeline.is_valid() || !text_pipeline.is_valid() || !grid_pipeline.is_valid()) {
                return "can't init some pipelines";
        }

        ray_error text_error = text_line_manager.init(pipe, ray_pipeline_order::world_obj + 10);
        if (text_error.has_value()) {
                return text_error;
        }

        ray_error crate_sim_error = crate_sim.init(win, pipe);
        if (crate_sim_error) {
                return crate_sim_error;
        }

        //pipeline_handle<glyph_pipeline> text_pipeline;
        lifetime_pipelines.push_back(rainbow_pipeline);
        lifetime_pipelines.push_back(rect_pipeline);
        lifetime_pipelines.push_back(text_pipeline);
        lifetime_pipelines.push_back(grid_pipeline);

        //std::vector<pipeline_handle<object_2d_pipeline<>>> a = hud_info.get_pipelines();
        //world_processor.register_pipelines(a);

        world_processor.register_pipeline(rainbow_pipeline);
        world_processor.register_pipeline(rect_pipeline);
        world_processor.register_pipeline(text_line_manager.get_pipeline());
        world_processor.register_pipeline(text_pipeline);
        world_processor.register_pipeline(grid_pipeline);
        world_processor.register_pipelines(crate_sim.get_pipelines());

        rainbow_a = rainbow_pipeline.create_draw_obj();
        rainbow_b = rainbow_pipeline.create_draw_obj();
        rect_1 = rect_pipeline.create_draw_obj();
        rect_2 = rect_pipeline.create_draw_obj();
        rect_3 = rect_pipeline.create_draw_obj();

        text_M_handle = text_pipeline.create_draw_obj();
        text_K_handle = text_pipeline.create_draw_obj();

        visual_grid_handle[0] = grid_pipeline.create_draw_obj();
        visual_grid_handle[1] = grid_pipeline.create_draw_obj();
        visual_grid_handle[2] = grid_pipeline.create_draw_obj();
        visual_grid_handle[3] = grid_pipeline.create_draw_obj();


        crate_sim.add_crate(glm::vec2(0, 0), 50, 'G', ray_colors::cyan, ray_colors::green);
        crate_sim.add_crate(glm::vec2(100, 100), 40, 'T', ray_colors::blue, ray_colors::red);
        crate_sim.add_crate(glm::vec2(100, -100), 30, 'A', ray_colors::maroon, ray_colors::pink);
        crate_sim.add_crate(glm::vec2(-100, 100), 20, '6', ray_colors::lime, ray_colors::orange);


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
                .transform = glm::vec4(400, 200, 0, 30), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
                .z_order = 10,
                .text_color = ray_colors::alpha(ray_colors::green, 0.1),
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

        //         in sync   in sync
        // screen  pure[0]   cam[1]
        // world   cam[2]    pure[3]

        // if (auto visual_grid_data = visual_grid_handle[0].access_draw_obj_data()) {
        //         visual_grid_data->space_basis = e_space_type::screen;
        //         visual_grid_data->z_order = 1;
        //         visual_grid_data->pivot_offset_ndc = {0, 0, 0.9, 0.9};//glm::vec4(0.1, 0.1, 0.5, 0.5);
        //         visual_grid_data->transform = glm::vec4(0, 0, 0, 0);
        //         visual_grid_data->color = ray_colors::lime;
        //         visual_grid_data->background_color = ray_colors::alpha(ray_colors::cyan, 0.05);
        //         visual_grid_data->grid_size_px = 50;
        //         visual_grid_data->line_size_px = 10;
        //         visual_grid_data->apply_camera_to_frag = true;
        // }
        //
        // if (auto visual_grid_data = visual_grid_handle[1].access_draw_obj_data()) {
        //         visual_grid_data->space_basis = e_space_type::screen;
        //         visual_grid_data->z_order = 1;
        //         visual_grid_data->pivot_offset_ndc = {};//glm::vec4(0.1, 0.1, 0.5, 0.5);
        //         visual_grid_data->transform = glm::vec4(20, -170, 200, 150);
        //         visual_grid_data->color = ray_colors::orange;
        //         visual_grid_data->background_color = ray_colors::alpha(ray_colors::cyan, 0.05);
        //         visual_grid_data->grid_size_px = 50;
        //         visual_grid_data->line_size_px = 5;
        //         visual_grid_data->apply_camera_to_frag = true;
        // }
        //
        // if (auto visual_grid_data = visual_grid_handle[2].access_draw_obj_data()) {
        //         visual_grid_data->space_basis = e_space_type::world;
        //         visual_grid_data->z_order = 1;
        //         visual_grid_data->pivot_offset_ndc = {};//glm::vec4(0.1, 0.1, 0.5, 0.5);
        //         visual_grid_data->transform = glm::vec4(-220, 20, 200, 150);
        //         visual_grid_data->color = ray_colors::lime;
        //         visual_grid_data->background_color = ray_colors::alpha(ray_colors::navy, 0.05);
        //         visual_grid_data->grid_size_px = 50;
        //         visual_grid_data->line_size_px = 5;
        //         visual_grid_data->apply_camera_to_frag = true;
        // }
        //
        // if (auto visual_grid_data = visual_grid_handle[3].access_draw_obj_data()) {
        //         visual_grid_data->space_basis = e_space_type::world;
        //         visual_grid_data->z_order = 1;
        //         visual_grid_data->pivot_offset_ndc = {};//glm::vec4(0.1, 0.1, 0.5, 0.5);
        //         visual_grid_data->transform = glm::vec4(-20, -20, 200, 150);
        //         visual_grid_data->color = ray_colors::orange;
        //         visual_grid_data->background_color = ray_colors::alpha(ray_colors::navy, 0.05);
        //         visual_grid_data->grid_size_px = 50;
        //         visual_grid_data->line_size_px = 5;
        //         visual_grid_data->apply_camera_to_frag = false;
        // }

        // if (auto text_1_handle_data = text_M_handle.access_draw_obj_data()) {
        //         text_1_handle_data->content_glyph = 'h';
        //         text_1_handle_data->text_outline_size_px = 10.0f;
        //         text_1_handle_data->text_outline_color = ray_colors::red;
        //         text_1_handle_data->background_color = ray_colors::blue;
        //         text_1_handle_data->space_basis = e_space_type::world;
        //         text_1_handle_data->z_order = 10;
        //         text_1_handle_data->transform = glm::vec4(0, 0, 150, 150); // x_pos, y_pos, x_size, y_size
        //         text_1_handle_data->color = ray_colors::cyan;
        // }
        //
        //  if (auto text_K_handle_data = text_K_handle.access_draw_obj_data()) {
        //          text_K_handle_data->content_glyph = 'h';
        //          text_K_handle_data->text_outline_size_px = 8.0f;
        //          text_K_handle_data->text_outline_color = ray_colors::solid(ray_colors::red);
        //          text_K_handle_data->background_color = ray_colors::alpha(ray_colors::cyan, 0.1);
        //          text_K_handle_data->space_basis = e_space_type::screen;
        //          text_K_handle_data->z_order = 2;
        //          text_K_handle_data->transform = glm::vec4(180, 80, 80, 80); // x_pos, y_pos, x_size, y_size
        //          text_K_handle_data->color = ray_colors::alpha(ray_colors::yellow, 0.1);
        //  }

        if (auto rainbow_a_data = rainbow_a.access_draw_obj_data()) {
                rainbow_a_data->space_basis = e_space_type::screen;
                rainbow_a_data->z_order = 100;
                rainbow_a_data->transform = glm::vec4(0, 0, 10, 10);
                rainbow_a_data->color = ray_colors::transparent;
        }

        if (auto rainbow_b_data = rainbow_b.access_draw_obj_data()) {
                rainbow_b_data->space_basis = e_space_type::world;
                rainbow_b_data->z_order = 200;
                rainbow_b_data->transform = glm::vec4(0, 0, 10, 10);
                rainbow_b_data->color = ray_colors::transparent;
        }

        // if (auto rect_1_data = rect_1.access_draw_obj_data()) {
        //         rect_1_data->space_basis = e_space_type::world;
        //         rect_1_data->z_order = 3;
        //         //transform_dyn_1 = glm::vec4(0, 0, 100, 100);
        //         rect_1_data->transform = glm::vec4(0, 0, 100, 100);
        //         //rect_1_data->pivot_offset_ndc = glm::vec4(0, 0, 1, 1);
        //         rect_1_data->color = ray_colors::blue;
        // }

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
        const bool is_success = base_scene::tick(win, pipe);
        if (!is_success) {
                return is_success;
        }


        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        crate_sim.tick(new_cam_transform, win, pipe);

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
        base_scene::cleanup(win, pipe);

        text_line_manager.destroy(pipe);

        for (auto p : lifetime_pipelines) {
                pipe.destroy_pipeline(p);
        }

        crate_sim.destroy(pipe);
}
