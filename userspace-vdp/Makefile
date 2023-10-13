# This makefile is for a pure c++ vdp build, for testing purposes.
# Use cargo build for the real thing
CXX ?= g++
CFLAGS = -Wall -O2 -fmax-errors=1 -std=gnu++11 -g -I. -I./dispdrivers
OPTIONS = 

SRCS = \
       canvas.cpp \
       codepages.cpp \
       collisiondetector.cpp \
       displaycontroller.cpp \
       esp32time.cpp \
       fabfonts.cpp \
       fabutils.cpp \
       fake_misc.cpp \
       HardwareSerial.cpp \
       kbdlayouts.cpp \
       keyboard.cpp \
       ps2controller.cpp \
       soundgen.cpp \
       terminal.cpp \
       terminfo.cpp \
       rust_glue.cpp \
       vga16controller.cpp \
       vga2controller.cpp \
       vga4controller.cpp \
       vga8controller.cpp \
       vgabasecontroller.cpp \
       vgacontroller.cpp \
       vgapalettedcontroller.cpp \

OBJS = $(SRCS:.cpp=.o)

.cpp.o:
	$(CXX) $(CFLAGS) $(OPTIONS) -fPIC -c $<

all: vdp_console8.so vdp_quark103.so

vdp_console8.so: $(OBJS) vdp-console8.o
	$(CXX) -shared $(OBJS) vdp-console8.o -o vdp_console8.so

vdp_quark103.so: $(OBJS) vdp-1.03.o
	$(CXX) -shared $(OBJS) vdp-1.03.o -o vdp_quark103.so

clean:
	-rm *.o
	-rm vdp_console8.so
	-rm vdp_quark103.so

SRC_VDP = vdp-1.03.cpp vdp-console8.cpp

depends: $(SRCS) $(SRC_VDP)
	$(CXX) -MM $(CFLAGS) $(SRCS) $(SRC_VDP) > Makefile.dep

format:
	clang-format -i rust_glue.cpp

ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
