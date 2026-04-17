#pragma once
#include "utils/ray_error.h"

#include "glm/glm.hpp"

#include <string_view>


namespace ray::network {

// Attempts a TCP connection to the given host and port.
// Returns std::nullopt on success, or an error description on failure.
ray_error test_tcp_connection(std::string_view host, glm::u16 port);

};
