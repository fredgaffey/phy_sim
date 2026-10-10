CXX = c++
UNAME_S := $(shell uname -s)

# Taken from Tools/ardupilotwaf/boards.py (the clang branch)
CXXFLAGS = -std=gnu++11 -g \
           -fno-exceptions -fno-rtti \
           -fsigned-char \
           -Wall -Wextra -Wno-unused-parameter \
           -Werror=shadow -Werror=narrowing -Werror=return-type \
           -Werror=unused-result -Werror=unused-variable \
           -Werror=sign-compare -Werror=switch -Werror=uninitialized \
           -Werror=implicit-fallthrough -Werror=reorder \
           -Wdouble-promotion

# clang-only flag; gcc (default c++ on Linux) rejects it
ifneq (,$(findstring clang,$(shell $(CXX) --version)))
CXXFLAGS += -cl-single-precision-constant
endif

# `make debug`: full debug info, no optimisation, plus runtime checkers
#   -g3                      debug info including macros (e.g. `p MAX_EDGE_CNT`)
#   -O0 -fno-inline          code runs line-by-line as written, all variables visible
#   -fno-omit-frame-pointer  reliable backtraces
#   -fsanitize=address       stops on out-of-bounds / use-after-free with the exact line
#   -fsanitize=undefined     stops on signed overflow, null deref, bad shifts etc.
DEBUG_FLAGS = -g3 -O0 -fno-inline -fno-omit-frame-pointer \
              -fsanitize=address,undefined -fno-sanitize-recover=all

SRCS = $(wildcard *.cpp)
BANNED_HEADERS = vector|string|map|list|set|unordered_map|memory|functional|thread|mutex|iostream|sstream|fstream|chrono|exception|stdexcept

RAYLIB = raylib/src
# Per-OS raylib build so a lib built on mac never gets linked on linux (and vice versa)
RAYLIB_OUT = $(CURDIR)/build/$(UNAME_S)
RAYLIB_LIB = $(RAYLIB_OUT)/libraylib.a
LDLIBS = $(RAYLIB_LIB)
ifeq ($(UNAME_S),Darwin)
LDLIBS += -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL -framework QuartzCore
DEBUGGER = lldb
else
LDLIBS += -lGL -lm -lpthread -ldl -lrt -lX11
DEBUGGER = gdb
# main.cpp includes macOS-only <_string.h>; give Linux a shim that forwards to <string.h>
COMPAT = compat
CXXFLAGS += -isystem $(COMPAT)
COMPAT_HDRS = $(COMPAT)/_string.h
endif

prog: $(SRCS) check-headers $(RAYLIB_LIB) $(COMPAT_HDRS)
	$(CXX) $(CXXFLAGS) -isystem $(RAYLIB) $(SRCS) -o prog $(LDLIBS)
	@# Same idea as check_elf_symbols in Tools/ardupilotwaf/ardupilotwaf.py
	@if nm -C prog | grep -E 'operator new(\[\])?\(unsigned long\)$$|std::__throw'; then \
		echo "ERROR: plain new or exception symbol found - use new(std::nothrow)"; rm prog; exit 1; fi

# raylib drops its .o files in raylib/src, so clean those first in case they came from the other OS
$(RAYLIB_LIB):
	@mkdir -p $(RAYLIB_OUT)
	$(MAKE) -C $(RAYLIB) clean PLATFORM=PLATFORM_DESKTOP RAYLIB_RELEASE_PATH=$(RAYLIB_OUT)
	$(MAKE) -C $(RAYLIB) PLATFORM=PLATFORM_DESKTOP RAYLIB_RELEASE_PATH=$(RAYLIB_OUT)

$(COMPAT)/_string.h:
	@mkdir -p $(COMPAT)
	@echo '#include <string.h>' > $@

check-headers:
	@if grep -nE '#include *<($(BANNED_HEADERS))>' $(SRCS); then \
		echo "ERROR: STL header not allowed in ArduPilot style"; exit 1; fi

run: prog
	./prog

# Always rebuilds from clean so optimised and debug builds never mix
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: clean prog

# Build debug and launch straight into the debugger (lldb on mac, gdb on linux)
lldb gdb: debug
	$(DEBUGGER) ./prog

clean:
	rm -rf prog prog.dSYM $(COMPAT)

.PHONY: check-headers run clean debug lldb gdb
