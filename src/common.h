#ifndef XDD__COMMON_H__
#define XDD__COMMON_H__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct QueueFamilyIndices {
    uint8_t  has_value_bitmap;
    uint32_t graphic_family;
    uint32_t present_family;
} QueueFamilyIndices;

typedef struct ApplicationData {
    GLFWwindow*      window_handle;
    VkInstance       vulkan_instance;
    VkPhysicalDevice physical_device;
    VkDevice         vulkan_device;
    VkQueue          graphic_queue;
    VkSurfaceKHR     surface;
} ApplicationData;

bool hasReqValidationLayerSupport(
    const uint32_t validation_layers_size,
    const char**   validation_layers
);

bool isDeviceSuitable(VkPhysicalDevice device);

QueueFamilyIndices
findQueueFamilies(const ApplicationData* app_data);

#endif