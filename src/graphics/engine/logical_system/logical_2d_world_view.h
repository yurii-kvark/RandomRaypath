#pragma once
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/pipeline.h"

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

class window;
class pipeline_manager;

// handling camera and timing
class logical_2d_world_view {
public:
        void tick(window& win, pipeline_manager& rend);

        // will be auto cleared
        void register_pipeline(const pipeline_handle<object_2d_pipeline<>>& pipe_to_register);
        void unregister_pipeline(const pipeline_handle<object_2d_pipeline<>>& pipe_to_unregister);

        glm::vec4 get_camera_transform() const;

private:
        void tick_camera_transform(window& win, const pipeline_manager& pipe);

        std::vector<pipeline_handle<object_2d_pipeline<>>> world_2d_pipes;
        glm::vec4 camera_transform = {0, 0, 1, 0};
        std::optional<glm::vec2> base_move_position = std::nullopt;
};

#endif
};