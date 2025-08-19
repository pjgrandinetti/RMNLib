# Makefile for RMNLib with best-practice build structure (output in build/lib)

.DEFAULT_GOAL := all
.SUFFIXES:

#──────── OS / toolchain detection ────────
UNAME_S := $(shell uname -s)
ARCH    := $(shell uname -m)

# Compiler (prefer Homebrew LLVM on macOS if present, override with CC=..)
ifeq ($(origin CC),default)
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

AR     := ar
LEX    := flex
YACC   := bison
YFLAGS := -d

# curl (prefer curl-config, fall back to pkg-config, then -lcurl)
CURL_CFLAGS := $(shell curl-config --cflags 2>/dev/null || pkg-config --cflags libcurl 2>/dev/null)
CURL_LIBS   := $(shell curl-config --libs   2>/dev/null || pkg-config --libs   libcurl 2>/dev/null || echo -lcurl)

RM      := rm -f
MKDIR_P := mkdir -p

#──────── Layout ────────
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
OCT_LIBDIR      := $(TP_LIB_DIR)
SIT_LIBDIR      := $(TP_LIB_DIR)

REQUIRED_DIRS := \
  $(BUILD_DIR) $(OBJ_DIR) $(GEN_DIR) $(BIN_DIR) $(LIB_DIR) \
  $(THIRD_PARTY_DIR) $(TP_LIB_DIR) $(INCLUDE_DIR) $(OCT_INCLUDE) $(SIT_INCLUDE) \
  $(OBJ_DIR)/core $(OBJ_DIR)/core/dependent_variable $(OBJ_DIR)/core/dimension \
  $(OBJ_DIR)/importers $(OBJ_DIR)/spectroscopy $(OBJ_DIR)/utils

#──────── Flags ────────
CPPFLAGS := -I. -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/core/dependent_variable \
            -I$(SRC_DIR)/core/dimension -I$(SRC_DIR)/importers -I$(SRC_DIR)/spectroscopy \
            -I$(SRC_DIR)/utils -I$(SRC_DIR)/third_party -I$(TEST_SRC_DIR) \
            -I$(OCT_INCLUDE) -I$(SIT_INCLUDE) $(CURL_CFLAGS)

# macOS includes for LLVM/OpenMP if installed
ifeq ($(UNAME_S),Darwin)
  ifneq (,$(wildcard /opt/homebrew/opt/llvm/include))
    CPPFLAGS += -I/opt/homebrew/opt/llvm/include
  endif
  ifneq (,$(wildcard /opt/homebrew/opt/libomp/include))
    CPPFLAGS += -I/opt/homebrew/opt/libomp/include
  endif
endif

CFLAGS   := -fPIC -O3 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter \
            -Wno-missing-field-initializers -Wno-unused-function -MMD -MP -DSTB_IMAGE_AVAILABLE
CFLAGS_DEBUG := -fPIC -O0 -g -Wall -Wextra -Werror -MMD -MP

# BLAS / LAPACK
ifeq ($(UNAME_S),Darwin)
  CFLAGS       += -DACCELERATE_NEW_LAPACK -DACCELERATE_LAPACK_ILP64
  BLAS_LDFLAGS := -framework Accelerate
else ifeq ($(UNAME_S),Linux)
  BLAS_LDFLAGS := -lopenblas -llapacke
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  BLAS_LDFLAGS := -lopenblas -lm
  CPPFLAGS     += -I/mingw64/include/openblas
  CFLAGS       += -Wno-unknown-pragmas
else
  BLAS_LDFLAGS :=
endif

# Linker group flags for Linux to resolve circular deps
ifeq ($(UNAME_S),Linux)
  GROUP_START := -Wl,--start-group
  GROUP_END   := -Wl,--end-group
else
  GROUP_START :=
  GROUP_END   :=
endif

# OpenMP detection
OPENMP_TEST := $(shell echo 'int main(){return 0;}' | $(CC) -fopenmp -x c - -o /dev/null 2>/dev/null && echo yes)
ifeq ($(OPENMP_TEST),yes)
  CFLAGS += -fopenmp
  ifeq ($(UNAME_S),Darwin)
    ifneq (,$(wildcard /opt/homebrew/opt/libomp/lib))
      OPENMP_LDFLAGS := -fopenmp -L/opt/homebrew/opt/libomp/lib
    else
      OPENMP_LDFLAGS := -fopenmp
    endif
  else
    OPENMP_LDFLAGS := -fopenmp
  endif
