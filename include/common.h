#ifndef XDD__COMMON_H__
#define XDD__COMMON_H__

#include "cglm/types.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

extern const char *REQUIRED_DEVICE_EXTENSION[];
extern const uint32_t REQUIRED_DEVICE_EXTENSION_SIZE;
extern const uint32_t INDICES_LEN;

struct Vertex {
	vec3 pos;
};

struct UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
};

struct QueueFamilyIndices {
	uint8_t has_value_bitmap;
	uint32_t graphic_family;
	uint32_t present_family;
};

struct SwapchainSupportDetail {
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR *formats;
	uint32_t format_size;
	VkPresentModeKHR *present_mode;
	uint32_t present_mode_size;
};

struct AppState {
	GLFWwindow *window_handle;
	VkInstance vulkan_instance;

	VkDevice vulkan_device;
	VkPhysicalDevice physical_device;

	VkSurfaceKHR surface;

	VkQueue graphic_queue;
	VkQueue present_queue;

	VkSwapchainKHR swapchain;

	VkCommandPool command_pool;
	VkCommandBuffer *command_buffer;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSet *descriptor_set;

	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_mem;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_mem;
	VkBuffer *uniform_buffers;
	VkDeviceMemory *uniform_buffers_mem;
	void **mapped_uniform_buffers;

	VkSampleCountFlagBits msaa_samples;
	VkImage color_image;
	VkDeviceMemory color_image_mem;
	VkImageView color_image_view;

	VkImageView texture_view;
	VkImage texture;
	VkDeviceMemory texture_buffer;
	VkSampler texture_sampler;

	VkRenderPass render_pass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	VkSemaphore *image_ready_write;
	VkSemaphore *image_ready_read;
	VkFence *image_inflight;
	bool framebuffer_resized;

	VkImage *swapchain_images;
	VkImageView *swapchain_image_views;
	VkFramebuffer *swapchain_frame_buffers;
	uint32_t swapchain_image_size;
	VkFormat swapchain_format;
	VkExtent2D swapchain_extent;
};

bool hasReqValidationLayerSupport(
    const uint32_t validation_layers_size,
    const char **validation_layers
);

bool isDeviceSuitable(
    const VkPhysicalDevice device,
    const VkSurfaceKHR surface
);

struct QueueFamilyIndices findQueueFamilies(
    const VkPhysicalDevice device,
    const VkSurfaceKHR surface
);

VkDeviceQueueCreateInfo *makeQueueCreateInfo(
    uint32_t *create_info_size,
    const float priority,
    const uint32_t family_indices[],
    const uint32_t indices_size
);
void destroyQueueCreateInfo(VkDeviceQueueCreateInfo *queue_create_info);

struct SwapchainSupportDetail *querySwapchainSupport(
    const VkPhysicalDevice device,
    const VkSurfaceKHR surface
);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const VkSurfaceFormatKHR *formats,
    const uint32_t format_size
);
VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR *capabilities,
    GLFWwindow *window_handle
);
VkPresentModeKHR chooseSwapPresentMode(
    const VkPresentModeKHR *present_mode,
    const uint32_t present_mode_size
);

void destroySwapchainSupportDetail(
    struct SwapchainSupportDetail *support_detail
);

VkShaderModule createShaderModule(
    const char *code,
    const int64_t size,
    const VkDevice device
);

void recordCommandBuffer(
    const uint32_t image_index,
    const uint32_t current_frame,
    const struct AppState *app_state
);

void createBuffer(
    const VkDevice device,
    const VkPhysicalDevice physical_device,
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    VkBuffer *buffer,
    VkDeviceMemory *buffer_memory
);

void copyBuffer(
    const VkDevice device,
    const VkCommandPool command_pool,
    const VkQueue queue,
    const VkBuffer src,
    const VkBuffer dst,
    const VkDeviceSize size
);

void createImage(
    const VkDevice device,
    const VkPhysicalDevice physical_device,
    VkImage *texture,
    VkDeviceMemory *texture_buffer,
    const uint32_t width,
    const uint32_t height,
    const VkFormat image_format,
    const VkImageTiling tiling_mode,
    const VkImageUsageFlags usage_flags,
    const VkMemoryPropertyFlags mem_properties,
    const VkSampleCountFlagBits msaa_sample_count
);

void transitionImageLayout(
    const VkDevice device,
    const VkCommandPool command_pool,
    const VkQueue queue,
    const VkImage image,
    const VkImageLayout old_layout,
    const VkImageLayout new_layout
);

void copyBufferToImage(
    const VkDevice device,
    const VkCommandPool command_pool,
    const VkQueue queue,
    const VkBuffer buffer,
    const VkImage image,
    const uint32_t width,
    const uint32_t height
);

VkSampleCountFlagBits getMaxUsableSampleCount(
    const VkPhysicalDevice physical_device
);

// ================
// Vertex Helpers

VkVertexInputBindingDescription getVertexBindingDescription(void);
VkVertexInputAttributeDescription *getAttributeDescription(void);

// ================

char *readFile(
    const char *file,
    int64_t *read_size
);

#endif
