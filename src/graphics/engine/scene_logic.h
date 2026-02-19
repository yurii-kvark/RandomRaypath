#pragma once
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/pipeline.h"
#include "graphics/rhi/pipeline/impl/rainbow_rect_pipeline.h"
#include "graphics/rhi/pipeline/impl/solid_rect_pipeline.h"

#include <vector>


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE
class window;
class renderer;

class scene_logic {
public:
        scene_logic(window& win, renderer& rend);
        ~scene_logic();

        bool tick(window& win, renderer& rend);

        void cleanup(window& win, renderer& rend);

public:
        void tick_camera_movement(window& win, renderer& rend);

        glm::vec4 transform_dyn_3 = {};
        glm::vec4 transform_dyn_4 = {};

        std::vector<pipeline_handle<base_pipeline<>>> all_pipelines;
        std::vector<pipeline_handle<object_2d_pipeline<>>> world_pipelines;

        draw_obj_handle<rainbow_rect_pipeline> rainbow_1_screen;
        draw_obj_handle<rainbow_rect_pipeline> rainbow_2_screen;
        draw_obj_handle<solid_rect_pipeline> rect_3_dyn_world;
        draw_obj_handle<solid_rect_pipeline> rect_4_dyn_world;
        draw_obj_handle<solid_rect_pipeline> rect_5_world;

        glm::u64 last_time_ns = 0;
        glm::u64 last_delta_time_ns = 0;

        glm::vec4 camera_transform = {0, 0, 1, 0};
        std::optional<glm::vec2> base_move_position = std::nullopt;
};

#endif
};