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
	return 0;
}

int sqrt(int x) {
	return (int)sqrtf((float)x);
}

void digitalWrite(int, int) {}
