# RAM optimization: 4 GiB discrete GPUs

## Target and observed hardware

Investigated on 2026-10-25: NVIDIA RTX 3050 Ti Laptop (4 GiB dedicated VRAM),
Intel i7-12700H, 15.73 GiB usable system RAM, Windows. The player reports 10–12 GB
RAM and 10–15 FPS. That original baseline has not been reproduced with a controlled
same-scene comparison. The main Windows renderer has now been built and tested using
the player's CUSA00004 executable (verified 1.07 hash despite APP_VER 01.00 metadata).
The latest three-minute test used precise readbacks and on-demand shader loading:
7.900–8.325 GiB working set, 8.371–8.728 GiB private commit; the last 24 frame-stat
windows ranged from 25.8 to 30 FPS (median 29), with a worst frame of 200.4 ms.
The target of at most 8 GB RAM and stable 30 FPS is **not achieved**.

PS4's shared CPU/GPU memory does not automatically translate to the same PC footprint.
The port can retain guest data, Vulkan backing, staging and driver state simultaneously.
Overcommitted VRAM can also consume system RAM. Conversely, mapped aliases and shared
pages mean process working set is not a count of unique physical bytes. Record RAM,
private commit, GPU dedicated/shared memory and frametimes separately.

## Changes

These changes target the main `port/gpu` renderer, not the separate experimental `gpu_v2`.

1. **Sparse page metadata.** `PageManager` previously value-initialized two byte counters
   for every 4 KiB page in a 40-bit address space: 512 MiB on Windows. It now uses a
   2 MiB directory and allocates one 2 KiB metadata block per watched 4 MiB region.
   Six GiB of contiguous watched guest memory needs 5 MiB including the directory.
   Counters retain all eight bits, the existing region locks protect first allocation,
   and blocks remain alive until shutdown so page references cannot dangle.
2. **Denser sparse GPU backing.** Discrete GPUs with a nonzero startup budget of up to
   4 GiB use 2 MiB residency granules and 64 MiB allocation chunks, instead of 16/256 MiB.
   Suballocation remains in use, avoiding an allocation per sparse page. The device's
   required sparse-block alignment still takes precedence. Other GPUs keep 16/256 MiB.
3. **Bounded upload packing.** On those small GPUs, large buffer uploads reuse staging
   windows of at most 64 MiB (other GPUs: 256 MiB). Merged CPU writes now use this same
   path rather than making a staging allocation as large as all merged writes. Disjoint
   ranges are packed with 16-byte source alignment; address gaps are not copied.
   Reuse waits for completion, preserving order and every payload byte. This bounds the
   temporary allocation for one large buffer upload, not all in-flight uploads or images.
4. **Current VRAM budget.** The image collector follows the current driver budget on
   small discrete GPUs as well as integrated GPUs. An explicit launcher budget is capped
   by a smaller current driver budget, instead of allowing the launcher to override it.
5. **Shared host staging retention.** The previous change shares a 128 MiB idle retention
   budget between upload/readback caches. In-flight data can exceed it. Existing explicit
   256/512 MiB performance presets still take precedence.
6. **Optional demand loading.** `ISS_SHADER_ON_DEMAND=1` uses the existing per-program
   SPIR-V loader without bulk-loading the whole game's pipeline cache. It works without
   pipeline-binary extensions, retains the existing cache, and uses the normal first-use
   pipeline build path. The advanced launcher option defaults off; the local test command
   enables it. This did not materially reduce measured peak working set in the short test
   versus the preceding precise session, so it is not evidence of meeting the RAM target.

These changes pack and stream existing bytes; they do not recompress game assets or
change shader-visible formats. Arbitrary compressed data cannot be consumed by the
existing shaders without a decoder or an additional unpacked copy. Hot-path compression
would need profiling of compression ratio, decode cost and access patterns first.

## Verification and limits

`ram-layout-test` covers zero initialization, full address range, counter precision,
region-locked concurrent allocation, and byte-exact packing of disjoint ranges including
odd sizes, tails and a 2400 MiB transfer, for both 64/256 MiB windows.
`staging-retention-test` covers retention budgets and GPU/CPU ownership guards.
Both targets are available in `port/gpu/CMakeLists.txt` and the dependency-light
`port/tests/perf/CMakeLists.txt` project. The GPU CI discovers `*-test` targets automatically.

Windows standalone microbenchmark, same executable and checksum (3145728), representing
6 GiB of watched addresses:

