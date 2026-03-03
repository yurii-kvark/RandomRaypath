#include "graphical_loop.h"

#include "logical_scene/dev_test_scene.h"
#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"
#include "logical_scene/main_scene.h"

#include <memory>

using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

std::unique_ptr<i_logical_scene> make_scene_by_name(std::string_view scene_class_name) {
        if (scene_class_name == "main") {
                return std::unique_ptr<i_logical_scene>(new main_scene());
        }

        if (scene_class_name == "dev_test") {
                return std::unique_ptr<i_logical_scene>(new dev_test_scene());
        }

        return nullptr;
}

struct render_thread {
        render_thread(config::client_renderer in_config)
                : cfg(std::move(in_config)) {}

        void operator()(std::stop_token stop_t) const {
                window win(cfg.window);
                renderer rend(win.get_gl_window());

                std::unique_ptr<i_logical_scene> logic = make_scene_by_name(cfg.logical_scene);

                if (!logic) {
                        ray_log(e_log_type::fatal, "Can't load scene by config name: {}", cfg.logical_scene);
                        return;
                }

                ray_error init_scene_error = logic->init(win, rend.pipe);

                if (init_scene_error.has_value()) {
                        ray_log(e_log_type::fatal, "Scene initialization failed: {}", init_scene_error.value());
                        return;
                }

                while (!stop_t.stop_requested()) {
                        if (!tick(win, rend, *logic)) {
                                break;
                        }
                }

                logic->cleanup(win, rend.pipe);
        }

        config::client_renderer cfg;

private:
        static bool tick(window& win, renderer& rend, i_logical_scene& logic) {
                bool valid_view = false;
                const bool window_success = win.draw_window(valid_view);
                if (!window_success) {
                        return false;
                }

                if (!valid_view) {
                        return true;
                }

                const bool logic_success = logic.tick(win, rend.pipe);
                if (!logic_success) {
                        return false;
                }

                const bool render_success = rend.draw_frame();
                if (!render_success) {
                        return false;
                }

                return true;
        }
};


async_graphical_loop::async_graphical_loop(config::client_renderer in_config)
        : worker_t(render_thread {std::move(in_config)}) {
}


async_graphical_loop::~async_graphical_loop(){
        signal_terminate();
        if (worker_t.joinable()) {
                worker_t.join();
        }
}


bool async_graphical_loop::is_alive() const{
        return worker_t.joinable();
}


void async_graphical_loop::signal_terminate() {
        worker_t.request_stop();
}


void async_graphical_loop::wait_blocking(){
        if (worker_t.joinable()) {
                worker_t.join();
        }
}
#endif