#include "logical_hud_info.h"

#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "utils/ray_colors.h"
#include "utils/ray_error.h"
#include "utils/ray_time.h"

#if RAY_GRAPHICS_ENABLE

using namespace ray;
using namespace ray::graphics;

//
// fps [avg, min | max] / 0.1 sec: [ 1324.3, 2312.8 | 3233.5 ] (0.0210 ms frame)
// cam: world[ 1023.3, 123.2 ]
// mouse: screen[ 1023.3, 123.2 ] | world[ 43323.2, 343.8 ]

ray_error logical_hud_info::init(window& win, pipeline_manager& pipe) {
        ray_error manager_error = text_line_manager.init(pipe, 100500);
        if (manager_error.has_value()) {
                return manager_error;
        }

        fps_text_line = text_line_manager.create_text_line ( {
                        .content_text = "hell",
                        .static_capacity = 128,
                        .space_basis = e_space_type::screen,
                        .transform = glm::vec4(0, 0, 0, 12), // x_pos_px, y_pos_px, 0, y_size_px (pivot top left) / 8
                        .z_order = 10,
                        .text_color = ray_colors::solid(ray_colors::green),
                        .outline_size_px = 0.3f,
                        .outline_color = ray_colors::solid(ray_colors::black),
                        .background_color = ray_colors::transparent,
                        .pivot_offset_ndc = glm::vec2(-1.f, -1.f),
                });

        return {};
}


void logical_hud_info::tick(window& win, pipeline_manager& pipe) {
        glm::u64 curr_time_ns = now_ticks_ns();
        last_delta_time_ns = curr_time_ns - last_time_ns;
        last_time_ns = curr_time_ns;

        if (!!fps_text_line) {
                std::chrono::duration<glm::u64, std::nano> ns_duration {last_delta_time_ns};
                double sec_duration = std::chrono::duration_cast<std::chrono::duration<double>>(ns_duration).count();
                double fps = 1.f / sec_duration;
                const glm::vec4 cam_vec = camera_transform ? *camera_transform : glm::vec4();
                const std::string fps_ms_str = std::format("print('Hello Google!') {}:{}:{} | delta ms: {:.3f}, fps: {:.1f}", cam_vec.x, cam_vec.y, cam_vec.z, (float)sec_duration * 1000.f, (float)fps);
                fps_text_line->update_content(fps_ms_str);
        }
}

void logical_hud_info::destroy(window& win, pipeline_manager& pipe) {
        text_line_manager.destroy(pipe);
        fps_text_line = nullptr;
}


pipeline_handle<object_2d_pipeline<>> logical_hud_info::get_pipeline() {
        return text_line_manager.get_pipeline();
}


void logical_hud_info::update_camera_transform(glm::vec4 new_cam) {
        camera_transform = new_cam;
}

#endif