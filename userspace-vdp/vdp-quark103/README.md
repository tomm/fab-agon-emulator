# agon-vdp

Part of the official Quark firmware for the Agon series of microcomputers

### What is the Agon

Agon is a modern, fully open-source, 8-bit microcomputer and microcontroller in one small, low-cost board. As a computer, it is a standalone device that requires no host PC: it puts out its own video (VGA), audio (2 identical mono channels), accepts a PS/2 keyboard and has its own mass-storage in the form of a µSD card.

https://www.thebyteattic.com/p/agon.html

### What is the VDP

The VDP is a serial graphics terminal that takes a BBC Basic text output stream as input. The output is via the VGA connector on the Agon.

It will process any valid BBC Basic VDU commands (starting with a character between 0 and 31).

For example:

`VDU 25, mode, x; y;` is the same as `PLOT mode, x, y` 

### Documentation

The AGON documentation can now be found on the [Agon Light Documentation Wiki](https://github.com/breakintoprogram/agon-docs/wiki)

### Building

The ESP32 is programmed via the USB connector at the top of the board using the Arduino IDE. It has been tested on version 1.8.19 and the latest 2.0.4.

#### Arduino IDE settings

In order to add the ESP32 as a supported board in the Arudino IDE, you will need to add in this URL into the board manager:

Select Preferences from the File menu

In the Additional Board Manager URLs text box, enter the following URL:

`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

In the Board Manager (from the Tools menu), select the esp32 board, making sure version 2.0.4 is installed.

Now the board can be selected and configured:

* Board: “ESP32 Dev Module”
* Upload Speed: “921600”
* CPU Frequency: “240Mhz (WiFi/BT)”
* Flash Frequency: “80Mhz”
* Flash Mode: “QIO”
* Flash Size: “4MB (32Mb)”
* Partition Scheme: “Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)”
* Core Debug Level: “None”
* PSRAM: “Enabled”

Bernado writes:

> Although I am using 4MB for the PSRAM coupled to the ESP32 (the FabGL assumption), that memory actually has 8MB, so if you find a way to use the extra 4MB, please change the configuration to 8MB.

And for the Port, you will need to determine the Com port that the Agon Light is assigned from your OS after it is connected.

Now the third party libraries will need to be installed from the Library Manager in the Tools menu

* FabGL version 1.0.8
* ESP32Time version 2.0.0

It is important you use these exact versions otherwise I cannot guarantee the code will compile or run correctly.

NB:

- If you are using version 2.0.x of the IDE and get the following message during the upload stage: `ModuleNotFoundError: No module named 'serial'` then you will need to install the python3-serial package.
- If you are using an Apple Mac with an M chipset and are having difficulties uploading to the Agon, try changing the upload speed from 921600 to 115200 - some users have reported that works.