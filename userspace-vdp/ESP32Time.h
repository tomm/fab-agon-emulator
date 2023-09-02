#pragma once

struct ESP32Time {
	ESP32Time(int);
	int getYear();
	int getMonth();
	int getDay();
	int getDayofYear();
	int getDayofWeek();
	int getHour(bool);
	int getMinute();
	int getSecond();
	void setTime(int, int, int, int, int, int);
};