else
  OPENMP_LDFLAGS :=
endif

#──────── Shared library settings + how to link OCTypes/SITypes ────────

# Version information - extracted from git tags or manual override
VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || echo "0.1.5")
VERSION_MAJOR := $(shell echo $(VERSION) | cut -d. -f1)
VERSION_MINOR := $(shell echo $(VERSION) | cut -d. -f2)
VERSION_PATCH := $(shell echo $(VERSION) | cut -d. -f3)

ifeq ($(UNAME_S),Darwin)
  SHLIB_EXT     = .dylib
  SHLIB_FLAGS   = -dynamiclib -fPIC
  SHLIB_LDFLAGS = -install_name @rpath/libRMN.dylib -current_version $(VERSION) -compatibility_version $(VERSION_MAJOR).$(VERSION_MINOR)
  OCTYPES_LINKLIB := -L$(OCT_LIBDIR) -lOCTypes
  SITYPES_LINKLIB := -L$(SIT_LIBDIR) -lSITypes
  RPATH_FLAGS   = -Wl,-rpath,$(TP_LIB_DIR)
else ifeq ($(UNAME_S),Linux)
  SHLIB_EXT     = .so
  SHLIB_FLAGS   = -shared -fPIC
  SHLIB_LDFLAGS =
  OCTYPES_LINKLIB := -L$(OCT_LIBDIR) -lOCTypes
  SITYPES_LINKLIB := -L$(SIT_LIBDIR) -lSITypes
  RPATH_FLAGS   = -Wl,-rpath,$(TP_LIB_DIR)
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  SHLIB_EXT     = .dll
  SHLIB_FLAGS   = -shared -Wl,--export-all-symbols -Wl,--enable-auto-import
  SHLIB_LDFLAGS = -Wl,--out-implib=$(LIB_DIR)/libRMN.dll.a
  OCTYPES_LINKLIB := -L$(OCT_LIBDIR) -lOCTypes
  SITYPES_LINKLIB := -L$(SIT_LIBDIR) -lSITypes
  RPATH_FLAGS   = 
else
  SHLIB_EXT     = .so
  SHLIB_FLAGS   = -shared -fPIC
  SHLIB_LDFLAGS =
  OCTYPES_LINKLIB := -lOCTypes
  SITYPES_LINKLIB := -lSITypes
  RPATH_FLAGS   = 
endif

SHLIB := $(LIB_DIR)/libRMN$(SHLIB_EXT)

