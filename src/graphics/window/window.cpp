module;
#include "src/graphics/graphic_libs.h"

#include <iostream>
#include <print>
module ray.graphics.window;

using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

window::window(const config& in_config) {
        used_config = in_config;

        if (!used_config.graphics_window_enabled) {
                return;
        }

        if (!glfwInit()) {
                std::printf("glfwInit failed.\n");
                return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_POSITION_X, used_config.window_position.x);
        glfwWindowHint(GLFW_POSITION_Y, used_config.window_position.y);

        GLFWmonitor* monitor_ptr = nullptr;
        if (used_config.window_mode == e_window_mode::fullscreen) {
                monitor_ptr = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor_ptr);

                glfwWindowHint(GLFW_RED_BITS, mode->redBits);
                glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
                glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
                glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

                used_config.window_size.x = mode->width;
                used_config.window_size.y = mode->height;
        }

        gwin = glfwCreateWindow(used_config.window_size.x, used_config.window_size.y, "Random Raypath", monitor_ptr, nullptr);

        if (!gwin) {
                std::printf("glfwCreateWindow failed.\n");
                return;
        }

        renderer_instance = new renderer(gwin);
}


void window::blocking_loop() {
        if (!gwin) {
                return;
        }

        if (!renderer_instance) {
                return;
        }

        while (!glfwWindowShouldClose(gwin)) {
                int w=0,h=0;
                glfwGetFramebufferSize(gwin, &w, &h);

                if (w == 0 || h == 0) {
                        glfwWaitEvents();
                        continue;
                }
                glfwPollEvents();
                renderer_instance->draw_frame();
        }
}


window::~window() {
        if (!!renderer_instance) {
                delete renderer_instance;
        }
        glfwDestroyWindow(gwin);
        glfwTerminate();
}
#endif