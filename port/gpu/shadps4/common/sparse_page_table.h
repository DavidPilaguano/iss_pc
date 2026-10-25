// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>

namespace Common {
// Allocate metadata only for address regions actually watched by the GPU. The caller
// must hold its existing region lock for access; different blocks can be used in parallel.
// Blocks stay alive until destruction so a page reference cannot be invalidated by unmap.
template <typename T, std::size_t Count, std::size_t BlockEntries>
class SparsePageTable {
    static_assert(BlockEntries != 0 && Count % BlockEntries == 0);
    using Block = std::array<T, BlockEntries>;

public:
    T& operator[](std::size_t index) {
        assert(index < Count);
        auto& block = blocks[index / BlockEntries];
        if (!block) {
            block = std::make_unique<Block>(); // value-initialize counters to zero
            allocated.fetch_add(1, std::memory_order_relaxed);
        }
        return (*block)[index % BlockEntries];
    }

    std::size_t AllocatedBytes() const noexcept {
        return sizeof(blocks) + allocated.load(std::memory_order_relaxed) * sizeof(Block);
    }

private:
    std::array<std::unique_ptr<Block>, Count / BlockEntries> blocks{};
    std::atomic<std::size_t> allocated{};
};
} // namespace Common
