# Makefile for RMNLib with best-practice build structure (output in build/lib)

.DEFAULT_GOAL := all
.SUFFIXES:

# Detect OS early for various decisions
UNAME_S := $(shell uname -s)

# Tools
# Use LLVM Clang with OpenMP on macOS by default, but allow override via CC environment variable
ifeq ($(origin CC),default)
  # Default case: use LLVM clang if available on macOS
  ifeq ($(UNAME_S),Darwin)
    ifneq (,$(wildcard /opt/homebrew/opt/llvm/bin/clang))
      CC := /opt/homebrew/opt/llvm/bin/clang
    else
      CC := clang
    endif
  else
    CC := clang
  endif
endif
AR      := ar
LEX     := flex
YACC    := bison
YFLAGS  := -d
CURL_CFLAGS := $(shell curl-config --cflags)
CURL_LIBS   := $(shell curl-config --libs)

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
REQUIRED_DIRS := $(BUILD_DIR) $(OBJ_DIR) $(GEN_DIR) $(BIN_DIR) $(LIB_DIR) $(THIRD_PARTY_DIR) \
                 $(OBJ_DIR)/core $(OBJ_DIR)/core/dependent_variable $(OBJ_DIR)/importers $(OBJ_DIR)/spectroscopy $(OBJ_DIR)/utils

# Flags
CPPFLAGS := -I. -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/core/dependent_variable -I$(SRC_DIR)/importers -I$(SRC_DIR)/spectroscopy \
            -I$(SRC_DIR)/utils -I$(SRC_DIR)/third_party -I$(TEST_SRC_DIR) -I$(OCT_INCLUDE) -I$(SIT_INCLUDE)
# Add LLVM/OpenMP include paths on macOS if available
ifeq ($(UNAME_S),Darwin)
  ifneq (,$(wildcard /opt/homebrew/opt/llvm/include))
    CPPFLAGS += -I/opt/homebrew/opt/llvm/include
  endif
  ifneq (,$(wildcard /opt/homebrew/opt/libomp/include))
    CPPFLAGS += -I/opt/homebrew/opt/libomp/include
  endif
endif
CFLAGS   := -fPIC -O3 -Wall -Wextra \
             -Wno-sign-compare -Wno-unused-parameter \
             -Wno-missing-field-initializers -Wno-unused-function \
             -MMD -MP -DSTB_IMAGE_AVAILABLE
CFLAGS_DEBUG := -fPIC -O0 -g -Wall -Wextra -Werror -MMD -MP

# Detect OS for BLAS/LAPACK and macOS deprecation silence
ifeq ($(UNAME_S),Darwin)
  CFLAGS       += -DACCELERATE_NEW_LAPACK -DACCELERATE_LAPACK_ILP64
  BLAS_LDFLAGS := -framework Accelerate
else ifeq ($(UNAME_S),Linux)
  BLAS_LDFLAGS := -lopenblas -llapacke
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  BLAS_LDFLAGS := -lopenblas -lm
  # OpenBLAS headers on MSYS2 live under /mingw64/include/openblas
  CPPFLAGS     += -I/mingw64/include/openblas
endif

# OS-specific library linking and shared library configuration
ARCH := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
  # Prefer static link on macOS to avoid @rpath runtime issues
  OCTYPES_LINKLIB := $(OCT_LIBDIR)/libOCTypes.a
  SITYPES_LINKLIB := $(SIT_LIBDIR)/libSITypes.a
  # Shared library configuration for macOS
  SHLIB_EXT      = .dylib
  SHLIB_FLAGS    = -dynamiclib -fPIC
  SHLIB_LDFLAGS  = -install_name @rpath/libRMN.dylib
else ifeq ($(UNAME_S),Linux)
  # Prefer static link on Linux to avoid runtime loader issues with .so resolution
  OCTYPES_LINKLIB := $(OCT_LIBDIR)/libOCTypes.a
  SITYPES_LINKLIB := $(SIT_LIBDIR)/libSITypes.a
  # Shared library configuration for Linux
  SHLIB_EXT      = .so
  SHLIB_FLAGS    = -shared -fPIC
  SHLIB_LDFLAGS  =
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  # Prefer static link on Windows to avoid DLL deployment issues
  OCTYPES_LINKLIB := $(OCT_LIBDIR)/libOCTypes.a
  SITYPES_LINKLIB := $(SIT_LIBDIR)/libSITypes.a
  # Shared library configuration for Windows
  SHLIB_EXT      = .dll
  SHLIB_FLAGS    = -shared
  SHLIB_LDFLAGS  = -Wl,--out-implib=libRMN.dll.a
