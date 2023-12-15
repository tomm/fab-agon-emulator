#include "Arduino.h"

// VDP uses various functions before their definition, so here
// vdp-console8.h gives those functions prototypes to avoid errors
#include "vdp-1.03.h"

// Define soundGeneratorMutex, as this vdp doesn't actually have one
std::mutex soundGeneratorMutex;

fabgl::SoundGenerator *getVDPSoundGenerator() {
	return &SoundGenerator;
}

#include "vdp-quark103/video.ino"
