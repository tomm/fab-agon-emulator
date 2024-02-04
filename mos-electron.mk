# take the output of ZDS and copy and prepare bin
ZDS_DIR = ../../software/AgonElectronOS/Release
FIRMWARE_DIR = firmware
HEX2BIN = hex2bin

all:
	cp $(ZDS_DIR)/mos_electron.map $(FIRMWARE_DIR)/mos_electron.map
	cp $(ZDS_DIR)/mos_electron.hex $(FIRMWARE_DIR)/mos_electron.hex
	$(HEX2BIN) $(FIRMWARE_DIR)/mos_electron.hex

clean:
	rm $(FIRMWARE_DIR)/mos_electron.*