#──────── Source discovery ────────
STATIC_SRC := \
  $(wildcard $(SRC_DIR)/*.c) \
  $(wildcard $(SRC_DIR)/core/*.c) \
  $(wildcard $(SRC_DIR)/core/dependent_variable/*.c) \
  $(wildcard $(SRC_DIR)/core/dimension/*.c) \
  $(wildcard $(SRC_DIR)/importers/*.c) \
  $(wildcard $(SRC_DIR)/spectroscopy/*.c) \
  $(wildcard $(SRC_DIR)/utils/*.c)

OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(STATIC_SRC))

#──────── Release asset names for OCTypes/SITypes ────────
# Platform detection for Windows/MINGW
IS_MINGW := $(findstring MINGW,$(UNAME_S))

ifeq ($(UNAME_S),Darwin)
  OCT_LIB_BIN := libOCTypes-macos-latest.zip
  SIT_LIB_BIN := libSITypes-macos-latest.zip
else ifeq ($(UNAME_S),Linux)
  ifneq (,$(filter $(ARCH),aarch64 arm64))
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

OCT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(OCT_LIB_BIN)
OCT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libOCTypes-headers.zip
SIT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(SIT_LIB_BIN)
SIT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libSITypes-headers.zip

#──────── Phony targets ────────
.PHONY: all dirs clean clean-third-party update-deps fetchlibs octypes sitypes \
        test test-imports test-all test-asan test-imports-asan \
        doxygen html docs install install-shared shared synclib \
        xcode xcode-open xcode-run help FORCE

# Only fetch third_party when empty (speedy rebuilds)
EMPTY_TP := $(shell [ -d $(THIRD_PARTY_DIR) ] && [ -z "$(wildcard $(THIRD_PARTY_DIR)/*)" ] && echo 1)
ifeq ($(EMPTY_TP),1)
TP_DEPS := octypes sitypes
else
TP_DEPS :=
endif

all: dirs $(TP_DEPS) $(LIB_DIR)/libRMN.a $(SHLIB)

dirs: $(REQUIRED_DIRS)
$(REQUIRED_DIRS):
	$(MKDIR_P) $@

#──────── Download OCTypes/SITypes (latest releases) ────────
FORCE:

fetchlibs: octypes sitypes
	@echo "Both OCTypes and SITypes libraries are up to date."

update-deps: clean-third-party
	@echo "Purged $(THIRD_PARTY_DIR). Fetching latest OCTypes/SITypes…"
	$(MAKE) octypes sitypes

clean-third-party:
	$(RM) -r $(THIRD_PARTY_DIR)

# Archives
$(OCT_LIB_ARCHIVE): FORCE | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes library: $(OCT_LIB_BIN)"
ifeq ($(IS_MINGW),)
	@curl -fL --retry 3 --retry-delay 2 -z "$@" -o "$@" \
	  https://github.com/pjgrandinetti/OCTypes/releases/latest/download/$(OCT_LIB_BIN)
else
	@curl -fL --retry 3 --retry-delay 2 -o "$@" \
	  https://github.com/pjgrandinetti/OCTypes/releases/latest/download/$(OCT_LIB_BIN)
endif

$(OCT_HEADERS_ARCHIVE): FORCE | $(THIRD_PARTY_DIR)
	@echo "Fetching OCTypes headers"
ifeq ($(IS_MINGW),)
	@curl -fL --retry 3 --retry-delay 2 -z "$@" -o "$@" \
	  https://github.com/pjgrandinetti/OCTypes/releases/latest/download/libOCTypes-headers.zip
else
	@curl -fL --retry 3 --retry-delay 2 -o "$@" \
	  https://github.com/pjgrandinetti/OCTypes/releases/latest/download/libOCTypes-headers.zip
endif

$(SIT_LIB_ARCHIVE): FORCE | $(THIRD_PARTY_DIR)
	@echo "Fetching SITypes library: $(SIT_LIB_BIN)"
ifeq ($(IS_MINGW),)
	@curl -fL --retry 3 --retry-delay 2 -z "$@" -o "$@" \
	  https://github.com/pjgrandinetti/SITypes/releases/latest/download/$(SIT_LIB_BIN)
else
	@curl -fL --retry 3 --retry-delay 2 -o "$@" \
	  https://github.com/pjgrandinetti/SITypes/releases/latest/download/$(SIT_LIB_BIN)
endif

$(SIT_HEADERS_ARCHIVE): FORCE | $(THIRD_PARTY_DIR)
	@echo "Fetching SITypes headers"
ifeq ($(IS_MINGW),)
	@curl -fL --retry 3 --retry-delay 2 -z "$@" -o "$@" \
	  https://github.com/pjgrandinetti/SITypes/releases/latest/download/libSITypes-headers.zip
else
	@curl -fL --retry 3 --retry-delay 2 -o "$@" \
	  https://github.com/pjgrandinetti/SITypes/releases/latest/download/libSITypes-headers.zip
endif

# OCTypes
octypes: $(TP_LIB_DIR)/libOCTypes.a $(OCT_INCLUDE)/OCTypes.h
$(TP_LIB_DIR)/libOCTypes.a: $(OCT_LIB_ARCHIVE) | $(TP_LIB_DIR)
ifeq ($(IS_MINGW),)
	@echo "Extracting OCTypes library (unix unzip)"
	@unzip -o -j -q "$<" -d "$(TP_LIB_DIR)"
else
	@echo "Extracting OCTypes library (PowerShell)"
	@powershell -NoProfile -Command "Expand-Archive -Path '$<' -DestinationPath '$(TP_LIB_DIR)' -Force"
endif

$(OCT_INCLUDE)/OCTypes.h: $(OCT_HEADERS_ARCHIVE) | $(OCT_INCLUDE)
ifeq ($(IS_MINGW),)
	@echo "Extracting OCTypes headers (unix unzip)"
	@unzip -o -j -q "$<" -d "$(OCT_INCLUDE)"
else
	@echo "Extracting OCTypes headers (PowerShell)"
	@powershell -NoProfile -Command "Expand-Archive -Path '$<' -DestinationPath 'temp_extract_oct' -Force; Get-ChildItem -Path 'temp_extract_oct' -Filter '*.h' -Recurse | Copy-Item -Destination '$(OCT_INCLUDE)'; Start-Sleep -Milliseconds 100; if (Test-Path 'temp_extract_oct') { Remove-Item -Path 'temp_extract_oct' -Recurse -Force -ErrorAction SilentlyContinue }"
endif

# SITypes
sitypes: $(TP_LIB_DIR)/libSITypes.a $(SIT_INCLUDE)/SITypes.h
$(TP_LIB_DIR)/libSITypes.a: $(SIT_LIB_ARCHIVE) | $(TP_LIB_DIR)
ifeq ($(IS_MINGW),)
	@echo "Extracting SITypes library (unix unzip)"
	@unzip -o -j -q "$<" -d "$(TP_LIB_DIR)"
else
	@echo "Extracting SITypes library (PowerShell)"
	@powershell -NoProfile -Command "Expand-Archive -Path '$<' -DestinationPath '$(TP_LIB_DIR)' -Force"
endif

$(SIT_INCLUDE)/SITypes.h: $(SIT_HEADERS_ARCHIVE) | $(SIT_INCLUDE)
ifeq ($(IS_MINGW),)
	@echo "Extracting SITypes headers (unix unzip)"
	@unzip -o -j -q "$<" -d "$(SIT_INCLUDE)"
else
	@echo "Extracting SITypes headers (PowerShell)"
	@powershell -NoProfile -Command "Expand-Archive -Path '$<' -DestinationPath 'temp_extract_sit' -Force; Get-ChildItem -Path 'temp_extract_sit' -Filter '*.h' -Recurse | Copy-Item -Destination '$(SIT_INCLUDE)'; Start-Sleep -Milliseconds 100; if (Test-Path 'temp_extract_sit') { Remove-Item -Path 'temp_extract_sit' -Recurse -Force -ErrorAction SilentlyContinue }"
endif

#──────── Build rules ────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Static library
$(LIB_DIR)/libRMN.a: $(OBJ)
	$(AR) rcs $@ $^

# Shared library (link against OCTypes/SITypes + BLAS/OpenMP/curl)
$(SHLIB): $(OBJ) | dirs octypes sitypes
	$(CC) $(CFLAGS) $(SHLIB_FLAGS) $(SHLIB_LDFLAGS) -o $@ \
	  $(filter %.o,$^) $(OCTYPES_LINKLIB) $(SITYPES_LINKLIB) \
	  $(RPATH_FLAGS) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm $(CURL_LIBS)

shared: $(SHLIB)

#──────── Tests (core, imports, all) ────────
TEST_SRC := $(wildcard $(TEST_SRC_DIR)/*.c)
CORE_TEST_SRC := $(filter-out $(TEST_SRC_DIR)/main_imports.c $(TEST_SRC_DIR)/main_all.c, $(TEST_SRC))
CORE_TEST_OBJ := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CORE_TEST_SRC))
IMPORT_TEST_SRC := $(TEST_SRC_DIR)/main_imports.c $(TEST_SRC_DIR)/test_CSDM.c $(TEST_SRC_DIR)/test_Image.c \
                   $(TEST_SRC_DIR)/test_JCAMP.c $(TEST_SRC_DIR)/test_Tecmag.c $(TEST_SRC_DIR)/test_utils.c
IMPORT_TEST_OBJ := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(IMPORT_TEST_SRC))
ALL_TEST_SRC := $(filter-out $(TEST_SRC_DIR)/main.c $(TEST_SRC_DIR)/main_imports.c $(TEST_SRC_DIR)/main_all.c, $(TEST_SRC)) \
                $(TEST_SRC_DIR)/main_all.c
ALL_TEST_OBJ := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(ALL_TEST_SRC))

$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c | dirs octypes sitypes
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/runTests: $(LIB_DIR)/libRMN.a $(CORE_TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(CORE_TEST_OBJ) \
	  $(GROUP_START) $(LIB_DIR)/libRMN.a $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(GROUP_END) \
	  $(RPATH_FLAGS) $(CURL_LIBS) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm -o $@

$(BIN_DIR)/runImportTests: $(LIB_DIR)/libRMN.a $(IMPORT_TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(IMPORT_TEST_OBJ) \
	  $(GROUP_START) $(LIB_DIR)/libRMN.a $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(GROUP_END) \
	  $(RPATH_FLAGS) $(CURL_LIBS) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm -o $@

$(BIN_DIR)/runAllTests: $(LIB_DIR)/libRMN.a $(ALL_TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(ALL_TEST_OBJ) \
	  $(GROUP_START) $(LIB_DIR)/libRMN.a $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(GROUP_END) \
	  $(RPATH_FLAGS) $(CURL_LIBS) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm -o $@

# Windows: Copy required DLLs to the bin directory for test executables
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
copy-dlls: octypes sitypes
	@if [ -f $(TP_LIB_DIR)/libOCTypes.dll ]; then cp $(TP_LIB_DIR)/libOCTypes.dll $(BIN_DIR)/; fi
	@if [ -f $(TP_LIB_DIR)/libSITypes.dll ]; then cp $(TP_LIB_DIR)/libSITypes.dll $(BIN_DIR)/; fi
	@if [ -f $(LIB_DIR)/libRMN.dll ]; then cp $(LIB_DIR)/libRMN.dll $(BIN_DIR)/; fi
else
copy-dlls:
	@# No-op on non-Windows platforms
endif

test: $(BIN_DIR)/runTests copy-dlls
	@echo "Running core tests (fast, no imports)"
	$<

test-imports: $(BIN_DIR)/runImportTests copy-dlls
	@echo "Running import tests (slow) with TEST_DATA_ROOT=$(TEST_DATA_ROOT)"
	CSDM_TEST_ROOT="$(TEST_DATA_ROOT)" $<

test-all: $(BIN_DIR)/runAllTests copy-dlls
	@echo "Running all tests (core + imports) with TEST_DATA_ROOT=$(TEST_DATA_ROOT)"
	CSDM_TEST_ROOT="$(TEST_DATA_ROOT)" $<

$(BIN_DIR)/runTests.asan: $(LIB_DIR)/libRMN.a $(CORE_TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS_DEBUG) -fsanitize=address -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(CORE_TEST_OBJ) \
	  $(GROUP_START) $(LIB_DIR)/libRMN.a $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(GROUP_END) \
	  $(RPATH_FLAGS) $(CURL_LIBS) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm -o $@

$(BIN_DIR)/runImportTests.asan: $(LIB_DIR)/libRMN.a $(IMPORT_TEST_OBJ) octypes sitypes
	$(CC) $(CFLAGS_DEBUG) -fsanitize=address -I$(SRC_DIR) -I$(TEST_SRC_DIR) $(IMPORT_TEST_OBJ) \
	  $(GROUP_START) $(LIB_DIR)/libRMN.a $(SITYPES_LINKLIB) $(OCTYPES_LINKLIB) $(GROUP_END) \
	  $(RPATH_FLAGS) $(CURL_LIBS) $(BLAS_LDFLAGS) $(OPENMP_LDFLAGS) -lm -o $@

test-asan: $(BIN_DIR)/runTests.asan
	@echo "Running ASan core tests with leak tracking (fast)"
	OC_LEAK_TRACKING=1 $<

test-imports-asan: $(BIN_DIR)/runImportTests.asan
	@echo "Running ASan import tests with leak tracking (slow) TEST_DATA_ROOT=$(TEST_DATA_ROOT)"
	OC_LEAK_TRACKING=1 CSDM_TEST_ROOT="$(TEST_DATA_ROOT)" $<

#──────── Docs ────────
.PHONY: doxygen html docs

doxygen:
	@echo "Generating Doxygen XML…"
	@cd docs && doxygen Doxyfile

html: doxygen
	@echo "Building Sphinx HTML…"
	@cd docs && sphinx-build -b html . _build/html

docs: html

#──────── Install (preserve header subfolders) ────────
INSTALL_DIR      ?= install
INSTALL_LIB_DIR  := $(INSTALL_DIR)/lib
INSTALL_INC_DIR  := $(INSTALL_DIR)/include/RMNLib

install: all
	$(MKDIR_P) $(INSTALL_LIB_DIR) $(INSTALL_INC_DIR)
	cp $(LIB_DIR)/libRMN.a $(INSTALL_LIB_DIR)/
	cp $(SHLIB) $(INSTALL_LIB_DIR)/
ifneq ($(findstring MINGW,$(UNAME_S)),)
	@if [ -f $(LIB_DIR)/libRMN.dll.a ]; then cp $(LIB_DIR)/libRMN.dll.a $(INSTALL_LIB_DIR)/; fi
endif
	# Copy ONLY public headers (.h), keep folder structure, exclude *_private.h and internal folders
ifeq ($(IS_MINGW),)
	$(eval ROOT_DIR := $(shell pwd))
	$(eval TAR_DEST := $(shell if [ "$$(echo '$(INSTALL_INC_DIR)' | cut -c1)" = "/" ]; then echo "$(INSTALL_INC_DIR)"; else echo "$(ROOT_DIR)/$(INSTALL_INC_DIR)"; fi))
	(cd src && tar cf - --exclude='*_private.h' --exclude='*/dependent_variable' --exclude='*/dimension' . | (cd $(TAR_DEST) && tar xf -))
	find $(INSTALL_INC_DIR) ! -name "*.h" ! -type d -delete
else
	@echo "Using bash for Windows header copying (MSYS2/MinGW)..."
	@find src -name "*.h" ! -name "*_private.h" ! -path "*/dependent_variable/*" ! -path "*/dimension/*" -exec bash -c ' \
	  for file; do \
	    relpath=$$(echo "$$file" | sed "s|^src/||"); \
	    destdir="$(INSTALL_INC_DIR)/$$(dirname "$$relpath")"; \
	    mkdir -p "$$destdir"; \
	    cp "$$file" "$(INSTALL_INC_DIR)/$$relpath"; \
	  done' _ {} +
endif

install-shared: install

#──────── Sync from local OCTypes/SITypes builds (optional, avoids downloads) ────────
synclib:
	@echo "Copying OCTypes and SITypes into third_party/lib and include…"
	$(MKDIR_P) $(THIRD_PARTY_DIR) $(TP_LIB_DIR) $(INCLUDE_DIR) $(OCT_INCLUDE) $(SIT_INCLUDE)
	@cp -f ../OCTypes/install/lib/libOCTypes.a $(TP_LIB_DIR)/ 2>/dev/null || true
	@cp -f ../SITypes/install/lib/libSITypes.a $(TP_LIB_DIR)/ 2>/dev/null || true
	@cp -f ../OCTypes/install/include/OCTypes/*.h $(OCT_INCLUDE)/ 2>/dev/null || true
	@cp -f ../SITypes/install/include/SITypes/*.h $(SIT_INCLUDE)/ 2>/dev/null || true
	# Touch archives so fetch step is skipped
	@touch $(OCT_LIB_ARCHIVE) $(OCT_HEADERS_ARCHIVE) $(SIT_LIB_ARCHIVE) $(SIT_HEADERS_ARCHIVE)
	@echo "✓ synclib complete."

#──────── Xcode workspace (monorepo) ────────
ROOT_DIR := $(shell cd $(dir $(firstword $(MAKEFILE_LIST))).. && pwd)
XCODE_BUILD := $(CURDIR)/build-xcode

xcode: clean dirs
	@echo "Generating Xcode project in $(XCODE_BUILD)…"
	@mkdir -p $(XCODE_BUILD)
	@cmake -G "Xcode" -S $(ROOT_DIR) -B $(XCODE_BUILD)
	@echo "✅ Xcode project: $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj"

xcode-open: xcode
	@echo "Opening Xcode project…"
	open $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj

xcode-run: xcode
	@echo "Building RMNLib in Xcode…"
	xcodebuild -project $(XCODE_BUILD)/$(notdir $(ROOT_DIR)).xcodeproj \
	  -configuration Debug -scheme RMNLib -destination 'platform=macOS' \
	  build | xcpretty || true

#──────── Clean / help ────────
clean:
	$(RM) -r $(BUILD_DIR) $(THIRD_PARTY_DIR) $(INSTALL_DIR) \
	  libRMN.a $(LIB_DIR)/libRMN$(SHLIB_EXT) $(LIB_DIR)/libRMN.dll.a

TEST_DATA_ROOT := $(ROOT_DIR)/RMNLib/tests/CSDM-TestFiles-1.0

help:
	@echo "RMNLib Makefile targets:"
	@echo "  all              Build static and shared libraries (default)"
	@echo "  fetchlibs        Fetch OCTypes/SITypes from GitHub releases"
	@echo "  synclib          Copy OCTypes/SITypes from ../OCTypes & ../SITypes install/"
	@echo "  test             Run core tests (fast)"
	@echo "  test-imports     Run import tests (slow)"
	@echo "  test-all         Run core+imports (comprehensive)"
	@echo "  test-asan        Core tests under AddressSanitizer (OC_LEAK_TRACKING=1)"
	@echo "  docs             Build Doxygen+Sphinx HTML docs"
	@echo "  install          Install libs + public headers into ./install"
	@echo "  xcode            Generate Xcode workspace at build-xcode/"
	@echo "  clean            Remove build and install artifacts"
	