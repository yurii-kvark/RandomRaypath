module;
#include "src/graphics/graphic_libs.h"

#include <iostream>
#include <memory>
#include <print>
module ray.graphics.window;

using namespace ray;
using namespace ray::graphics;

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

        // TODO: apply config
        gwin = glfwCreateWindow(used_config.window_size.x, used_config.window_size.y, "Random Raypath", nullptr, nullptr);

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