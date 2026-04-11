
#include "minecraft_scene.h"

#include "engine/logical_system/logical_crate_sim.h"
#include "utils/ray_log.h"

using namespace ray;
using namespace ray::graphics;
using namespace ray::logical;


ray_error minecraft_scene::init(window& win, pipeline_manager& pipe) {
        ray_error init_error = base_scene::init(win, pipe);
        if (init_error) {
                return init_error;
        }

        ray_error crate_error = crate_sim.init(win, pipe);
        if (crate_error) {
                return crate_error;
        }

        world_processor.register_pipelines(crate_sim.get_pipelines());

        crate_sim.add_crate(glm::vec2(-112, -102), 70, 'M', glm::vec4(102.f, 153.f, 51.f, 255.f) / 255.f, glm::vec4(128.f, 93.f, 21.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-30, -82), 50, 'i', glm::vec4(144.f, 191.f, 96.f, 255.f) / 255.f, glm::vec4(18.f, 54.f, 82.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(32, -82), 50, 'n', glm::vec4(67.f, 115.f, 51.f, 19.f) / 255.f, glm::vec4(212.f, 178.f, 106.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(94, -82), 50, 'e', glm::vec4(192.f, 230.f, 153.f, 255.f) / 255.f, glm::vec4(41.f, 79.f, 109.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-140, 0), 50, 'c', glm::vec4(36.f, 96.f, 104.f, 255.f) / 255.f, glm::vec4(134.f, 161.f, 54.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-80, 0), 50, 'r', glm::vec4(105.f, 150.f, 156.f, 255.f) / 255.f, glm::vec4(176.f, 202.f, 101.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-20, 10), 40, 'a', glm::vec4(66.f, 122.f, 130.f, 255.f) / 255.f, glm::vec4(95.f, 121.f, 20.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(30, -10), 60, 'f', glm::vec4(36.f, 96.f, 104.f, 255.f) / 255.f, glm::vec4(134.f, 161.f, 54.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(100, 0), 50, 't', glm::vec4(66.f, 122.f, 130.f, 255.f) / 255.f, glm::vec4(95.f, 121.f, 20.f, 255.f) / 255.f);

        return {};
}

bool minecraft_scene::tick(const tick_time_info& tick_time, window& win, pipeline_manager& pipe) {
        const bool is_success = base_scene::tick(tick_time, win, pipe);
        if (!is_success) {
                return is_success;
        }

        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        crate_sim.tick(new_cam_transform, win, pipe);

        return true;
}



void minecraft_scene::cleanup(window& win, pipeline_manager& pipe) {
        base_scene::cleanup(win, pipe);
        crate_sim.destroy(pipe);
}
