#pragma once
#include "logical_text_line.h"
#include "graphics/rhi/pipeline/impl/solid_rect_pipeline.h"

namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace graphics;

class logical_hud_info {
public:
        static constexpr double fps_smooth_sec = 0.5;
        static constexpr double fps_maxmin_delay_sec = 1;

        ray_error init(window& win, pipeline_manager& pipe, glm::vec4 text_color);
        void tick(window& win, pipeline_manager& pipe);
        void destroy(window& win, pipeline_manager& pipe);

        void update_camera_transform_info(glm::vec4 new_cam);

        std::string get_last_full_text() const;

        int frame_counter = 0;

private:
        logical_text_line_manager text_line_manager;
        logical_text_line_handler fps_text_line = nullptr;
        logical_text_line_handler cam_text_line = nullptr;
        logical_text_line_handler mouse_text_line = nullptr;
        logical_text_line_handler frame_text_line = nullptr;

        pipeline_handle<solid_rect_pipeline> background_rect_pipeline;
        draw_obj_handle<solid_rect_pipeline> background_fps_obj;

        std::optional<glm::vec4> camera_transform;
        glm::u64 last_time_ns = 0;
        glm::u64 last_delta_time_ns = 0;


        double smoothed_fps = 0.0;
        double fps_delay_reset_sec = 0.0;
        double max_fps = 0.0;
        double min_fps = 0.0;
        double collecting_max_fps = 0.0;
        double collecting_min_fps = 0.0;

        std::string last_full_text;
};
};