| Metadata implementation | Payload allocation | Process working set | Private commit |
|---|---:|---:|---:|
| Previous dense layout | 512 MiB | 541405184 bytes | 538562560 bytes |
| Sparse layout | 5 MiB | 9895936 bytes | 6127616 bytes |

These are isolated metadata-process measurements, **not whole-game savings or FPS**.
Allocator overhead and active address regions affect results. A synthetic model with
64 small requests spaced 16 MiB apart uses 1024 MiB of old granules versus 128 MiB of
new granules; this is a model of fragmentation, not a measured gameplay allocation.

Reproduce the metadata measurement after building `ram-layout-test`:

```text
ram-layout-test
ram-layout-test dense
ram-layout-test sparse
```

For gameplay, build the complete runtime and GPU library with the supported MSYS2
CLANG64 toolchain (`port/packaging/windows/README.md`). The full main-renderer Windows
build and the RAM/retention/warm-up tests passed locally. Further gameplay validation
is needed for other scenes and long sessions. Compare old/new builds with the
same save, route, resolution, preset and warmed shader cache for at least 3 minutes.
Enable `ISS_FRAME_STATS=1` to record FPS and worst-frame windows in the game log.
Sample the running loader's PID from PowerShell (replace the example PID):

```powershell
Get-Process | Where-Object ProcessName -Match 'tlg-probe|infamous'
.\scripts\measure_ram.ps1 -ProcessId 1234 -Seconds 180 -OutputFile "$env:TEMP\iss-memory.csv"
```

The sampler records process working set, private commit, and dedicated/shared GPU usage
when Windows exposes those counters. Do not add these columns: they have different
accounting and may overlap. It does not measure FPS. Stable 30 FPS needs frame times
near 33.3 ms; five-second average FPS alone cannot establish stability.

Smaller granules can increase sparse-bind work, and smaller upload windows can add
submission waits during large loads. These tradeoffs require in-game A/B validation.
No hard 8 GB cap is imposed: withholding live memory or discarding GPU-written bytes
would risk failed allocations or corrupt output.

The first live launch inherited `gpu_readbacks=0` (fast mode) from the existing player
settings. Its roughly 30 FPS observations are invalid for judging the full-effects target.
The setting was corrected to precise mode (`ISS_READBACKS=2`, eager readback off), with
a backup of the previous launcher settings. Neon particles and illumination were observed
in the precise run; smoke and every other scene/effect have not been exhaustively validated.
Raw latest results are in `port/out/ram-demand.csv`, `ram-demand-results.json` and the
game log recorded in `ram-demand-logpath.txt`; these local artifacts are ignored by Git.

## Bibliography and application

- [Erfan Ahmadi, Keeping your staging buffer fixed size!, Vulkanised 2023](https://www.vulkan.org/user/pages/09.events/vulkanised-2023/vulkanised_2023_keeping_your_staging_buffer_fixed_size.pdf),
  slides 4–7: bounded streaming allocations, submit on overflow, reuse after a completion
  fence/timeline. Applied here to merged writes and the existing part uploader.
- [Khronos Vulkan Guide: Memory Allocation](https://docs.vulkan.org/guide/latest/memory_allocation.html):
  suballocation and the difference between discrete-memory transfers and UMA. Applied
  by keeping chunk suballocation while reducing granularity on small discrete GPUs.
- [Vulkan Memory Allocator: Staying within budget](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/staying_within_budget.html):
  dynamic budgets, smaller allocation blocks, and overcommit potentially moving GPU
  memory into system RAM. Applied to allocation chunk size and the collector's budget.
- [VMA: Recommended usage patterns](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html):
  direct host access versus staged device-local resources has hardware-dependent
  bandwidth tradeoffs; zero-copy is not enabled globally on the discrete NVIDIA GPU.

## Shared-cache save snapshots

Shared compiler mode now serializes the live, internally synchronized driver cache directly. It avoids the temporary Vulkan cache previously populated with a full merge on every changed save. Private worker mode retains the scratch merge. Failed or incomplete snapshots leave the existing disk cache intact and retry later. This removes a transient duplicate; it does not establish a specific resident-memory saving or FPS improvement. The concurrent snapshot/reload test passed on the RTX 3050 Ti Laptop GPU. See [vkGetPipelineCacheData](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetPipelineCacheData.html).

The Qt launcher now has a Mi equipo page reusing the existing asynchronous local hardware report (CPU, RAM, GPU, VRAM, driver and Vulkan), without adding another hardware polling loop. The local test installation has convenience executables in `port/out/`; those machine-specific build artifacts are not distributed in the fork. For a source checkout, use the upstream build and launcher instructions in the main README and `port/packaging/windows/README.md`.
