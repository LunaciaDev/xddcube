#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_FRAME_IN_FLIGHT 2

#ifdef DEBUG
#define VALIDATION_LAYERS_SIZE 1
static const char *VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};
#endif

const uint32_t REQUIRED_DEVICE_EXTENSION_SIZE = 1;
const char *REQUIRED_DEVICE_EXTENSION[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#define DYNAMIC_STATES_SIZE 2
static const uint32_t DYNAMIC_STATES[] = {
    VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
};

static struct AppState app_state = {};

// =================================

static void initWindow(void)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	app_state.window_handle = glfwCreateWindow(
	    WINDOW_WIDTH, WINDOW_HEIGHT, "xddcube", NULL, NULL
	);
}

static void selectPhysicalDevice(void)
{
	app_state.physical_device = VK_NULL_HANDLE;

	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(
	    app_state.vulkan_instance, &device_count, NULL
	);

	if (device_count == 0) {
		printf("Cannot find any GPU with Vulkan support.\n");
		abort();
	}

	VkPhysicalDevice devices[device_count];
	vkEnumeratePhysicalDevices(
	    app_state.vulkan_instance, &device_count, devices
	);

	for (uint32_t device_index = 0; device_index < device_count;
	     device_index++) {
		if (isDeviceSuitable(
			devices[device_index], app_state.surface
		    )) {
			app_state.physical_device = devices[device_index];
			break;
		}
	}

	if (app_state.physical_device == VK_NULL_HANDLE) {
		printf("Cannot find any suitable GPU.\n");
		abort();
	}
}

static void createLogicalDevice(void)
{
	struct QueueFamilyIndices family_indices =
	    findQueueFamilies(app_state.physical_device, app_state.surface);

	uint32_t create_info_size;
	VkDeviceQueueCreateInfo *queue_create_info = makeQueueCreateInfo(
	    &create_info_size,
	    1.0f,
	    (uint32_t[]){family_indices.graphic_family,
			 family_indices.present_family},
	    2
	);

	VkPhysicalDeviceFeatures device_feature = {};

	VkDeviceCreateInfo create_info = {
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
		app_state.physical_device,
		&create_info,
		NULL,
		&app_state.vulkan_device
	    )
	    != VK_SUCCESS) {
		printf("Failed to create logical device.\n");
		destroyQueueCreateInfo(queue_create_info);
		abort();
	}
	destroyQueueCreateInfo(queue_create_info);

	vkGetDeviceQueue(
	    app_state.vulkan_device,
	    family_indices.graphic_family,
	    0,
	    &app_state.graphic_queue
	);
	vkGetDeviceQueue(
	    app_state.vulkan_device,
	    family_indices.present_family,
	    0,
	    &app_state.present_queue
	);
}

