#pragma once
#include "config/client_renderer.h"
#include "utils/ray_error.h"


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

class window;
class pipeline_manager;

class i_logical_scene {
public:
        void setup_visual_style(config::visual_style in_style) {
                style = std::move(in_style);
        }

        virtual ray_error init(window& win, pipeline_manager& pipe) = 0;
        virtual bool tick(window& win, pipeline_manager& pipe) = 0;
        virtual void cleanup(window& win, pipeline_manager& pipe) = 0;

        virtual ~i_logical_scene() = default;

protected:
        config::visual_style style;
};

#endif

};