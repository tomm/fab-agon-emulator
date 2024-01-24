#include "updater.h"
#include "hal.h"
#include "esp_ota_ops.h"

vdu_updater::vdu_updater () : 
    locked (true)
{
}
void vdu_updater::unlock ()
{
	uint8_t buffer[sizeof(unlockCode)] = {0};
	size_t bytes_read = ez80_serial.readBytes (buffer, sizeof(buffer)-1);
	if(bytes_read!=(sizeof(buffer)-1)) {
		hal_printf("Read unlock code failed!\n\r");
	}
	else if(memcmp(buffer, unlockCode, sizeof(unlockCode)-1) == 0) 
    {
		hal_printf("Updater %s\n\r",unlock_response);
		locked = false;
	}
}
uint8_t vdu_updater::get_unlock_response (uint8_t index)
{
	return unlock_response [index];
}
void vdu_updater::receiveFirmware()
{
	hal_printf("Initialize VDP firmware update\r\n");
    uint32_t update_size = 0;
	size_t bytes_read = ez80_serial.readBytes((uint8_t*)&update_size, sizeof(update_size) - 1); // -1 because its 24bits
	if(bytes_read!=sizeof(update_size) - 1) 
    {
		hal_printf("Read size failed!\n\r");
		return;
	}

	if(locked) {
		hal_printf("Updater is locked, bytes will be discarded!\n\r");
		return;
	}
	
	hal_printf("Received update size: %u bytes\n\r", update_size);
	hal_printf("Receiving VDP firmware update");

	uint32_t start = millis();

	esp_err_t err;
	esp_ota_handle_t update_handle = 0 ;
	const esp_partition_t *update_partition = NULL;

	update_partition = esp_ota_get_next_update_partition(NULL);
	err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
	if (err != ESP_OK) 
    {
		hal_printf("esp_ota_begin failed, error=%d\n\r", err);
		return;
	}

	uint32_t remaining_bytes = update_size;
	uint8_t code = 0;
	const size_t buffer_size = 1024;
	while(remaining_bytes > 0) 
    {
		size_t bytes_to_read = buffer_size;

		if(remaining_bytes < buffer_size)
			bytes_to_read = remaining_bytes;

		uint8_t buffer[buffer_size];
		bytes_read = ez80_serial.readBytes(buffer, bytes_to_read);
		if(bytes_read!=bytes_to_read) {
			hal_printf("\n\rRead buffer failed at byte %u!\n\r", remaining_bytes);
			return;
		}

		for(int i = 0; i < bytes_to_read; i++) {
			code += ((uint8_t*)buffer)[i];
		}

		hal_printf(".");
		remaining_bytes -= bytes_to_read;

		err = esp_ota_write( update_handle, (const void *)buffer, bytes_to_read);
		if (err != ESP_OK) 
        {
			hal_printf("\n\resp_ota_write failed, error=%d\n\r", err);
			return;
		}
	}
	hal_printf("\n\r");
	
	uint32_t end = millis();
	hal_printf("Upload done in %u ms\n\r", end - start);
	hal_printf("Bandwidth: %u kbit/s\n\r", update_size / (end - start) * 8);
	
	// checksum check
	auto checksum_complement = ez80_serial.read();
	if (checksum_complement == -1) {
		hal_printf("Checksum not received!\n\r");
		return;
	}
	hal_printf("checksum_complement: 0x%x\n\r", checksum_complement);
	if(uint8_t(code + (uint8_t)checksum_complement)) 
    {
		hal_printf("checksum error!\n\r");
		return;
	}
	hal_printf("checksum ok!\n\r");

	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK) 
    {
		hal_printf("esp_ota_set_boot_partition failed! err=0x%x\n\r", err);
		return;
	}

	hal_printf("Rebooting in ");
	for(int i = 3; i > 0; i--) 
    {
		hal_printf("%d...", i);
		delay(1000);
	}
	hal_printf("0!\n\r");
	
	esp_restart();
}
void vdu_updater::switchFirmware()
{
    if (locked) {
		hal_printf("Updater is locked!\n\r");
		return;
	}

	esp_err_t err;
	const esp_partition_t *running = esp_ota_get_running_partition();
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(running);

	// TODO: check if update_partition is valid
	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK)
		hal_printf("esp_ota_set_boot_partition failed! err=0x%x\n\r", err);
	hal_printf("restart!\n\r");
	esp_restart();
}
void vdu_updater::test ()
{
	uint8_t cnt = esp_ota_get_app_partition_count();
	hal_printf ("we have %d partitions\r\n",cnt);
	hal_printf ("boot partition\r\n");
	const esp_partition_t* boot = esp_ota_get_boot_partition();
	hal_printf (" type: %d\r\n",boot->type);
	hal_printf (" subtype: %d\r\n",boot->subtype);
	hal_printf (" size: %d\r\n",boot->size);
	hal_printf (" address: %d\r\n",boot->address);
	hal_printf (" label: %s\r\n",boot->label);
	hal_printf ("running partition\r\n");
	const esp_partition_t* running = esp_ota_get_running_partition();
	hal_printf (" type: %d\r\n",running->type);
	hal_printf (" subtype: %d\r\n",running->subtype);
	hal_printf (" size: %d\r\n",running->size);
	hal_printf (" address: %d\r\n",running->address);
	hal_printf (" label: %s\r\n",running->label);
	hal_printf ("next partition\r\n");
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(running);
	hal_printf (" type: %d\r\n",update_partition->type);
	hal_printf (" subtype: %d\r\n",update_partition->subtype);
	hal_printf (" size: %d\r\n",update_partition->size);
	hal_printf (" address: %d\r\n",update_partition->address);
	hal_printf (" label: %s\r\n",update_partition->label);
	hal_printf ("next next partition\r\n");
	const esp_partition_t *update_partition2 = esp_ota_get_next_update_partition(update_partition);
	hal_printf (" type: %d\r\n",update_partition2->type);
	hal_printf (" subtype: %d\r\n",update_partition2->subtype);
	hal_printf (" size: %d\r\n",update_partition2->size);
	hal_printf (" address: %d\r\n",update_partition2->address);
	hal_printf (" label: %s\r\n",update_partition2->label);
}

void vdu_updater::command (uint8_t mode)
{
    switch(mode) 
    {
		case 0: 
			unlock();
		    break;
		case 1: 
			receiveFirmware();
		    break;
		case 2: 
			switchFirmware();
            break;
		case 3:
			test();
			break;
    }
}
