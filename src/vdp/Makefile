CXX ?= g++
CFLAGS = -Wall -Werror=return-type -O2 -fmax-errors=1 -std=c++11 $(EXTRA_FLAGS) -DUSERSPACE -fPIC -g -I.. -I. -I./dispdrivers -I./userspace-vdp-gl/src -I./userspace-vdp-gl/src/userspace-platform -I./userspace-vdp-gl/src/dispdrivers -I./userspace-vdp-gl/src/userspace-platform/matrix
OPTIONS = 

ELECTRONHAL_CFLAGS = -I./AgonElectronHAL/include 
CONSOLE8_CFLAGS = -I./vdp-console8/video

SRCS = rust_glue.cpp

OBJS = $(SRCS:.cpp=$(SUFFIX).o)

%-console8$(SUFFIX).o: %-console8.cpp
	$(CXX) $(CFLAGS) $(CONSOLE8_CFLAGS) $(OPTIONS) -c $< -o $@

%_electron$(SUFFIX).o: vdp_electron.cpp
	$(CXX) $(CFLAGS) $(ELECTRONHAL_CFLAGS) $(OPTIONS) -c $< -o $@

%$(SUFFIX).o: %.cpp
	$(CXX) $(CFLAGS) $(OPTIONS) -c $< -o $@

all: userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp_console8$(SUFFIX).so \
	vdp_quark$(SUFFIX).so directcontroller_test$(SUFFIX).so vdp_electron$(SUFFIX).so

userspace-vdp-gl/vdp_gl$(SUFFIX).a:
	$(MAKE) -C userspace-vdp-gl/src

test_main$(SUFFIX): $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a test_main$(SUFFIX).o
	$(CXX) $(CFLAGS) $(OBJS) -pthread test_main$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o test_main$(SUFFIX)

directcontroller_test$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a directcontroller_test$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) directcontroller_test$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o directcontroller_test$(SUFFIX).so

vdp_console8$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp-console8$(SUFFIX).o
	$(CXX) $(CFLAGS) $(CONSOLE8_CFLAGS) -shared $(OBJS) vdp-console8$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_console8$(SUFFIX).so

vdp_quark$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp-quark$(SUFFIX).o
	$(CXX) $(CFLAGS) -shared $(OBJS) vdp-quark$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_quark$(SUFFIX).so

vdp_electron$(SUFFIX).so: $(OBJS) userspace-vdp-gl/vdp_gl$(SUFFIX).a vdp_electron$(SUFFIX).o
	$(CXX) $(CFLAGS) $(ELECTRONHAL_CFLAGS) -shared $(OBJS) vdp_electron$(SUFFIX).o userspace-vdp-gl/src/vdp-gl$(SUFFIX).a -o vdp_electron$(SUFFIX).so

clean:
	-rm *.o
	-rm *.so
	$(MAKE) -C userspace-vdp-gl/src clean

SRC_ALL = $(SRCS) vdp-console8.cpp vdp-quark.cpp directcontroller_test.cpp vdp_electron.cpp

depends: $(SRC_ALL)
	$(CXX) -MM $(CFLAGS) $(CONSOLE8_CFLAGS) $(ELECTRONHAL_CFLAGS) $(SRC_ALL) > Makefile.dep

lipo:
	$(foreach file, $(wildcard *.x86_64.so), lipo -create -output $(notdir $(file:.x86_64.so=.so)) $(file) $(notdir $(file:.x86_64.so=.arm64.so));)


ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
