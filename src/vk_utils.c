#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"

bool hasReqValidationLayerSupport(
    const uint32_t validation_layers_size,
    const char**   validation_layers
) {
    uint32_t avail_layer_size;
    vkEnumerateInstanceLayerProperties(&avail_layer_size, NULL);

    VkLayerProperties available_layers[avail_layer_size];
    vkEnumerateInstanceLayerProperties(&avail_layer_size, available_layers);

    for (uint32_t req_layer_index = 0; req_layer_index < validation_layers_size;
         req_layer_index++) {
        bool layer_found = false;

        for (uint32_t avail_layer_index = 0;
             avail_layer_index < avail_layer_size; avail_layer_index++) {
            if (strcmp(
                    validation_layers[req_layer_index],
                    available_layers[avail_layer_index].layerName
                ) == 0) {
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