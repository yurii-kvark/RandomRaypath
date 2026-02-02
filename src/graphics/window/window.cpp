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

        GLFWmonitor* monitor_ptr = used_config.window_mode == e_window_mode::fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        gwin = glfwCreateWindow(used_config.window_size.x, used_config.window_size.y, "Random Raypath", monitor_ptr, nullptr);

        if (!gwin) {
                std::printf("glfwCreateWindow failed.\n");
                return;
        }

        renderer_instance = new renderer(gwin);

        while (!glfwWindowShouldClose(gwin)) {
                glfwPollEvents();
        }
}


void window::blocking_loop() {
        if (!gwin) {
                return;
        }

        while (!glfwWindowShouldClose(gwin)) {
                glfwPollEvents();
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