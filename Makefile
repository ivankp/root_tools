CXX := g++
STD := -std=c++14
DF := $(STD) -Iinclude
CF := $(STD) -Wall -O3 -flto -Iinclude -fmax-errors=3
# CF := $(STD) -Wall -g -Iinclude -fmax-errors=3
LF := $(STD) -O3 -flto

ifneq (0, $(words $(LIBRARY_PATH)))
LF += $(shell sed 's/^/-L/;s/:/ -L/g' <<< "$$LIBRARY_PATH")
endif

ROOT_CFLAGS := $(shell root-config --cflags)
ROOT_LIBS   := $(shell root-config --libs)

# RPATH
rpath_script := ldd `root-config --libdir`/libTreePlayer.so \
  | sed -nr 's|.*=> (.+)/.+\.so[.0-9]* \(.*|\1|p' \
  | sort -u \
  | sed -nr '/^(\/usr)?\/lib/!s/^/-Wl,-rpath=/p'
ROOT_LIBS += $(shell $(rpath_script))

C_hed := $(ROOT_CFLAGS)
L_hed := $(ROOT_LIBS) -lboost_regex

C_envelopes := $(ROOT_CFLAGS)
L_envelopes := $(ROOT_LIBS)

C_hed/hist := $(ROOT_CFLAGS)
C_hed/hist_functions := $(ROOT_CFLAGS)

SRC := src
BIN := bin
BLD := .build
EXT := .cc

SRCS := $(shell find $(SRC) -type f -name '*$(EXT)')
DEPS := $(patsubst $(SRC)/%$(EXT),$(BLD)/%.d,$(SRCS))

GREP_EXES := grep -rl '^ *int \+main *(' $(SRC) --include='*$(EXT)'
EXES := $(patsubst $(SRC)%$(EXT),$(BIN)%,$(shell $(GREP_EXES)))

NODEPS := clean
.PHONY: all clean

all: $(EXES)

bin/hed: \
  $(BLD)/program_options.o $(BLD)/hed/regex.o $(BLD)/hed/hist.o \
  $(BLD)/hed/hist_functions.o
bin/envelopes: $(BLD)/program_options.o

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
-include $(DEPS)
endif

.SECONDEXPANSION:

$(DEPS): $(BLD)/%.d: $(SRC)/%$(EXT) | $(BLD)/$$(dir %)
	$(CXX) $(DF) -MM -MT '$(@:.d=.o)' $< -MF $@

$(BLD)/%.o: | $(BLD)
	$(CXX) $(CF) $(C_$*) -c $(filter %$(EXT),$^) -o $@

$(BIN)/%: $(BLD)/%.o | $(BIN)
	$(CXX) $(LF) $(filter %.o,$^) -o $@ $(L_$*)

$(BIN):
	mkdir -p $@

$(BLD)/%/:
	mkdir -p $@

clean:
	@rm -rfv $(BLD) $(BIN)
