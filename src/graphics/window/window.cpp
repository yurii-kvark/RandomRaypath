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
                    glfwSetErrorCallback([](int code, const char* desc) {
                        ray_log(e_log_type::fatal, "GLFW error %d: %s\n", code, desc);
                    });

                    ok = (glfwInit() == GLFW_TRUE);
                });

                if (!ok) {
                        ray_log(e_log_type::fatal, "glfwInit failed.\n");
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

        // BUG TODO: actually, it will work only in the same thread for multi-window.
        // to use it with multiple graphical loop, you need multi-window thread single-owner or single-thread loop multi-window implementation instead of async_graphical_loop
        GLFWwindow* gl_win_ptr = glfwCreateWindow(used_config.window_size.x, used_config.window_size.y, "Random Raypath", monitor_ptr, nullptr);

        if (!gl_win_ptr) {
                ray_log(e_log_type::fatal, "glfwCreateWindow failed.\n");
                return;
        }

        gl_win = std::shared_ptr<GLFWwindow>( gl_win_ptr, glfw_window_deleter {} );

        glfwSetWindowUserPointer(gl_win_ptr, this);
        mouse_wheel_delta_frame = 0;
        glfwSetScrollCallback(gl_win_ptr, glfw_mouse_scroll_callback);

        glfwSetInputMode(gl_win_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        glfw_alloc_cursors();
        set_mouse_cursor(e_mouse_cursor::arrow_pointer);
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

        mouse_wheel_delta_frame = 0;
        glfwPollEvents();

        out_valid_view = true;

        return true;
}


std::weak_ptr<GLFWwindow> window::get_gl_window() const {
        return gl_win;
}


glm::vec2 window::get_mouse_position() const {
        double x_pos = 0, y_pos = 0;
        if (gl_win) {
                glfwGetCursorPos(gl_win.get(), &x_pos, &y_pos);
        }
        return {x_pos, y_pos};
}


bool window::get_mouse_button_left() const {
        int state = GLFW_RELEASE;
        if (gl_win) {
                state = glfwGetMouseButton(gl_win.get(), GLFW_MOUSE_BUTTON_LEFT);
        }
        return state == GLFW_PRESS;
}


bool window::get_mouse_button_right() const {
        int state = GLFW_RELEASE;
        if (gl_win) {
                state = glfwGetMouseButton(gl_win.get(), GLFW_MOUSE_BUTTON_RIGHT);
        }
        return state == GLFW_PRESS;
}


glm::f64 window::get_mouse_wheel_delta() const {
        return mouse_wheel_delta_frame * used_config.zoom_speed;
}


void window::set_mouse_cursor(e_mouse_cursor in_type){
        assert(in_type < e_mouse_cursor::count);

        GLFWcursor* cursor = glfw_hot_cursors[(int)in_type];
        if (gl_win && !!cursor) {
                glfwSetCursor(gl_win.get(), cursor);
        }
}


void window::glfw_mouse_scroll_callback(GLFWwindow *glfw_win, double x_offset, double y_offset) {
        window* instance = static_cast<window*>(glfwGetWindowUserPointer(glfw_win));

        if (instance) {
                instance->mouse_wheel_delta_frame += y_offset; // y is mouse wheel
        }
}


void window::glfw_alloc_cursors(){
        glfw_hot_cursors[(int)e_mouse_cursor::arrow_pointer] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        glfw_hot_cursors[(int)e_mouse_cursor::move_hand] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
}


void window::glfw_dealloc_cursors(){
        for (int i = 0; i < (int)e_mouse_cursor::count; i++) {
                GLFWcursor* cursor = glfw_hot_cursors[i];
                if (!!cursor) {
                        glfwDestroyCursor(cursor);
                }

                glfw_hot_cursors[i] = nullptr;
        }
}


window::~window() {
        glfw_dealloc_cursors();
        gl_win.reset();
        // glfwTerminate(); OS will handle it on close
}
#endif