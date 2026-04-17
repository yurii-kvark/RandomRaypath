#pragma once
#include "core/base_scene.h"
#include "core/i_logical_scene.h"
#include "engine/logical_system/logical_crate_sim.h"


namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace ray::graphics;

class ph_crates_scene : public base_scene {
public:
        virtual ray_error init(window& win, pipeline_manager& pipe) override;
        virtual bool tick(const tick_time_info& tick_time, window& win, pipeline_manager& pipe) override;
        virtual void cleanup(window& win, pipeline_manager& pipe) override;

private:
        logical_crate_sim crate_sim;
};
};