#include "logical_grid.h"

#include "graphics/rhi/pipeline/pipeline_manager.h"
#include "utils/ray_visual_config.h"

#include <optional>

using namespace ray;
using namespace ray::logical;
using namespace ray::graphics;


ray_error logical_grid::init(window& win, pipeline_manager& pipe, glm::vec4 grid_color){
        grid_pipeline_handle = pipe.create_pipeline<visual_grid_pipeline>(ray_pipeline_order::grid);

        grid_lvl_1 = grid_pipeline_handle.create_draw_obj();
        grid_lvl_2 = grid_pipeline_handle.create_draw_obj();
        grid_lvl_3 = grid_pipeline_handle.create_draw_obj();

        if (auto visual_grid_data = grid_lvl_1.access_draw_obj_data()) {
                visual_grid_data->space_basis = e_space_type::screen;
                visual_grid_data->z_order = 1;
                visual_grid_data->pivot_offset_ndc = {0, 0, 1, 1};
                visual_grid_data->transform = glm::vec4(0, 0, 0, 0);
                visual_grid_data->color = grid_color;
                visual_grid_data->background_color = ray_colors::alpha(grid_color, 0);
                visual_grid_data->grid_size_px = grid_size_lvl_1;
                visual_grid_data->line_size_px = grid_line_lvl_1;
                visual_grid_data->apply_camera_to_frag = true;
        }

        if (auto visual_grid_data = grid_lvl_2.access_draw_obj_data()) {
                visual_grid_data->space_basis = e_space_type::screen;
                visual_grid_data->z_order = 1;
                visual_grid_data->pivot_offset_ndc = {0, 0, 1, 1};
                visual_grid_data->transform = glm::vec4(0, 0, 0, 0);
                visual_grid_data->color = grid_color;
                visual_grid_data->background_color = ray_colors::alpha(grid_color, 0);
                visual_grid_data->grid_size_px = grid_size_lvl_2;
                visual_grid_data->line_size_px = grid_line_lvl_2;
                visual_grid_data->apply_camera_to_frag = true;
        }

        if (auto visual_grid_data = grid_lvl_3.access_draw_obj_data()) {
                visual_grid_data->space_basis = e_space_type::screen;
                visual_grid_data->z_order = 1;
                visual_grid_data->pivot_offset_ndc = {0, 0, 1, 1};
                visual_grid_data->transform = glm::vec4(0, 0, 0, 0);
                visual_grid_data->color = grid_color;
                visual_grid_data->background_color = ray_colors::alpha(grid_color, 0);
                visual_grid_data->grid_size_px = grid_size_lvl_3;
                visual_grid_data->line_size_px = grid_line_lvl_3;
                visual_grid_data->apply_camera_to_frag = true;
        }

        return {};
}


void logical_grid::destroy(window& win, pipeline_manager& pipe){
        pipe.destroy_pipeline(grid_pipeline_handle);
}


pipeline_handle<object_2d_pipeline<>> logical_grid::get_pipeline(){
        return grid_pipeline_handle;
}
