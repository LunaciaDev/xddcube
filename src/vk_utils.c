#include <stdbool.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "common.h"

bool hasReqValidationLayerSupport(
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

bool               isDeviceSuitable(VkPhysicalDevice device) { return true; }

QueueFamilyIndices findQueueFamilies(const ApplicationData* app_data) {
    QueueFamilyIndices indices = {.has_value_bitmap = 0, .graphic_family = 0};

    uint32_t           queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        app_data->physical_device, &queue_family_count, NULL
    );

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(
        app_data->physical_device, &queue_family_count, queue_families
    );

    for (uint32_t queue_family_index = 0;
         queue_family_index < queue_family_count; queue_family_index++) {
        if (queue_families[queue_family_index].queueFlags &
            VK_QUEUE_GRAPHICS_BIT) {
            indices.graphic_family = queue_family_index;
            indices.has_value_bitmap += 0b1;
        }

        {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                app_data->physical_device, queue_family_index,
                app_data->surface, &present_support
            );

            if (present_support) {
                indices.present_family = queue_family_index;
                indices.has_value_bitmap += 0b10;
            }
        }

        if (indices.has_value_bitmap == 0b11) {
            break;
        }
    }

    return indices;
}