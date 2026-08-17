# make            -> debug build (default)
# make release    -> release build (PGO + LTO + O3)
# make clean      -> nuke build artifacts
#
# needs a posix-ish shell (msys2 ucrt64, git bash, linux, macos)

# build debug as default
BUILD ?= debug

ifeq ($(filter $(BUILD),debug release),)
    $(error Invalid BUILD '$(BUILD)' - must be 'debug' or 'release')
endif

CC      ?= gcc
MKDIR   := mkdir -p
RM      := rm -rf

REQUIRED_TOOLS := $(CC)

# add .exe extension on windows
EXE_EXT := $(if $(filter Windows_NT,$(OS)),.exe,)
TARGET  := mochee$(EXE_EXT)

BUILD_DIR   := build
PROFILE_DIR := $(BUILD_DIR)/profile_data

INCLUDES := -Iinc -Ideps/tinycthread
WFLAGS   := -Wall -Wextra -Wunused
LIBS     := -lm

SOURCES := $(wildcard src/*.c) $(wildcard deps/tinycthread/*.c)

CFLAGS := -fdiagnostics-color=always

ifeq ($(BUILD),release)
    CFLAGS += -march=native -flto -O3 -DNDEBUG
else
    CFLAGS += -g -O0
endif

.PHONY: all debug release clean build_dir check_tools

all: check_tools build_dir $(TARGET)

debug:
	@$(MAKE) BUILD=debug all

release:
	@$(MAKE) BUILD=release all

# check prerequisites first
check_tools:
	@for tool in $(REQUIRED_TOOLS); do \
		command -v $$tool >/dev/null 2>&1 || { \
			echo "Error: required tool '$$tool' not found on PATH."; \
			exit 1; \
		}; \
	done

build_dir:
	@$(MKDIR) "$(PROFILE_DIR)"

# release = two-pass PGO build: compile with instrumentation, run a bench to gather profile data, then recompile using that data
ifeq ($(BUILD),release)
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -fprofile-generate -fprofile-dir=$(PROFILE_DIR) \
	    $(WFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LIBS)
	./$(TARGET) bench
	$(CC) $(CFLAGS) -fprofile-use -fprofile-correction -fprofile-dir=$(PROFILE_DIR) \
	    $(WFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LIBS)
else
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(WFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LIBS)
endif

clean:
	@$(RM) $(TARGET) $(BUILD_DIR)