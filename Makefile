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

all: prep debug

prep:
	mkdir -p $(DEBUG_DIR) $(RELEASE_DIR)

debug: CFLAGS += -DDEBUG -g
debug: $(DEBUG_OBJS)
	$(CC) $(CFLAGS) $^ -o xddcube $(LDFLAGS)

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

release: CFLAGS += -O2
release: $(RELEASE_OBJS)
	$(CC) $(CFLAGS) $^ -o xddcube $(LDFLAGS)

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(RELEASE_DIR)
	rm -r $(DEBUG_DIR)
	rm xddcube

-include $(DEPS)