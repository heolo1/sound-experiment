# generic makefile i made to drag into basically any tiny project i have

define DIR_RULE
./$1/:
	mkdir -p $$@
endef

# $1 is the full relative path, $2 is compiler, $3 is any includes (e.g. $(MAINS)/$(MAIN0) for a main)
define SRC_RULE
-include $(BLD_DIR)/$1.d
$(BLD_DIR)/$1.o: $1 | $(BLD_DIR)/$(shell dirname $1)/
	$($2) -I$(INCL_DIR) -I$(EXT_DIR) $(addprefix -I,$3) -c -MMD -MF $(BLD_DIR)/$1.d -o $$@ $$< $($(2)FLAGS)
endef

# $1 is the name of a main (e.g. $(MAIN0))
define MAIN_RULE
MCXXSRCS := $(call GET_FILES,$(MNS_DIR)/$1,cpp)
MCUSRCS  := $(call GET_FILES,$(MNS_DIR)/$1,cu)

ifneq ($$(strip $$(MCXXSRCS) $$(MCUSRCS)),)
MSRC_DIRS := $$(shell dirname $$(MCXXSRCS) $$(MCUSRCS) | sort -u)

$$(foreach DIR,$$(MSRC_DIRS),$$(eval $$(call DIR_RULE,$(BLD_DIR)/$$(DIR))))

ifneq ($$(strip $$(MCXXSRCS)),)
$$(foreach SRC,$$(MCXXSRCS),$$(eval $$(call SRC_RULE,$$(SRC),CXX,$(MNS_DIR)/$1)))
endif

ifneq ($$(strip $$(MCUSRCS)),)
$$(foreach SRC,$$(MCUSRCS),$$(eval $$(call SRC_RULE,$$(SRC),NVCC,$(MNS_DIR)/$1)))

MNVDLINK := $(BLD_DIR)/$(MNS_DIR)/$1/dlink.o

$$(MNVDLINK): $$(MCUSRCS:%=$(BLD_DIR)/%.o)
	$(NVCC) -dlink -o $$@ $$^ $(NVCCFLAGS)
endif

ifeq ($$(strip $$(MCUSRCS)),)
$(BIN_DIR)/$(PRJ_NAME)-$1: $$(patsubst %,$(BLD_DIR)/%.o,$$(MCXXSRCS) $$(MCUSRCS)) $(NVDLINK) $(TARGET)
else
$(BIN_DIR)/$(PRJ_NAME)-$1: $$(patsubst %,$(BLD_DIR)/%.o,$$(MCXXSRCS) $$(MCUSRCS)) $$(MNVDLINK) $(NVDLINK) $(TARGET)
endif
	$(CXX) -o $$@ $$^ $(CXXFLAGS) $(LINKFLAGS) 

.PHONY: $1 run-$1

$1: $(BIN_DIR)/$(PRJ_NAME)-$1

run-$1: $(BIN_DIR)/$(PRJ_NAME)-$1
	@ $$<
endif
endef

# $1 is dir, $2 is ext
define GET_FILES
$(shell find $1 -type f -not -path "*/.*" -name "*.$2")
endef



SHELL     := /bin/sh
CC        := gcc
CXX       := g++
NVCC      := nvcc
CPPFLAGS  := -g -Wall -pedantic -Ofast -lpthread -lm $(shell pkg-config --libs sdl2)
CCFLAGS   := $(CPPFLAGS) -std=c17 
CXXFLAGS  := $(CPPFLAGS) -std=c++20
NVCCARCHS := 75 86
NVCCFLAGS := $(foreach NVCCARCH,$(NVCCARCHS),-gencode arch=compute_$(NVCCARCH),code=sm_$(NVCCARCH)) \
	     -O3 -std=c++20 -rdc=true -Xcompiler -Ofast,-g,-Wall,-std=c++20
LINKFLAGS := -L/usr/local/cuda/lib64 -lcuda -lcudart
PRJ_NAME  := $(notdir $(abspath .))

SRC_DIR   := ./src
INCL_DIR  := ./include
EXT_DIR   := ./ext
BLD_DIR   := ./build
BIN_DIR   := ./bin
MNS_DIR	  := ./mains

NVDLINK   := $(BLD_DIR)/dlink.o
TARGET    := $(BIN_DIR)/lib$(PRJ_NAME).a
MAINS     := $(notdir $(wildcard $(MNS_DIR)/*))
MAIN0     := audio-gen # this should be in MAINS

CSRCS     := $(call GET_FILES,$(SRC_DIR),c)
CXXSRCS   := $(call GET_FILES,$(SRC_DIR),cpp)
CUSRCS    := $(call GET_FILES,$(SRC_DIR),cu)
SRC_DIRS  := $(shell dirname $(CSRCS) $(CXXSRCS) $(CUSRCS) | sort -u)

# for now, we just assume that each main contains some header files and some source files, and that they are not indexed into a src and include directory.

.SUFFIXES: .cpp .hpp .o .d .a
.PHONY: all target clean run test $(addprefix run-,$(MAINS))

all: $(TARGET) $(addprefix $(BIN_DIR)/$(PRJ_NAME)-,$(MAINS))

target: $(TARGET)

test:

clean:
	- rm -rf ./$(BLD_DIR) ./$(BIN_DIR)

$(eval $(call DIR_RULE,$(BIN_DIR)))
$(foreach DIR,$(SRC_DIRS),$(eval $(call DIR_RULE,$(BLD_DIR)/$(DIR))))
$(foreach SRC,$(CSRCS),$(eval $(call SRC_RULE,$(SRC),CC)))
$(foreach SRC,$(CXXSRCS),$(eval $(call SRC_RULE,$(SRC),CXX)))
$(foreach SRC,$(CUSRCS),$(eval $(call SRC_RULE,$(SRC),NVCC)))

$(NVDLINK): $(CUSRCS:%=$(BLD_DIR)/%.o)
	$(NVCC) -dlink -o $@ $^ $(NVCCFLAGS)

$(TARGET): $(patsubst %,$(BLD_DIR)/%.o,$(CSRCS) $(CXXSRCS) $(CUSRCS)) | $(dir $(TARGET))
	$(AR) rcs $@ $^

$(foreach MAIN,$(MAINS),$(eval $(call MAIN_RULE,$(MAIN))))

run: run-$(MAIN0)
