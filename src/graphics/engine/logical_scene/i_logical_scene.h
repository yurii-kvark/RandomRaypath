#pragma once
#include "utils/ray_error.h"


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

class window;
class pipeline_manager;

class i_logical_scene {
public:
        virtual ray_error init(window& win, pipeline_manager& rend) = 0;
        virtual bool tick(window& win, pipeline_manager& rend) = 0;
        virtual void cleanup(window& win, pipeline_manager& rend) = 0;

        virtual ~i_logical_scene() = default;
};

#endif

};