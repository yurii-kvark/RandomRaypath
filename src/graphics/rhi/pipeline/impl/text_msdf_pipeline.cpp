#include "text_msdf_pipeline.h"


void ray::graphics::text_msdf_pipeline::update_render_obj(glm::u32 frame_index, bool dirty_update) {

}


std::filesystem::path ray::graphics::text_msdf_pipeline::get_vertex_shader_path() const {
        return "text_msdf.vert.spv";
}


std::filesystem::path ray::graphics::text_msdf_pipeline::get_fragment_shader_path() const {
        return "text_msdf.frag.spv";
}