#include "Arduino.h"

// VDP uses various functions before their definition, so here
// vdp-console8.h gives those functions prototypes to avoid errors
#include "vdp-quark.h"

// Define soundGeneratorMutex, as this vdp doesn't actually have one
std::mutex soundGeneratorMutex;

// VDP version-agnostic getters :)
fabgl::SoundGenerator *getVDPSoundGenerator() {
	return &SoundGenerator;
}

#include "vdp-quark/video/video.ino"
