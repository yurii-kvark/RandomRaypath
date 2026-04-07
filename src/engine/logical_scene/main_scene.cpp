
#include "main_scene.h"

#include "utils/ray_log.h"

using namespace ray;
using namespace ray::graphics;
using namespace ray::logical;


ray_error main_scene::init(window& win, pipeline_manager& pipe) {
        ray_error hud_error = hud_info.init(win, pipe, style.color_hud_info);
        if (hud_error) {
                return hud_error;
        }

        ray_error grid_error = grid_system.init(win, pipe, style.color_grid);
        if (grid_error) {
                return grid_error;
        }

        world_processor.register_pipeline(grid_system.get_pipeline());

        return {};
}

bool main_scene::tick(window& win, pipeline_manager& pipe) {
        world_processor.tick(win, pipe);

        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        hud_info.update_camera_transform_info(new_cam_transform);

        hud_info.tick(win, pipe);
        return true;
}


void main_scene::cleanup(window& win, pipeline_manager& pipe) {
        hud_info.destroy(win, pipe);
        grid_system.destroy(win, pipe);
}