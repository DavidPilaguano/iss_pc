// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>

namespace VideoCore {
// Pack disjoint guest ranges into one reusable staging window without copying the
// gaps. Offset and byte count remain exact: no lossy texture/data conversion.
class UploadPacking {
public:
    explicit UploadPacking(std::uint64_t capacity) : capacity{capacity} {
        assert(capacity != 0 && capacity % Alignment == 0);
    }
    struct Slice {
        std::uint64_t offset;
        std::uint64_t size;
        bool full;
    };
    Slice Next(std::uint64_t remaining) {
        assert(remaining != 0);
        const auto offset = (used + Alignment - 1) / Alignment * Alignment;
        const auto size = std::min(remaining, capacity - offset);
        used = offset + size;
        // Flush when the next aligned slice would not fit, even if a few padding
        // bytes remain. In particular, odd-sized merged writes must not loop on a
        // zero-length slice or give the next copy an unaligned source offset.
        const bool full = capacity - used < Alignment;
        if (full) {
            used = 0;
        }
        return {offset, size, full};
    }
private:
    static constexpr std::uint64_t Alignment = 16;
    std::uint64_t capacity;
    std::uint64_t used = 0;
};
} // namespace VideoCore
