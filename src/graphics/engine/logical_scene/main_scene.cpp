
#include "main_scene.h"

#include "utils/ray_log.h"

using namespace ray;
using namespace ray::graphics;


#if RAY_GRAPHICS_ENABLE


ray_error main_scene::init(window& win, pipeline_manager& rend) {
        return {};
}

bool main_scene::tick(window& win, pipeline_manager& rend) {
        world_processor.tick(win, rend);
        return true;
}


void main_scene::cleanup(window& win, pipeline_manager& rend) {

}

#endif