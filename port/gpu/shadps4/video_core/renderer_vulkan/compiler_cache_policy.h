// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <string_view>

namespace Vulkan {
inline bool ShareCompilerPipelineCache(std::uint64_t budget, bool integrated,
                                       const char* option = nullptr) {
    if (option && std::string_view{option} == "1") {
        return true;
    }
    if (option && std::string_view{option} == "0") {
        return false;
    }
    return !integrated && budget != 0 && budget <= (std::uint64_t{4} << 30);
}
} // namespace Vulkan
