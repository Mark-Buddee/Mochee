# --- COMPILER & TOOLS ---
CC      := gcc
MKDIR   := powershell -Command New-Item -ItemType Directory -Force
RM      := del /Q /S

# --- FREQUENT TOGGLES ---
# Uncomment these lines to enable them by default, 
# or pass them via CLI: mingw32-make DEBUG=1
# --------------------------------------------
# RELEASE     ?= 1
# PROFILE     ?= 1
# PROFILE_USE ?= 1
# SANITIZE    ?= 1
# FAST_MATH   ?= 1

CFLAGS := -march=native -flto -fdiagnostics-color=always

# --- Logic for Toggles ---
ifdef RELEASE
    CFLAGS += -O3 -DNDEBUG
else
    CFLAGS += -g -O0
endif

ifdef PROFILE
    CFLAGS += -fprofile-generate -fprofile-dir=build/profile_data
endif

ifdef PROFILE_USE
	CFLAGS += -fprofile-use -fprofile-dir=build/profile_data
endif

ifdef SANITIZE
    CFLAGS += -fsanitize=address -fsanitize=undefined
endif

ifdef FAST_MATH
    CFLAGS += -ffast-math
endif

# --- WARNINGS & STANDARDS ---
WFLAGS  := -Wall -Wextra -Wunused

# --- INCLUDES & SEARCH PATHS ---
INCLUDES := -Iinc -Ideps/tinycthread

# --- FILES ---
SOURCES := $(wildcard src/*.c) $(wildcard deps/tinycthread/*.c)
TARGET  := mochee.exe

# --- LINKING ---
LIBS    := -lm

# --- RULES ---

all: build_dir $(TARGET)

build_dir:
	@if not exist "build\profile_data" $(MKDIR) "build\profile_data" > nul

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(WFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LIBS)

clean:
	@if exist $(TARGET) $(RM) $(TARGET)
ifdef PROFILE
	@echo "Profile flag detected: Clearing build directory..."
	@if exist "build" rd /S /Q "build"
else
	@echo "Profile flag NOT set: Preserving build directory."
endif

.PHONY: all clean build_dir