else
  OCTYPES_LINKLIB := -lOCTypes
  SITYPES_LINKLIB := -lSITypes
  SHLIB_EXT      = .so
  SHLIB_FLAGS    = -shared -fPIC
  SHLIB_LDFLAGS  =
endif
SHLIB = $(LIB_DIR)/libRMN$(SHLIB_EXT)

# Detect OpenMP support (optional for parallel processing)
# Test if compiler supports OpenMP by attempting compilation
OPENMP_TEST := $(shell echo 'int main(){return 0;}' | $(CC) -fopenmp -x c - -o /dev/null 2>/dev/null && echo yes)
ifeq ($(OPENMP_TEST),yes)
  CFLAGS       += -fopenmp
  # Set OpenMP linking flags based on OS and available libraries
  ifeq ($(UNAME_S),Darwin)
    ifneq (,$(wildcard /opt/homebrew/opt/libomp/lib))
      OPENMP_LDFLAGS := -fopenmp -L/opt/homebrew/opt/libomp/lib -lomp
    else
      OPENMP_LDFLAGS := -fopenmp
    endif
  else
    OPENMP_LDFLAGS := -fopenmp
  endif
  $(info OpenMP found - enabling parallel processing)
else
  OPENMP_LDFLAGS :=
  $(info OpenMP not found - using sequential processing)
endif

# OS-specific library ZIP selection (must come before Archives definitions)
ARCH    := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
  OCT_LIB_BIN := libOCTypes-macos-latest.zip
  SIT_LIB_BIN := libSITypes-macos-latest.zip
else ifeq ($(UNAME_S),Linux)
  ifeq ($(ARCH),aarch64)
    OCT_LIB_BIN := libOCTypes-ubuntu-latest.arm64.zip
    SIT_LIB_BIN := libSITypes-ubuntu-latest.arm64.zip
  else
    OCT_LIB_BIN := libOCTypes-ubuntu-latest.x64.zip
    SIT_LIB_BIN := libSITypes-ubuntu-latest.x64.zip
  endif
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  OCT_LIB_BIN := libOCTypes-windows-latest.zip
  SIT_LIB_BIN := libSITypes-windows-latest.zip
endif

# Archives
OCT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(OCT_LIB_BIN)
OCT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libOCTypes-headers.zip
SIT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(SIT_LIB_BIN)
SIT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libSITypes-headers.zip

.PHONY: all dirs clean prepare octypes sitypes test test-asan docs doxygen html install install-shared shared synclib fetchlibs

fetchlibs: octypes sitypes
	@echo "Both OCTypes and SITypes libraries are up to date."

# Only fetch third-party libs when third_party is empty
EMPTY_TP := $(shell [ -d $(THIRD_PARTY_DIR) ] && [ -z "$(wildcard $(THIRD_PARTY_DIR)/*)" ] && echo 1)
ifeq ($(EMPTY_TP),1)
TP_DEPS := octypes sitypes
else
TP_DEPS :=
endif

all: dirs $(TP_DEPS) prepare $(LIB_DIR)/libRMN.a $(SHLIB)

dirs: $(REQUIRED_DIRS)

$(REQUIRED_DIRS):
	$(MKDIR_P) $@

