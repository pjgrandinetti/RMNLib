# Makefile for RMNLib with best-practice build structure (output in build/lib)

.DEFAULT_GOAL := all
.SUFFIXES:

# Tools
CC      := clang
AR      := ar
LEX     := flex
YACC    := bison
YFLAGS  := -d

RM      := rm -f
MKDIR_P := mkdir -p
# Xcode build directory under RMNLib
XCODE_BUILD := $(CURDIR)/build-xcode

# Directories
SRC_DIR         := src
TEST_SRC_DIR    := tests
BUILD_DIR       := build
OBJ_DIR         := $(BUILD_DIR)/obj
GEN_DIR         := $(BUILD_DIR)/gen
BIN_DIR         := $(BUILD_DIR)/bin
LIB_DIR         := $(BUILD_DIR)/lib
THIRD_PARTY_DIR := third_party
TP_LIB_DIR      := $(THIRD_PARTY_DIR)/lib
INCLUDE_DIR     := $(THIRD_PARTY_DIR)/include
OCT_INCLUDE     := $(INCLUDE_DIR)/OCTypes
SIT_INCLUDE     := $(INCLUDE_DIR)/SITypes

# Include and library paths
OCT_LIBDIR  := $(TP_LIB_DIR)
SIT_LIBDIR  := $(TP_LIB_DIR)

# All required directories
REQUIRED_DIRS := $(BUILD_DIR) $(OBJ_DIR) $(GEN_DIR) $(BIN_DIR) $(LIB_DIR) $(THIRD_PARTY_DIR)

# Flags
CPPFLAGS := -I. -I$(SRC_DIR) -I$(TEST_SRC_DIR) -I$(OCT_INCLUDE) -I$(SIT_INCLUDE)
CFLAGS   := -O3 -Wall -Wextra \
             -Wno-sign-compare -Wno-unused-parameter \
             -Wno-missing-field-initializers -Wno-unused-function \
             -MMD -MP
CFLAGS_DEBUG := -O0 -g -Wall -Wextra -Werror -MMD -MP

# OS-specific library ZIP selection (must come before Archives definitions)
UNAME_S := $(shell uname -s)
ARCH    := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
  OCT_LIB_BIN := libOCTypes-libOCTypes-macos-latest.zip
  SIT_LIB_BIN := libSITypes-libSITypes-macos-latest.zip
else ifeq ($(UNAME_S),Linux)
  ifeq ($(ARCH),aarch64)
    OCT_LIB_BIN := libOCTypes-libOCTypes-linux-arm64.zip
    SIT_LIB_BIN := libSITypes-libSITypes-linux-arm64.zip
  else
    OCT_LIB_BIN := libOCTypes-libOCTypes-ubuntu-latest.zip
    SIT_LIB_BIN := libSITypes-libSITypes-ubuntu-latest.zip
  endif
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  OCT_LIB_BIN := libOCTypes-libOCTypes-windows-latest.zip
  SIT_LIB_BIN := libSITypes-libSITypes-windows-latest.zip
endif

# Archives
OCT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(OCT_LIB_BIN)
OCT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libOCTypes-headers.zip
SIT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(SIT_LIB_BIN)
SIT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libSITypes-headers.zip

.PHONY: all dirs clean prepare octypes sitypes test test-asan docs synclib fetchlibs

fetchlibs: octypes sitypes
	@echo "Both OCTypes and SITypes libraries are up to date."

# Only fetch third-party libs when third_party is empty
EMPTY_TP := $(shell [ -d $(THIRD_PARTY_DIR) ] && [ -z "$(wildcard $(THIRD_PARTY_DIR)/*)" ] && echo 1)
ifeq ($(EMPTY_TP),1)
TP_DEPS := octypes sitypes
else
TP_DEPS :=
endif

all: dirs $(TP_DEPS) prepare $(LIB_DIR)/libRMNLib.a

dirs: $(REQUIRED_DIRS)

$(REQUIRED_DIRS):
	$(MKDIR_P) $@

# Define object files
STATIC_SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(STATIC_SRC))

# Download and extract OCTypes
octypes: $(OCT_LIBDIR)/libOCTypes.a $(OCT_INCLUDE)/OCLibrary.h