static void createSwapChain(void)
{
	struct SwapchainSupportDetail *swapchain_support =
	    querySwapchainSupport(
		app_state.physical_device, app_state.surface
	    );
	struct QueueFamilyIndices queue_family_indices =
	    findQueueFamilies(app_state.physical_device, app_state.surface);

	VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(
	    swapchain_support->formats, swapchain_support->format_size
	);
	VkExtent2D extent = chooseSwapExtent(
	    &swapchain_support->capabilities, app_state.window_handle
	);
	VkPresentModeKHR present_mode = chooseSwapPresentMode(
	    swapchain_support->present_mode,
	    swapchain_support->present_mode_size
	);

	uint32_t image_count =
	    swapchain_support->capabilities.minImageCount + 1;
	if (swapchain_support->capabilities.maxImageCount != 0
	    && image_count > swapchain_support->capabilities.maxImageCount) {
		image_count = swapchain_support->capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {
	    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface = app_state.surface,
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

	if (queue_family_indices.graphic_family
	    != queue_family_indices.present_family) {
		swapchain_create_info.imageSharingMode =
		    VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.pQueueFamilyIndices =
		    (uint32_t[]){queue_family_indices.graphic_family,
				 queue_family_indices.present_family};
	} else {
		swapchain_create_info.imageSharingMode =
		    VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(
		app_state.vulkan_device,
		&swapchain_create_info,
		NULL,
		&app_state.swapchain
	    )) {
		printf("Failed to create swapchain\n");
		abort();
	}

	vkGetSwapchainImagesKHR(
	    app_state.vulkan_device,
	    app_state.swapchain,
	    &app_state.swapchain_image_size,
	    NULL
	);
	app_state.swapchain_images =
	    malloc(sizeof(VkImage) * app_state.swapchain_image_size);
	assert(app_state.swapchain_images != NULL);
	vkGetSwapchainImagesKHR(
	    app_state.vulkan_device,
	    app_state.swapchain,
	    &app_state.swapchain_image_size,
	    app_state.swapchain_images
	);

	app_state.swapchain_format = surface_format.format;
	app_state.swapchain_extent = extent;

	destroySwapchainSupportDetail(swapchain_support);
}

static void createImageView(void)
{
	app_state.swapchain_image_views =
	    malloc(sizeof(VkImageView) * app_state.swapchain_image_size);
	assert(app_state.swapchain_image_views != NULL);

	for (uint32_t index = 0; index < app_state.swapchain_image_size;
	     index++) {
		VkImageViewCreateInfo view_create_info = {
		    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		    .image = app_state.swapchain_images[index],
		    .viewType = VK_IMAGE_VIEW_TYPE_2D,
		    .format = app_state.swapchain_format,
		    .components =
			{VK_COMPONENT_SWIZZLE_IDENTITY,
				     VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
				     VK_COMPONENT_SWIZZLE_IDENTITY},
		    .subresourceRange = {
				     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				     .baseMipLevel = 0,
				     .levelCount = 1,
				     .baseArrayLayer = 0,
				     .layerCount = 1
		    }
		};

		if (vkCreateImageView(
			app_state.vulkan_device,
			&view_create_info,
			NULL,
			&app_state.swapchain_image_views[index]
		    )
		    != VK_SUCCESS) {
			printf("Failed to create image view\n");
			abort();
		}
	}
}

static void createRenderPass(void)
{
	VkAttachmentDescription color_attachment = {
	    .format = app_state.swapchain_format,
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

	VkSubpassDependency subpass_dependency = {
	    .srcSubpass = VK_SUBPASS_EXTERNAL,
	    .dstSubpass = 0,
	    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .srcAccessMask = 0,
	    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo render_pass_info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .attachmentCount = 1,
	    .pAttachments = &color_attachment,
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	    .dependencyCount = 1,
	    .pDependencies = &subpass_dependency
	};

	if (vkCreateRenderPass(
		app_state.vulkan_device,
		&render_pass_info,
		NULL,
		&app_state.render_pass
	    )
	    != VK_SUCCESS) {
		printf("Cannot create render pass");
		abort();
	}
}

static void createGraphicPipeline(void)
{
	int64_t frag_shader_size, vert_shader_size;

	char *frag_shader = readFile("shaders/frag.spv", &frag_shader_size);
	char *vert_shader = readFile("shaders/vert.spv", &vert_shader_size);

	VkShaderModule frag_shader_module = createShaderModule(
	    frag_shader, frag_shader_size, app_state.vulkan_device
	);
	VkShaderModule vert_shader_module = createShaderModule(
	    vert_shader, vert_shader_size, app_state.vulkan_device
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
	    .sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    .primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewport_state_info = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .viewportCount = 1,
	    .scissorCount = 1
	};

	VkPipelineRasterizationStateCreateInfo rasterization_info = {
	    .sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
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
	    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
			    | VK_COLOR_COMPONENT_G_BIT
			    | VK_COLOR_COMPONENT_B_BIT
			    | VK_COLOR_COMPONENT_A_BIT,
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
		app_state.vulkan_device,
		&pipeline_layout_info,
		NULL,
		&app_state.pipeline_layout
	    )
	    != VK_SUCCESS) {
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
	    .layout = app_state.pipeline_layout,
	    .renderPass = app_state.render_pass,
	    .subpass = 0
	};

	if (vkCreateGraphicsPipelines(
		app_state.vulkan_device,
		VK_NULL_HANDLE,
		1,
		&graphic_pipeline_info,
		NULL,
		&app_state.pipeline
	    )
	    != VK_SUCCESS) {
		printf("Failed to create graphic pipeline\n");
		abort();
	}

	vkDestroyShaderModule(
	    app_state.vulkan_device, frag_shader_module, NULL
	);
	vkDestroyShaderModule(
	    app_state.vulkan_device, vert_shader_module, NULL
	);

	free(frag_shader);
	free(vert_shader);
}

static void createFramebuffers(void)
{
	app_state.swapchain_frame_buffers =
	    malloc(sizeof(VkFramebuffer) * app_state.swapchain_image_size);
	assert(app_state.swapchain_frame_buffers != NULL);

	for (uint32_t index = 0; index < app_state.swapchain_image_size;
	     index++) {
		VkImageView attachment[] = {
		    app_state.swapchain_image_views[index]
		};

		VkFramebufferCreateInfo frame_create_info = {
		    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    .renderPass = app_state.render_pass,
		    .attachmentCount = 1,
		    .pAttachments = attachment,
		    .width = app_state.swapchain_extent.width,
		    .height = app_state.swapchain_extent.height,
		    .layers = 1
		};

		if (vkCreateFramebuffer(
			app_state.vulkan_device,
			&frame_create_info,
			NULL,
			app_state.swapchain_frame_buffers + index
		    )
		    != VK_SUCCESS) {
			printf("Failed to create framebuffer");
			abort();
		}
	}
}

static void createCommandPool(void)
{
	struct QueueFamilyIndices queue_family_indices =
	    findQueueFamilies(app_state.physical_device, app_state.surface);

	VkCommandPoolCreateInfo pool_info = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	    .queueFamilyIndex = queue_family_indices.graphic_family
	};

	vkCreateCommandPool(
	    app_state.vulkan_device, &pool_info, NULL, &app_state.command_pool
	);
}

static void createCommandBuffers(void)
{
	app_state.command_buffer =
	    malloc(sizeof(VkCommandBuffer) * MAX_FRAME_IN_FLIGHT);
	assert(app_state.command_buffer != NULL);

	VkCommandBufferAllocateInfo allocate_info = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .commandPool = app_state.command_pool,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandBufferCount = MAX_FRAME_IN_FLIGHT
	};

	if (vkAllocateCommandBuffers(
		app_state.vulkan_device,
		&allocate_info,
		app_state.command_buffer
	    )
	    != VK_SUCCESS) {
		printf("Cannot allocate command buffer");
		abort();
	}
}

