#include "logical_2d_world_view.h"

#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/pipeline.h"
#include "graphics/window/window.h"
#include "utils/ray_time.h"
#include <algorithm>

using namespace ray;
using namespace ray::logical;
using namespace ray::graphics;

void logical_2d_world_view::tick(window& win, pipeline_manager& pipe) {

        tick_camera_transform(win, pipe);

        const glm::u64 last_time_ns = now_ticks_ns();
        const glm::u32 last_time_ms = static_cast<glm::u32>(last_time_ns / 1'000'000);
        bool need_cleanup = false;

        for (auto& world_pipeline : world_2d_pipes) {
                auto world_data = world_pipeline.access_pipeline_data();
                if (!world_data) {
                        need_cleanup = true;
                        continue;
                }

                world_data->time_ms = last_time_ms;
                world_data->camera_transform = camera_transform;
        }

        if (need_cleanup) {
                std::erase_if(world_2d_pipes, [](const pipeline_handle<object_2d_pipeline<>>& pipe) {
                        return !pipe.is_valid();
                });
        }
}


void logical_2d_world_view::register_pipeline(const pipeline_handle<object_2d_pipeline<>>& pipe_to_register) {
        world_2d_pipes.push_back(pipe_to_register);
}


void logical_2d_world_view::register_pipelines(const std::vector<pipeline_handle<object_2d_pipeline<>>>& pipes_to_register) {
        for (auto& to_register : pipes_to_register) {
                register_pipeline(to_register);
        }
}


void logical_2d_world_view::unregister_pipeline(const pipeline_handle<object_2d_pipeline<>>& pipe_to_unregister) {
        world_2d_pipes.push_back(pipe_to_unregister);
}


glm::vec4 logical_2d_world_view::get_camera_transform() const {
        return camera_transform;
}


void logical_2d_world_view::tick_camera_transform(window& win, const pipeline_manager& pipe) {
        glm::vec2 curr_mouse_pos = win.get_mouse_position();
        glm::f32 delta_zoom = win.get_mouse_wheel_delta();

        if (std::abs(delta_zoom) > 0.0001) {
                glm::vec2 viewport_px = (glm::vec2)pipe.get_target_resolution();
                glm::vec2 world_mouse_before = (curr_mouse_pos - viewport_px * 0.5f) * (1.0f / camera_transform.z);

                delta_zoom = std::exp(delta_zoom);
                camera_transform.z *= delta_zoom;
                camera_transform.z = std::clamp(camera_transform.z, 0.004f, 120.f);

                glm::vec2 world_mouse_after = (curr_mouse_pos - viewport_px * 0.5f) * (1.0f / camera_transform.z);

                glm::vec2 delta_zoom_move = world_mouse_before - world_mouse_after;

                camera_transform.x += delta_zoom_move.x;
                camera_transform.y += delta_zoom_move.y;
        }

        glm::vec2 delta_move = glm::vec2(0, 0);
        if (win.get_mouse_button_right()) {
                if (base_move_position) {
                        delta_move = *base_move_position - curr_mouse_pos;
                }
                base_move_position = curr_mouse_pos;
        } else {
                base_move_position = std::nullopt;
        }

        delta_move /= camera_transform.z;
        delta_move *= 1;

        camera_transform.x += delta_move.x;
        camera_transform.y += delta_move.y;
}