#include "remote_control.h"
#include "utils/asio_config.h"

#include <asio.hpp>
#include <glaze/glaze.hpp>

#include <array>
#include <chrono>
#include <string>
#include <vector>

using namespace ray;
using namespace ray::network;


struct json_cmd_entry_t {
        std::string command_type;
        bool enabled = false;
        std::array<double, 4> value {};
};

struct json_cmd_msg_t {
        uint32_t net_session_tag = 0;
        uint32_t net_id = 0;
        std::vector<json_cmd_entry_t> command_map;
};

struct json_ans_entry_t {
        std::string command_type;
        bool enabled = false;
        std::string verbal_message;
};

struct json_ans_msg_t {
        uint32_t net_session_tag = 0;
        uint32_t net_id = 0;
        std::vector<json_ans_entry_t> answer_map;
};

template <>
struct glz::meta<json_cmd_entry_t> {
        using T = json_cmd_entry_t;
        static constexpr auto value = object(
                "command_type", &T::command_type,
                "enabled",      &T::enabled,
                "value",        &T::value
        );
};

template <>
struct glz::meta<json_cmd_msg_t> {
        using T = json_cmd_msg_t;
        static constexpr auto value = object(
                "net_session_tag", &T::net_session_tag,
                "net_id",          &T::net_id,
                "command_map",     &T::command_map
        );
};

template <>
struct glz::meta<json_ans_entry_t> {
        using T = json_ans_entry_t;
        static constexpr auto value = object(
                "command_type",   &T::command_type,
                "enabled",        &T::enabled,
                "verbal_message", &T::verbal_message
        );
};

template <>
struct glz::meta<json_ans_msg_t> {
        using T = json_ans_msg_t;
        static constexpr auto value = object(
                "net_session_tag", &T::net_session_tag,
                "net_id",          &T::net_id,
                "answer_map",      &T::answer_map
        );
};


static constexpr std::array<std::pair<remote_command_type, const char*>, static_cast<int>(remote_command_type::count)> k_command_names = {{
        { remote_command_type::none,                 "none" },
        { remote_command_type::pass_ticks_after,     "pass_ticks_after" },
        { remote_command_type::set_camera_position,  "set_camera_position" },
        { remote_command_type::set_mouse_position,   "set_mouse_position" },
        { remote_command_type::set_mouse_left_button,"set_mouse_left_button" },
        { remote_command_type::set_mouse_right_button,"set_mouse_right_button" },
        { remote_command_type::add_mouse_scroll,     "add_mouse_scroll" },
        { remote_command_type::screenshot,           "screenshot" },
        { remote_command_type::hud_info,             "hud_info" },
        { remote_command_type::debug_command,        "debug_command" },
        { remote_command_type::shutdown,        "shutdown" },
        { remote_command_type::session_log_rename,        "session_log_rename" },
}};


remote_command_frame_set remote_control_client::command_frame_from_json(const std::string& in_json) {
        json_cmd_msg_t msg{};
        auto ec = glz::read_json(msg, in_json);
        if (ec) {
                ray_log(e_log_type::warning, "remote_control: json parse error: {}", glz::format_error(ec, in_json));
                return {};
        }

        remote_command_frame_set frame{};
        frame.net_session_tag = msg.net_session_tag;
        frame.net_id          = msg.net_id;

        for (const auto& entry : msg.command_map) {
                if (!entry.enabled) {
                        continue;
                }

                remote_command_type type = remote_command_type_index(entry.command_type);

                if (type == remote_command_type::none) {
                        continue;
                }

                auto idx = static_cast<int>(type);

                frame.command_map[idx].enabled = true;
                frame.command_map[idx].value = glm::vec4(entry.value[0], entry.value[1], entry.value[2], entry.value[3]);
        }

        return frame;
}

static void replace_all_str(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // Move past the last replacement
        }
}

std::string remote_control_client::answer_frame_to_json(const remote_answer_frame_set& in_answer) {
        json_ans_msg_t msg{};
        msg.net_session_tag = in_answer.net_session_tag;
        msg.net_id          = in_answer.net_id;

        for (int i = 0; i < static_cast<int>(remote_command_type::count); ++i) {
                const auto& entry = in_answer.answer_map[i];
                if (!entry.enabled) {
                        continue;
                }

                msg.answer_map.push_back({
                        .command_type   = remote_command_type_name(static_cast<remote_command_type>(i)),
                        .enabled        = true,
                        .verbal_message = entry.verbal_message,
                });
        }

        auto result = glz::write_json(msg);
        if (!result) {
                ray_log(e_log_type::warning, "remote_control: json serialize error");
                return "{}";
        }

        replace_all_str(*result, "\n", " <> ");
        replace_all_str(*result, "\\n", " <> ");
        return std::move(result).value();
}


const char* remote_control_client::remote_command_type_name(remote_command_type index) {
        for (const auto& [type, name] : k_command_names) {
                if (index == type) {
                        return name;
                }
        }
        return "none";
}


remote_command_type remote_control_client::remote_command_type_index(std::string_view name) {
        for (const auto& [type, type_name] : k_command_names) {
                if (name == type_name) {
                        return type;
                }
        }
        return remote_command_type::none;
}


void remote_control_client::async_launch(std::string_view server_full_addr) {
        auto colon = server_full_addr.rfind(':');
        if (colon == std::string_view::npos || colon == 0 || colon == server_full_addr.size() - 1) {
                ray_log(e_log_type::warning, "remote_control: invalid network.server_control_addr address: '{}'", server_full_addr);
                return;
        }

        std::string host(server_full_addr.substr(0, colon));
        std::string port(server_full_addr.substr(colon + 1));

        net_thread = std::jthread([this, h = std::move(host), p = std::move(port)](std::stop_token stop) {
                network_loop(std::move(stop), h, p);
        });
}


