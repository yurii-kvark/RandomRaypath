#pragma once
#include "../object_2d_pipeline.h"


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE


struct rainbow_rect_pipeline_data_model {
        struct pipeline : object_2d_pipeline_data_model::pipeline {
        };

        struct draw_obj : object_2d_pipeline_data_model::draw_obj {
        };

        using pipe2d_frame_ubo = object_2d_pipeline_data_model::pipe2d_frame_ubo;
        using pipe2d_draw_obj_ssbo = object_2d_pipeline_data_model::pipe2d_draw_obj_ssbo;
};

class rainbow_rect_pipeline final : public object_2d_pipeline<rainbow_rect_pipeline_data_model> {
public:
        using object_2d_pipeline::object_2d_pipeline;

        virtual std::filesystem::path get_fragment_shader_path() const override;
};

#endif
};