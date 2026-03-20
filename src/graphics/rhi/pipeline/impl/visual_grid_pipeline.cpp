#include "visual_grid_pipeline.h"

using namespace ray::graphics;
using namespace ray;


void visual_grid_pipeline::update_render_obj(const typename visual_grid_pipeline_data_model::draw_obj& inout_draw_data, typename visual_grid_pipeline_data_model::pipe2d_draw_obj_ssbo& inout_ssbo_obj) {
        object_2d_pipeline::update_render_obj(inout_draw_data, inout_ssbo_obj);

        inout_ssbo_obj.background_color = inout_draw_data.background_color;
        inout_ssbo_obj.apply_camera_to_frag = inout_draw_data.apply_camera_to_frag;

        { // convert size_ndc in transform_ndc space
                const glm::vec2 scale_size_ndc = {inout_ssbo_obj.transform_ndc.z, inout_ssbo_obj.transform_ndc.w};
                const glm::vec2 scale_size_px = (glm::vec2)this->resolution * scale_size_ndc;

                inout_ssbo_obj.grid_size_ndc = inout_draw_data.grid_size_px / scale_size_px;
                inout_ssbo_obj.line_size_ndc = inout_draw_data.line_size_px / scale_size_px;
        }
}


std::filesystem::path visual_grid_pipeline::get_vertex_shader_path() const {
        return "visual_grid.vert.spv";
}


std::filesystem::path visual_grid_pipeline::get_fragment_shader_path() const {
        return "visual_grid.frag.spv";
}

