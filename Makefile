CC := clang
CFLAGS := -std=c99 -MMD -MP -Wall -Werror
LDFLAGS := -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
SRC_DIR := src
DEBUG_DIR := debug
RELEASE_DIR := release
EXECUTABLE := xddcube

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

run_debug: prep debug
	cd $(DEBUG_DIR) && \
	./$(EXECUTABLE)

run_release: prep release
	cd $(RELEASE_DIR) && \
	./$(EXECUTABLE)

prep:
	mkdir -p $(DEBUG_DIR) $(RELEASE_DIR) $(DEBUG_DIR)/shaders $(RELEASE_DIR)/shaders

debug: CFLAGS += -DDEBUG -g
debug: DEPS = $(DEBUG_DEPS)
debug: prep $(DEBUG_SHADER_OBJS) $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $(DEBUG_OBJS) -o $(DEBUG_DIR)/$(EXECUTABLE) $(LDFLAGS)

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_DIR)/shaders/%.spv: shaders/shader.%
	glslang -V $< -o $@

release: CFLAGS += -O2
release: DEPS = $(RELEASE_DEPS)
release: prep $(RELEASE_SHADER_OBJS) $(RELEASE_OBJS)
	$(CC) $(CFLAGS) $(RELEASE_OBJS) -o $(RELEASE_DIR)/$(EXECUTABLE) $(LDFLAGS)

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RELEASE_DIR)/shaders/%.spv: shaders/shader.%
	glslang -V $< -o $@

.PHONY: clean
clean:
	rm -r $(RELEASE_DIR)
	rm -r $(DEBUG_DIR)

-include $(DEPS)