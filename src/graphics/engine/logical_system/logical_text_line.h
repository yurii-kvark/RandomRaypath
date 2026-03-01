#pragma once
#include "glm/fwd.hpp"
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/impl/glyph_pipeline.h"

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

class pipeline_manager;

struct logical_text_line_args {
        std::string_view content_text;
        glm::u32 static_capacity = 16;
        e_space_type space_basis = e_space_type::screen;
        glm::vec4 transform = glm::vec4(0.f, 0.f, 0.f, 50.f); // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
        glm::u32 z_order = 0;
        glm::vec4 text_color = glm::vec4(1);
        glm::f32 outline_size_ndc = 0.1f;
        glm::vec4 outline_color = glm::vec4(0.5f, 1.f, 0.5f, 1.f);
        glm::vec4 background_color = glm::vec4(0);
};


class logical_text_line {
public:
        void update_content(std::string_view in_new_content);

        void init(const glyph_font_data_loader& data_loader, const pipeline_handle<glyph_pipeline>& in_pipe, logical_text_line_args in_args);
        void destroy();

protected:
        pipeline_handle<glyph_pipeline> pipe {};
        std::vector<draw_obj_handle_id> draw_obj_handles {};
};


using logical_text_line_handler = std::shared_ptr<logical_text_line>;

class logical_text_line_manager {
public:
        logical_text_line_manager() = default;
        ~logical_text_line_manager() = default;

        void init(pipeline_manager& pipe, glm::u32 render_order);
        void destroy(pipeline_manager& pipe);

        pipeline_handle<object_2d_pipeline<>> get_pipeline();

        logical_text_line_handler create_text_line(logical_text_line_args in_args);

protected:
        pipeline_handle<glyph_pipeline> text_pipeline_handle {};
        std::shared_ptr<glyph_font_data_loader> data_loader = nullptr;
};

#endif
};