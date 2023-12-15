CXX ?= g++
CFLAGS = -Wall -O2 -fmax-errors=1 -std=c++11 $(EXTRA_FLAGS) -DUSERSPACE -fPIC -fno-exceptions -g -I. -I./dispdrivers -I./userspace-vdp-gl/src -I./userspace-vdp-gl/src/userspace-platform -I./userspace-vdp-gl/src/dispdrivers
OPTIONS = 

SRCS = rust_glue.cpp

OBJS = $(SRCS:.cpp=.o)

.cpp.o:
	$(CXX) $(CFLAGS) $(OPTIONS) -c $< -o $@

all: userspace-vdp-gl/vdp_gl.a vdp_console8.so vdp_quark103.so vdp_quark.so test_main

userspace-vdp-gl/vdp_gl.a:
	$(MAKE) -C userspace-vdp-gl/src

test_main: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp-1.03.o test_main.o
	$(CXX) $(OBJS) -pthread test_main.o vdp-1.03.o userspace-vdp-gl/src/vdp-gl.a -o test_main

vdp_console8.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp-console8.o
	$(CXX) -shared $(OBJS) vdp-console8.o userspace-vdp-gl/src/vdp-gl.a -o vdp_console8.so

vdp_quark103.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp-1.03.o
	$(CXX) -shared $(OBJS) vdp-1.03.o userspace-vdp-gl/src/vdp-gl.a -o vdp_quark103.so

vdp_quark.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp-quark.o
	$(CXX) -shared $(OBJS) vdp-quark.o userspace-vdp-gl/src/vdp-gl.a -o vdp_quark.so

clean:
	-rm *.o
	-rm *.so
	$(MAKE) -C userspace-vdp-gl/src clean

SRC_ALL = $(SRCS) vdp-1.03.cpp vdp-console8.cpp vdp-quark.cpp

depends: $(SRC_ALL)
	$(CXX) -MM $(CFLAGS) $(SRC_ALL) > Makefile.dep

ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
