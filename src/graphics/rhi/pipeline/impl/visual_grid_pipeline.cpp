#include "visual_grid_pipeline.h"

using namespace ray::graphics;
using namespace ray;


void visual_grid_pipeline::update_render_obj(const typename visual_grid_pipeline_data_model::draw_obj& inout_draw_data, typename visual_grid_pipeline_data_model::pipe2d_draw_obj_ssbo& inout_ssbo_obj) {
        object_2d_pipeline::update_render_obj(inout_draw_data, inout_ssbo_obj);

        inout_ssbo_obj.background_color = inout_draw_data.background_color;
        inout_ssbo_obj.apply_camera_to_frag = inout_draw_data.apply_camera_to_frag;

        const glm::vec2 pivot_scale_offset = {inout_draw_data.pivot_offset_ndc.z, inout_draw_data.pivot_offset_ndc.w};
        const glm::vec2 raw_transform_px = {inout_draw_data.transform.z ,inout_draw_data.transform.w };
        const glm::vec2 actual_size_px = raw_transform_px * (pivot_scale_offset + 1.f);

        inout_ssbo_obj.grid_size_ndc = inout_draw_data.grid_size_px / actual_size_px;
        inout_ssbo_obj.line_size_ndc = inout_draw_data.line_size_px / actual_size_px;
}


std::filesystem::path visual_grid_pipeline::get_vertex_shader_path() const {
        return "visual_grid.vert.spv";
}


std::filesystem::path visual_grid_pipeline::get_fragment_shader_path() const {
        return "visual_grid.frag.spv";
}

