#pragma once
#include "g_app_driver.h"
#include "../graphic_libs.h"
#include "glm/glm.hpp"



namespace ray::graphics {
#if RAY_GRAPHICS_ENABLE

template <class PipeLine>
struct draw_object {
        using tick_data_model = PipeLine::tick_data_model;
        tick_data_model draw_data;
};

class base_pipeline {
public:
        struct tick_data_model {
                glm::u32 z_order = 0;
        };

        base_pipeline();

        virtual void init_pipeline() = 0;

        //void init_draw_object(draw_object<base_pipeline>);
        void tick_draw_object(draw_object<base_pipeline>);

        virtual ~base_pipeline() {}

protected:
       // std::vector<draw_object<base_pipeline>::data_model> draw_objects;
};

enum class space_type {
        screen = 0,
        world
};

class object_2d_pipeline : public base_pipeline {
};

class texture_pipeline : public object_2d_pipeline {

};

class solid_rect_pipeline : public object_2d_pipeline {

};

class solid_rect_pipeline : public object_2d_pipeline {

};

class text_pipeline : public object_2d_pipeline {

};
#endif
};
