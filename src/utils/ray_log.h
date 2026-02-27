#pragma once
#include <print>
#include <stacktrace>

namespace ray {
        enum class e_log_type : uint8_t {
                info,
                warning,
                fatal
        };

        template<typename... _Args>
        void ray_log(e_log_type type, std::format_string<_Args...> __fmt, _Args&&... __args) {
                static const char* naming[] = { "RAY INFO: ", "RAY WARNING: ", "RAY FATAL ERROR: " };

                const std::string message = std::format(__fmt, std::forward<_Args>(__args)...);
                std::println("{}{}", naming[(uint8_t)type], message);

                if (type == e_log_type::fatal) {
                        std::println("stacktrace: {}", std::stacktrace::current());
                        std::fflush(stdout);
                        std::terminate();
                }
        }
};

