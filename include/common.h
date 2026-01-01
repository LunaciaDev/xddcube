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
    float priority,
    uint32_t family_indices[],
    uint32_t indices_size
);
void destroyQueueCreateInfo(VkDeviceQueueCreateInfo *queue_create_info);

struct SwapchainSupportDetail *querySwapchainSupport(
    VkPhysicalDevice device,
    VkSurfaceKHR surface
);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const VkSurfaceFormatKHR *formats,
    uint32_t format_size
);
VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR *capabilities,
    GLFWwindow *window_handle
);
VkPresentModeKHR chooseSwapPresentMode(
    const VkPresentModeKHR *present_mode,
    uint32_t present_mode_size
);

void destroySwapchainSupportDetail(
    struct SwapchainSupportDetail *support_detail
);

VkShaderModule createShaderModule(
    char *code,
    int64_t size,
    VkDevice device
);

void recordCommandBuffer(
    uint32_t image_index,
    uint32_t current_frame,
    struct AppState *app_state
);

void createBuffer(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer *buffer,
    VkDeviceMemory *buffer_memory
);

void copyBuffer(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size
);

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
    VkMemoryPropertyFlags mem_properties,
    VkSampleCountFlagBits msaa_sample_count
);

void transitionImageLayout(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout
);

void copyBufferToImage(
    VkDevice device,
    VkCommandPool command_pool,
    VkQueue queue,
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height
);

VkSampleCountFlagBits getMaxUsableSampleCount(
    VkPhysicalDevice physical_device
);

// ================
// Vertex Helpers

VkVertexInputBindingDescription getVertexBindingDescription(void);
VkVertexInputAttributeDescription *getAttributeDescription(void);

// ================

char *readFile(
    char *file,
    int64_t *read_size
);

#endif
