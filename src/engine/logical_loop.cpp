#include "logical_loop.h"

#include "logical_scene/dev_test_scene.h"
#include "graphics/window/window.h"
#include "graphics/rhi/renderer.h"
#include "logical_scene/main_scene.h"
#include "logical_scene/core/base_scene.h"
#include "logical_scene/minecraft_scene.h"
#include "network/remote_control/remote_control.h"
#include "utils/ray_profile.h"

#include <memory>


using namespace ray;
using namespace ray::logical;


std::unique_ptr<i_logical_scene> make_scene_by_name(std::string_view scene_class_name) {
        if (scene_class_name == "main") {
                return std::unique_ptr<i_logical_scene>(new main_scene());
        }

        if (scene_class_name == "dev_test") {
                return std::unique_ptr<i_logical_scene>(new dev_test_scene());
        }

        if (scene_class_name == "minecraft") {
                return std::unique_ptr<i_logical_scene>(new minecraft_scene());
        }

        return nullptr;
}

struct logical_thread {
        logical_thread(config::render_server_config in_config)
                : cfg(std::move(in_config)) {}

        void operator()(std::stop_token stop_t) const {
                window win(cfg);
                renderer rend(win.get_gl_window(), cfg.style);

                std::unique_ptr<network::remote_control_client> remote_client;

                if (cfg.network.enable_remote_control) {
                        remote_client = std::make_unique<network::remote_control_client>();
                }

                if (!!remote_client) {
                        remote_client->async_launch(cfg.network.server_control_addr);
                }

                std::unique_ptr<i_logical_scene> logic = make_scene_by_name(cfg.scene.logical_scene);

                if (!logic) {
                        ray_log(e_log_type::fatal, "Can't load scene by config name: {}", cfg.scene.logical_scene);
                        return;
                }

                logic->setup_config(cfg);

                ray_error init_scene_error = logic->init(win, rend.pipe);

                if (init_scene_error.has_value()) {
                        ray_log(e_log_type::fatal, "Scene initialization failed: {}", init_scene_error.value());
                        return;
                }

                std::queue<network::remote_command_frame_set> command_queue;
                int pass_control_frames = 0;

                std::optional<network::remote_command_frame_set> this_frame_command = {};
                std::optional<network::remote_answer_frame_set> answer_set_pre = {};

                while (!stop_t.stop_requested()) {
                        if (!!remote_client) {
                                RAY_PROFILE_SCOPE("remote_control_pre", glm::vec3(0., 1., 1.));

                                win.clear_injected_input();

                                if (pass_control_frames > 0) {
                                        pass_control_frames -= 1;
                                } else {
                                        std::optional<network::remote_command_frame_set> extracted_frame_command = remote_client->receive_next_frame_command();

                                        if (extracted_frame_command.has_value()) {
                                                command_queue.push(extracted_frame_command.value());
                                        }

                                        if (cfg.scene.tickless_mode && command_queue.empty()) {
                                                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                                                continue;
                                        }

                                        if (!command_queue.empty()) {
                                                this_frame_command = command_queue.front();

                                                const auto& pass_ticks_after = this_frame_command->command_map[(int)network::remote_command_type::pass_ticks_after];
                                                answer_set_pre = logic->inject_remote_control_pre(win, rend, *this_frame_command);

                                                if (pass_ticks_after.enabled) {
                                                        pass_control_frames = (int)std::round(pass_ticks_after.value.x);

                                                        auto& answer_pass_ticks = answer_set_pre->answer_map[(int)network::remote_command_type::pass_ticks_after];
                                                        answer_pass_ticks.enabled = true;
                                                        answer_pass_ticks.verbal_message = std::format("{}+1 frames will be executed", pass_control_frames);
                                                }

                                                command_queue.pop();
                                        }
                                }
                        }

                        if (!tick(win, rend, *logic)) {
                                break;
                        }

                        if (!!remote_client && this_frame_command.has_value()) {
                                RAY_PROFILE_SCOPE("remote_control_post", glm::vec3(0., 1., 1.));
                                assert(answer_set_pre.has_value());

                                network::remote_answer_frame_set answer_set = logic->inject_remote_control_post(win, rend, *answer_set_pre, *this_frame_command);

                                remote_client->send_answer(answer_set);

                                this_frame_command = {};
                                answer_set_pre = {};
                        }
                }

                logic->cleanup(win, rend.pipe);

#if RAY_DEBUG_NO_OPT
                rend.pipe.verify_pipeline_destruction();
#endif

                if (!!remote_client) {
                        remote_client->wait_shutdown();
                }
        }

        config::render_server_config cfg;

private:
        static bool tick(window& win, renderer& rend, i_logical_scene& logic) {
                RAY_PROFILE_FRAME();

                {
                        RAY_PROFILE_SCOPE("draw_window", glm::vec3(0., 0., 1.));

                        bool valid_view = false;
                        const bool window_success = win.draw_window(valid_view);
                        if (!window_success) {
                                return false;
                        }

                        if (!valid_view) {
                                return true;
                        }
                }

                {
                        RAY_PROFILE_SCOPE("logic_tick", glm::vec3(0, 1, 0.));

                        const bool logic_success = logic.tick(win, rend.pipe);
                        if (!logic_success) {
                                return false;
                        }
                }

                {
                        RAY_PROFILE_SCOPE("draw_frame", ray_colors::orange);

                        const bool render_success = rend.draw_frame();
                        if (!render_success) {
                                return false;
                        }
                }

                return true;
        }
};


async_logical_loop::async_logical_loop(config::app_config in_config)
        : worker_t( logical_thread {std::move(in_config.server_config)}) {
}


async_logical_loop::~async_logical_loop() {
        signal_terminate();
        if (worker_t.joinable()) {
                worker_t.join();
        }
}


bool async_logical_loop::is_alive() const {
        return worker_t.joinable();
}


void async_logical_loop::signal_terminate() {
        worker_t.request_stop();
}


void async_logical_loop::wait_blocking() {
        if (worker_t.joinable()) {
                worker_t.join();
        }
}