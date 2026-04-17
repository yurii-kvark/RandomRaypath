#include "network/connection_test.h"
#include "../utils/asio_config.h"

#include <asio.hpp>

#include <format>
#include <chrono>


namespace ray::network {

ray_error test_tcp_connection(std::string_view host, glm::u16 port) {
        using namespace asio;
        using namespace std::chrono_literals;

        std::error_code ec;

        io_context io_ctx;

        // Resolve the host/port pair
        ip::tcp::resolver resolver(io_ctx);
        auto endpoints = resolver.resolve(
                std::string(host),
                std::to_string(port),
                ec
        );

        if (ec) {
                return std::format("DNS resolve failed for {}:{} - {}", host, port, ec.message());
        }

        ray_log(e_log_type::info, "Resolved {}:{} ({} endpoint(s))",
                host, port, std::distance(endpoints.begin(), endpoints.end()));

        // Set up a TCP socket with a connection timeout
        ip::tcp::socket socket(io_ctx);
        bool connected = false;
        bool timed_out = false;

        // Deadline timer for connection timeout (5 seconds)
        steady_timer timer(io_ctx, 5s);

        timer.async_wait([&](const std::error_code& timer_ec) {
                if (!timer_ec) {
                        // Timer fired before connection completed — cancel the socket
                        timed_out = true;
                        socket.cancel();
                }
        });

        // Async connect so the timer can interrupt it
        async_connect(socket, endpoints,
                [&](const std::error_code& connect_ec,
                    const ip::tcp::endpoint& endpoint) {
                        timer.cancel(); // Connection finished — cancel the timer
                        if (!connect_ec) {
                                connected = true;
                                ray_log(e_log_type::info, "Connected to {}:{}",
                                        endpoint.address().to_string(), endpoint.port());
                        } else {
                                ec = connect_ec;
                        }
                }
        );

        // Run the io_context — blocks until both async ops complete or are cancelled
        io_ctx.run();

        // Gracefully close the socket if it connected
        if (socket.is_open()) {
                socket.shutdown(ip::tcp::socket::shutdown_both, ec);
                socket.close(ec);
        }

        if (timed_out) {
                return std::format("Connection to {}:{} timed out (5s)", host, port);
        }

        if (!connected) {
                return std::format("Connection to {}:{} failed - {}", host, port, ec.message());
        }

        ray_log(e_log_type::info, "TCP connection test to {}:{} succeeded", host, port);
        return std::nullopt;
}

};