void remote_control_client::wait_shutdown() {
        if (net_thread.joinable()) {
                net_thread.request_stop();
                net_thread.join();
        }
}


std::optional<remote_command_frame_set> remote_control_client::receive_next_frame_command() {
        std::lock_guard lock(incoming_mtx);
        if (incoming_queue.empty()) {
                return std::nullopt;
        }

        auto front = incoming_queue.front();
        incoming_queue.pop();
        return front;
}


void remote_control_client::send_answer(const remote_answer_frame_set& command_answer) {
        std::lock_guard lock(outgoing_mtx);
        outgoing_queue.push(command_answer);
}

void remote_control_client::network_loop(std::stop_token stop, std::string host, std::string port) {
        using namespace asio;
        using namespace std::chrono_literals;

        ray_log(e_log_type::info, "remote_control: network thread started. server target {}:{}", host, port);

        while (!stop.stop_requested()) {
                io_context io;
                ip::tcp::resolver resolver(io);
                std::error_code ec;

                auto endpoints = resolver.resolve(host, port, ec);
                if (ec) {
                        ray_log(e_log_type::warning, "remote_control: resolve failed: {}. retry in 2 sec.", ec.message());
                        for (int i = 0; i < 20 && !stop.stop_requested(); ++i)
                                std::this_thread::sleep_for(100ms);
                        continue;
                }

                ip::tcp::socket socket(io);
                bool connect_done = false;
                bool connect_ok   = false;

                async_connect(socket, endpoints,
                        [&](const std::error_code& connect_ec, const ip::tcp::endpoint& ep) {
                                connect_done = true;
                                if (!connect_ec) {
                                        connect_ok = true;
                                        ray_log(e_log_type::info, "remote_control: connected to network target = {}:{}.",
                                                ep.address().to_string(), ep.port());
                                } else {
                                        ec = connect_ec;
                                }
                        });

                while (!stop.stop_requested() && !connect_done) {
                        io.run_one_for(100ms);
                }

                if (!connect_done) {
                        socket.cancel(ec);
                        io.run();
                        break; // stop requested
                }

                if (!connect_ok) {
                        ray_log(e_log_type::warning, "remote_control: connect failed: {}. retry in 2 sec.", ec.message());
                        for (int i = 0; i < 20 && !stop.stop_requested(); ++i)
                                std::this_thread::sleep_for(100ms);
                        continue;
                }

                ray_log(e_log_type::info, "remote_control: remote control granted.");

                streambuf read_buf;
                bool connection_alive = true;

                std::stop_callback stop_cb(stop, [&]() {
                        std::error_code ignored;
                        socket.shutdown(ip::tcp::socket::shutdown_both, ignored);
                        socket.close(ignored);
                });

                io.restart();

                // read
                co_spawn(io,
                        [&]() -> awaitable<void> {
                                while (connection_alive) {
                                        auto [read_ec, n] = co_await async_read_until(
                                                socket, read_buf, '\n',
                                                as_tuple(use_awaitable));

                                        if (read_ec) {
                                                if (read_ec != error::operation_aborted) {
                                                        ray_log(e_log_type::warning, "remote_control: read error: {}",
                                                                read_ec.message());
                                                }
                                                connection_alive = false;
                                                co_return;
                                        }

                                        std::istream is(&read_buf);
                                        std::string line;
                                        std::getline(is, line);

                                        if (line.empty()) {
                                                continue;
                                        }

                                        {
                                                std::lock_guard lock(incoming_mtx);
                                                incoming_queue.push(command_frame_from_json(line));
                                        }
                                }
                        },
                        detached);

                // write
                co_spawn(io,
                        [&]() -> awaitable<void> {
                                steady_timer timer(co_await this_coro::executor);
                                while (connection_alive) {
                                        timer.expires_after(20ms);
                                        auto [timer_ec] = co_await timer.async_wait(
                                                as_tuple(use_awaitable));
                                        if (timer_ec) {
                                                co_return;
                                        }

                                        std::queue<remote_answer_frame_set> pending;
                                        {
                                                std::lock_guard lock(outgoing_mtx);
                                                std::swap(pending, outgoing_queue);
                                        }

                                        while (!pending.empty() && connection_alive) {
                                                auto json = answer_frame_to_json(pending.front());
                                                pending.pop();
                                                if (json.empty()) {
                                                        continue;
                                                }

                                                json.push_back('\n');

                                                auto [write_ec, bytes] = co_await async_write(
                                                        socket, buffer(json),
                                                        as_tuple(use_awaitable));

                                                if (write_ec) {
                                                        ray_log(e_log_type::warning, "remote_control: write error: {}",
                                                                write_ec.message());
                                                        connection_alive = false;
                                                        co_return;
                                                }
                                        }
                                }
                        },
                        detached);

                io.run();

                if (socket.is_open()) {
                        socket.shutdown(ip::tcp::socket::shutdown_both, ec);
                        socket.close(ec);
                }

                if (!stop.stop_requested()) {
                        ray_log(e_log_type::info, "remote_control: disconnected, retrying in 500ms...");
                        for (int i = 0; i < 10 && !stop.stop_requested(); ++i) {
                                std::this_thread::sleep_for(50ms);
                        }
                }
        }

        ray_log(e_log_type::info, "remote_control: network thread stopped");
}
