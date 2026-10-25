// SPDX-License-Identifier: GPL-2.0-or-later
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <vector>
#include <vulkan/vulkan.h>

#include "video_core/renderer_vulkan/compiler_cache_policy.h"

static void Check(bool ok) {
    if (!ok) { std::fputs("compiler cache test failed\n", stderr); std::exit(1); }
}
static void Success(VkResult result) { Check(result == VK_SUCCESS); }

int main() {
    using Vulkan::ShareCompilerPipelineCache;
    constexpr std::uint64_t GiB = std::uint64_t{1} << 30;
    Check(ShareCompilerPipelineCache(4 * GiB, false));
    Check(!ShareCompilerPipelineCache(8 * GiB, false));
    Check(!ShareCompilerPipelineCache(0, false));
    Check(!ShareCompilerPipelineCache(4 * GiB, true));
    Check(!ShareCompilerPipelineCache(4 * GiB, false, "0"));
    Check(ShareCompilerPipelineCache(8 * GiB, false, "1"));
    Check(ShareCompilerPipelineCache(4 * GiB, false, "auto"));

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &app;
    VkInstance instance{};
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS) {
        std::puts("SKIP: Vulkan unavailable (policy checks passed)");
        return 0;
    }
    std::uint32_t count = 0;
    Success(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    if (!count) {
        vkDestroyInstance(instance, nullptr);
        std::puts("SKIP: no Vulkan device (policy checks passed)");
        return 0;
    }
    std::vector<VkPhysicalDevice> devices(count);
    Success(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
    auto physical = devices.front();
    VkPhysicalDeviceProperties props{};
    for (auto candidate : devices) {
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physical = candidate;
            break;
        }
    }
    vkGetPhysicalDeviceProperties(physical, &props);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, queues.data());
    std::uint32_t family = 0;
    while (family < count && !(queues[family].queueFlags & VK_QUEUE_COMPUTE_BIT)) { ++family; }
    Check(family < count);
    const float priority = 1;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;
    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    VkDevice device{};
    Success(vkCreateDevice(physical, &dci, nullptr, &device));

    std::ifstream file(CACHE_TEST_SHADER_PATH, std::ios::binary | std::ios::ate);
    Check(bool(file));
    const auto bytes = static_cast<std::size_t>(file.tellg());
    Check(bytes && bytes % sizeof(std::uint32_t) == 0);
    std::vector<std::uint32_t> code(bytes / sizeof(std::uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), bytes);
    Check(bool(file));
    VkShaderModuleCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sci.codeSize = bytes;
    sci.pCode = code.data();
    VkShaderModule module{};
    Success(vkCreateShaderModule(device, &sci, nullptr, &module));
    VkDescriptorSetLayoutBinding binding{};
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    VkDescriptorSetLayoutCreateInfo dsci{};
    dsci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsci.bindingCount = 1;
    dsci.pBindings = &binding;
    VkDescriptorSetLayout descriptor{};
    Success(vkCreateDescriptorSetLayout(device, &dsci, nullptr, &descriptor));
    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &descriptor;
    VkPipelineLayout layout{};
    Success(vkCreatePipelineLayout(device, &plci, nullptr, &layout));
    VkPipelineCacheCreateInfo cci{};
    cci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    // No EXTERNALLY_SYNCHRONIZED flag: concurrent creation must be driver-synchronized.
    VkPipelineCache cache{};
    Success(vkCreatePipelineCache(device, &cci, nullptr, &cache));
    std::atomic<unsigned> completed{0};
    std::vector<std::thread> workers;
    for (unsigned worker = 0; worker < 6; ++worker) {
        workers.emplace_back([&, worker] {
            for (unsigned job = 0; job < 8; ++job) {
                const std::uint32_t value = worker * 8 + job;
                VkSpecializationMapEntry entry{};
                entry.size = sizeof(value);
                VkSpecializationInfo specialization{};
                specialization.mapEntryCount = 1;
                specialization.pMapEntries = &entry;
                specialization.dataSize = sizeof(value);
                specialization.pData = &value;
                VkComputePipelineCreateInfo pci{};
                pci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                pci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                pci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                pci.stage.module = module;
                pci.stage.pName = "main";
                pci.stage.pSpecializationInfo = &specialization;
                pci.layout = layout;
                VkPipeline pipeline{};
                Success(vkCreateComputePipelines(device, cache, 1, &pci, nullptr, &pipeline));
                vkDestroyPipeline(device, pipeline, nullptr);
                ++completed;
            }
        });
    }
    // Exercise the same source-cache snapshot/merge the game's saver uses, while
    // workers compile. The destination is private to this thread.
    while (completed < 48) {
        // Shared-mode saver snapshots the live cache without allocating a second cache.
        std::size_t live_size = 0;
        Success(vkGetPipelineCacheData(device, cache, &live_size, nullptr));
        std::vector<unsigned char> live_snapshot(live_size);
        const auto live_result = vkGetPipelineCacheData(device, cache, &live_size,
                                                       live_snapshot.data());
        Check(live_result == VK_SUCCESS || live_result == VK_INCOMPLETE);
        VkPipelineCache scratch{};
        Success(vkCreatePipelineCache(device, &cci, nullptr, &scratch));
        Success(vkMergePipelineCaches(device, scratch, 1, &cache));
        std::size_t size = 0;
        Success(vkGetPipelineCacheData(device, scratch, &size, nullptr));
        std::vector<unsigned char> snapshot(size);
        Success(vkGetPipelineCacheData(device, scratch, &size, snapshot.data()));
        vkDestroyPipelineCache(device, scratch, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    for (auto& worker : workers) { worker.join(); }
    std::size_t size = 0;
    Success(vkGetPipelineCacheData(device, cache, &size, nullptr));
    std::vector<unsigned char> snapshot(size);
    Success(vkGetPipelineCacheData(device, cache, &size, snapshot.data()));
    cci.initialDataSize = size;
    cci.pInitialData = snapshot.data();
    VkPipelineCache restored{};
    Success(vkCreatePipelineCache(device, &cci, nullptr, &restored));
    vkDestroyPipelineCache(device, restored, nullptr);
    vkDestroyPipelineCache(device, cache, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptor, nullptr);
    vkDestroyShaderModule(device, module, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::printf("PASS: shared compiler cache, 48 pipelines on 6 workers, snapshot/reload (%s)\n",
                props.deviceName);
}
