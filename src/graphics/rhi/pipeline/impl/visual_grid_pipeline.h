#pragma once
#include "../object_2d_pipeline.h"


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE


struct visual_grid_pipeline_data_model {
        struct pipeline : object_2d_pipeline_data_model::pipeline {
        };

        struct draw_obj : object_2d_pipeline_data_model::draw_obj {
                glm::vec4 background_color {};
                glm::f32 grid_size_px;
                glm::f32 line_size_px;
                bool apply_camera_to_frag;
        };

        using pipe2d_frame_ubo = object_2d_pipeline_data_model::pipe2d_frame_ubo;

        struct alignas(16) pipe2d_draw_obj_ssbo {
                glm::vec4 transform_ndc = {}; // x_ndc, y_ndc, w_ndc, h_ndc
                glm::vec4 color = {};
                glm::vec4 background_color {};
                glm::vec2 grid_size_ndc; // by w (y-scale)
                glm::vec2 line_size_ndc; // by w (y-scale)
                glm::u32 space_basis = 0;
                glm::u32 apply_camera_to_frag = 0;
        };
};

class visual_grid_pipeline final : public object_2d_pipeline<visual_grid_pipeline_data_model> {
public:
        using object_2d_pipeline::object_2d_pipeline;

        virtual void update_render_obj(const typename visual_grid_pipeline_data_model::draw_obj& inout_draw_data, typename visual_grid_pipeline_data_model::pipe2d_draw_obj_ssbo& inout_ssbo_obj) override;

protected:
        virtual std::filesystem::path get_vertex_shader_path() const override;
        virtual std::filesystem::path get_fragment_shader_path() const override;
};

#endif
};
