CC := gcc
FLAGS :=
BUILD_DIR := out

SOURCES := $(shell find . -name "*.c" -not -path "./srclib/picohttpparser/bench.c" -not -path "./srclib/picohttpparser/test.c" -not -path "./cmake-build-debug/*")
TARGETS := $(basename $(SOURCES))
.SECONDEXPANSION: $(TARGETS)

$(info $(TARGETS))

all: server

server: $(TARGETS)
	$(CC) $(FLAGS) -o $(BUILD_DIR)/$@ $(BUILD_DIR)/*.o -lpthread -lm -lconfuse

$(TARGETS): $$@.c
	$(CC) $(FLAGS) -c $@.c -o $(BUILD_DIR)/$(notdir $@).o