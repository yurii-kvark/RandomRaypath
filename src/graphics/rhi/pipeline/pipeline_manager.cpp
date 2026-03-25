#include "pipeline_manager.h"
#include "graphics/graphic_libs.h"

#include <memory>

using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE


void pipeline_manager::renderer_set_swapchain_format(VkFormat in_swapchain_format, glm::uvec2 in_resolution) {
        swapchain_format = in_swapchain_format;
        resolution = in_resolution;

        for (auto& pipe : pipe_instances) {
                if (pipe) {
                        pipe->update_swapchain(in_swapchain_format, resolution);
                }
        }
}


void pipeline_manager::renderer_perform_draw(VkCommandBuffer command_buffer, glm::u32 frame_index) {
        for (auto& pipe : pipe_instances) {
                if (pipe) {
                        pipe->draw_commands(command_buffer, frame_index);
                }
        }
}


void pipeline_manager::renderer_shutdown() {
        pipe_instances.clear();
}


glm::uvec2 pipeline_manager::get_target_resolution() const {
        return resolution;
}

#if RAY_DEBUG_NO_OPT
void pipeline_manager::verify_pipeline_destruction() {
        if (!pipe_instances.empty()) {
                ray_log(e_log_type::fatal, "destruction of pipelines did not processed correctly. '{}' pipelines is still alive.", pipe_instances.size());
        }
}
#endif

#endif