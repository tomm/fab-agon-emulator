#include <stdio.h>
#include "fabgl.h"
#include "devdrivers/soundgen.h"

extern bool vdp_debug_logging; /* in rust_glue.cpp */

static inline void debug_log(const char *format, ...)
{
	if (vdp_debug_logging) {
		va_list ap;
		va_start(ap, format);
		auto size = vsnprintf(nullptr, 0, format, ap) + 1;
		if (size > 0) {
			va_end(ap);
			va_start(ap, format);
			char buf[size + 1];
			vsnprintf(buf, size, format, ap);
			fputs("ESP32 DBGSerial: ", stdout);
			fputs(buf, stdout);
		}
		va_end(ap);
	}
}

extern void setup();
extern void loop();
extern fabgl::VGABaseController *getVDPVGAController();
extern fabgl::SoundGenerator *getVDPSoundGenerator();
extern std::mutex soundGeneratorMutex;
