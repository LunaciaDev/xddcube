#ifndef XDD__COMMON_H__
#define XDD__COMMON_H__

#include <stdbool.h>
#include <stdint.h>

bool hasReqValidationLayerSupport(
    const uint32_t validation_layers_size,
    const char**   validation_layers
);

#endif