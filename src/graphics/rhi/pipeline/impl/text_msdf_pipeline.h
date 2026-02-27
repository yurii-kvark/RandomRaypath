#pragma once
#include "../object_2d_pipeline.h"


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE


struct text_msdf_pipeline_data_model {
        struct pipeline : object_2d_pipeline_data_model::pipeline {
        };

        struct draw_obj : object_2d_pipeline_data_model::draw_obj {
                glm::f32 text_size = 1.f; // in px height
                glm::f32 text_weight = 1.f;
                glm::f32 text_outline_size = 1.f; // in px
                glm::vec4 text_outline_color {};
                glm::vec4 background_color {};

                bool content_need_update = true;
                std::u8string text_content;
        };

        using pipe2d_frame_ubo = object_2d_pipeline_data_model::pipe2d_frame_ubo;

        struct alignas(16) pipe2d_draw_obj_ssbo {
                glm::vec4 transform_ndc = {}; // x_ndc, y_ndc, w_ndc, h_ndc
                glm::vec4 uv_rect; // u0,v0,u1,v1 in atlas UV
                glm::vec4 color = {};
                glm::u32 space_basis = 0.f;
                glm::f32 weight = 0.f;
                glm::f32 outline_size = 0.f;
                glm::i32 _pad0 = 0;

                glm::vec4 outline_color;
                glm::vec4 background_color;
        };
};

class text_msdf_pipeline final : public object_2d_pipeline<text_msdf_pipeline_data_model> {
public:
        using object_2d_pipeline::object_2d_pipeline;
public:
        virtual void update_render_obj(glm::u32 frame_index, bool dirty_update) override;

protected:
        virtual std::filesystem::path get_vertex_shader_path() const override;
        virtual std::filesystem::path get_fragment_shader_path() const override;
};

#endif
};