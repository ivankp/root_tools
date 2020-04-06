SHELL := bash
CPPFLAGS := -Iinclude
CXXFLAGS := -Wall -O3 -fmax-errors=3
# CXXFLAGS := $(STD) -Wall -g -Iinclude -fmax-errors=3
LDFLAGS :=
LDLIBS :=

SRC := src
BIN := bin
BLD := .build
EXT := .cc

.PHONY: all clean

ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean)))

LDFLAGS += $(shell sed -r 's/([^:]+)(:|$$)/ -L\1/g' <<< "$$LIBRARY_PATH")

ROOT_CXXFLAGS := $(shell root-config --cflags)
ROOT_LDFLAGS  := $(shell root-config --ldflags)
ROOT_LDLIBS   := $(shell root-config --libs)

# RPATH
rpath_script := ldd $(shell root-config --libdir)/libTreePlayer.so \
  | sed -nr 's|.*=> (.+)/.+\.so[.0-9]* \(.*|\1|p' \
  | sort -u \
  | sed -nr '/^(\/usr)?\/lib/!s/^/-Wl,-rpath=/p'
ROOT_LDLIBS += $(shell $(rpath_script))

CXXFLAGS += $(ROOT_CXXFLAGS)
LDFLAGS += $(ROOT_LDFLAGS)
LDLIBS += $(ROOT_LDLIBS) -lTreePlayer

SRCS := $(shell find $(SRC) -type f -name '*$(EXT)')
DEPS := $(patsubst $(SRC)/%$(EXT),$(BLD)/%.d,$(SRCS))

GREP_EXES := grep -rl '^ *int \+main *(' $(SRC) --include='*$(EXT)'
EXES := $(patsubst $(SRC)%$(EXT),$(BIN)%,$(shell $(GREP_EXES)))

all: $(EXES)

$(BIN)/hed: LDLIBS += -lboost_regex
$(BIN)/trw: LDLIBS += -lboost_regex

$(BIN)/hed: \
  $(BLD)/program_options.o \
  $(BLD)/hed/expr.o $(BLD)/hed/hist.o $(BLD)/hed/canv.o \
  $(BLD)/hed/hist_functions.o $(BLD)/hed/canv_functions.o
$(BIN)/trw: $(BLD)/program_options.o $(BLD)/sed.o
$(BIN)/envelopes $(BIN)/yoda2root: $(BLD)/program_options.o

-include $(DEPS)

.SECONDEXPANSION:

$(DEPS): $(BLD)/%.d: $(SRC)/%$(EXT) | $(BLD)/$$(dir %)
	$(CXX) $(CPPFLAGS) -MM -MT '$(@:.d=.o)' $< -MF $@

$(BLD)/%.o: | $(BLD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(filter %$(EXT),$^) -o $@

$(BIN)/%: $(BLD)/%.o | $(BIN)
	$(CXX) $(LDFLAGS) $(filter %.o,$^) -o $@ $(LDLIBS)

$(BIN):
	mkdir -p $@

$(BLD)/%/:
	mkdir -p $@

endif

clean:
	@rm -rfv $(BLD) $(BIN)
