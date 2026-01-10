# generic makefile i made to drag into basically any tiny project i have

define DIR_RULE
$(BLD_DIR)/$1:
	mkdir -p $$@
endef

# $1 is the full relative path, $2 is compiler, $3 is any includes (e.g. $(MAINS)/$(MAIN0) for a main)
define SRC_RULE
-include $(BLD_DIR)/$1.d
$(BLD_DIR)/$1.o: $1 | $(BLD_DIR)/$(shell dirname $1)
	$($2) -I$(INCL_DIR) -I$(EHEAD_DIR) $(addprefix -I,$3) -c -fPIC -MMD -MF $(BLD_DIR)/$1.d -o $$@ $$< $($(2)FLAGS)
endef

# $1 is the name of a main (e.g. $(MAIN0))
define MAIN_RULE
MSRCS := $(call GET_FILES,$(MNS_DIR)/$1,cpp)
MSRC_DIRS := $$(shell dirname $$(MSRCS) | sort -u)

$$(foreach DIR,$$(MSRC_DIRS),$$(eval $$(call DIR_RULE,$$(DIR))))
$$(foreach SRC,$$(MSRCS),$$(eval $$(call SRC_RULE,$$(SRC),CXX,$(MNS_DIR)/$1)))

$(BIN_DIR)/$(PRJ_NAME)-$1: $$(MSRCS:%=$(BLD_DIR)/%.o) $(TARGET)
	$(CXX) -o $$@ $$^ $(CXXFLAGS)

.PHONY: $1 run-$1

$1: $(BIN_DIR)/$(PRJ_NAME)-$1

run-$1: $(BIN_DIR)/$(PRJ_NAME)-$1
	@ $$<
endef

# $1 is dir, $2 is ext
define GET_FILES
$(shell find $1 -type f -not -path "*/.*" -name "*.$2")
endef

SHELL     := /bin/sh
CC        := gcc
CXX       := g++
CPPFLAGS  := -g -Wall -pedantic -lpthread -lm
CFLAGS    := $(CPPFLAGS) -std=c17 
CXXFLAGS  := $(CPPFLAGS) -std=c++20
PRJ_NAME  := $(notdir $(abspath .))

SRC_DIR   := ./src
INCL_DIR  := ./include
EHEAD_DIR := ./extheaders
BLD_DIR   := ./build
BIN_DIR   := ./bin
MNS_DIR	  := ./mains

TARGET    := $(BIN_DIR)/lib$(PRJ_NAME).a
MAINS     := $(notdir $(wildcard $(MNS_DIR)/*))
MAIN0     := audio-read-write # this should be in MAINS

CSRCS     := $(call GET_FILES,$(SRC_DIR),c)
CXXSRCS   := $(call GET_FILES,$(SRC_DIR),cpp)
SRC_DIRS  := $(shell dirname $(CSRCS) $(CXXSRCS) | sort -u)

# for now, we just assume that each main contains some header files and some source files, and that they are not indexed into a src and include directory.

.SUFFIXES: .cpp .hpp .o .d .a
.PHONY: all target clean run $(addprefix run-,$(MAINS))

all: $(TARGET) $(addprefix $(BIN_DIR)/$(PRJ_NAME)-,$(MAINS))

target: $(TARGET)

clean:
	- rm -rf ./$(BLD_DIR)/* ./$(BIN_DIR)/*

$(foreach DIR,$(SRC_DIRS),$(eval $(call DIR_RULE,$(DIR))))
$(foreach SRC,$(CSRCS),$(eval $(call SRC_RULE,$(SRC),CC)))
$(foreach SRC,$(CXXSRCS),$(eval $(call SRC_RULE,$(SRC),CXX)))

$(TARGET): $(CSRCS:%=$(BLD_DIR)/%.o) $(CXXSRCS:%=$(BLD_DIR)/%.o)
	$(AR) rcs $@ $^

$(foreach MAIN,$(MAINS),$(eval $(call MAIN_RULE,$(MAIN))))

run: run-$(MAIN0)