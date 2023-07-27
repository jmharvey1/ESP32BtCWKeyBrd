/*
 * main.h
 *
 *  Created on: Feb 19, 2023
 *      Author: jim
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_
#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
//#include "driver/timer.h"
#include <driver/gptimer.h>//JMH added for Windows 10 version
//#include "gptimer.h"//JMH removed for Windows 10 version
#include "esp_system.h"
#include "esp32-hal-delay.h" //included to support the arduino function 'delay(int ms)'
struct DF_t
{
	char MyCall[10];
	char MemF2[80];
	char MemF3[80];
	char MemF4[80];
	char MemF5[80];
	int WPM;
	int DeBug;
	int ModeCnt;
	int TRGT_FREQ;
	bool AutoTune;
	float Grtzl_Gain;
};

// typedef struct {
//     timer_group_t timer_group;
//     timer_idx_t timer_idx;
//     int alarm_interval;
//     bool auto_reload;
// } KB_timer_info_t;
/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
// typedef struct {
//     KB_timer_info_t info;
//     uint64_t timer_counter_value;
// } KB_timer_event_t;

//static xQueueHandle s_timer_queue;
extern int DeBug; // factory default setting; 0 => Debug "OFF"; 1 => Debug "ON"
extern SemaphoreHandle_t mutex;//JMH Added
extern char StrdTxt[20];
extern char MyCall[10];
extern char MemF2[80];
extern bool clrbuf;
//extern bool trapFlg;
extern COREDUMP_DRAM_ATTR uint8_t global_var;
uint8_t Read_NVS_Str(const char *key, char *value);
template <class T>
uint8_t Read_NVS_Val(const char *key, T &value);
uint8_t Write_NVS_Str(const char *key, char *value);
uint8_t Write_NVS_Val(const char *key,  int value);


#endif /* INC_MAIN_H_ */