#pragma once
#include "logical_text_line.h"

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

class window;
class pipeline_manager;

class logical_hud_info {
public:
        ray_error init(window& win, pipeline_manager& pipe);
        void tick(window& win, pipeline_manager& pipe);
        void destroy(window& win, pipeline_manager& pipe);

        pipeline_handle<object_2d_pipeline<>> get_pipeline();
        void update_camera_transform(glm::vec4 new_cam);

private:
        logical_text_line_manager text_line_manager;
        logical_text_line_handler fps_text_line = nullptr;
        logical_text_line_handler cam_text_line = nullptr;
        logical_text_line_handler mouse_text_line = nullptr;

        std::optional<glm::vec4> camera_transform;
        glm::u64 last_time_ns = 0; // TODO: move fps and other HUD stats to logical_system
        glm::u64 last_delta_time_ns = 0;
};

#endif
};