static void createSyncObjects(void)
{
	app_state.image_ready_write =
	    malloc(sizeof(VkSemaphore) * MAX_FRAME_IN_FLIGHT);
	app_state.image_ready_read =
	    malloc(sizeof(VkSemaphore) * app_state.swapchain_image_size);
	app_state.image_inflight =
	    malloc(sizeof(VkFence) * MAX_FRAME_IN_FLIGHT);

	assert(app_state.image_ready_write != NULL);
	assert(app_state.image_ready_read != NULL);
	assert(app_state.image_inflight != NULL);

	VkSemaphoreCreateInfo semaphore_info = {
	    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_info = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (uint32_t index = 0; index < MAX_FRAME_IN_FLIGHT; index++) {
		if (vkCreateSemaphore(
			app_state.vulkan_device,
			&semaphore_info,
			NULL,
			app_state.image_ready_write + index
		    ) != VK_SUCCESS
		    || vkCreateFence(
			   app_state.vulkan_device,
			   &fence_info,
			   NULL,
			   app_state.image_inflight + index
		       ) != VK_SUCCESS) {
			printf("Failed to create sync objects\n");
			abort();
		}
	}

	for (uint32_t index = 0; index < app_state.swapchain_image_size;
	     index++) {
		if (vkCreateSemaphore(
			app_state.vulkan_device,
			&semaphore_info,
			NULL,
			app_state.image_ready_read + index
		    )
		    != VK_SUCCESS) {
			printf("Failed to create sync objects\n");
			abort();
		}
	}
}

static void initVulkan(void)
{
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

	uint32_t glfw_extension_count = 0;
	const char **glfw_extensions =
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

	if (vkCreateInstance(&create_info, NULL, &app_state.vulkan_instance)
	    != VK_SUCCESS) {
		abort();
	}

	if (glfwCreateWindowSurface(
		app_state.vulkan_instance,
		app_state.window_handle,
		NULL,
		&app_state.surface
	    )
	    != VK_SUCCESS) {
		printf("Failed to create window surface\n");
		abort();
	}

	selectPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageView();
	createRenderPass();
	createGraphicPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}

static void drawFrame(uint32_t *current_frame)
{
	vkWaitForFences(
	    app_state.vulkan_device,
	    1,
	    app_state.image_inflight + *current_frame,
	    VK_TRUE,
	    UINT64_MAX
	);
	vkResetFences(
	    app_state.vulkan_device,
	    1,
	    app_state.image_inflight + *current_frame
	);

	uint32_t image_index;
	vkAcquireNextImageKHR(
	    app_state.vulkan_device,
	    app_state.swapchain,
	    UINT64_MAX,
	    app_state.image_ready_write[*current_frame],
	    VK_NULL_HANDLE,
	    &image_index
	);
	vkResetCommandBuffer(app_state.command_buffer[*current_frame], 0);
	recordCommandBuffer(image_index, *current_frame, &app_state);

	VkSemaphore wait_semaphores[] = {
	    app_state.image_ready_write[*current_frame]
	};
	VkPipelineStageFlags wait_stages[] = {
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	VkSemaphore signal_semaphores[] = {
	    app_state.image_ready_read[image_index]
	};

	VkSubmitInfo submit_info = {
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores = wait_semaphores,
	    .pWaitDstStageMask = wait_stages,
	    .commandBufferCount = 1,
	    .pCommandBuffers = app_state.command_buffer + *current_frame,
	    .signalSemaphoreCount = 1,
	    .pSignalSemaphores = signal_semaphores
	};

	if (vkQueueSubmit(
		app_state.graphic_queue,
		1,
		&submit_info,
		app_state.image_inflight[*current_frame]
	    )
	    != VK_SUCCESS) {
		printf("Failed to submit draw command buffer\n");
		abort();
	}

	VkPresentInfoKHR present_info = {
	    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    .waitSemaphoreCount = 1,
	    .pWaitSemaphores = signal_semaphores,
	    .swapchainCount = 1,
	    .pSwapchains = (VkSwapchainKHR[]){app_state.swapchain},
	    .pImageIndices = &image_index
	};

	vkQueuePresentKHR(app_state.present_queue, &present_info);
	*current_frame = (*current_frame + 1) % MAX_FRAME_IN_FLIGHT;
}

static void mainLoop(void)
{
	uint32_t current_frame = 0;

	while (!glfwWindowShouldClose(app_state.window_handle)) {
		glfwPollEvents();
		drawFrame(&current_frame);
	}

	vkDeviceWaitIdle(app_state.vulkan_device);
}

static void cleanup(void)
{
	for (uint32_t index = 0; index < MAX_FRAME_IN_FLIGHT; index++) {
		vkDestroySemaphore(
		    app_state.vulkan_device,
		    app_state.image_ready_write[index],
		    NULL
		);
		vkDestroyFence(
		    app_state.vulkan_device,
		    app_state.image_inflight[index],
		    NULL
		);
	}

	for (uint32_t index = 0; index < app_state.swapchain_image_size;
	     index++) {
		vkDestroySemaphore(
		    app_state.vulkan_device,
		    app_state.image_ready_read[index],
		    NULL
		);
	}

	vkDestroyCommandPool(
	    app_state.vulkan_device, app_state.command_pool, NULL
	);

	for (uint32_t index = 0; index < app_state.swapchain_image_size;
	     index++) {
		vkDestroyFramebuffer(
		    app_state.vulkan_device,
		    app_state.swapchain_frame_buffers[index],
		    NULL
		);
	}

	vkDestroyPipeline(app_state.vulkan_device, app_state.pipeline, NULL);
	vkDestroyPipelineLayout(
	    app_state.vulkan_device, app_state.pipeline_layout, NULL
	);
	vkDestroyRenderPass(
	    app_state.vulkan_device, app_state.render_pass, NULL
	);

	for (uint32_t index = 0; index < app_state.swapchain_image_size;
	     index++) {
		vkDestroyImageView(
		    app_state.vulkan_device,
		    app_state.swapchain_image_views[index],
		    NULL
		);
	}

	vkDestroySwapchainKHR(
	    app_state.vulkan_device, app_state.swapchain, NULL
	);
	vkDestroyDevice(app_state.vulkan_device, NULL);
	vkDestroySurfaceKHR(
	    app_state.vulkan_instance, app_state.surface, NULL
	);
	vkDestroyInstance(app_state.vulkan_instance, NULL);

	glfwDestroyWindow(app_state.window_handle);
	glfwTerminate();
}

int main(void)
{
	initWindow();
	initVulkan();

	printf("Initialization complete.\n");

	mainLoop();
	cleanup();

	return 0;
}
