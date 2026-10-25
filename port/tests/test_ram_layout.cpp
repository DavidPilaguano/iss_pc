// SPDX-License-Identifier: GPL-2.0-or-later
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string_view>
#include <thread>
#include <vector>

#include "common/sparse_page_table.h"
#include "video_core/buffer_cache/residency_policy.h"
#include "video_core/buffer_cache/upload_packing.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

static void PrintProcessMemory() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX memory{};
    if (K32GetProcessMemoryInfo(GetCurrentProcess(),
                               reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
        std::cout << " working_set_bytes=" << memory.WorkingSetSize
                  << " private_commit_bytes=" << memory.PrivateUsage;
    }
#endif
}

static void Check(bool ok) {
    if (!ok) {
        std::cerr << "RAM layout check failed\n";
        std::exit(1);
    }
}

constexpr std::uint64_t MiB = 1024 * 1024;
constexpr std::size_t PageCount = std::size_t{1} << 28;
constexpr std::size_t ActivePages = (std::size_t{6} << 30) / 4096;
struct Counters { std::uint8_t writes, reads; };
using Table = Common::SparsePageTable<Counters, PageCount, 1024>;

static void SparseTableTest() {
    auto table = std::make_unique<Table>();
    Check(table->AllocatedBytes() == 2 * MiB);
    for (std::size_t i = 0; i < ActivePages; ++i) {
        auto& state = (*table)[i];
        Check(state.writes == 0 && state.reads == 0);
        state.writes = 2;
        state.reads = 1;
    }
    Check(table->AllocatedBytes() == 5 * MiB);
    for (std::size_t i = 0; i < ActivePages; ++i) {
        Check((*table)[i].writes == 2 && (*table)[i].reads == 1);
    }
    Check((*table)[PageCount - 1].writes == 0); // full 40-bit address space
    (*table)[PageCount - 1].reads = 255;
    Check((*table)[PageCount - 1].reads == 255); // no counter precision lost

    // The production region lock serializes access to each block, including its
    // first allocation. Exercise first-touch races under that same contract.
    auto concurrent = std::make_unique<Table>();
    std::array<std::mutex, 4> locks;
    std::vector<std::thread> workers;
    for (unsigned t = 0; t < 8; ++t) {
        workers.emplace_back([&, t] {
            for (unsigned i = 0; i < 100; ++i) {
                const auto block = t % locks.size();
                std::scoped_lock lock{locks[block]};
                ++(*concurrent)[block * 1024 + i].writes;
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    Check(concurrent->AllocatedBytes() == 2 * MiB + 4 * 2048);
    for (unsigned b = 0; b < 4; ++b) {
        for (unsigned i = 0; i < 100; ++i) {
            Check((*concurrent)[b * 1024 + i].writes == 2);
        }
    }
}

static void UploadTest(std::uint64_t capacity) {
    VideoCore::UploadPacking packing{capacity};
    const std::array<std::uint64_t, 10> ranges{0, 4096, capacity - 4096, 16,
                                              2400 * MiB, 1, 31, 5, capacity - 1, 4096};
    std::uint64_t window_used = 0, total = 0, expected = 0;
    for (auto bytes : ranges) {
        expected += bytes;
        while (bytes != 0) {
            const auto slice = packing.Next(bytes);
            Check(slice.size > 0 && slice.size <= bytes);
            Check(slice.offset == (window_used + 15) / 16 * 16);
            Check(slice.offset % 16 == 0);
            Check(slice.offset + slice.size <= capacity);
            total += slice.size;
            bytes -= slice.size;
            window_used = slice.offset + slice.size;
            if (slice.full) {
                Check(window_used <= capacity && capacity - window_used < 16);
                window_used = 0;
            }
        }
    }
    Check(total == expected); // every byte, including disjoint/tail ranges, exactly once
}

static void ResidencyTest() {
    const auto small = VideoCore::ChooseResidencyPolicy(4 * 1024 * MiB, false);
    const auto large = VideoCore::ChooseResidencyPolicy(8 * 1024 * MiB, false);
    Check(small.granule == 2 * MiB && small.chunk == 64 * MiB);
    Check(small.upload_part == 64 * MiB);
    Check(large.granule == 16 * MiB && large.chunk == 256 * MiB);
    Check(VideoCore::ChooseResidencyPolicy(4 * 1024 * MiB, true).granule == large.granule);
    Check(VideoCore::ChooseResidencyPolicy(0, false).granule == large.granule);
    std::set<std::uint64_t> old_granules, new_granules;
    for (unsigned i = 0; i < 64; ++i) {
        const std::uint64_t address = i * 16 * MiB + 4096;
        old_granules.insert(address / large.granule);
        new_granules.insert(address / small.granule);
    }
    Check(old_granules.size() * large.granule == 1024 * MiB);
    Check(new_granules.size() * small.granule == 128 * MiB);
    UploadTest(small.upload_part);
    UploadTest(large.upload_part);
}

int main(int argc, char** argv) {
    if (argc == 2) {
        const auto start = std::chrono::steady_clock::now();
        std::uint64_t checksum = 0;
        std::uint64_t bytes = 0;
        if (std::string_view{argv[1]} == "dense") {
            std::vector<Counters> table(PageCount);
            for (std::size_t i = 0; i < ActivePages; ++i) {
                table[i].writes = 2;
            }
            for (const auto state : table) {
                checksum += state.writes;
            }
            bytes = table.size() * sizeof(Counters);
            PrintProcessMemory();
        } else {
            auto table = std::make_unique<Table>();
            for (std::size_t i = 0; i < ActivePages; ++i) {
                (*table)[i].writes = 2;
            }
            for (std::size_t i = 0; i < ActivePages; ++i) {
                checksum += (*table)[i].writes;
            }
            bytes = table->AllocatedBytes();
            PrintProcessMemory();
        }
        Check(checksum == ActivePages * 2);
        std::cout << " metadata_bytes=" << bytes << " checksum=" << checksum
                  << " elapsed_ms=" << std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start).count() << '\n';
        return 0;
    }
    SparseTableTest();
    ResidencyTest();
    std::cout << "RAM layout ok: metadata 512 -> 5 MiB for 6 GiB tracked; "
                 "sparse residency model 1024 -> 128 MiB; uploads bounded to 64 MiB\n";
}