# Define object files - collect from all subdirectories
STATIC_SRC := $(wildcard $(SRC_DIR)/*.c) \
              $(wildcard $(SRC_DIR)/core/*.c) \
              $(wildcard $(SRC_DIR)/core/dependent_variable/*.c) \
              $(wildcard $(SRC_DIR)/importers/*.c) \
              $(wildcard $(SRC_DIR)/spectroscopy/*.c) \
              $(wildcard $(SRC_DIR)/utils/*.c)

# Map all source files to object files, preserving directory structure
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(STATIC_SRC))

# Download and extract OCTypes
octypes: $(TP_LIB_DIR)/libOCTypes.a $(OCT_INCLUDE)/OCTypes.h

$(OCT_LIB_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes library: $(OCT_LIB_BIN)"
	@curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.0/$(OCT_LIB_BIN) -o $@

$(OCT_HEADERS_ARCHIVE): | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes headers"
	@curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.0/libOCTypes-headers.zip -o $@

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
$(OCT_INCLUDE)/OCTypes.h: $(OCT_HEADERS_ARCHIVE) | $(OCT_INCLUDE)
ifeq ($(IS_MINGW),)
	@echo "Extracting OCTypes headers (linux/macOS)"
	@if [ -f "$@" ]; then \
	  echo "  → OCTypes.h exists, skipping"; \
	else \
	  unzip -o -j -q "$<" -d "$(OCT_INCLUDE)"; \
	fi
else
	@echo "Extracting OCTypes headers (Windows)"
	@powershell -NoProfile -Command \
	  "Expand-Archive -Path '$<' -DestinationPath '$(OCT_INCLUDE)' -Force"
endif

# Download and extract SITypes
sitypes: $(TP_LIB_DIR)/libSITypes.a $(SIT_INCLUDE)/SITypes.h

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
$(SIT_INCLUDE)/SITypes.h: $(SIT_HEADERS_ARCHIVE) | $(SIT_INCLUDE)
ifeq ($(IS_MINGW),)
	@echo "Extracting SITypes headers (linux/macOS)"
	@if [ -f "$@" ]; then \
	  echo "  → SITypes.h exists, skipping"; \
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
$(LIB_DIR)/libRMN.a: $(OBJ)
	$(AR) rcs $@ $^

# Build shared library
$(SHLIB): $(OBJ) | dirs octypes sitypes
ifneq ($(findstring MINGW,$(UNAME_S)),)
	$(CC) $(CFLAGS) $(SHLIB_FLAGS) $(SHLIB_LDFLAGS) -o $@ $(filter %.o,$^) $(OCTYPES_LINKLIB) $(SITYPES_LINKLIB) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm $(CURL_LIBS)
else
	$(CC) $(CFLAGS) $(SHLIB_FLAGS) $(SHLIB_LDFLAGS) -o $@ $(filter %.o,$^) -L$(OCT_LIBDIR) -L$(SIT_LIBDIR) -lOCTypes -lSITypes $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm $(CURL_LIBS)
endif

# Convenience target for shared library
shared: $(SHLIB)

# Test sources and objects
TEST_SRC := $(wildcard $(TEST_SRC_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRC))

# 1) FIRST: compile tests/*.c
$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

# 2) THEN: compile src/*.c and all subdirectories
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

# Subdirectory compilation rules
$(OBJ_DIR)/core/%.o: $(SRC_DIR)/core/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/core/dependent_variable/%.o: $(SRC_DIR)/core/dependent_variable/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/importers/%.o: $(SRC_DIR)/importers/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/spectroscopy/%.o: $(SRC_DIR)/spectroscopy/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/utils/%.o: $(SRC_DIR)/utils/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CURL_CFLAGS) $(CFLAGS) -c -o $@ $<

# Test binary
$(BIN_DIR)/runTests: $(LIB_DIR)/libRMN.a $(TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L$(LIB_DIR) -L$(SIT_LIBDIR) -L$(OCT_LIBDIR) \
		-lRMN $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(CURL_LIBS) \
		$(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm \
		-o $@

# AddressSanitizer test binary
$(BIN_DIR)/runTests.asan: $(LIB_DIR)/libRMN.a $(TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS_DEBUG) -fsanitize=address -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L$(LIB_DIR) -L$(SIT_LIBDIR) -L$(OCT_LIBDIR) \
		-lRMN $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(CURL_LIBS) \
		$(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm \
		-o $@

test: $(BIN_DIR)/runTests
	@echo "Running tests with CSDM_TEST_ROOT=$(TEST_DATA_ROOT)"
	CSDM_TEST_ROOT="$(TEST_DATA_ROOT)" $<

test-asan: $(BIN_DIR)/runTests.asan
	@echo "Running ASan tests with CSDM_TEST_ROOT=$(TEST_DATA_ROOT)"
	CSDM_TEST_ROOT="$(TEST_DATA_ROOT)" $<

clean:
	$(RM) -r $(BUILD_DIR) libRMN.a $(LIB_DIR)/libRMN$(SHLIB_EXT) libRMN.dll.a
	$(RM) -rf $(THIRD_PARTY_DIR)

# Determine repository root and Xcode build dir
ROOT_DIR := $(shell cd $(dir $(firstword $(MAKEFILE_LIST))).. && pwd)
XCODE_BUILD := $(CURDIR)/build-xcode
TEST_DATA_ROOT := $(ROOT_DIR)/RMNLib/tests/CSDM-TestFiles-1.0

#────────────────────────────────────────────────────────────────────────────
# Xcode support
#────────────────────────────────────────────────────────────────────────────
.PHONY: xcode xcode-open xcode-run
# Combined Xcode workspace for OCTypes, SITypes, and RMNLib
xcode: clean dirs
	@echo "Generating combined Xcode workspace for OCTypes, SITypes & RMNLib in $(XCODE_BUILD)"
	@mkdir -p $(XCODE_BUILD)
	@cmake -G "Xcode" -S $(ROOT_DIR) -B $(XCODE_BUILD)
	@echo "✅ Xcode workspace created: $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj"

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
.PHONY: doxygen
doxygen:
	@echo "Generating Doxygen XML..."
	@cd docs && doxygen Doxyfile

.PHONY: html
html: doxygen
	@echo "Building Sphinx HTML..."
	@cd docs && sphinx-build -b html . _build/html

.PHONY: docs
docs: html

# Install target: package headers and library
INSTALL_DIR := install
INSTALL_LIB_DIR := $(INSTALL_DIR)/lib
INSTALL_INC_DIR := $(INSTALL_DIR)/include/RMNLib

.PHONY: install
install: all
	$(MKDIR_P) $(INSTALL_LIB_DIR) $(INSTALL_INC_DIR)
	cp $(LIB_DIR)/libRMN.a $(INSTALL_LIB_DIR)/
	cp $(SHLIB) $(INSTALL_LIB_DIR)/
	cp src/RMNLibrary.h $(INSTALL_INC_DIR)/
	$(MKDIR_P) $(INSTALL_INC_DIR)/core $(INSTALL_INC_DIR)/importers $(INSTALL_INC_DIR)/spectroscopy $(INSTALL_INC_DIR)/utils
	cp src/core/*.h $(INSTALL_INC_DIR)/core/
	cp src/importers/*.h $(INSTALL_INC_DIR)/importers/
	cp src/spectroscopy/*.h $(INSTALL_INC_DIR)/spectroscopy/
	cp src/utils/*.h $(INSTALL_INC_DIR)/utils/

# Install both static and shared libraries
.PHONY: install-shared
install-shared: $(LIB_DIR)/libRMN.a $(SHLIB)
	$(MKDIR_P) $(INSTALL_LIB_DIR) $(INSTALL_INC_DIR)
	cp $(LIB_DIR)/libRMN.a $(INSTALL_LIB_DIR)/
	cp $(SHLIB) $(INSTALL_LIB_DIR)/
	cp src/RMNLibrary.h $(INSTALL_INC_DIR)/
	$(MKDIR_P) $(INSTALL_INC_DIR)/core $(INSTALL_INC_DIR)/importers $(INSTALL_INC_DIR)/spectroscopy $(INSTALL_INC_DIR)/utils
	cp src/core/*.h $(INSTALL_INC_DIR)/core/
	cp src/importers/*.h $(INSTALL_INC_DIR)/importers/
	cp src/spectroscopy/*.h $(INSTALL_INC_DIR)/spectroscopy/
	cp src/utils/*.h $(INSTALL_INC_DIR)/utils/

.PHONY: synclib
synclib:
	@echo "Copying OCTypes and SITypes into third_party/lib and include..."
	@$(MKDIR_P) $(THIRD_PARTY_DIR)
	@$(RM) -r $(THIRD_PARTY_DIR)/lib $(THIRD_PARTY_DIR)/include
	@$(MKDIR_P) $(THIRD_PARTY_DIR)/lib $(THIRD_PARTY_DIR)/include/OCTypes $(THIRD_PARTY_DIR)/include/SITypes
	@cp ../OCTypes/install/lib/libOCTypes.a        $(THIRD_PARTY_DIR)/lib/
	@if [ -f ../OCTypes/install/lib/libOCTypes.dylib ]; then cp ../OCTypes/install/lib/libOCTypes.dylib $(THIRD_PARTY_DIR)/lib/; fi
	@if [ -f ../OCTypes/install/lib/libOCTypes.so ]; then cp ../OCTypes/install/lib/libOCTypes.so $(THIRD_PARTY_DIR)/lib/; fi
	@if [ -f ../OCTypes/install/lib/libOCTypes.dll ]; then cp ../OCTypes/install/lib/libOCTypes.dll $(THIRD_PARTY_DIR)/lib/; fi
	@cp ../OCTypes/install/include/OCTypes/*.h     $(THIRD_PARTY_DIR)/include/OCTypes/
	@cp ../SITypes/install/lib/libSITypes.a        $(THIRD_PARTY_DIR)/lib/
	@if [ -f ../SITypes/install/lib/libSITypes.dylib ]; then cp ../SITypes/install/lib/libSITypes.dylib $(THIRD_PARTY_DIR)/lib/; fi
	@if [ -f ../SITypes/install/lib/libSITypes.so ]; then cp ../SITypes/install/lib/libSITypes.so $(THIRD_PARTY_DIR)/lib/; fi
	@if [ -f ../SITypes/install/lib/libSITypes.dll ]; then cp ../SITypes/install/lib/libSITypes.dll $(THIRD_PARTY_DIR)/lib/; fi
	@cp ../SITypes/install/include/SITypes/*.h     $(THIRD_PARTY_DIR)/include/SITypes/
	@# Create dummy archives to satisfy fetch prerequisites and prevent re-fetch
	@touch $(OCT_LIB_ARCHIVE) $(OCT_HEADERS_ARCHIVE) $(SIT_LIB_ARCHIVE) $(SIT_HEADERS_ARCHIVE)