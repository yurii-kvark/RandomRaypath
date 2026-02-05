#include "window.h"

#include <iostream>
#include <mutex>
#include <print>

using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

struct glfw_window_deleter {
        void operator()(GLFWwindow* w) const {
                if (w) {
                        glfwDestroyWindow(w);
                }
        }
};


struct glfw_runtime_init {
        glfw_runtime_init() {
                static std::once_flag once;
                static bool ok = false;

                std::call_once(once, [] {
                    ok = (glfwInit() == GLFW_TRUE);
                });

                if (!ok) {
                        std::printf("glfwInit failed.\n");
                        return;
                }
        }
};

window::window(const config& in_config)
        : used_config(in_config) {

        if (!used_config.graphics_window_enabled) {
                return;
        }

        glfw_runtime_init {};

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

        GLFWwindow* gl_win_ptr = glfwCreateWindow(used_config.window_size.x, used_config.window_size.y, "Random Raypath", monitor_ptr, nullptr);

        if (!gl_win_ptr) {
                std::printf("glfwCreateWindow failed.\n");
                return;
        }

        gl_win = std::shared_ptr<GLFWwindow>( gl_win_ptr, glfw_window_deleter {} );
}


bool window::draw_window(bool& out_valid_view) {
        out_valid_view = false;

        if (!gl_win) {
                return false;
        }

        if (glfwWindowShouldClose(gl_win.get())) {
                return false;
        }

        {
                int width, height = 0;
                glfwGetFramebufferSize(gl_win.get(), &width, &height);

                if (width == 0 || height == 0) {
                        glfwWaitEvents();
                        return true;
                }
        }

        glfwPollEvents();

        out_valid_view = true;

        return true;
}

std::weak_ptr<GLFWwindow> window::get_gl_window() const {
        return gl_win;
}


window::~window() {
        gl_win.reset();
        // glfwTerminate(); OS will handle it on close
}
#endif