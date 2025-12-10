#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define VALIDATION_LAYERS_SIZE 1
static const char* VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};

const uint32_t     REQUIRED_DEVICE_EXTENSION_SIZE = 1;
const char* REQUIRED_DEVICE_EXTENSION[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static ApplicationData app_data = {};

// =================================

static void initWindow(void) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    app_data.window_handle =
        glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "xddcube", NULL, NULL);
}

static void selectPhysicalDevice(void) {
    app_data.physical_device = VK_NULL_HANDLE;

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(app_data.vulkan_instance, &device_count, NULL);

    if (device_count == 0) {
        printf("Cannot find any GPU with Vulkan support.\n");
        abort();
    }

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(
        app_data.vulkan_instance, &device_count, devices
    );

    for (uint32_t device_index = 0; device_index < device_count;
         device_index++) {
        if (isDeviceSuitable(devices[device_index], app_data.surface)) {
            app_data.physical_device = devices[device_index];
            break;
        }
    }

    if (app_data.physical_device == VK_NULL_HANDLE) {
        printf("Cannot find any suitable GPU.\n");
        abort();
    }
}

static void createLogicalDevice(void) {
    QueueFamilyIndices family_indices =
        findQueueFamilies(app_data.physical_device, app_data.surface);

    uint32_t                 create_info_size;
    VkDeviceQueueCreateInfo* queue_create_info = makeQueueCreateInfo(
        &create_info_size, 1.0f,
        (uint32_t[]){family_indices.graphic_family,
                     family_indices.present_family},
        2
    );

    VkPhysicalDeviceFeatures device_feature = {};

    VkDeviceCreateInfo       create_info = {
              .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
              .pQueueCreateInfos = queue_create_info,
              .queueCreateInfoCount = create_info_size,
              .pEnabledFeatures = &device_feature,
              .enabledExtensionCount = REQUIRED_DEVICE_EXTENSION_SIZE,
              .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSION,
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
        destroyQueueCreateInfo(queue_create_info);
        abort();
    }
    destroyQueueCreateInfo(queue_create_info);

    vkGetDeviceQueue(
        app_data.vulkan_device, family_indices.graphic_family, 0,
        &app_data.graphic_queue
    );
    vkGetDeviceQueue(
        app_data.vulkan_device, family_indices.present_family, 0,
        &app_data.present_queue
    );
}

static void createSwapChain(void) {
    SwapchainSupportDetail* swapchain_support =
        querySwapchainSupport(app_data.physical_device, app_data.surface);
    QueueFamilyIndices queue_family_indices =
        findQueueFamilies(app_data.physical_device, app_data.surface);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(
        swapchain_support->formats, swapchain_support->format_size
    );
    VkExtent2D extent = chooseSwapExtent(
        &swapchain_support->capabilities, app_data.window_handle
    );
    VkPresentModeKHR present_mode = chooseSwapPresentMode(
        swapchain_support->present_mode, swapchain_support->present_mode_size
    );

    uint32_t image_count = swapchain_support->capabilities.minImageCount + 1;
    if (swapchain_support->capabilities.maxImageCount != 0 &&
        image_count > swapchain_support->capabilities.maxImageCount) {
        image_count = swapchain_support->capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = app_data.surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapchain_support->capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (queue_family_indices.graphic_family !=
        queue_family_indices.present_family) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices =
            (uint32_t[]){queue_family_indices.graphic_family,
                         queue_family_indices.present_family};
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if (vkCreateSwapchainKHR(
            app_data.vulkan_device, &swapchain_create_info, NULL,
            &app_data.swapchain
        )) {
        printf("Failed to create swapchain\n");
        abort();
    }

    vkGetSwapchainImagesKHR(
        app_data.vulkan_device, app_data.swapchain,
        &app_data.swapchain_image_size, NULL
    );
    app_data.swapchain_images =
        malloc(sizeof(VkImage) * app_data.swapchain_image_size);
    vkGetSwapchainImagesKHR(
        app_data.vulkan_device, app_data.swapchain,
        &app_data.swapchain_image_size, app_data.swapchain_images
    );

    app_data.swapchain_format = surface_format.format;
    app_data.swapchain_extent = extent;

    destroySwapchainSupportDetail(swapchain_support);
}

static void createImageView(void) {
    app_data.swapchain_image_views =
        malloc(sizeof(VkImageView) * app_data.swapchain_image_size);

    for (uint32_t index = 0; index < app_data.swapchain_image_size; index++) {
        VkImageViewCreateInfo view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = app_data.swapchain_images[index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = app_data.swapchain_format,
            .components =
                {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                 VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        if (vkCreateImageView(
                app_data.vulkan_device, &view_create_info, NULL,
                &app_data.swapchain_image_views[index]
            ) != VK_SUCCESS) {
            printf("Failed to create image view\n");
            abort();
        }
    }
}


static void createGraphicPipeline(void) {
    
}

static void initVulkan(void) {
#ifdef DEBUG
    if (!hasReqValidationLayerSupport(
            VALIDATION_LAYERS_SIZE, VALIDATION_LAYERS
        )) {
        printf("Requested validation layers support unavailable.\n");
        abort();
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
        abort();
    }

    if (glfwCreateWindowSurface(
            app_data.vulkan_instance, app_data.window_handle, NULL,
            &app_data.surface
        ) != VK_SUCCESS) {
        printf("Failed to create window surface\n");
        abort();
    }

    selectPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageView();
    createGraphicPipeline();
}

static void mainLoop(void) {
    while (!glfwWindowShouldClose(app_data.window_handle)) {
        glfwPollEvents();
    }
}

static void cleanup(void) {
    for (uint32_t index = 0; index < app_data.swapchain_image_size; index++) {
        vkDestroyImageView(
            app_data.vulkan_device, app_data.swapchain_image_views[index], NULL
        );
    }

    vkDestroySwapchainKHR(app_data.vulkan_device, app_data.swapchain, NULL);
    vkDestroyDevice(app_data.vulkan_device, NULL);
    vkDestroyInstance(app_data.vulkan_instance, NULL);

    // destroy application data
    free(app_data.swapchain_images);
    free(app_data.swapchain_image_views);

    glfwDestroyWindow(app_data.window_handle);
    glfwTerminate();
}

int main(void) {
    initWindow();
    initVulkan();

    printf("Initialization complete.\n");

    mainLoop();
    cleanup();

    return 0;
}
