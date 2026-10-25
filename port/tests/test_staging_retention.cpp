// SPDX-License-Identifier: GPL-2.0-or-later
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "video_core/renderer_vulkan/staging_retention.h"

using namespace Vulkan::StagingRetention;
using namespace std::chrono_literals;

static void Check(bool result) {
    if (!result) {
        std::cerr << "staging retention check failed\n";
        std::exit(1);
    }
}

int main() {
    Check(Budget(nullptr) == 128 * MiB);
    Check(Budget("0") == 0);
    Check(Budget("64") == 64 * MiB);
    Check(Budget("2048") == 2048 * MiB);
    for (const char* invalid : {"", "-1", "garbage", "512x", "2049",
                                "18446744073709551615", "18446744073709551616"}) {
        Check(Budget(invalid) == DefaultBytes);
    }
    Check(PrewarmBlocks(Budget("0")) == 0);
    Check(PrewarmBlocks(Budget("64")) == 4);
    Check(PrewarmBlocks(Budget("512")) == 8);

    Check(!ShouldTrim(4999ms, 512 * MiB, DefaultBytes, false, true));
    Check(ShouldTrim(5s, 512 * MiB, DefaultBytes, false, true));
    Check(!ShouldTrim(60s, DefaultBytes, DefaultBytes, false, true));
    Check(!ShouldTrim(60s, 512 * MiB, 0, false, false)); // GPU still using it
    Check(!ShouldTrim(60s, 512 * MiB, 0, true, true, true)); // CPU-held readback
    Check(!ShouldTrim(29s, 64 * MiB, DefaultBytes, true, true));
    Check(ShouldTrim(30s, 64 * MiB, DefaultBytes, true, true));
    Check(ShouldTrim(5s, BlockBytes, 0, false, true));

    // A loading burst left 512 MiB of uploads plus 256 MiB of readbacks.
    // Both caches share the budget and only completed allocations are removed.
    struct Entry { std::uint64_t bytes; bool free; bool held; };
    std::vector<Entry> entries{{256 * MiB, true, false}, {BlockBytes, false, false},
                               {BlockBytes, true, true}};
    for (int i = 0; i < 30; ++i) {
        entries.push_back({BlockBytes, true, false});
    }
    std::uint64_t allocated = 768 * MiB;
    for (const auto& entry : entries) {
        if (ShouldTrim(5s, allocated, DefaultBytes, false, entry.free, entry.held)) {
            allocated -= entry.bytes;
        }
    }
    Check(allocated == DefaultBytes);
    std::cout << "staging retention ok\n";
}
