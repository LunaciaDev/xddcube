#ifndef XDD__COMMON_H__
#define XDD__COMMON_H__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

extern const char*    REQUIRED_DEVICE_EXTENSION[];
extern const uint32_t REQUIRED_DEVICE_EXTENSION_SIZE;

typedef struct QueueFamilyIndices {
    uint8_t  has_value_bitmap;
    uint32_t graphic_family;
    uint32_t present_family;
} QueueFamilyIndices;

typedef struct SwapchainSupportDetail {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR*      formats;
    uint32_t                 format_size;
    VkPresentModeKHR*        present_mode;
    uint32_t                 present_mode_size;
} SwapchainSupportDetail;

typedef struct ApplicationData {
    GLFWwindow*      window_handle;
    VkInstance       vulkan_instance;

    VkDevice         vulkan_device;
    VkPhysicalDevice physical_device;

    VkSurfaceKHR     surface;

    VkQueue          graphic_queue;
    VkQueue          present_queue;

    VkSwapchainKHR   swapchain;

    VkCommandPool    command_pool;
    VkCommandBuffer  command_buffer;

    VkRenderPass     render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline       pipeline;

    VkImage*         swapchain_images;
    VkImageView*     swapchain_image_views;
    VkFramebuffer*   swapchain_frame_buffers;
    uint32_t         swapchain_image_size;
    VkFormat         swapchain_format;
    VkExtent2D       swapchain_extent;
} ApplicationData;

bool hasReqValidationLayerSupport(
    const uint32_t validation_layers_size,
    const char**   validation_layers
);

bool isDeviceSuitable(
    const VkPhysicalDevice device,
    const VkSurfaceKHR     surface
);

QueueFamilyIndices
findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface);

VkDeviceQueueCreateInfo* makeQueueCreateInfo(
    uint32_t* create_info_size,
    float     priority,
    uint32_t  family_indices[],
    uint32_t  indices_size
);
void destroyQueueCreateInfo(VkDeviceQueueCreateInfo* queue_create_info);

SwapchainSupportDetail*
querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const VkSurfaceFormatKHR* formats,
    uint32_t                  format_size
);
VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR* capabilities,
    GLFWwindow*                     window_handle
);
VkPresentModeKHR chooseSwapPresentMode(
    const VkPresentModeKHR* present_mode,
    uint32_t                present_mode_size
);

void destroySwapchainSupportDetail(SwapchainSupportDetail* support_detail);

VkShaderModule createShaderModule(char* code, int64_t size, VkDevice device);

void recordCommandBuffer(uint32_t image_index, ApplicationData* app_data);

// ================

char* readFile(char* file, int64_t* read_size);

#endif