#include <vulkan/vulkan_core.h>
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

#define DYNAMIC_STATES_SIZE 2
static const uint32_t DYNAMIC_STATES[] = {
    VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
};

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

static void createRenderPass(void) {
    VkAttachmentDescription color_attachment = {
        .format = app_data.swapchain_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    if (vkCreateRenderPass(
            app_data.vulkan_device, &render_pass_info, NULL,
            &app_data.render_pass
        ) != VK_SUCCESS) {
        printf("Cannot create render pass");
        abort();
    }
}

static void createGraphicPipeline(void) {
    int64_t frag_shader_size, vert_shader_size;

    char*   frag_shader = readFile("shaders/frag.spv", &frag_shader_size);
    char*   vert_shader = readFile("shaders/vert.spv", &vert_shader_size);

    VkShaderModule frag_shader_module = createShaderModule(
        frag_shader, frag_shader_size, app_data.vulkan_device
    );
    VkShaderModule vert_shader_module = createShaderModule(
        vert_shader, vert_shader_size, app_data.vulkan_device
    );

    VkPipelineShaderStageCreateInfo vert_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader_module,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo frag_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader_module,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shader_stage_info_vec[] = {
        vert_stage_info, frag_stage_info
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = DYNAMIC_STATES_SIZE,
        .pDynamicStates = DYNAMIC_STATES
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewport_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterization_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE
    };

    VkPipelineMultisampleStateCreateInfo multisample_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };

    if (vkCreatePipelineLayout(
            app_data.vulkan_device, &pipeline_layout_info, NULL,
            &app_data.pipeline_layout
        ) != VK_SUCCESS) {
        printf("Cannot create pipeline layout\n");
        abort();
    }

    VkGraphicsPipelineCreateInfo graphic_pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stage_info_vec,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state_info,
        .layout = app_data.pipeline_layout,
        .renderPass = app_data.render_pass,
        .subpass = 0
    };

    if (vkCreateGraphicsPipelines(
            app_data.vulkan_device, VK_NULL_HANDLE, 1, &graphic_pipeline_info,
            NULL, &app_data.pipeline
        ) != VK_SUCCESS) {
        printf("Failed to create graphic pipeline\n");
        abort();
    }

    vkDestroyShaderModule(app_data.vulkan_device, frag_shader_module, NULL);
    vkDestroyShaderModule(app_data.vulkan_device, vert_shader_module, NULL);

    free(frag_shader);
    free(vert_shader);
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
    createRenderPass();
    createGraphicPipeline();
}

static void mainLoop(void) {
    while (!glfwWindowShouldClose(app_data.window_handle)) {
        glfwPollEvents();
    }
}

static void cleanup(void) {
    vkDestroyPipeline(app_data.vulkan_device, app_data.pipeline, NULL);
    vkDestroyPipelineLayout(
        app_data.vulkan_device, app_data.pipeline_layout, NULL
    );
    vkDestroyRenderPass(app_data.vulkan_device, app_data.render_pass, NULL);

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
