#pragma once
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/pipeline.h"

namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace graphics;

// handling camera and timing
class logical_2d_world_view {
public:
        void tick(window& win, pipeline_manager& rend);

        // will be auto cleared
        void register_pipeline(const graphics::pipeline_handle<graphics::object_2d_pipeline<>>& pipe_to_register);
        void register_pipelines(const std::vector<graphics::pipeline_handle<graphics::object_2d_pipeline<>>>& pipes_to_register);
        void unregister_pipeline(const graphics::pipeline_handle<graphics::object_2d_pipeline<>>& pipe_to_unregister);

        glm::vec4 get_camera_transform() const;

private:
        void tick_camera_transform(window& win, const pipeline_manager& pipe);

        std::vector<graphics::pipeline_handle<graphics::object_2d_pipeline<>>> world_2d_pipes;
        glm::vec4 camera_transform = {0, 0, 1, 0};
        std::optional<glm::vec2> base_move_position = std::nullopt;
};

};