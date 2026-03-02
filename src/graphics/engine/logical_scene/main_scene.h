#pragma once
#include "i_logical_scene.h"
#include "graphics/engine/logical_system/logical_2d_world_view.h"

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE
class window;
class renderer;

class main_scene : public i_logical_scene {
public:
        virtual ray_error init(window& win, pipeline_manager& rend) override;
        virtual bool tick(window& win, pipeline_manager& rend) override;
        virtual void cleanup(window& win, pipeline_manager& rend) override;

private:
        logical_2d_world_view world_processor;
};

#endif
};