#pragma once
#include "i_logical_scene.h"
#include "graphics/engine/logical_system/logical_2d_world_view.h"
#include "graphics/engine/logical_system/logical_grid.h"
#include "graphics/engine/logical_system/logical_hud_info.h"

namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE
class window;
class renderer;

class main_scene : public i_logical_scene {
public:
        virtual ray_error init(window& win, pipeline_manager& pipe) override;
        virtual bool tick(window& win, pipeline_manager& pipe) override;
        virtual void cleanup(window& win, pipeline_manager& pipe) override;

private:
        logical_2d_world_view world_processor;
        logical_hud_info hud_info;
        logical_grid grid_system;
};

#endif
};