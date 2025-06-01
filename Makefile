# Makefile for RMNLib static library

# Default target
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
TEST_SRC_DIR   := tests

# Third-party dependencies
OCTYPES_DIR    := ../OCTypes
OCT_INCLUDE    := $(OCTYPES_DIR)/include
OCT_LIBDIR     := $(OCTYPES_DIR)/lib

SITYPES_DIR    := ../SITypes
SI_INCLUDE      := $(SITYPES_DIR)/include
SI_LIBDIR       := $(SITYPES_DIR)/lib

# Compiler flags
CPPFLAGS := -I. -I$(SRC_DIR) -I$(OCT_INCLUDE) -I$(SI_INCLUDE)
CFLAGS   := -O3 -Wall -Wextra \
             -Wno-sign-compare -Wno-unused-parameter \
             -Wno-missing-field-initializers -Wno-unused-function \
             -MMD -MP
CFLAGS_DEBUG := -O0 -g -Wall -Wextra -Werror -MMD -MP

# Source files
LEX_SRC       := $(wildcard $(SRC_DIR)/*.l)
YACC_SRC      := $(wildcard $(SRC_DIR)/*Parser.y)
GEN_PARSER_C  := $(patsubst $(SRC_DIR)/%Parser.y,%.tab.c,$(YACC_SRC))
GEN_PARSER_H  := $(patsubst $(SRC_DIR)/%Parser.y,%.tab.h,$(YACC_SRC))
GEN_SCANNER   := $(patsubst $(SRC_DIR)/%Scanner.l,%Scanner.c,$(LEX_SRC))
GEN_C         := $(GEN_PARSER_C) $(GEN_SCANNER)
GEN_H         := $(GEN_PARSER_H)

STATIC_SRC    := $(filter-out $(YACC_SRC) $(LEX_SRC),$(wildcard $(SRC_DIR)/*.c))
ALL_C         := $(GEN_C) $(notdir $(STATIC_SRC))
OBJ           := $(ALL_C:.c=.o)
DEP           := $(OBJ:.o=.d)

TEST_C_FILES  := $(wildcard $(TEST_SRC_DIR)/*.c)
TEST_OBJ      := $(notdir $(TEST_C_FILES:.c=.o))

# After detecting OS, define linker group flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  GROUP_START := -Wl,--start-group
  GROUP_END   := -Wl,--end-group
else
  GROUP_START :=
  GROUP_END   :=
endif

.PHONY: all prepare test test-debug test-asan run-asan test-werror \
        install uninstall clean clean-objects clean-docs docs doxygen html

all: prepare libRMNLib.a

prepare: $(GEN_PARSER_H)

# Library target
libRMNLib.a: $(OBJ)
	$(AR) rcs $@ $^

# Pattern rules for compilation
%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: $(TEST_SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Bison rule: generate .tab.c and .tab.h
%.tab.c %.tab.h: $(SRC_DIR)/%Parser.y
	$(YACC) $(YFLAGS) $<
	mv y.tab.c $*.tab.c
	mv y.tab.h $*.tab.h

# Flex rule: generate scanner .c
%Scanner.c: $(SRC_DIR)/%Scanner.l $(patsubst %Scanner.c,%.tab.h,$@)
	$(LEX) -o $@ $<

# Tests
runTests: libRMNLib.a $(TEST_OBJ)
	$(CC) $(CFLAGS) -Isrc -I$(TEST_SRC_DIR) $(TEST_OBJ) \
	  -L. -L$(OCT_LIBDIR) -L$(SI_LIBDIR) \
	  $(GROUP_START) -lOCTypes -lSITypes -lRMNLib $(GROUP_END) \
	  -lm -o runTests

test: libRMNLib.a $(TEST_OBJ)
	$(CC) $(CFLAGS) -Isrc -Itests $(TEST_OBJ) \
	  -L. -L$(OCT_LIBDIR) -L$(SI_LIBDIR) \
	  $(GROUP_START) -lOCTypes -lSITypes -lRMNLib $(GROUP_END) \
	  -lm -o runTests
	./runTests

# Debug tests
test-debug: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
test-debug: clean all test

# AddressSanitizer test target: rebuild with ASan-enabled flags and run
test-asan: CFLAGS += -DLEAK_SANITIZER
test-asan: libRMNLib.a $(TEST_OBJ)
	$(CC) $(CFLAGS) -g -O1 -fsanitize=address -fno-omit-frame-pointer -Isrc -I$(TEST_SRC_DIR) $(TEST_OBJ) \
	  -L. -L$(OCT_LIBDIR) $(GROUP_START) -lOCTypes -lRMNLib $(GROUP_END) \
	  -lm -o runTests.asan
	@echo "Running AddressSanitizer build..."
	@./runTests.asan

# Treat warnings as errors
test-werror: CFLAGS := $(CFLAGS_DEBUG)
test-werror: clean all test

# Install/uninstall
PREFIX    ?= /usr/local
INCDIR    := $(PREFIX)/include/RMNLib
LIBDIR    := $(PREFIX)/lib

install: libRMNLib.a $(GEN_H)
	install -d $(DESTDIR)$(INCDIR)
	install -m 0644 $(GEN_H) $(DESTDIR)$(INCDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -m 0644 libRMNLib.a $(DESTDIR)$(LIBDIR)

uninstall:
	$(RM) $(DESTDIR)$(LIBDIR)/libRMNLib.a
	$(RM) $(DESTDIR)$(INCDIR)/*.h
	-rmdir --ignore-fail-on-non-empty $(DESTDIR)$(INCDIR)

# Documentation targets
.PHONY: docs doxygen html

doxygen:
	@echo "Generating Doxygen XML..."
	@cd docs && doxygen Doxyfile

html: doxygen
	@echo "Building Sphinx HTML..."
	@cd docs && sphinx-build -W -E -b html . _build/html

# Alias "make docs" to build HTML
docs: html

clean-objects:
	$(RM) $(OBJ) $(TEST_OBJ)

clean:
	$(RM) libRMNLib.a $(GEN_C) $(GEN_H) runTests runTests.debug runTests.asan *.dSYM -rf
	$(RM) $(DEP)

clean-docs:
	$(RM) -rf docs/doxygen build/html

# Copy locally built OCTypes and SITypes libraries and headers
.PHONY: sync-libs

sync-libs: ../SITypes/lib/libSITypes.a ../OCTypes/lib/libOCTypes.a
	@echo "Copying locally built OCTypes and SITypes libraries and headers..."
	@$(RM) -r third_party/OCTypes third_party/SITypes
	@$(MKDIR_P) third_party/OCTypes/lib third_party/OCTypes/include/OCTypes
	@$(MKDIR_P) third_party/SITypes/lib third_party/SITypes/include/SITypes
	@cp ../OCTypes/lib/libOCTypes.a third_party/OCTypes/lib/
	@cp ../OCTypes/include/OCTypes/*.h third_party/OCTypes/include/OCTypes/
	@cp ../SITypes/lib/libSITypes.a third_party/SITypes/lib/
	@cp ../SITypes/include/SITypes/*.h third_party/SITypes/include/SITypes/
