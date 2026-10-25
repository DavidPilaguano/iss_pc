// SPDX-License-Identifier: GPL-2.0-or-later
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <vulkan/vulkan.h>

static void Check(bool ok) {
    if (!ok) { std::fputs("streaming upload test failed\n", stderr); std::exit(1); }
}
static void Success(VkResult result) { Check(result == VK_SUCCESS); }

int main() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    VkInstance instance{};
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS) {
        std::puts("SKIP: Vulkan unavailable"); return 0;
    }
    std::uint32_t count = 0;
    Success(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    if (!count) { vkDestroyInstance(instance, nullptr); std::puts("SKIP: no GPU"); return 0; }
    std::vector<VkPhysicalDevice> devices(count);
    Success(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
    auto physical = devices.front();
    VkPhysicalDeviceProperties props{};
    for (auto candidate : devices) {
        vkGetPhysicalDeviceProperties(candidate, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { physical = candidate; break; }
    }
    vkGetPhysicalDeviceProperties(physical, &props);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, families.data());
    std::uint32_t family = 0;
    while (family < count && !(families[family].queueFlags & VK_QUEUE_TRANSFER_BIT)) ++family;
    Check(family < count);
    const float priority = 1;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = family; qci.queueCount = 1; qci.pQueuePriorities = &priority;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
    VkDevice device{};
    Success(vkCreateDevice(physical, &dci, nullptr, &device));
    VkQueue queue{};
    vkGetDeviceQueue(device, family, 0, &queue);
    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceMemoryProperties(physical, &memory);
    struct Buffer { VkBuffer buffer{}; VkDeviceMemory memory{}; void* mapped{}; };
    const auto make_buffer = [&](VkDeviceSize size, VkBufferUsageFlags usage) {
        Buffer out;
        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size = size; bci.usage = usage;
        Success(vkCreateBuffer(device, &bci, nullptr, &out.buffer));
        VkMemoryRequirements needs{};
        vkGetBufferMemoryRequirements(device, out.buffer, &needs);
        std::uint32_t type = 0;
        constexpr auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        while (type < memory.memoryTypeCount &&
               (!(needs.memoryTypeBits & (1u << type)) ||
                (memory.memoryTypes[type].propertyFlags & flags) != flags)) ++type;
        Check(type < memory.memoryTypeCount);
        VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        mai.allocationSize = needs.size; mai.memoryTypeIndex = type;
        Success(vkAllocateMemory(device, &mai, nullptr, &out.memory));
        Success(vkBindBufferMemory(device, out.buffer, out.memory, 0));
        Success(vkMapMemory(device, out.memory, 0, VK_WHOLE_SIZE, 0, &out.mapped));
        return out;
    };
    constexpr VkDeviceSize Part = 64 * 1024, Total = 24 * Part + 20;
    auto destination = make_buffer(Total, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    struct Slot { Buffer data; VkCommandPool pool{}; VkCommandBuffer command{}; VkFence fence{}; };
    std::array<Slot, 2> slots{};
    for (auto& slot : slots) {
        slot.data = make_buffer(Part, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pci.queueFamilyIndex = family;
        Success(vkCreateCommandPool(device, &pci, nullptr, &slot.pool));
        VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cai.commandPool = slot.pool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 1;
        Success(vkAllocateCommandBuffers(device, &cai, &slot.command));
        VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        Success(vkCreateFence(device, &fci, nullptr, &slot.fence));
    }
    std::vector<std::uint8_t> expected(Total);
    for (std::size_t i = 0; i < expected.size(); ++i) expected[i] = (i * 17 + i / 251) & 255;
    unsigned part = 0;
    for (VkDeviceSize offset = 0; offset < Total; offset += Part, ++part) {
        auto& slot = slots[part % slots.size()];
        Success(vkWaitForFences(device, 1, &slot.fence, VK_TRUE, UINT64_MAX));
        Success(vkResetFences(device, 1, &slot.fence));
        Success(vkResetCommandPool(device, slot.pool, 0));
        const auto size = std::min(Part, Total - offset);
        std::memcpy(slot.data.mapped, expected.data() + offset, size);
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Success(vkBeginCommandBuffer(slot.command, &begin));
        VkBufferCopy copy{0, offset, size};
        vkCmdCopyBuffer(slot.command, slot.data.buffer, destination.buffer, 1, &copy);
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = destination.buffer; barrier.offset = offset; barrier.size = size;
        vkCmdPipelineBarrier(slot.command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0, 0, nullptr, 1, &barrier, 0, nullptr);
        Success(vkEndCommandBuffer(slot.command));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1; submit.pCommandBuffers = &slot.command;
        Success(vkQueueSubmit(queue, 1, &submit, slot.fence));
    }
    for (auto& slot : slots) Success(vkWaitForFences(device, 1, &slot.fence, VK_TRUE, UINT64_MAX));
    Check(!std::memcmp(destination.mapped, expected.data(), expected.size()));
    const auto free_buffer = [&](Buffer& buffer) {
        vkUnmapMemory(device, buffer.memory);
        vkDestroyBuffer(device, buffer.buffer, nullptr);
        vkFreeMemory(device, buffer.memory, nullptr);
    };
    for (auto& slot : slots) {
        vkDestroyFence(device, slot.fence, nullptr);
        vkDestroyCommandPool(device, slot.pool, nullptr);
        free_buffer(slot.data);
    }
    free_buffer(destination);
    vkDestroyDevice(device, nullptr); vkDestroyInstance(instance, nullptr);
    std::printf("PASS: two-slot Vulkan uploads, 25 parts, exact tail and bytes (%s)\n", props.deviceName);
}
