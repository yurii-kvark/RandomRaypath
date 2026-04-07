#pragma once
#include "i_logical_scene.h"
#include "engine/logical_system/logical_2d_world_view.h"
#include "engine/logical_system/logical_grid.h"
#include "engine/logical_system/logical_hud_info.h"

namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace ray::graphics;

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

};