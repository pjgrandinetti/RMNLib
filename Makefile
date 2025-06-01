# Makefile for RMNLib static library (modern layout with OCTypes and SITypes dependencies)

.DEFAULT_GOAL := all
.SUFFIXES:

# Tools
CC      := clang
AR      := ar
LEX     := flex
YACC    := bison -y
YFLAGS  := -d

RM      := rm -f
MKDIR_P := mkdir -p

SRC_DIR        := src
TEST_SRC_DIR   := test
BUILD_DIR      := build
OBJ_DIR        := $(BUILD_DIR)/obj
GEN_DIR        := $(BUILD_DIR)/gen
BIN_DIR        := $(BUILD_DIR)/bin

# Third-party dependencies
THIRD_PARTY_DIR := third_party
OCTYPES_DIR     := $(THIRD_PARTY_DIR)/OCTypes
SITYPES_DIR     := $(THIRD_PARTY_DIR)/SITypes

OCT_INCLUDE     := $(OCTYPES_DIR)/include
OCT_LIBDIR      := $(OCTYPES_DIR)/lib
SIT_INCLUDE     := $(SITYPES_DIR)/include
SIT_LIBDIR      := $(SITYPES_DIR)/lib

# Auto-detect OS/ARCH for third-party archives
UNAME_S := $(shell uname -s)
ARCH := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
  OCT_LIB_BIN := libOCTypes-libOCTypes-macos-latest.zip
  SIT_LIB_BIN := libSITypes-macos-latest.zip
else ifeq ($(UNAME_S),Linux)
  ifeq ($(ARCH),aarch64)
    OCT_LIB_BIN := libOCTypes-libOCTypes-linux-arm64.zip
    SIT_LIB_BIN := libSITypes-linux-arm64.zip
  else
    OCT_LIB_BIN := libOCTypes-libOCTypes-ubuntu-latest.zip
    SIT_LIB_BIN := libSITypes-ubuntu-latest.zip
  endif
else ifneq ($(findstring MINGW,$(UNAME_S)),)
  OCT_LIB_BIN := libOCTypes-libOCTypes-windows-latest.zip
  SIT_LIB_BIN := libSITypes-windows-latest.zip
endif

OCT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(OCT_LIB_BIN)
OCT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libOCTypes-headers.zip
SIT_LIB_ARCHIVE     := $(THIRD_PARTY_DIR)/$(SIT_LIB_BIN)
SIT_HEADERS_ARCHIVE := $(THIRD_PARTY_DIR)/libSITypes-headers.zip

CPPFLAGS := -I. -I$(SRC_DIR) -I$(OCT_INCLUDE) -I$(SIT_INCLUDE)
CFLAGS   := -O3 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter \
            -Wno-missing-field-initializers -Wno-unused-function -MMD -MP
CFLAGS_DEBUG := -O0 -g -Wall -Wextra -Werror -MMD -MP

