#include "fabgl.h"
#include "devdrivers/soundgen.h"

extern void setup();
extern void loop();
extern fabgl::VGABaseController *getVDPVGAController();
extern fabgl::SoundGenerator *getVDPSoundGenerator();
extern std::mutex soundGeneratorMutex;
