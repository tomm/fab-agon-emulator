#include "ESP32Time.h"

ESP32Time::ESP32Time(int) {}

int ESP32Time::getYear() { return 2020; }
int ESP32Time::getMonth() { return 0; }
int ESP32Time::getDay() { return 0; }
int ESP32Time::getDayofWeek() { return 0; }
int ESP32Time::getDayofYear() { return 0; }
int ESP32Time::getHour(bool) { return 0; }
int ESP32Time::getMinute() { return 0; }
int ESP32Time::getSecond() { return 0; }
void ESP32Time::setTime(int, int, int, int, int, int) {}
