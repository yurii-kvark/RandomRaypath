#pragma once
#include <print>
#include <stacktrace>
#include <atomic>
#include <mutex>
#include <fstream>
#include <filesystem>

namespace ray {
        enum class e_log_type : uint8_t {
                info,
                warning,
                fatal
        };

        struct log_file_state {
                std::atomic<bool> enabled{false};
                std::mutex mtx;
        };
        inline log_file_state g_log_file_state;

        inline void ray_log_init(bool enable) {
                g_log_file_state.enabled.store(enable, std::memory_order_relaxed);
        }

        inline void ray_log_rename(std::string_view new_log_file) { // session_0012.log
                // implement
        }

        template<typename... Args>
        void ray_log(e_log_type type, std::format_string<Args...> fmt, Args&&... args) {
                static const char* naming[] = { "RAY INFO: ", "RAY WARNING: ", "RAY FATAL ERROR: " };

                const std::string message = std::format(fmt, std::forward<Args>(args)...);
                std::println("{}{}", naming[static_cast<uint8_t>(type)], message);

                if (g_log_file_state.enabled.load(std::memory_order_relaxed)) {
                        std::lock_guard lock{g_log_file_state.mtx};
                        std::error_code ec;
                        std::filesystem::create_directories("../mcp_logs/", ec);
                        std::ofstream ofs("../mcp_logs/default.log", std::ios::app);

                        if (type == e_log_type::fatal) {
                                ofs << naming[static_cast<uint8_t>(type)] << message << "\n stacktrace:" << std::stacktrace::current() << '\n';
                        } else {
                                ofs << naming[static_cast<uint8_t>(type)] << message << '\n';
                        }
                }

                if (type == e_log_type::fatal) {
                        std::println("stacktrace: {}", std::stacktrace::current());
                        std::fflush(stdout);
                        std::terminate();
                }
        }
};

