#pragma once
#include "../graphic_libs.h"
#include "graphics/rhi/renderer.h"

#include "glm/glm.hpp"

namespace ray::graphics {

enum class e_window_mode : glm::i8 {
        none = 0,
        windowed = none,
        fullscreen = 1,
        count
};

class window {
public:
        struct config {
                bool graphics_window_enabled;
                e_window_mode window_mode; // 0 - windowed, 1 - fullscreen
                glm::i32vec2 window_position;
                glm::i32vec2 window_size;
        };

#if RAY_GRAPHICS_ENABLE

        window(const config& in_config);
        ~window();

        window(const window&) = delete;
        window& operator=(const window&) = delete;
        window(window&&) = default;
        window& operator=(window&&) = default;

        void blocking_loop();

private:
        config used_config;
        renderer* renderer_instance;
        GLFWwindow* gwin;
#endif
};
};