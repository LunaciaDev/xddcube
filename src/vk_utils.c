#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include "common.h"

static uint32_t clamp(
    uint32_t value,
    uint32_t min,
    uint32_t max
)
{
	value = value < min ? min : value;
	value = value > max ? max : value;

	return value;
}

static const uint8_t CANNOT_FIND_ALL_FAMILIES = 0x37;

// =================================

bool hasReqValidationLayerSupport(
    const uint32_t validation_layers_size,
    const char **validation_layers
)
{
	uint32_t avail_layer_size;
	vkEnumerateInstanceLayerProperties(&avail_layer_size, NULL);

	VkLayerProperties available_layers[avail_layer_size];
	vkEnumerateInstanceLayerProperties(
	    &avail_layer_size, available_layers
	);

	for (uint32_t req_layer_index = 0;
	     req_layer_index < validation_layers_size;
	     req_layer_index++) {
		bool layer_found = false;

		for (uint32_t avail_layer_index = 0;
		     avail_layer_index < avail_layer_size;
		     avail_layer_index++) {
			if (strcmp(
				validation_layers[req_layer_index],
				available_layers[avail_layer_index].layerName
			    )
			    == 0) {
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

static bool checkDeviceExtensionSupport(const VkPhysicalDevice device)
{
	uint32_t avail_ext_count;
	vkEnumerateDeviceExtensionProperties(
	    device, NULL, &avail_ext_count, NULL
	);

	VkExtensionProperties avail_exts[avail_ext_count];
	vkEnumerateDeviceExtensionProperties(
	    device, NULL, &avail_ext_count, avail_exts
	);

	for (uint32_t req_ext_index = 0;
	     req_ext_index < REQUIRED_DEVICE_EXTENSION_SIZE;
	     req_ext_index++) {
		bool extension_found = false;

		for (uint32_t avail_ext_index = 0;
		     avail_ext_index < avail_ext_count;
		     avail_ext_index++) {
			if (strcmp(
				REQUIRED_DEVICE_EXTENSION[req_ext_index],
				avail_exts[avail_ext_index].extensionName
			    )
			    == 0) {
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
    const VkSurfaceKHR surface
)
{
	struct QueueFamilyIndices indices = findQueueFamilies(device, surface);
	bool has_extension_support = checkDeviceExtensionSupport(device);

	if (!has_extension_support) return false;

	struct SwapchainSupportDetail *details =
	    querySwapchainSupport(device, surface);
	bool adequate_swapchain_support =
	    details->formats != NULL && details->present_mode != NULL;
	destroySwapchainSupportDetail(details);

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	return indices.has_value_bitmap != CANNOT_FIND_ALL_FAMILIES
	    && adequate_swapchain_support
	    && features.samplerAnisotropy;
}

struct QueueFamilyIndices findQueueFamilies(
    const VkPhysicalDevice device,
    const VkSurfaceKHR surface
)
{
	struct QueueFamilyIndices indices = {
	    .has_value_bitmap = 0, .graphic_family = 0
	};

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
	    device, &queue_family_count, NULL
	);

	VkQueueFamilyProperties queue_families[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(
	    device, &queue_family_count, queue_families
	);

	for (uint32_t queue_family_index = 0;
	     // graphic-family queue support
	     queue_family_index < queue_family_count;
	     queue_family_index++) {
		if (queue_families[queue_family_index].queueFlags
		    & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphic_family = queue_family_index;
			indices.has_value_bitmap += 1;
		}

		// presentation-family queue support
		{
			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(
			    device,
			    queue_family_index,
			    surface,
			    &present_support
			);

			if (present_support) {
				indices.present_family = queue_family_index;
				indices.has_value_bitmap += 2;
			}
		}

		if (indices.has_value_bitmap == 3) {
			return indices;
		}
	}

	// not all families are supported by the given device
	indices.has_value_bitmap = CANNOT_FIND_ALL_FAMILIES;
	return indices;
}

VkDeviceQueueCreateInfo *makeQueueCreateInfo(
    uint32_t *create_info_size,
    float priority,
    uint32_t family_indices[],
    uint32_t indices_size
)
{
	// Deduplicate family_indices
	// This is ran once and on small array so a linear search is OK
	uint32_t deduplicated_family_indices[indices_size];
	uint32_t dd_size = 0;

	for (uint32_t fi_index = 0; fi_index < indices_size; fi_index++) {
		bool found_duplicate = false;

		for (uint32_t dd_index = 0; dd_index < dd_size; dd_index++) {
			if (deduplicated_family_indices[dd_index]
			    == family_indices[fi_index]) {
				found_duplicate = true;
				break;
			}
		}

		if (!found_duplicate) {
			deduplicated_family_indices[dd_size] =
			    family_indices[fi_index];
			dd_size++;
		}
	}

	// Construct queue create info
	*create_info_size = dd_size;
	VkDeviceQueueCreateInfo *queue_create_infos =
	    malloc(sizeof(VkDeviceQueueCreateInfo) * dd_size);
	assert(queue_create_infos != NULL);

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

void destroyQueueCreateInfo(VkDeviceQueueCreateInfo *queue_create_info)
{
	free(queue_create_info);
}

struct SwapchainSupportDetail *querySwapchainSupport(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
)
{
	struct SwapchainSupportDetail *details =
	    malloc(sizeof(struct SwapchainSupportDetail));
	assert(details != NULL);

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
	    device, surface, &details->capabilities
	);

	vkGetPhysicalDeviceSurfaceFormatsKHR(
	    device, surface, &details->format_size, NULL
	);

	if (details->format_size != 0) {
		details->formats =
		    malloc(sizeof(VkSurfaceFormatKHR) * details->format_size);
		assert(details->formats != NULL);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
		    device, surface, &details->format_size, details->formats
		);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(
	    device, surface, &details->present_mode_size, NULL
	);

	if (details->present_mode_size != 0) {
		details->present_mode = malloc(
		    sizeof(VkPresentModeKHR) * details->present_mode_size
		);
		assert(details->present_mode != NULL);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
		    device,
		    surface,
		    &details->present_mode_size,
		    details->present_mode
		);
	}

	return details;
}

void destroySwapchainSupportDetail(
    struct SwapchainSupportDetail *support_detail
)
{
	free(support_detail->formats);
	free(support_detail->present_mode);
	free(support_detail);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const VkSurfaceFormatKHR *formats,
    uint32_t format_size
)
{
	for (uint32_t index = 0; index < format_size; index++) {
		if (formats[index].format == VK_FORMAT_B8G8R8_SRGB
		    && formats[index].colorSpace
			   == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return formats[index];
		}
	}

	return formats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const VkPresentModeKHR *present_mode,
    uint32_t present_mode_size
)
{
	for (uint32_t index = 0; index < present_mode_size; index++) {
		if (present_mode[index] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR *capabilities,
    GLFWwindow *window_handle
)
{
	if (capabilities->currentExtent.width != UINT32_MAX) {
		return capabilities->currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(window_handle, &width, &height);

	width = clamp(
	    width,
	    capabilities->minImageExtent.width,
	    capabilities->maxImageExtent.width
	);
	height = clamp(
	    height,
	    capabilities->minImageExtent.height,
	    capabilities->maxImageExtent.height
	);

	return (VkExtent2D){width, height};
}

VkShaderModule createShaderModule(
    char *code,
    int64_t size,
    VkDevice device
)
{
	VkShaderModuleCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .codeSize = size,
	    .pCode = (uint32_t *)code
	};

	VkShaderModule shader_module;

	if (vkCreateShaderModule(device, &create_info, NULL, &shader_module)
	    != VK_SUCCESS) {
		printf("Cannot create shader module\n");
		abort();
	}

	return shader_module;
}

void recordCommandBuffer(
    uint32_t image_index,
    uint32_t current_frame,
    struct AppState *app_state
)
{
	VkCommandBufferBeginInfo begin_info = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};

	if (vkBeginCommandBuffer(
		app_state->command_buffer[current_frame], &begin_info
	    )
	    != VK_SUCCESS) {
		printf("Failed to begin recording command buffer\n");
		abort();
	}

	VkClearValue clear_color = {
	    .color = {.float32 = {0.5f, 0.5f, 0.5f, 1.0f}}
	};
	VkRenderPassBeginInfo render_pass_info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .renderPass = app_state->render_pass,
	    .framebuffer = app_state->swapchain_frame_buffers[image_index],
	    .renderArea =
		{.offset = {0, 0}, .extent = app_state->swapchain_extent},
	    .clearValueCount = 1,
	    .pClearValues = &clear_color
	};

	VkViewport viewport = {
	    .x = 0.0,
	    .y = 0.0,
	    .width = app_state->swapchain_extent.width,
	    .height = app_state->swapchain_extent.height,
	    .minDepth = 0.0,
	    .maxDepth = 1.0
	};
	VkRect2D scissor = {
	    .offset = {0, 0},
              .extent = app_state->swapchain_extent
	};

	vkCmdBeginRenderPass(
	    app_state->command_buffer[current_frame],
	    &render_pass_info,
	    VK_SUBPASS_CONTENTS_INLINE
	);
	vkCmdBindPipeline(
	    app_state->command_buffer[current_frame],
	    VK_PIPELINE_BIND_POINT_GRAPHICS,
	    app_state->pipeline
	);

	VkBuffer vertex_buffer[] = {app_state->vertex_buffer};
	VkDeviceSize offset[] = {0};
	vkCmdBindVertexBuffers(
	    app_state->command_buffer[current_frame],
	    0,
	    1,
	    vertex_buffer,
	    offset
	);
	vkCmdBindIndexBuffer(
	    app_state->command_buffer[current_frame],
	    app_state->index_buffer,
	    0,
	    VK_INDEX_TYPE_UINT16
	);

	vkCmdSetViewport(
	    app_state->command_buffer[current_frame], 0, 1, &viewport
	);
	vkCmdSetScissor(
	    app_state->command_buffer[current_frame], 0, 1, &scissor
	);
	vkCmdBindDescriptorSets(
	    app_state->command_buffer[current_frame],
	    VK_PIPELINE_BIND_POINT_GRAPHICS,
	    app_state->pipeline_layout,
	    0,
	    1,
	    (app_state->descriptor_set) + current_frame,
	    0,
	    NULL
	);
	vkCmdDrawIndexed(
	    app_state->command_buffer[current_frame], INDICES_LEN, 1, 0, 0, 0
	);
	vkCmdEndRenderPass(app_state->command_buffer[current_frame]);

	if (vkEndCommandBuffer(app_state->command_buffer[current_frame])
	    != VK_SUCCESS) {
		printf("Failed to end command buffer\n");
		abort();
	}
}

VkVertexInputBindingDescription getVertexBindingDescription(void)
{
	return (VkVertexInputBindingDescription){
	    .binding = 0,
	    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	    .stride = sizeof(struct Vertex)
	};
}

/**
 * The pointer returned by this function must be freed by the caller.
 */
VkVertexInputAttributeDescription *getAttributeDescription(void)
{
	VkVertexInputAttributeDescription *attribute_description =
	    malloc(sizeof(VkVertexInputAttributeDescription) * 3);

	attribute_description[0] = (VkVertexInputAttributeDescription){
	    .binding = 0,
	    .location = 0,
	    .format = VK_FORMAT_R32G32B32_SFLOAT,
	    .offset = offsetof(struct Vertex, pos)
	};

	attribute_description[1] = (VkVertexInputAttributeDescription){
	    .binding = 0,
	    .location = 1,
	    .format = VK_FORMAT_R32G32B32_SFLOAT,
	    .offset = offsetof(struct Vertex, color)
	};

	attribute_description[2] = (VkVertexInputAttributeDescription){
	    .binding = 0,
	    .location = 2,
	    .format = VK_FORMAT_R32G32_SFLOAT,
	    .offset = offsetof(struct Vertex, texture_coordinate)
	};

	return attribute_description;
}

static uint32_t findMemoryType(
    VkPhysicalDevice device,
    uint32_t type_filter,
    VkMemoryPropertyFlags properties
)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i))
		    && (mem_properties.memoryTypes[i].propertyFlags
			& properties)) {
			return i;
		}
	}

	printf("Failed to find suitable memory types\n");
	abort();
}

void createBuffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer *buffer,
    VkDeviceMemory *buffer_memory
)
{
	VkBufferCreateInfo buffer_info = {
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .size = size,
	    .usage = usage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
		printf("Failed to create vertex buffer\n");
		abort();
	}

	VkMemoryRequirements mem_requirement;
	vkGetBufferMemoryRequirements(device, *buffer, &mem_requirement);

	VkMemoryAllocateInfo allocate_info = {
	    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize = mem_requirement.size,
	    .memoryTypeIndex = findMemoryType(
		physical_device, mem_requirement.memoryTypeBits, properties
	    )
	};

	if (vkAllocateMemory(device, &allocate_info, NULL, buffer_memory)
	    != VK_SUCCESS) {
		printf("Failed to allocate vertex buffer memory\n");
		abort();
	}

	vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
}

