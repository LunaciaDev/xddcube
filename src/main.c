#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "common.h"

static const uint32_t  WINDOW_WIDTH = 800;
static const uint32_t  WINDOW_HEIGHT = 600;
static const uint32_t  VALIDATION_LAYERS_SIZE = 1;
static const char*     VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};

static ApplicationData app_data = {};

// =================================

static void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    app_data.window_handle =
        glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "xddcube", NULL, NULL);
}

static int selectPhysicalDevice() {
    app_data.physical_device = VK_NULL_HANDLE;

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(app_data.vulkan_instance, &device_count, NULL);

    if (device_count == 0) {
        printf("Cannot find any GPU with Vulkan support.\n");
        return -1;
    }

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(
        app_data.vulkan_instance, &device_count, devices
    );

    for (uint32_t device_index = 0; device_index < device_count;
         device_index++) {
        if (isDeviceSuitable(devices[device_count])) {
            app_data.physical_device = devices[device_index];
            break;
        }
    }

    if (app_data.physical_device == VK_NULL_HANDLE) {
        printf("Cannot find any suitable GPU.\n");
        return -1;
    }

    return 0;
}

static int createLogicalDevice() {
    const float        queue_priority = 1.0f;

    QueueFamilyIndices indices = findQueueFamilies(&app_data);

    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices.graphic_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    VkPhysicalDeviceFeatures device_feature = {};

    VkDeviceCreateInfo       create_info = {
              .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
              .pQueueCreateInfos = &queue_create_info,
              .queueCreateInfoCount = 1,
              .pEnabledFeatures = &device_feature,
              .enabledExtensionCount = 0,
#ifdef DEBUG
        .enabledLayerCount = VALIDATION_LAYERS_SIZE,
        .ppEnabledLayerNames = VALIDATION_LAYERS
#else
        .enabledLayerCount = 0
#endif
    };

    if (vkCreateDevice(
            app_data.physical_device, &create_info, NULL,
            &app_data.vulkan_device
        ) != VK_SUCCESS) {
        printf("Failed to create logical device.\n");
        return -1;
    }

    vkGetDeviceQueue(
        app_data.vulkan_device, indices.graphic_family, 0,
        &app_data.graphic_queue
    );

    return 0;
}

static int initVulkan() {
#ifdef DEBUG
    if (!hasReqValidationLayerSupport(
            VALIDATION_LAYERS_SIZE, VALIDATION_LAYERS
        )) {
        printf("Requested validation layers support unavailable.\n");
        return -1;
    }
#endif

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "xddcube",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "NoEngine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions,
#ifdef DEBUG
        .enabledLayerCount = VALIDATION_LAYERS_SIZE,
        .ppEnabledLayerNames = VALIDATION_LAYERS
#else
        .enabledLayerCount = 0
#endif
    };

    if (vkCreateInstance(&create_info, NULL, &app_data.vulkan_instance) !=
        VK_SUCCESS) {
        return -1;
    }

    if (glfwCreateWindowSurface(app_data.vulkan_instance, app_data.window_handle, NULL, &app_data.surface) != VK_SUCCESS) {
        printf("Failed to create window surface\n");
        return -1;
    }

    if (selectPhysicalDevice() != 0) {
        return -1;
    }

    if (createLogicalDevice() != 0) {
        return -1;
    }

    return 0;
}

static void mainLoop() {
    while (!glfwWindowShouldClose(app_data.window_handle)) {
        glfwPollEvents();
    }
}

static void cleanup() {
    vkDestroyInstance(app_data.vulkan_instance, NULL);
    vkDestroyDevice(app_data.vulkan_device, NULL);

    glfwDestroyWindow(app_data.window_handle);
    glfwTerminate();
}

int main() {
    initWindow();

    if (initVulkan() != 0) {
        printf("Cannot create a vulkan instance\n");
        return -1;
    }

    printf("Initialization complete.\n");

    mainLoop();
    cleanup();

    return 0;
}
