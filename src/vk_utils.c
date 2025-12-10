#include <stdlib.h>
#include <string.h>

#include "common.h"

static uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
    value = value < min ? min : value;
    value = value > max ? max : value;

    return value;
}

static const uint8_t CANNOT_FIND_ALL_FAMILIES = 0x37;

bool                 hasReqValidationLayerSupport(
                    const uint32_t validation_layers_size,
                    const char**   validation_layers
                ) {
    uint32_t avail_layer_size;
    vkEnumerateInstanceLayerProperties(&avail_layer_size, NULL);

    VkLayerProperties available_layers[avail_layer_size];
    vkEnumerateInstanceLayerProperties(&avail_layer_size, available_layers);

    for (uint32_t req_layer_index = 0; req_layer_index < validation_layers_size;
         req_layer_index++) {
        bool layer_found = false;

        for (uint32_t avail_layer_index = 0;
             avail_layer_index < avail_layer_size; avail_layer_index++) {
            if (strcmp(
                    validation_layers[req_layer_index],
                    available_layers[avail_layer_index].layerName
                ) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}

static bool checkDeviceExtensionSupport(const VkPhysicalDevice device) {
    uint32_t avail_ext_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &avail_ext_count, NULL);

    VkExtensionProperties avail_exts[avail_ext_count];
    vkEnumerateDeviceExtensionProperties(
        device, NULL, &avail_ext_count, avail_exts
    );

    for (uint32_t req_ext_index = 0;
         req_ext_index < REQUIRED_DEVICE_EXTENSION_SIZE; req_ext_index++) {
        bool extension_found = false;

        for (uint32_t avail_ext_index = 0; avail_ext_index < avail_ext_count;
             avail_ext_index++) {
            if (strcmp(
                    REQUIRED_DEVICE_EXTENSION[req_ext_index],
                    avail_exts[avail_ext_index].extensionName
                ) == 0) {
                extension_found = true;
                break;
            }
        }

        if (!extension_found) {
            return false;
        }
    }

    return true;
}

bool isDeviceSuitable(
    const VkPhysicalDevice device,
    const VkSurfaceKHR     surface
) {
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    bool has_extension_support = checkDeviceExtensionSupport(device);

    if (!has_extension_support) return false;

    SwapchainSupportDetail* details = querySwapchainSupport(device, surface);
    bool                    adequate_swapchain_support =
        details->formats != NULL && details->present_mode != NULL;
    destroySwapchainSupportDetail(details);

    return indices.has_value_bitmap != CANNOT_FIND_ALL_FAMILIES &&
           adequate_swapchain_support;
}

QueueFamilyIndices
findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
    QueueFamilyIndices indices = {.has_value_bitmap = 0, .graphic_family = 0};

    uint32_t           queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queue_family_count, queue_families
    );

    for (uint32_t queue_family_index = 0;
         // graphic-family queue support
         queue_family_index < queue_family_count; queue_family_index++) {
        if (queue_families[queue_family_index].queueFlags &
            VK_QUEUE_GRAPHICS_BIT) {
            indices.graphic_family = queue_family_index;
            indices.has_value_bitmap += 0b1;
        }

        // presentation-family queue support
        {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                device, queue_family_index, surface, &present_support
            );

            if (present_support) {
                indices.present_family = queue_family_index;
                indices.has_value_bitmap += 0b10;
            }
        }

        if (indices.has_value_bitmap == 0b11) {
            return indices;
        }
    }

    // not all families are supported by the given device
    indices.has_value_bitmap = CANNOT_FIND_ALL_FAMILIES;
    return indices;
}

VkDeviceQueueCreateInfo* makeQueueCreateInfo(
    uint32_t* create_info_size,
    float     priority,
    uint32_t  family_indices[],
    uint32_t  indices_size
) {
    // Deduplicate family_indices
    // This is ran once and on small array so a linear search is OK
    uint32_t deduplicated_family_indices[indices_size];
    uint32_t dd_size = 0;

    for (uint32_t fi_index = 0; fi_index < indices_size; fi_index++) {
        bool found_duplicate = false;

        for (uint32_t dd_index = 0; dd_index < dd_size; dd_index++) {
            if (deduplicated_family_indices[dd_index] ==
                family_indices[fi_index]) {
                found_duplicate = true;
                break;
            }
        }

        if (!found_duplicate) {
            deduplicated_family_indices[dd_size] = family_indices[fi_index];
            dd_size++;
        }
    }

    // Construct queue create info
    *create_info_size = dd_size;
    VkDeviceQueueCreateInfo* queue_create_infos =
        malloc(sizeof(VkDeviceQueueCreateInfo) * dd_size);

    for (uint32_t dd_index = 0; dd_index < dd_size; dd_index++) {
        queue_create_infos[dd_index] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = deduplicated_family_indices[dd_index],
            .queueCount = 1,
            .pQueuePriorities = &priority
        };
    }

    return queue_create_infos;
}

void destroyQueueCreateInfo(VkDeviceQueueCreateInfo* queue_create_info) {
    free(queue_create_info);
}

SwapchainSupportDetail*
querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainSupportDetail* details = malloc(sizeof(SwapchainSupportDetail));

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, surface, &details->capabilities
    );

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &details->format_size, NULL
    );

    if (details->format_size != 0) {
        details->formats =
            malloc(sizeof(VkSurfaceFormatKHR) * details->format_size);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, surface, &details->format_size, details->formats
        );
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &details->present_mode_size, NULL
    );

    if (details->present_mode_size != 0) {
        details->present_mode =
            malloc(sizeof(VkPresentModeKHR) * details->present_mode_size);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &details->present_mode_size, details->present_mode
        );
    }

    return details;
}

void destroySwapchainSupportDetail(SwapchainSupportDetail* support_detail) {
    free(support_detail->formats);
    free(support_detail->present_mode);
    free(support_detail);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const VkSurfaceFormatKHR* formats,
    uint32_t                  format_size
) {
    for (uint32_t index = 0; index < format_size; index++) {
        if (formats[index].format == VK_FORMAT_B8G8R8_SRGB &&
            formats[index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[index];
        }
    }

    return formats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const VkPresentModeKHR* present_mode,
    uint32_t                present_mode_size
) {
    for (uint32_t index = 0; index < present_mode_size; index++) {
        if (present_mode[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR* capabilities,
    GLFWwindow*                     window_handle
) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window_handle, &width, &height);

    width = clamp(
        width, capabilities->minImageExtent.width,
        capabilities->maxImageExtent.width
    );
    height = clamp(
        width, capabilities->minImageExtent.height,
        capabilities->maxImageExtent.height
    );

    return (VkExtent2D){width, height};
}