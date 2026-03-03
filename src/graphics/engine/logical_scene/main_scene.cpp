
#include "main_scene.h"

#include "utils/ray_log.h"

using namespace ray;
using namespace ray::graphics;


#if RAY_GRAPHICS_ENABLE


ray_error main_scene::init(window& win, pipeline_manager& pipe) {
        ray_error hud_error = hud_info.init(win, pipe);
        if (hud_error) {
                return hud_error;
        }

       // world_processor.register_pipelines(hud_info.get_pipelines());

        return {};
}

bool main_scene::tick(window& win, pipeline_manager& pipe) {
        world_processor.tick(win, pipe);

        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        hud_info.update_camera_transform(new_cam_transform);

        hud_info.tick(win, pipe);
        return true;
}


void main_scene::cleanup(window& win, pipeline_manager& pipe) {
        hud_info.destroy(win, pipe);
}

#endif