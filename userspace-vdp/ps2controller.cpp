#include "ps2controller.h"
#include "keyboard.h"

// stub

namespace fabgl {

PS2Controller::PS2Controller() {
	s_keyboard = new Keyboard();
}
PS2Controller::~PS2Controller() {}
Keyboard *PS2Controller::s_keyboard;
bool PS2Controller::s_initDone = false;

void PS2Controller::begin(PS2Preset preset, KbdMode keyboardMode) {
	s_initDone = true;
}

}
