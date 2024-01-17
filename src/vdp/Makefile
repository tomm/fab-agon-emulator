CXX ?= g++
CFLAGS = -Wall -fmax-errors=1 -std=c++11 -g -I. -I./dispdrivers -I./userspace-vdp-gl/src -I./userspace-vdp-gl/src/userspace-platform -I./userspace-vdp-gl/src/dispdrivers -Wno-narrowing -DUSERSPACE
OPTIONS = 

SRCS = rust_glue.cpp

OBJS = $(SRCS:.cpp=.o)

.cpp.o:
	$(CXX) $(CFLAGS) $(OPTIONS) -fPIC -c $< -o $@

all: userspace-vdp-gl/vdp_gl.a vdp_console8.so vdp_quark103.so vdp_quark104.so vdp_electronhal.so test_main

userspace-vdp-gl/vdp_gl.a:
	$(MAKE) -C userspace-vdp-gl/src

test_main: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp_quark104.o test_main.o
	$(CXX) $(OBJS) test_main.o vdp_quark104.o userspace-vdp-gl/src/vdp-gl.a -o test_main

vdp_console8.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp-console8.o
	$(CXX) -shared $(OBJS) vdp-console8.o userspace-vdp-gl/src/vdp-gl.a -o vdp_console8.so

vdp_quark103.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp-1.03.o
	$(CXX) -shared $(OBJS) vdp-1.03.o userspace-vdp-gl/src/vdp-gl.a -o vdp_quark103.so

vdp_quark104.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp_quark104.o
	$(CXX) -shared $(OBJS) vdp_quark104.o userspace-vdp-gl/src/vdp-gl.a -o vdp_quark104.so

vdp_electronhal.so: $(OBJS) userspace-vdp-gl/vdp_gl.a vdp_electronhal.o
	$(CXX) -shared $(OBJS) vdp_electronhal.o userspace-vdp-gl/src/vdp-gl.a -o vdp_electronhal.so

clean:
	-rm *.o
	-rm *.so
	$(MAKE) -C userspace-vdp-gl/src clean

SRC_ALL = $(SRCS) vdp-1.03.cpp vdp-console8.cpp vdp_quark104.cpp vdp_electronhal.cpp

depends: $(SRC_ALL)
	$(CXX) -MM $(CFLAGS) $(SRC_ALL) > Makefile.dep

ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
