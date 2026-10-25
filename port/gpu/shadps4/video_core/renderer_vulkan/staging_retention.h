// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <charconv>
#include <chrono>
#include <cstdint>
#include <string_view>

namespace Vulkan::StagingRetention {
constexpr std::uint64_t MiB = 1024 * 1024;
constexpr std::uint64_t BlockBytes = 16 * MiB;
constexpr std::uint64_t DefaultBytes = 128 * MiB;

// Bound user input before converting to bytes; negative/overflowing values must not
// accidentally disable trimming. An explicit zero also disables startup prewarming.
inline std::uint64_t Budget(const char* value) {
    if (!value) {
        return DefaultBytes;
    }
    const std::string_view text{value};
    std::uint64_t mb{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), mb);
    if (error != std::errc{} || end != text.data() + text.size() || mb > 2048) {
        return DefaultBytes;
    }
    return mb * MiB;
}

constexpr std::uint64_t PrewarmBlocks(std::uint64_t budget) {
    const auto blocks = budget / BlockBytes;
    return blocks < 8 ? blocks : 8;
}

// A soft retention budget: in-flight or CPU-held buffers are never reclaimed.
// Allow streaming bursts to reuse memory for five seconds before trimming excess.
inline bool ShouldTrim(std::chrono::steady_clock::duration idle,
                       std::uint64_t allocated, std::uint64_t budget,
                       bool dedicated, bool gpu_free, bool held = false) {
    return gpu_free && !held &&
           ((allocated > budget && idle >= std::chrono::seconds{5}) ||
            (dedicated && idle >= std::chrono::seconds{30}));
}
} // namespace Vulkan::StagingRetention
