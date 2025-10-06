// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//JMH 20230206 created this file for platformio adruino to espidf cross platform support


#ifndef _ESP32_HAL_DELAY_H_
#define _ESP32_HAL_DELAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdkconfig.h"
//#include <cstdint>
#include "stdint-gcc.h" //JMH 20230211 used this inplace of cstdint to avoid file not found error in this environment

//#ifndef BOARD_HAS_PSRAM
//#ifdef CONFIG_SPIRAM_SUPPORT
//#undef CONFIG_SPIRAM_SUPPORT
//#endif
//#ifdef CONFIG_SPIRAM
//#undef CONFIG_SPIRAM
//#endif
//#endif
void __attribute__ ((weak)) yield(void);
unsigned long micros();
unsigned long millis();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

#ifdef __cplusplus
}
#endif

#endif /* _ESP32_HAL_MISC_H_ */
