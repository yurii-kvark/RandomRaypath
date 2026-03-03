#pragma once
#include "i_logical_scene.h"
#include "graphics/engine/logical_system/logical_2d_world_view.h"
#include "graphics/engine/logical_system/logical_hud_info.h"
#include "graphics/engine/logical_system/logical_text_line.h"
#include "graphics/rhi/pipeline/impl/glyph_pipeline.h"
#include "graphics/rhi/pipeline/impl/rainbow_rect_pipeline.h"
#include "graphics/rhi/pipeline/impl/solid_rect_pipeline.h"

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE
class window;
class renderer;

class dev_test_scene : public i_logical_scene {
public:
        virtual ray_error init(window& win, pipeline_manager& pipe) override;
        virtual bool tick(window& win, pipeline_manager& pipe) override;
        virtual void cleanup(window& win, pipeline_manager& pipe) override;
public:
        //void tick_camera_movement(window& win, const pipeline_manager& pipe); // todo: move camera to logical_system

        logical_2d_world_view world_processor;
        logical_hud_info hud_info;

        glm::vec4 transform_dyn_1 = {};
        glm::vec4 transform_dyn_2 = {};

        std::vector<pipeline_handle<i_pipeline>> lifetime_pipelines;

        draw_obj_handle<rainbow_rect_pipeline> rainbow_a;
        draw_obj_handle<rainbow_rect_pipeline> rainbow_b;
        draw_obj_handle<solid_rect_pipeline> rect_1;
        draw_obj_handle<solid_rect_pipeline> rect_2;
        draw_obj_handle<solid_rect_pipeline> rect_3;

        draw_obj_handle<glyph_pipeline> text_M_handle;
        draw_obj_handle<glyph_pipeline> text_K_handle;

        glm::u64 last_time_ns = 0; // TODO: move fps and other HUD stats to logical_system
        glm::u64 last_delta_time_ns = 0;

        //glm::vec4 camera_transform = {0, 0, 1, 0};
        //std::optional<glm::vec2> base_move_position = std::nullopt;

        logical_text_line_manager text_line_manager;
        logical_text_line_handler new_line_1 = nullptr;
        logical_text_line_handler new_line_2 = nullptr;
        logical_text_line_handler new_line_3 = nullptr;
};

#endif
};