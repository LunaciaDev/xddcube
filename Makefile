CC := clang
CFLAGS := -std=c99 -Wall -Wpedantic -Wextra -Werror -MMD
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

.PHONY: clean debug release

all: debug

debug: prep build_debug
release: prep build_release

prep:
	mkdir -p $(DEBUG_DIR)/shaders $(RELEASE_DIR)/shaders

build_debug: CFLAGS += -DDEBUG -g
build_debug: $(DEBUG_SHADER_OBJS) $(DEBUG_EXECUTABLE)

build_release: CFLAGS += -O2
build_release: $(RELEASE_SHADER_OBJS) $(RELEASE_EXECUTABLE)

run_debug: debug
	cd $(DEBUG_DIR) && \
	./$(EXECUTABLE)

run_release: release
	cd $(RELEASE_DIR) && \
	./$(EXECUTABLE)

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
	rm -r $(RELEASE_DIR)
	rm -r $(DEBUG_DIR)

-include $(DEPS)