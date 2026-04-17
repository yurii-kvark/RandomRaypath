#pragma once
#include "graphics/rhi/pipeline/impl/solid_rect_pipeline.h"
#include "config/config.h"
#include "utils/ray_error.h"

namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace graphics;

class logical_cursor_visualize {
public:
        ray_error init(window& win, pipeline_manager& pipe, config::render_server_config cfg);
        void tick(window& win, pipeline_manager& pipe);
        void destroy(window& win, pipeline_manager& pipe);

private:
        pipeline_handle<solid_rect_pipeline> cursor_pipeline;
        draw_obj_handle<solid_rect_pipeline> border_rect;
        draw_obj_handle<solid_rect_pipeline> inner_rect;

        config::render_server_config cfg{};
};

};
