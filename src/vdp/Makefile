CXX ?= g++
CFLAGS = -Wall -O2 -fmax-errors=1 -std=c++11 $(EXTRA_FLAGS) -DUSERSPACE -fPIC -g -I. -I./dispdrivers -I./userspace-vdp-gl/src -I./userspace-vdp-gl/src/userspace-platform -I./userspace-vdp-gl/src/dispdrivers
OPTIONS = 

SRCS = rust_glue.cpp

OBJS = $(SRCS:.cpp=$(SUFFIX).o)

%$(SUFFIX).o: %.cpp
	$(CXX) $(CFLAGS) $(OPTIONS) -c $< -o $@

all: userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp_console8$(SUFFIX).so vdp_quark103$(SUFFIX).so \
	vdp_quark$(SUFFIX).so directcontroller_test$(SUFFIX).so vdp_electronhal$(SUFFIX).so

userspace-vdp-gl/vdp_gl$(SUFFIX).a:
	$(MAKE) -C userspace-vdp-gl/src

test_main$(SUFFIX): $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp-1.03$(SUFFIX).o test_main$(SUFFIX).o
	$(CXX) $(CFLAGS) $(OBJS) -pthread test_main$(SUFFIX).o vdp-1.03$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o test_main$(SUFFIX)

directcontroller_test$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a directcontroller_test$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) directcontroller_test$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o directcontroller_test$(SUFFIX).so

vdp_console8$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp-console8$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) vdp-console8$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_console8$(SUFFIX).so

vdp_quark103$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp-1.03$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) vdp-1.03$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_quark103$(SUFFIX).so

vdp_quark$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp-quark$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) vdp-quark$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_quark$(SUFFIX).so

vdp_electronhal$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp_electronhal$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) vdp_electronhal$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_electronhal$(SUFFIX).so

clean:
	-rm *.o
	-rm *.so
	$(MAKE) -C userspace-vdp-gl/src clean

SRC_ALL = $(SRCS) vdp-1.03.cpp vdp-console8.cpp vdp-quark.cpp directcontroller_test.cpp vdp_electronhal.cpp

depends: $(SRC_ALL)
	$(CXX) -MM $(CFLAGS) $(SRC_ALL) > Makefile.dep

ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
