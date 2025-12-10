CC := clang
CFLAGS := -std=c99 -MMD -MP -Wall -Werror
LDFLAGS := -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
SRC_DIR := src
DEBUG_DIR := debug
RELEASE_DIR := release

SRCS := $(shell find $(SRC_DIR) -name '*.c')
DEBUG_OBJS := $(SRCS:$(SRC_DIR)/%.c=$(DEBUG_DIR)/%.o)
DEBUG_DEPS := $(DEBUG_OBJS:.o=.d)
RELEASE_OBJS := $(SRCS:$(SRC_DIR)/%.c=$(RELEASE_DIR)/%.o)
RELEASE_DEPS := $(RELEASE_OBJS:.o=.d)

# shaders
SHADER_SRCS := $(shell find shaders -name 'shader.*')
DEBUG_SHADER_OBJS := $(SHADER_SRCS:shaders/shader.%=$(DEBUG_DIR)/shaders/%.spv)
RELEASE_SHADER_OBJS := $(SHADER_SRCS:shaders/shader.%=$(RELEASE_DIR)/shaders/%.spv)

all: prep debug

prep:
	mkdir -p $(DEBUG_DIR) $(RELEASE_DIR) $(DEBUG_DIR)/shaders $(RELEASE_DIR)/shaders

debug: CFLAGS += -DDEBUG -g
debug: $(DEBUG_OBJS) $(DEBUG_SHADER_OBJS)
	$(CC) $(CFLAGS) $(DEBUG_OBJS) -o $(DEBUG_DIR)/xddcube $(LDFLAGS)

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_DIR)/shaders/%.spv: shaders/shader.%
	glslang -V $< -o $@

release: CFLAGS += -O2
release: $(RELEASE_OBJS) $(RELEASE_SHADER_OBJS)
	$(CC) $(CFLAGS) $(RELEASE_OBJS) -o $(RELEASE_DIR)/xddcube $(LDFLAGS)

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RELEASE_DIR)/shaders/%.spv: shaders/shader.%
	glslang -V $< -o $@

.PHONY: clean
clean:
	rm -r $(RELEASE_DIR)
	rm -r $(DEBUG_DIR)

-include $(DEPS)