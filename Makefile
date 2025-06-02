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

# Directories
SRC_DIR         := src
TEST_SRC_DIR    := test
BUILD_DIR       := build
OBJ_DIR         := $(BUILD_DIR)/obj
GEN_DIR         := $(BUILD_DIR)/gen
BIN_DIR         := $(BUILD_DIR)/bin
LIB_DIR         := $(BUILD_DIR)/lib
THIRD_PARTY_DIR := third_party
OCTYPES_DIR     := $(THIRD_PARTY_DIR)/OCTypes
SITYPES_DIR     := $(THIRD_PARTY_DIR)/SITypes

# Include and library paths
OCT_INCLUDE := $(OCTYPES_DIR)/include
OCT_LIBDIR  := $(OCTYPES_DIR)/lib
SIT_INCLUDE := $(SITYPES_DIR)/include
SIT_LIBDIR  := $(SITYPES_DIR)/lib

# All required directories
REQUIRED_DIRS := $(BUILD_DIR) $(OBJ_DIR) $(GEN_DIR) $(BIN_DIR) $(LIB_DIR) $(THIRD_PARTY_DIR)

# Flags
CPPFLAGS := -I. -I$(SRC_DIR) -I$(OCT_INCLUDE) -I$(SIT_INCLUDE)
CFLAGS   := -O3 -Wall -Wextra \
             -Wno-sign-compare -Wno-unused-parameter \
             -Wno-missing-field-initializers -Wno-unused-function \
             -MMD -MP
CFLAGS_DEBUG := -O0 -g -Wall -Wextra -Werror -MMD -MP

# OS-specific library ZIP selection
UNAME_S := $(shell uname -s)
ARCH := $(shell uname -m)
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

.PHONY: all dirs clean prepare octypes sitypes test test-asan

all: dirs octypes sitypes prepare $(LIB_DIR)/libRMNLib.a

dirs: $(REQUIRED_DIRS)

$(REQUIRED_DIRS):
	$(MKDIR_P) $@

# Define object files
STATIC_SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(STATIC_SRC))

# Pattern rule for compiling object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Download and extract OCTypes
octypes: $(OCT_LIBDIR)/libOCTypes.a $(OCT_INCLUDE)/OCTypes/OCLibrary.h

$(OCT_LIB_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes library: $(OCT_LIB_BIN)"
	@curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.1/$(OCT_LIB_BIN) -o $@

$(OCT_HEADERS_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes headers"
	@curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.1/libOCTypes-headers.zip -o $@

$(OCT_LIBDIR)/libOCTypes.a: $(OCT_LIB_ARCHIVE)
	@echo "Extracting OCTypes library"
	@$(RM) -r $(OCT_LIBDIR)
	@$(MKDIR_P) $(OCT_LIBDIR)
	@unzip -q $< -d $(OCT_LIBDIR)

$(OCT_INCLUDE)/OCTypes/OCLibrary.h: $(OCT_HEADERS_ARCHIVE)
	@echo "Extracting OCTypes headers"
	@$(RM) -r $(OCT_INCLUDE)
	@$(MKDIR_P) $(OCT_INCLUDE)/OCTypes
	@unzip -q $< -d $(OCT_INCLUDE)
	@mv $(OCT_INCLUDE)/*.h $(OCT_INCLUDE)/OCTypes/ 2>/dev/null || true

# Download and extract SITypes
sitypes: $(SIT_LIBDIR)/libSITypes.a $(SIT_INCLUDE)/SITypes/SILibrary.h

$(SIT_LIB_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching SITypes library: $(SIT_LIB_BIN)"
	@curl -L https://github.com/pjgrandinetti/SITypes/releases/download/v0.1.0/$(SIT_LIB_BIN) -o $@

$(SIT_HEADERS_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching SITypes headers"
	@curl -L https://github.com/pjgrandinetti/SITypes/releases/download/v0.1.0/libSITypes-headers.zip -o $@

$(SIT_LIBDIR)/libSITypes.a: $(SIT_LIB_ARCHIVE)
	@echo "Extracting SITypes library"
	@$(RM) -r $(SIT_LIBDIR)
	@$(MKDIR_P) $(SIT_LIBDIR)
	@unzip -q $< -d $(SIT_LIBDIR)

$(SIT_INCLUDE)/SITypes/SILibrary.h: $(SIT_HEADERS_ARCHIVE)
	@echo "Extracting SITypes headers"
	@$(RM) -r $(SIT_INCLUDE)
	@$(MKDIR_P) $(SIT_INCLUDE)/SITypes
	@unzip -q $< -d $(SIT_INCLUDE)
	@mv $(SIT_INCLUDE)/*.h $(SIT_INCLUDE)/SITypes/ 2>/dev/null || true

prepare:
	@echo "Preparing generated files"

# Build static library
$(LIB_DIR)/libRMNLib.a: $(OBJ)
	$(AR) rcs $@ $^

# Test sources and objects
TEST_SRC := $(wildcard $(TEST_SRC_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRC))

$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Test binary
$(BIN_DIR)/runTests: $(LIB_DIR)/libRMNLib.a $(TEST_OBJ)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L$(LIB_DIR) -L$(SIT_LIBDIR) -L$(OCT_LIBDIR) \
		-lRMNLib -lSITypes -lOCTypes -o $@

# AddressSanitizer test binary
$(BIN_DIR)/runTests.asan: $(LIB_DIR)/libRMNLib.a $(TEST_OBJ)
	$(CC) $(CFLAGS_DEBUG) -fsanitize=address -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L$(LIB_DIR) -L$(SIT_LIBDIR) -L$(OCT_LIBDIR) \
		-lRMNLib -lSITypes -lOCTypes -o $@

test: $(BIN_DIR)/runTests
	$<

test-asan: $(BIN_DIR)/runTests.asan
	$<

clean:
	$(RM) -r $(BUILD_DIR) libRMNLib.a
	$(RM) -rf $(THIRD_PARTY_DIR)/OCTypes $(THIRD_PARTY_DIR)/SITypes
