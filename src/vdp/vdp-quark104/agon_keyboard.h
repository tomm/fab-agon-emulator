#ifndef AGON_KEYBOARD_H
#define AGON_KEYBOARD_H

#include <fabgl.h>

#include "agon.h"

fabgl::PS2Controller		_PS2Controller;		// The keyboard class

uint8_t			_keycode = 0;					// Last pressed key code
uint8_t			_modifiers = 0;					// Last pressed key modifiers
uint16_t		kbRepeatDelay = 500;			// Keyboard repeat delay ms (250, 500, 750 or 1000)		
uint16_t		kbRepeatRate = 100;				// Keyboard repeat rate ms (between 33 and 500)

// Get keyboard instance
inline fabgl::Keyboard* getKeyboard() {
	return _PS2Controller.keyboard();
}

// Keyboard setup
//
void setupKeyboard() {
	_PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);
	auto kb = getKeyboard();
	kb->setLayout(&fabgl::UKLayout);
	kb->setCodePage(fabgl::CodePages::get(1252));
	kb->setTypematicRateAndDelay(kbRepeatRate, kbRepeatDelay);
}

// Set keyboard layout
//
void setKeyboardLayout(uint8_t region) {
	auto kb = getKeyboard();
	switch(region) {
		case 1:	kb->setLayout(&fabgl::USLayout); break;
		case 2:	kb->setLayout(&fabgl::GermanLayout); break;
		case 3:	kb->setLayout(&fabgl::ItalianLayout); break;
		case 4:	kb->setLayout(&fabgl::SpanishLayout); break;
		case 5:	kb->setLayout(&fabgl::FrenchLayout); break;
		case 6:	kb->setLayout(&fabgl::BelgianLayout); break;
		case 7:	kb->setLayout(&fabgl::NorwegianLayout); break;
		case 8:	kb->setLayout(&fabgl::JapaneseLayout);break;
		default:
			kb->setLayout(&fabgl::UKLayout);
			break;
	}
}

// Get keyboard key presses
// returns true only if there's a new keypress update
//
bool getKeyboardKey(uint8_t *keycode, uint8_t *modifiers, uint8_t *vk, uint8_t *down) {
	auto kb = getKeyboard();
	fabgl::VirtualKeyItem item;

	#if SERIALKB == 1
	if (DBGSerial.available()) {
		_keycode = DBGSerial.read();
		*keycode = _keycode;
		*modifiers = 0;
		*vk = 0;
		*down = 0;
		return true;
	}
	#endif
	
	if (kb->getNextVirtualKey(&item, 0)) {
		if (item.down) {
			switch (item.vk) {
				case fabgl::VK_LEFT:
					_keycode = 0x08;
					break;
				case fabgl::VK_TAB:
					_keycode = 0x09;
					break;
				case fabgl::VK_RIGHT:
					_keycode = 0x15;
					break;
				case fabgl::VK_DOWN:
					_keycode = 0x0A;
					break;
				case fabgl::VK_UP:
					_keycode = 0x0B;
					break;
				case fabgl::VK_BACKSPACE:
					_keycode = 0x7F;
					break;
				default:
					_keycode = item.ASCII;	
					break;
			}
			// Pack the modifiers into a byte
			//
			_modifiers = 
				item.CTRL		<< 0 |
				item.SHIFT		<< 1 |
				item.LALT		<< 2 |
				item.RALT		<< 3 |
				item.CAPSLOCK	<< 4 |
				item.NUMLOCK	<< 5 |
				item.SCROLLLOCK << 6 |
				item.GUI		<< 7
			;
		}
		*keycode = _keycode;
		*modifiers = _modifiers;
		*vk = item.vk;
		*down = item.down;

		return true;
	}

	return false;
}

// Simpler keyboard read for CP/M Terminal Mode
//
bool getKeyboardKey(uint8_t *ascii) {
	auto kb = getKeyboard();
	fabgl::VirtualKeyItem item;

	// Read the keyboard and transmit to the Z80
	//
	if (kb->getNextVirtualKey(&item, 0)) {
		if (item.down) {
			*ascii = item.ASCII;
			return true;
		}
	}

	return false;
}

// Wait for shift key to be released, then pressed (used for paged mode)
// 
bool wait_shiftkey(uint8_t *ascii, uint8_t* vk, uint8_t* down) {
	auto kb = getKeyboard();
	fabgl::VirtualKeyItem item;

	// Wait for shift to be released
	//
	do {
		kb->getNextVirtualKey(&item, 0);
	} while (item.SHIFT);

	// And pressed again
	//
	do {
		kb->getNextVirtualKey(&item, 0);
		*ascii = item.ASCII;
		*vk = item.vk;
		*down = item.down;
		if (item.ASCII == 27) {	// Check for ESC
			return false;
		}
	} while (!item.SHIFT);

	return true;
}

void getKeyboardState(uint16_t *repeatDelay, uint16_t *repeatRate, uint8_t *ledState) {
	bool numLock;
	bool capsLock;
	bool scrollLock;
	getKeyboard()->getLEDs(&numLock, &capsLock, &scrollLock);
	*ledState = scrollLock | (capsLock << 1) | (numLock << 2);
    *repeatDelay = kbRepeatDelay;
    *repeatRate = kbRepeatRate;
}

void setKeyboardState(uint16_t delay, uint16_t rate, uint8_t ledState) {
	auto kb = getKeyboard();
	if (delay >= 250 && delay <= 1000) kbRepeatDelay = (delay / 250) * 250;	// In steps of 250ms
	if (rate >=  33 && rate <=  500) kbRepeatRate  = rate;
	if (ledState != 255) {
		kb->setLEDs(ledState & 4, ledState & 2, ledState & 1);
	}
    kb->setTypematicRateAndDelay(kbRepeatRate, kbRepeatDelay);
}

#endif // AGON_KEYBOARD_H
