// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>

namespace VideoCore {
struct ResidencyPolicy {
    std::uint64_t granule;
    std::uint64_t chunk;
    std::uint64_t upload_part;
};

// Small discrete GPUs cannot afford 16 MiB of backing for every tiny sparse range,
// or 256 MiB chunks with unused tails. Keep suballocation, but pack more tightly.
constexpr ResidencyPolicy ChooseResidencyPolicy(std::uint64_t budget, bool integrated) {
    constexpr std::uint64_t MiB = 1024 * 1024;
    return !integrated && budget != 0 && budget <= 4 * 1024 * MiB
               ? ResidencyPolicy{2 * MiB, 64 * MiB, 64 * MiB}
               : ResidencyPolicy{16 * MiB, 256 * MiB, 256 * MiB};
}
} // namespace VideoCore
