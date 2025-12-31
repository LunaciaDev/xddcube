CC := clang
CFLAGS := -std=c99 -Wall -Wpedantic -Wextra -Werror -MMD -Iinclude
LDFLAGS := -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
SRC_DIR := src
DEBUG_DIR := debug
RELEASE_DIR := release
EXECUTABLE := xddcube

SRCS := $(shell find $(SRC_DIR) -name '*.c')
DEPS := $(SRCS:$(SRC_DIR)/%.c=$(DEBUG_DIR)/%.d) $(SRCS:$(SRC_DIR)/%.c=$(RELEASE_DIR)/%.d)
DEBUG_OBJS := $(SRCS:$(SRC_DIR)/%.c=$(DEBUG_DIR)/%.o)
RELEASE_OBJS := $(SRCS:$(SRC_DIR)/%.c=$(RELEASE_DIR)/%.o)
DEBUG_EXECUTABLE := $(DEBUG_DIR)/$(EXECUTABLE)
RELEASE_EXECUTABLE := $(RELEASE_DIR)/$(EXECUTABLE)

# shaders
SHADER_SRCS := $(shell find shaders -name 'shader.*')
DEBUG_SHADER_OBJS := $(SHADER_SRCS:shaders/shader.%=$(DEBUG_DIR)/shaders/%.spv)
RELEASE_SHADER_OBJS := $(SHADER_SRCS:shaders/shader.%=$(RELEASE_DIR)/shaders/%.spv)

.PHONY: clean build build_release run run_release

all: build

build: CFLAGS += -DDEBUG -g
build: $(DEBUG_DIR) $(DEBUG_SHADER_OBJS) $(DEBUG_EXECUTABLE)

build_release: CFLAGS += -O2
build_release: $(RELEASE_DIR) $(RELEASE_SHADER_OBJS) $(RELEASE_EXECUTABLE)

run: build
	cd $(DEBUG_DIR) && \
	./$(EXECUTABLE)

run_release: build_release
	cd $(RELEASE_DIR) && \
	./$(EXECUTABLE)

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)/shaders

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)/shaders

$(DEBUG_EXECUTABLE): $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_DIR)/shaders/%.spv: shaders/shader.%
	glslang -V $< -o $@

$(RELEASE_EXECUTABLE): $(RELEASE_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(RELEASE_DIR)/shaders/%.spv: shaders/shader.%
	glslang -V $< -o $@

clean:
	rm -r $(RELEASE_DIR) $(DEBUG_DIR)

-include $(DEPS)