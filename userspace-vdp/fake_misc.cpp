#include "fake_fabgl.h"
#include <math.h>
#include <chrono>
#include <thread>

void *heap_caps_malloc(size_t sz, int) {
	return malloc(sz);
}
void *heap_caps_realloc(void *ptr, size_t sz, int) {
	return realloc(ptr, sz);
}
void *heap_caps_free(void *ptr) {
	free(ptr);
	return ptr;
}
size_t heap_caps_get_largest_free_block(int sz) {
	return sz;
}
int heap_caps_get_free_size(int) {
	return 1024*1024;
}

int esp_timer_get_time() {
	return 0;
}

/* Arduino.h */
void delay(int ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned long millis() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int sqrt(int x) {
	return (int)sqrtf((float)x);
}

void digitalWrite(int, int) {}

/* FreeRTOS */
int xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters, int uxPriority, TaskHandle_t *const pvCreatedTask, const int xCoreID)
{
	return xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask);
}

int xTaskCreate(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters, int uxPriority, TaskHandle_t *const pvCreatedTask)
{
	//printf("Spawning thread %s...\n", pcName);
	auto t = std::thread(pvTaskCode, pvParameters);
	t.detach();

	// XXX A horrible workaround. pvParameters can be a pointer to
	// local vars in the caller, which will be GONE by the time std::thread starts.
	// So wait around a bit in the hope that the thread gets going
	// before we wipe its arguments... :wince:
	delay(1);

	return 0;
}

void vTaskDelay(int n)
{
	// n isn't ms, but whatever
	delay(n);
}

bool is_fabgl_terminating = false;
