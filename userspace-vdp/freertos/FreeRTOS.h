#pragma once

#include <climits>
#include <cstdint>

typedef void TaskFunction_t(void *);
typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
typedef void *TimerHandle_t;
typedef void *SemaphoreHandle_t;
//#define xTaskCreatePinnedToCore(a,b,c,d,e,f,g) 0
extern int xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters, int uxPriority, TaskHandle_t *const pvCreatedTask, const int xCoreID);
extern void vTaskDelay(int);
#define vTaskDelete(n)
#define xTaskGetCurrentTaskHandle() 0
#define vTaskResume(n)
#define xSemaphoreCreateMutex() 0
#define xTaskCreate(a,b,c,d,e,f) 0
#define xTimerDelete(a,b)
#define xTimerCreate(a,b,c,d,e) 0
#define xTimerStart(a,b)
#define vQueueDelete(a)
#define vSemaphoreDelete(a)
#define xQueueCreate(a,b) 0
#define xTaskAbortDelay(a)
