module;
#include "src/graphics/graphic_libs.h"

#include <iostream>
#include <memory>
#include <vector>
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

        renderer_instance = std::make_unique<renderer>(gwin);

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


std::weak_ptr<renderer> get_weak_render_instance() {
        return renderer_instance;
}


window::~window() {
        renderer_instance.reset();
        glfwDestroyWindow(gwin);
        glfwTerminate();
}