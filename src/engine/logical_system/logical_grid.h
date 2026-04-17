#pragma once
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/impl/visual_grid_pipeline.h"
#include "utils/ray_error.h"


namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

class logical_grid {
public:
        static constexpr double grid_size_lvl_1 = 10;
        static constexpr double grid_line_lvl_1 = 0.3;

        static constexpr double grid_size_lvl_2 = 100;
        static constexpr double grid_line_lvl_2 = 1;

        static constexpr double grid_size_lvl_3 = 1000;
        static constexpr double grid_line_lvl_3 = 3;

        ray_error init(graphics::window& win, graphics::pipeline_manager& pipe, glm::vec4 grid_color);
        void destroy(graphics::window& win, graphics::pipeline_manager& pipe);

        graphics::pipeline_handle<graphics::object_2d_pipeline<>> get_pipeline();

private:
        graphics::pipeline_handle<graphics::visual_grid_pipeline> grid_pipeline_handle {};

        graphics::draw_obj_handle<graphics::visual_grid_pipeline> grid_lvl_1;
        graphics::draw_obj_handle<graphics::visual_grid_pipeline> grid_lvl_2;
        graphics::draw_obj_handle<graphics::visual_grid_pipeline> grid_lvl_3;
};
};