$(OCT_LIB_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes library: $(OCT_LIB_BIN)"
	@curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.1/$(OCT_LIB_BIN) -o $@

$(OCT_HEADERS_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes headers"
	@curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.1/libOCTypes-headers.zip -o $@

# Platform detection for shell vs PowerShell
IS_MINGW := $(findstring MINGW,$(UNAME_S))

# Ensure third-party lib and include dirs exist
$(TP_LIB_DIR) $(OCT_INCLUDE) $(SIT_INCLUDE):
	$(MKDIR_P) $@

# ──────────────── OCTypes library ─────────────────
$(TP_LIB_DIR)/libOCTypes.a: $(OCT_LIB_ARCHIVE) | $(TP_LIB_DIR)
ifeq ($(IS_MINGW),)
	@echo "Extracting OCTypes library (linux/macOS)"
	@if [ -f "$@" ]; then \
	  echo "  → libOCTypes.a already exists, skipping"; \
	else \
	  unzip -o -j -q "$<" -d "$(TP_LIB_DIR)"; \
	fi
else
	@echo "Extracting OCTypes library (Windows)"
	@powershell -NoProfile -Command \
	  "if (!(Test-Path '$(TP_LIB_DIR)/libOCTypes.a')) { \
	     Expand-Archive -Path '$<' -DestinationPath '$(TP_LIB_DIR)' -Force \
	   }"
endif

# ──────────────── OCTypes headers ─────────────────
$(OCT_INCLUDE)/OCLibrary.h: $(OCT_HEADERS_ARCHIVE) | $(OCT_INCLUDE)
ifeq ($(IS_MINGW),)
	@echo "Extracting OCTypes headers (linux/macOS)"
	@if [ -f "$@" ]; then \
	  echo "  → OCLibrary.h exists, skipping"; \
	else \
	  unzip -o -j -q "$<" -d "$(OCT_INCLUDE)"; \
	fi
else
	@echo "Extracting OCTypes headers (Windows)"
	@powershell -NoProfile -Command \
	  "Expand-Archive -Path '$<' -DestinationPath '$(OCT_INCLUDE)' -Force"
endif

# Download and extract SITypes
sitypes: $(SIT_LIBDIR)/libSITypes.a $(SIT_INCLUDE)/SILibrary.h

$(SIT_LIB_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching SITypes library: $(SIT_LIB_BIN)"
	@curl -L https://github.com/pjgrandinetti/SITypes/releases/download/v0.1.0/$(SIT_LIB_BIN) -o $@

$(SIT_HEADERS_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching SITypes headers"
	@curl -L https://github.com/pjgrandinetti/SITypes/releases/download/v0.1.0/libSITypes-headers.zip -o $@

# ──────────────── SITypes library ─────────────────
$(TP_LIB_DIR)/libSITypes.a: $(SIT_LIB_ARCHIVE) | $(TP_LIB_DIR)
ifeq ($(IS_MINGW),)
	@echo "Extracting SITypes library (linux/macOS)"
	@if [ -f "$@" ]; then \
	  echo "  → libSITypes.a already exists, skipping"; \
	else \
	  unzip -o -j -q "$<" -d "$(TP_LIB_DIR)"; \
	fi
else
	@echo "Extracting SITypes library (Windows)"
	@powershell -NoProfile -Command \
	  "if (!(Test-Path '$(TP_LIB_DIR)/libSITypes.a')) { \
	     Expand-Archive -Path '$<' -DestinationPath '$(TP_LIB_DIR)' -Force \
	   }"
endif

# ──────────────── SITypes headers ─────────────────
$(SIT_INCLUDE)/SILibrary.h: $(SIT_HEADERS_ARCHIVE) | $(SIT_INCLUDE)
ifeq ($(IS_MINGW),)
	@echo "Extracting SITypes headers (linux/macOS)"
	@if [ -f "$@" ]; then \
	  echo "  → SILibrary.h exists, skipping"; \
	else \
	  unzip -o -j -q "$<" -d "$(SIT_INCLUDE)"; \
	fi
else
	@echo "Extracting SITypes headers (Windows)"
	@powershell -NoProfile -Command \
	  "Expand-Archive -Path '$<' -DestinationPath '$(SIT_INCLUDE)' -Force"
endif

prepare:
	@echo "Preparing generated files"

# Build static library
$(LIB_DIR)/libRMNLib.a: $(OBJ)
	$(AR) rcs $@ $^

# Test sources and objects
TEST_SRC := $(wildcard $(TEST_SRC_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRC))

# 1) FIRST: compile tests/*.c
$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# 2) THEN: compile src/*.c
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Test binary
$(BIN_DIR)/runTests: $(LIB_DIR)/libRMNLib.a $(TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L$(LIB_DIR) -L$(SIT_LIBDIR) -L$(OCT_LIBDIR) \
		-lRMNLib -lSITypes -lOCTypes -o $@

# AddressSanitizer test binary
$(BIN_DIR)/runTests.asan: $(LIB_DIR)/libRMNLib.a $(TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS_DEBUG) -fsanitize=address -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L$(LIB_DIR) -L$(SIT_LIBDIR) -L$(OCT_LIBDIR) \
		-lRMNLib -lSITypes -lOCTypes -o $@

test: $(BIN_DIR)/runTests
	$<

test-asan: $(BIN_DIR)/runTests.asan
	$<

clean:
	$(RM) -r $(BUILD_DIR) libRMNLib.a
	$(RM) -rf $(THIRD_PARTY_DIR)

# Determine repository root and Xcode build dir
ROOT_DIR := $(shell cd $(dir $(firstword $(MAKEFILE_LIST))).. && pwd)
XCODE_BUILD := $(CURDIR)/build

#────────────────────────────────────────────────────────────────────────────
# Xcode support
#────────────────────────────────────────────────────────────────────────────
.PHONY: xcode xcode-open xcode-run
# Combined Xcode workspace for OCTypes, SITypes, and RMNLib
xcode: clean dirs
	@echo "Generating combined OCTypes+SITypes+RMNLib Xcode project in $(XCODE_BUILD)/..."
	@mkdir -p $(XCODE_BUILD)
	@cmake -G "Xcode" -S $(ROOT_DIR) -B $(XCODE_BUILD)
	@echo "✅ Combined Xcode project created: $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj"

xcode-open: xcode
	@echo "Opening Xcode project..."
	open $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj

xcode-run: xcode
	@echo "Building RMNLib (and dependencies) inside Xcode workspace..."
	xcodebuild -project $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj \
	           -configuration Debug \
	           -scheme RMNLib \
	           -destination 'platform=macOS' \
	build | xcpretty || true

#────────────────────────────────────────────────────────────────────────────
# Documentation
#────────────────────────────────────────────────────────────────────────────
.PHONY: docs
docs:
	@echo "Generating Doxygen XML..."
	@cd docs && doxygen Doxyfile
	@echo "Building Sphinx HTML..."
	@cd docs && sphinx-build -b html . _build/html

.PHONY: synclib
synclib:
	@echo "Copying OCTypes and SITypes into third_party/lib and include..."
	@$(MKDIR_P) $(THIRD_PARTY_DIR)
	@$(RM) -r $(THIRD_PARTY_DIR)/lib $(THIRD_PARTY_DIR)/include
	@$(MKDIR_P) $(THIRD_PARTY_DIR)/lib $(THIRD_PARTY_DIR)/include/OCTypes $(THIRD_PARTY_DIR)/include/SITypes
	@cp ../OCTypes/install/lib/libOCTypes.a        $(THIRD_PARTY_DIR)/lib/
	@cp ../OCTypes/install/include/OCTypes/*.h     $(THIRD_PARTY_DIR)/include/OCTypes/
	@cp ../SITypes/install/lib/libSITypes.a        $(THIRD_PARTY_DIR)/lib/
	@cp ../SITypes/install/include/SITypes/*.h     $(THIRD_PARTY_DIR)/include/SITypes/
	@# Create dummy archives to satisfy fetch prerequisites and prevent re-fetch
	@touch $(OCT_LIB_ARCHIVE) $(OCT_HEADERS_ARCHIVE) $(SIT_LIB_ARCHIVE) $(SIT_HEADERS_ARCHIVE)