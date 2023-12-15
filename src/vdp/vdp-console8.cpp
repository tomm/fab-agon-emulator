#include "Arduino.h"

// VDP uses various functions before their definition, so here
// vdp-console8.h gives those functions prototypes to avoid errors
#include "vdp-console8.h"

// VDP version-agnostic getters :)
fabgl::SoundGenerator *getVDPSoundGenerator() {
	return &*soundGenerator;
}

#include "vdp-console8/video/video.ino"
