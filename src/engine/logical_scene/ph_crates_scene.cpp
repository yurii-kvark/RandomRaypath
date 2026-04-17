
#include "ph_crates_scene.h"

#include "engine/logical_system/logical_crate_sim.h"
#include "utils/ray_log.h"

using namespace ray;
using namespace ray::graphics;
using namespace ray::logical;


ray_error ph_crates_scene::init(window& win, pipeline_manager& pipe) {
        ray_error init_error = base_scene::init(win, pipe);
        if (init_error) {
                return init_error;
        }

        ray_error crate_error = crate_sim.init(win, pipe);
        if (crate_error) {
                return crate_error;
        }

        world_processor.register_pipelines(crate_sim.get_pipelines());

        // Neon Cyberpunk (session_17820012)
        crate_sim.add_crate(glm::vec2(-180, -55), 50, 'r', glm::vec4(15.f, 5.f, 30.f, 255.f) / 255.f, glm::vec4(255.f, 0.f, 200.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-120, -55), 50, 'a', glm::vec4(5.f, 20.f, 10.f, 255.f) / 255.f, glm::vec4(0.f, 255.f, 100.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-60, -55), 50, 'y', glm::vec4(20.f, 0.f, 40.f, 255.f) / 255.f, glm::vec4(200.f, 0.f, 255.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(0, -55), 50, 'p', glm::vec4(0.f, 20.f, 30.f, 255.f) / 255.f, glm::vec4(0.f, 230.f, 255.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(60, -55), 50, 'a', glm::vec4(30.f, 10.f, 0.f, 255.f) / 255.f, glm::vec4(255.f, 180.f, 0.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(120, -55), 50, 't', glm::vec4(10.f, 0.f, 25.f, 255.f) / 255.f, glm::vec4(255.f, 50.f, 180.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(180, -55), 50, 'h', glm::vec4(0.f, 15.f, 5.f, 255.f) / 255.f, glm::vec4(50.f, 255.f, 80.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-180, 10), 50, 'c', glm::vec4(0.f, 5.f, 30.f, 255.f) / 255.f, glm::vec4(0.f, 200.f, 255.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-120, 10), 50, 'r', glm::vec4(25.f, 0.f, 35.f, 255.f) / 255.f, glm::vec4(255.f, 20.f, 220.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(-60, 10), 50, 'a', glm::vec4(5.f, 25.f, 5.f, 255.f) / 255.f, glm::vec4(20.f, 255.f, 120.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(0, 10), 50, 't', glm::vec4(0.f, 10.f, 25.f, 255.f) / 255.f, glm::vec4(0.f, 220.f, 240.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(60, 10), 50, 'e', glm::vec4(28.f, 5.f, 0.f, 255.f) / 255.f, glm::vec4(255.f, 200.f, 10.f, 255.f) / 255.f);
        crate_sim.add_crate(glm::vec2(120, 10), 50, 's', glm::vec4(15.f, 0.f, 28.f, 255.f) / 255.f, glm::vec4(210.f, 0.f, 255.f, 255.f) / 255.f);

        return {};
}

bool ph_crates_scene::tick(const tick_time_info& tick_time, window& win, pipeline_manager& pipe) {
        const bool is_success = base_scene::tick(tick_time, win, pipe);
        if (!is_success) {
                return is_success;
        }

        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        crate_sim.tick(new_cam_transform, win, pipe);

        return true;
}



void ph_crates_scene::cleanup(window& win, pipeline_manager& pipe) {
        base_scene::cleanup(win, pipe);
        crate_sim.destroy(pipe);
}
