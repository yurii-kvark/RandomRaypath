#include "graphical_loop.h"

#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"

#include <memory>

using namespace ray;
using namespace ray::graphics;

struct render_thread {
        render_thread(config::client_renderer in_config)
                : cfg(std::move(in_config)) {}

        void operator()(std::stop_token stop_t) const {
                window win(cfg.window);
                renderer rend(win.get_gl_window()); // raw pointer, non-owning

                while (!stop_t.stop_requested()) {
                        if (!tick(win, rend)) {
                                break;
                        }
                }
        }

        config::client_renderer cfg;

private:
        static bool tick(window& win, renderer& rend) {
                bool valid_view = false;
                const bool window_success = win.draw_window(valid_view);

                if (!window_success) {
                        return false;
                }

                if (!valid_view) {
                        return true;
                }

                const bool render_success = rend.draw_frame();

                return render_success;
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
