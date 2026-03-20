#pragma once
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/impl/visual_grid_pipeline.h"
#include "utils/ray_error.h"

#include <vector>

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

class window;
class pipeline_manager;

class logical_grid {
public:
        static constexpr double grid_size_lvl_1 = 10;
        static constexpr double grid_line_lvl_1 = 0.5;

        static constexpr double grid_size_lvl_2 = 100;
        static constexpr double grid_line_lvl_2 = 2;

        static constexpr double grid_size_lvl_3 = 1000;
        static constexpr double grid_line_lvl_3 = 5;

        ray_error init(window& win, pipeline_manager& pipe, glm::vec4 grid_color);
        void destroy(window& win, pipeline_manager& pipe);

        pipeline_handle<object_2d_pipeline<>> get_pipeline();

private:
        pipeline_handle<visual_grid_pipeline> grid_pipeline_handle {};

        draw_obj_handle<visual_grid_pipeline> grid_lvl_1;
        draw_obj_handle<visual_grid_pipeline> grid_lvl_2;
        draw_obj_handle<visual_grid_pipeline> grid_lvl_3;
};

#endif
};
