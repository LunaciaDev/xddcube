#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "common.h"

typedef struct ApplicationData {
    GLFWwindow* window_handle;
    VkInstance  vulkan_instance;
} ApplicationData;

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

static int initVulkan() {
#ifdef DEBUG
    if (!hasReqValidationLayerSupport(
            VALIDATION_LAYERS_SIZE, &VALIDATION_LAYERS[0]
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
        .ppEnabledLayerNames = &VALIDATION_LAYERS[0]
#else
        .enabledLayerCount = 0
#endif
    };

    if (vkCreateInstance(&create_info, NULL, &app_data.vulkan_instance) !=
        VK_SUCCESS) {
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