void createImage(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkImage *texture,
    VkDeviceMemory *texture_buffer,
    uint32_t width,
    uint32_t height,
    VkFormat image_format,
    VkImageTiling tiling_mode,
    VkImageUsageFlags usage_flags,
    VkMemoryPropertyFlags mem_properties
)
{
	VkImageCreateInfo create_info = {
	    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    .imageType = VK_IMAGE_TYPE_2D,
	    .extent = {.width = width, .height = height, .depth = 1},
	    .mipLevels = 1,
	    .arrayLayers = 1,
	    .format = image_format,
	    .tiling = tiling_mode,
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	    .usage = usage_flags,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .samples = VK_SAMPLE_COUNT_1_BIT
	};

	if (vkCreateImage(device, &create_info, NULL, texture) != VK_SUCCESS) {
		printf("Failed to create image\n");
		abort();
	}

	VkMemoryRequirements requirement;
	vkGetImageMemoryRequirements(device, *texture, &requirement);

	VkMemoryAllocateInfo alloc_info = {
	    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    .allocationSize = requirement.size,
	    .memoryTypeIndex = findMemoryType(
		physical_device, requirement.memoryTypeBits, mem_properties
	    )
	};

	if (vkAllocateMemory(device, &alloc_info, NULL, texture_buffer)
	    != VK_SUCCESS) {
		printf("Failed to allocate buffer for image\n");
		abort();
	}

	vkBindImageMemory(device, *texture, *texture_buffer, 0);
}