# Flex/Bison sources
LEX_SRC        := $(wildcard $(SRC_DIR)/*.l)
YACC_SRC       := $(wildcard $(SRC_DIR)/*Parser.y)

GEN_PARSER_C   := $(patsubst $(SRC_DIR)/%Parser.y,$(GEN_DIR)/%Parser.tab.c,$(YACC_SRC))
GEN_PARSER_H   := $(patsubst $(SRC_DIR)/%Parser.y,$(GEN_DIR)/%Parser.tab.h,$(YACC_SRC))
GEN_SCANNER    := $(patsubst $(SRC_DIR)/%.l,$(GEN_DIR)/%.c,$(LEX_SRC))
GEN_C          := $(GEN_PARSER_C) $(GEN_SCANNER)
GEN_H          := $(GEN_PARSER_H)

STATIC_SRC     := $(filter-out $(YACC_SRC) $(LEX_SRC),$(wildcard $(SRC_DIR)/*.c))
OBJ_SRC        := $(STATIC_SRC) $(GEN_C)
OBJ            := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter %.c,$(STATIC_SRC))) \
                  $(patsubst $(GEN_DIR)/%.c,$(OBJ_DIR)/%.o,$(GEN_C))
DEP            := $(OBJ:.o=.d)

TEST_C_FILES   := $(wildcard $(TEST_SRC_DIR)/*.c)
TEST_OBJ       := $(patsubst $(TEST_SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_C_FILES))

ifeq ($(shell uname -s), Linux)
  GROUP_START := -Wl,--start-group
  GROUP_END   := -Wl,--end-group
else
  GROUP_START :=
  GROUP_END   :=
endif

LIB_NAME := libRMNLib.a
INSTALL_DIR := install
INSTALL_LIB_DIR := $(INSTALL_DIR)/lib
INSTALL_INC_DIR := $(INSTALL_DIR)/include/RMNLib

.PHONY: all dirs prepare sync-libs sync-deps install uninstall clean clean-docs clean-objects \
        test test-debug test-asan test-werror docs doxygen html xcode

all: dirs sync-libs prepare $(LIB_NAME)

dirs:
	$(MKDIR_P) $(BUILD_DIR) $(OBJ_DIR) $(GEN_DIR) $(BIN_DIR)

# === Dependency Sync ===

sync-libs: $(OCT_LIBDIR)/libOCTypes.a $(OCT_INCLUDE)/OCTypes/OCLibrary.h \
           $(SIT_LIBDIR)/libSITypes.a $(SIT_INCLUDE)/SITypes/SILibrary.h

sync-deps: sync-libs

prepare: $(GEN_H)

# === Build Library ===

$(LIB_NAME): $(OBJ)
	$(AR) rcs $@ $^

# === Pattern Rules ===

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(GEN_DIR)/%.c | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(GEN_DIR) -c -o $@ $<

$(OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c | dirs
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(GEN_DIR)/%Parser.tab.c $(GEN_DIR)/%Parser.tab.h: $(SRC_DIR)/%Parser.y | dirs
	$(YACC) $(YFLAGS) $<
	mv y.tab.c $(GEN_DIR)/$*Parser.tab.c
	mv y.tab.h $(GEN_DIR)/$*Parser.tab.h

$(GEN_DIR)/%Scanner.c: $(SRC_DIR)/%Scanner.l $(GEN_DIR)/%Parser.tab.h | dirs
	$(LEX) -o $@ $<

$(GEN_DIR)/%.c: $(SRC_DIR)/%.l | dirs
	$(LEX) -o $@ $<

# === Tests ===

$(BIN_DIR)/runTests: $(LIB_NAME) $(TEST_OBJ)
	$(CC) $(CFLAGS) -Isrc -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L. -L$(OCT_LIBDIR) -L$(SIT_LIBDIR) \
		$(GROUP_START) -lOCTypes -lSITypes -lRMNLib $(GROUP_END) -lm -o $@

# Run tests
test: $(BIN_DIR)/runTests
	$<

test-debug: CFLAGS := $(CFLAGS_DEBUG)
test-debug: clean all test

test-asan: CFLAGS += -DLEAK_SANITIZER
test-asan: $(LIB_NAME) $(TEST_OBJ)
	$(CC) $(CFLAGS) -g -O1 -fsanitize=address -fno-omit-frame-pointer -Isrc -I$(TEST_SRC_DIR) $(TEST_OBJ) \
		-L. -L$(OCT_LIBDIR) -L$(SIT_LIBDIR) $(GROUP_START) -lOCTypes -lSITypes -lRMNLib $(GROUP_END) -lm -o $(BIN_DIR)/runTests.asan
	@./$(BIN_DIR)/runTests.asan

test-werror: CFLAGS := $(CFLAGS_DEBUG)
test-werror: clean all test

# === Install ===

install: all
	$(MKDIR_P) $(INSTALL_LIB_DIR) $(INSTALL_INC_DIR)
	cp $(LIB_NAME) $(INSTALL_LIB_DIR)/
	cp src/*.h $(INSTALL_INC_DIR)/

uninstall:
	$(RM) $(INSTALL_LIB_DIR)/$(LIB_NAME)
	$(RM) $(INSTALL_INC_DIR)/*.h
	-rmdir --ignore-fail-on-non-empty $(INSTALL_INC_DIR)

# === Cleaning ===

clean-objects:
	$(RM) $(OBJ) $(TEST_OBJ)

clean:
	$(RM) -r $(BUILD_DIR) $(LIB_NAME) runTests runTests.debug runTests.asan *.dSYM
	$(RM) *.tab.* *Scanner.c *.d core.*
	$(RM) -rf docs/doxygen docs/_build docs/html build-xcode install third_party/OCTypes third_party/SITypes

clean-docs:
	@rm -rf docs/doxygen docs/_build

# === Documentation ===

doxygen:
	cd docs && doxygen Doxyfile

html: doxygen
	cd docs && sphinx-build -W -E -b html . _build/html

docs: html

xcode:
	@echo "Generating Xcode project..."
	@mkdir -p build-xcode
	@cmake -G "Xcode" -S . -B build-xcode

# === Dependency Tracking ===

-include $(DEP)

$(OCT_LIB_ARCHIVE):
    @mkdir -p $(THIRD_PARTY_DIR)
    @echo "Fetching OCTypes library: $(OCT_LIB_BIN)"
    @curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.1/$(OCT_LIB_BIN) -o $@

$(OCT_HEADERS_ARCHIVE):
    @mkdir -p $(THIRD_PARTY_DIR)
    @echo "Fetching OCTypes headers"
    @curl -L https://github.com/pjgrandinetti/OCTypes/releases/download/v0.1.1/libOCTypes-headers.zip -o $@

$(SIT_LIB_ARCHIVE):
    @mkdir -p $(THIRD_PARTY_DIR)
    @echo "Fetching SITypes library: $(SIT_LIB_BIN)"
    @curl -L https://github.com/pjgrandinetti/SITypes/releases/download/v0.1.0/$(SIT_LIB_BIN) -o $@

$(SIT_HEADERS_ARCHIVE):
    @mkdir -p $(THIRD_PARTY_DIR)
    @echo "Fetching SITypes headers"
    @curl -L https://github.com/pjgrandinetti/SITypes/releases/download/v0.1.0/libSITypes-headers.zip -o $@

# Extract third-party libs and headers
$(OCT_LIBDIR)/libOCTypes.a: $(OCT_LIB_ARCHIVE)
	@echo "Extracting OCTypes library"
	@rm -rf $(OCT_LIBDIR)
	@mkdir -p $(OCT_LIBDIR)
	@unzip -q $< -d $(OCT_LIBDIR)

$(OCT_INCLUDE)/OCTypes/OCLibrary.h: $(OCT_HEADERS_ARCHIVE)
	@echo "Extracting OCTypes headers"
	@mkdir -p $(OCT_INCLUDE)/OCTypes
	@unzip -j -q $< "*.h" -d $(OCT_INCLUDE)/OCTypes

$(SIT_LIBDIR)/libSITypes.a: $(SIT_LIB_ARCHIVE)
	@echo "Extracting SITypes library"
	@rm -rf $(SIT_LIBDIR)
	@mkdir -p $(SIT_LIBDIR)
	@unzip -q $< -d $(SIT_LIBDIR)

$(SIT_INCLUDE)/SITypes/SILibrary.h: $(SIT_HEADERS_ARCHIVE)
	@echo "Extracting SITypes headers"
	@mkdir -p $(SIT_INCLUDE)/SITypes
	@unzip -j -q $< "*.h" -d $(SIT_INCLUDE)/SITypes
