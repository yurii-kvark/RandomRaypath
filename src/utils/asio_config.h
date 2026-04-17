#pragma once

// Must be included BEFORE <asio.hpp> in every translation unit that uses Asio.
// Provides the required asio::detail::throw_exception when ASIO_NO_EXCEPTIONS
// is defined (project builds with -fno-exceptions).

#include "utils/ray_log.h"

#include <asio/detail/config.hpp>

#if defined(ASIO_NO_EXCEPTIONS)

#include <asio/detail/throw_exception.hpp>

namespace asio::detail {

template <typename Exception>
[[noreturn]] void throw_exception(
        const Exception& e
        ASIO_SOURCE_LOCATION_DEFAULTED_PARAM)
{
        ray_log(ray::e_log_type::fatal, "asio fatal: {}", e.what());
        std::abort();
}

} // namespace asio::detail

#endif // ASIO_NO_EXCEPTIONS