static VkCommandBuffer beginCmdBuf(
    VkDevice device,
    VkCommandPool command_pool
)
{
	VkCommandBufferAllocateInfo alloc_info = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    .commandPool = command_pool,
	    .commandBufferCount = 1
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

static void endCmdBuf(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkCommandBuffer command_buffer
)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
	    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    .commandBufferCount = 1,
	    .pCommandBuffers = &command_buffer
	};
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void copyBuffer(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size
)
{
	VkCommandBuffer command_buffer = beginCmdBuf(device, command_pool);

	VkBufferCopy copy_region = {.size = size};
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

	endCmdBuf(device, command_pool, queue, command_buffer);
}

void transitionImageLayout(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout
)
{
	VkCommandBuffer command_buffer = beginCmdBuf(device, command_pool);

	VkImageMemoryBarrier barrier = {
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	    .oldLayout = old_layout,
	    .newLayout = new_layout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = image,
	    .subresourceRange = {
				 .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				 .baseMipLevel = 0,
				 .levelCount = 1,
				 .baseArrayLayer = 0,
				 .layerCount = 1
	    },
	};
	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags dst_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED
	    && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		   && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		printf("Unsupported args\n");
		abort();
	}

	vkCmdPipelineBarrier(
	    command_buffer,
	    source_stage,
	    dst_stage,
	    0,
	    0,
	    NULL,
	    0,
	    NULL,
	    1,
	    &barrier
	);

	endCmdBuf(device, command_pool, queue, command_buffer);
}

void copyBufferToImage(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height
)
{
	VkCommandBuffer command_buffer = beginCmdBuf(device, command_pool);

	VkBufferImageCopy region = {
	    .bufferOffset = 0,
	    .bufferRowLength = 0,
	    .bufferImageHeight = 0,
	    .imageSubresource =
		{
				   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				   .mipLevel = 0,
				   .baseArrayLayer = 0,
				   .layerCount = 1,
				   },
	    .imageOffset = {				      0,      0,    0 },
	    .imageExtent = {				  width, height,    1 }
	};

	vkCmdCopyBufferToImage(
	    command_buffer,
	    buffer,
	    image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    1,
	    &region
	);

	endCmdBuf(device, command_pool, queue, command_buffer);
}
