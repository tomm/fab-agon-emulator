# This makefile is for a pure c++ vdp build, for testing purposes.
# Use cargo build for the real thing
CXX = g++
LIBS = 
CFLAGS = -fmax-errors=1 -g -I. -I./dispdrivers
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
       test_main.cpp \
       vga16controller.cpp \
       vga2controller.cpp \
       vga4controller.cpp \
       vga8controller.cpp \
       vgabasecontroller.cpp \
       vgacontroller.cpp \
       vgapalettedcontroller.cpp \
       video.cpp \

OBJS = $(SRCS:.cpp=.o)

.cpp.o:
	$(CXX) $(CFLAGS) $(OPTIONS) -c $<

userspace-vdp: $(OBJS)
	$(CXX) -o userspace-vdp $(OBJS) $(LIBS)

clean:
	-rm *.o
	-rm userspace-vdp

depends: $(SRCS)
	$(CXX) -MM $(CFLAGS) $(SRCS) > Makefile.dep

format:
	clang-format -i vgabasecontroller.cpp fabgl.h

ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
