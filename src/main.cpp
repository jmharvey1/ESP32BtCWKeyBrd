/**
 * @file main.cpp
 * @brief ESP32 CW Decoder & Bluetooth Keyboard Main Application
 *
 * This file implements the main application logic for an ESP32-based CW (Morse code) decoder and Bluetooth keyboard interface.
 * It supports both Linux and Windows 10 environments and is designed to work with a TFT display (ILI9341 driver) and various
 * peripherals including ADC, Bluetooth HID keyboards, and FreeRTOS tasks.
 *
 * ## Features:
 * - Bluetooth keyboard pairing, scan, and event handling (supports Logitech K380 and others)
 * - Real-time CW decoding using Goertzel algorithm on ADC audio input
 * - Adjustable WPM (words per minute), auto-tune, and gain settings
 * - TFT display management for decoded text, status, and user interaction
 * - User-configurable memory messages (F2-F5), callsign, and settings stored in NVS
 * - Advanced Morse code parsing and error correction (AdvParser class)
 * - Multi-tasking using FreeRTOS for ADC processing, display updates, CW decoding, and post-processing
 * - Mutex and semaphore protection for shared resources
 * - Support for diagnostic and debug output
 * - Persistent user settings via NVS (Non-Volatile Storage)
 *
 * ## Main Components:
 * - **GoertzelHandler**: Processes ADC samples for tone detection and frequency measurement.
 * - **DisplayUpDt**: Updates the TFT display with decoded messages and status.
 * - **CWDecodeTask**: Main loop for CW decoding and error correction.
 * - **AdvParserTask**: Post-processing and advanced parsing of decoded Morse code.
 * - **ProcsKeyEntry**: Handles keyboard input, including special function keys and message sending.
 * - **NVS Read/Write Functions**: Manage persistent storage of user preferences and settings.
 * - **Timer ISRs**: Handle periodic display refresh and dot clock for outgoing Morse (CW) timing.
 *
 * ## Usage:
 * - On startup, initializes hardware, peripherals, and tasks.
 * - Pairs with a Bluetooth keyboard and waits for user input.
 * - Decodes incoming CW signals from ADC and displays results.
 * - Allows sending of pre-configured or custom messages via keyboard.
 * - User can access settings screen to configure preferences.
 *
 * ## Notable Definitions:
 * - `DF_t DFault`: Structure holding factory/user default settings.
 * - `TFTMsgBox tftmsgbx`: Handles display messaging.
 * - `CWSNDENGN CWsndengn`: Manages CW sending engine.
 * - `BTKeyboard bt_keyboard`: Bluetooth keyboard handler.
 * - `TxtNtryBox txtntrybox`: Text entry box for user input.
 *
 * ## Task Priorities:
 * - Goertzel Task: 5 (highest, for real-time ADC processing)
 * - CW Decode Task: 4
 * - Display Update Task: 3
 * - AdvParser Task: 2
 *
 * ## History:
 * - Extensive changelog documents ongoing improvements, bug fixes, and feature additions from 2020 through 2025.
 *
 * @author Jim (CW Decoder, ESP32 integration, enhancements)
 * @author Guy Turcotte (original BT-Keyboard code)
 * @copyright MIT License
 */
/* BT-Keyboard code (bt_keyboard.cpp) based on source code with the same file name &
   Copyright (c) 2020 by Guy Turcotte
   MIT License. Look at file licenses.txt for details.
*/
/*20230310 added logitech K380 configure function keys command */
/*20230317 added Memory F3 & F4 function keys */
/*20230328 moved to GitHub*/
/*20230405 reconfigued project files to run both on Linux & Windows 10*/
/*20230418 expanded send buffer from 160 to 400 charater (4 line to 10 dispaly lines)*/
/*20230429 Added code to CWSendEngn.cpp set character timing speed to a minimum of 15wpm*/
/*20230430 fixed crash issue related to changing speed while buffered code is being sent.*/
/*20230502 reworked WPM screen refresh & dotclk updated to timing period to avoid TFT display crashes.*/
/*20230503 Added code to bt_keyboard.cpp test for 'del' key presses an convert to hex code 0x2A*/
/*20230507 Reversed bavior of "Up" & "Down" Arrow keys & corrected dotclock startup timing to actually match the User stored WPM setting*/
/*20230607 DcoderCW.cpp CLrDCdValBuf() added other buffers & codevals to reset to stop decode artifacts of what the CWgen code was making during user send */
/*20230608 Enabled AutoTune indecoder default configuration */
/*20230609 Added code to the KEYISR (DcodeCW.cpp) to improve recovery/WPM tracking from slow to fast CW */
/*20230610 Remove '@'from the decoder dictionary */
/*20230615 reworked Goertzel.cpp & DcodeCW.cpp to better track the incoming WPM speed of the received station plus slight improvement in decoding CW @ > 35WPM As well at lower SNRs */
/*20230616 Added decoder mode & autotune to NVS eeprom; reworked startup (app_main) sequence to reduce crashes during 1st time pairing process */
/*20230617 To support advanced DcodeCW "Bug" processes, reworked dispMsg2(void) to handle ASCII chacacter 0x8 "Backspace" symbol*/
/*20230624 Minor tweaks to tone detect code & installed mutexs in bletooth callback to try to better manage SPI resource calls*/
/*20230701  Reworked Goertzel tone detection & added adjustable glitch detection; Also reworked Goertzel sample count code */
/*20230707 Added shortcuts (left/right Ctrl+g) user cntrolled gain setting; and reworked Glitch detection*/
/*20230708 reworked Glitch detection by adding a front end average keydown section to the tone detect routine; it now sets the glitch inteval*/
/*20230711 minor tweaks to concatenate processes (DcodeCW.cpp) to imporve delete character managment */
/*20230715 minor tweaks */
/*20230719 added "noreturn" patch to crash handler code and removed printf() calls originally in IRAM_ATTR DotClk_ISR(void *arg)*/
/*20230721 Moved project to github*/
/*20230727 Fixed Crash issue related to BT Keyboard sending corrupted keystroke data; fix mostly containg bt_keyboard.hid callback code*/
/*20230727 Reworked pairing code & mail code to avoid watchdog timer crashes*/
/*20230728 reworked code to improve pairing of multiple keyboard */
/*20230729 added calls to DcodeCW SetLtrBrk() & chkChrCmplt() to ensure that the letter break gets refreshed with each ADC update (i.e., every 4ms)*/
/*20230730 blocked the gudsig function to imporve noisy signal decoding */
/*20230731 Now launching the chkChrCmplt() strickly from goertzel/Chk4KeyDwn() task/routine*/
/*20230801 Added Short/Long (4ms/8ms) sample interval; User selectable via Ctrl+g */
/*20230804 Minor timing tweaks to  DecodeCW & Goertzel code*/
/*20230807 rewrote sloppy string check code for ESP32; See CcodeCW.cpp */
/*20230810 beta fix for crash linked to 1st time connection to previously 'paired keyboard. Requires stopping ADC sampling during the 'open' Keyboard event*/
/*20230811 beta2 fix for crash linked to 1st time connection to previously 'paired keyboard. Requires stopping ADC sampling during the 'open' Keyboard event*/
/*20230812 fix crash when using backspace key to delete unsent text */
/*20230814 changed ltrbrk timing for slow speed Bg2 mode */
/*20230815 revised Bg2 timing & rewote ltrbreak debugging output code */
/*20230818 fixed pairing crash linked to display SPI conflcit during BT pairing event */
/*20230916 Changed Auto-tune method; & reworked tone level detection; Added 3rd gain mode/setting */
/*20230918 Refined Auto-tune code; added autogain setting shift from slow to fast; added StrechLtrcmplt() to DcodeCW.cpp to better manage letterbreak detection */
/*20231103 reworked  DcodeCW.cpp, dit/dah decision tests & in Goertzel reworked auotsample rate detection code */
/*20231215 added DblChkDitDah() to DcodeCW.cpp, to further improve dit/dah decision tests */
/*20231217 reworked file references to match framework-espidf @ 3.50100.0 (5.1.0) */
/*20231221 minor tweek to DblChkDitDah (DeCodeCW.cpp) routine to improve resolving dits from dahs*/
/*20231229 More tweaks to bug2 letter break code (DeCodeCW.cpp)*/
/*20240101 More tweaks to bug2 letter break code (DeCodeCW.cpp)*/
/*20240103 Modified extented sysmbolset timing to only effect Bg1 mode*/
/*20240110 Added Advparser Class */
/*20240112 Continued development of Advparser Class */
/*20240115 Continued development/enhancement of Advparser Class */
/*20240117 added Dcode4Dahs() to AdvParser class; parses 4 dahs into "TO" or "OT", "MM" also a possible result */
/*20240119 added test for paddle/keybrd by finding all dahs have the same interval;*/
/*20240120 reworked DitDahBugTst() method to better detect bug vs paddle/keyboard signals plus minor tweaks to bug rule set*/
/*20240122 revised AdvParser.cpp DitDahSplitVal averaging algorithm to be based on last 30 symbol set elements*/
/*20240123 AdvParser.cpp - Added letterbrk bug test "2", to look for lttrbk based on "long" dah*/
/*20240124 DcodeCW.cpp added requirement that Key up & down arrays match in length before attempting to do a post reparse of the last word captured*/
/*20240129 AdvParser.cpp - improved method for calculating DitIntrvlVal + other tweaks to bug parsing rules*/
/*20240201 AdvParser.cpp - Added Straight Key Rule Set & Detection; Revised Bug1 RS dit & dah runs parsing*/
/*20240202 AdvParser.cpp - Added Bg1SplitPt & refined Bug1 "Dah run" code*/
/*20240203 Added initialization values to AdvParser to imporve 1st decode/parsing results*/
/*20240204 added GetState method to CWSndEngn class; primarily to notify decoder/Goertzl side to go into 'standby/sleep' mode*/
/*20240205 added WrdBrkVal property Advparser Class + changes to Bug1 & DitDahBugTst*/
/*20240206 added StrchdDah property to Advparser Class*/
/*20240207 ammended 'Advparser.AdvSrch4Match' method for setting up 'follow on' search*/
/*20240208 AdvParser.cpp, reworked 'LstLtrBrkCnt' management to better track the number of keyevents since the last letterbreak event */
/*20240210 created new class method 'Advparser.SrchAgn()'*/
/*20240211 Made chnges to AdParser.cpp, mainly to align dit/dah decision points with the ruleset in play */
/*20240215 AdParser.cpp - More minor tweaks to Bug1 Rule set & DitDahBugTst()*/
/*20240216 AdParser.cpp - Added 'FixClassicErrors()' method*/
/*20240220 AdParser.cpp - added 'SloppyBgRules()' method. Plus numerous changes to integrate this rule set into the existing code */
/*20240223 numerous 'tweaks' to AdParser.cpp, mostly to better delineate between different key types*/
/*20240225 AdParser.cpp - more 'tweaks' to bug2 & sloppybug rule sets */
/*20240225 setup post parsing to run as a task (AdvParserTask)*/
/*20240227 reworked AdvParserTask() & setup to run on core 1; reworked AdvParser.cpp - GetMsgLen(void), FixClassicErrors(void), & SloppyBgRules(int& n)*/
/*20240301 AdParser.cpp -Changed BG2 dah run to detect letterbreak on UnitIntvrlx2r5; chages to SetSpltPt()*/
/*20240304 Reworked deployment of mutex to reduce code crashes linked to keyboard entries*/
/*20240307 AdParser.cpp Added SrchEsReplace() function & and simplified the code in FixClassicErrors()*/
/*20240309 AdParser.cpp - Moved straight key test ahead of sloppy test*/
/*20240309 AdParser.cpp - Mostly tweaks in FixClassicErrors() code */
/*20240313 AdParser.cpp - rewrote FixClassicErrors() to sequence through SrchRplcDict[] array to test for mangled character strings */
/*20240315 AdvParser.cpp - rewrote FixClassicErrors() to use a 'for' loop to sequence through the SrchRplcDict[]*/
/*20240317 changed task management mainly through priority level assignments; reworked AdParser.SetSpltPt() method to better handle cootie type keying*/
/*20240322 expanded SrchRplcDict[] to 192 entries */
/*20240323 expanded SrchRplcDict[] to 212 entries*/
/*20240330 expanded SrchRplcDict[] to 289 entries*/
/*20240405 expanded SrchRplcDict[] to 329 entries*/
/*20240408 expanded SrchRplcDict[] to 346 entries*/
/*20240411 expanded SrchRplcDict[] to 400 entries*/
/*20240414 expanded SrchRplcDict[] to 413 entries*/
/*20240420 expanded SrchRplcDict[] to 472 entries*/
/*20240427 expanded SrchRplcDict[] to 526 entries*/
/*20240502 added entries 527 - 586 to  SrchRplcDict[] (superslopy) */
/*20240504 expanded SrchRplcDict[] to 634 entries*/
/*20240518 expanded SrchRplcDict[] to 682 entries*/
/*20240519 AdvParser.cp Added glitch detection*/
/*20240522 Goertzel.cpp, added auto glitch detection,to support post parser (AdvParser.cpp)*/
/*20240601 Minor tweaks to AdvParser.cpp to improve rule set selection & adds SrchRplcDict[]*/
/*20240608 Expanded SrchRplcDict[] to 731 entries*/
/*20250930 restructed to work with framework-espidf@~3.50202.0 */
/*20251006 bt_keyboard.cpp reworked keyboard pairing & reconnect */

#include "sdkconfig.h" //added for timer support
#include "globals.h"
#include "main.h"
#include "bt_keyboard.hpp"
#include <esp_log.h>
#include <iostream>
//#include "esp32-hal-spi.h"

#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
///////////////////////////////
//#include "esp_core_dump.h"
//////////////////////////////
#include "TFT_eSPI.h" // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include <Stream.h>
#include "SetUpScrn.h" //added to support references to the setttings screen; used to configure CW keyboard user perferences
#include "TxtNtryBox.h"
/*helper ADC files*/
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "Goertzel.h"
#include "DcodeCW.h"
/**blackpill tftmsgbox spport*/
#include "TFTMsgBox.h"
#include "CWSndEngn.h"
#include "esp_hid_host_exmpl.h"
/* the following defines were taken from "hal/adc_hal.h" 
 and are here for help in finding the true ADC sample rate based on a given declared sample frequency*/
#define ADC_LL_CLKM_DIV_NUM_DEFAULT       15
#define ADC_LL_CLKM_DIV_B_DEFAULT         1
#define ADC_LL_CLKM_DIV_A_DEFAULT         0
#define ADC_LL_DEFAULT_CONV_LIMIT_EN  6    0


#define TFT_GREY 0x5AEB // New colour

TFT_eSPI tft = TFT_eSPI(); // Invoke TFT Display library
//////////////////////////////
/*timer interrupt support*/
#include "esp_timer.h"
esp_timer_handle_t DotClk_hndl;
// esp_timer_handle_t DsplTmr_hndl;
TimerHandle_t DisplayTmr;
/* END timer interrupt support          */

/*Debug & backtrace analysis*/
uint8_t global_var;
/*To make these variables available to other files/section of this App, Moved the following to main.h*/

DF_t DFault;
int DeBug = 0; // Debug factory default setting; 0 => Debug "OFF"; 1 => Debug "ON"
char StrdTxt[20] = {'\0'};
/*Factory Default Settings*/
char RevDate[9] = "20250930";
char MyCall[10] = "KW4KD";
char MemF2[80] = "VVV VVV TEST DE KW4KD";
char MemF3[80] = "CQ CQ CQ DE KW4KD KW4KD";
char MemF4[80] = "TU 73 ES GL";
char MemF5[80] = "RST 5NN";
esp_err_t ret;
char Title[120];
bool setupFlg = false;
//bool EnDsplInt = true;
bool clrbuf = false;
bool PlotFlg = false;
bool UrTurn = true;
bool WokeFlg = false;
bool QuequeFulFlg = false;
bool mutexFLG =false;
bool adcON =false;
uint8_t QueFullstate;
//bool PairFlg = false;
//bool trapFlg = false;
volatile int DotClkstate = 0;
volatile int CurHiWtrMrk = 0;
static const uint8_t state_que_len = 100;//50;
static QueueHandle_t state_que;
//static const uint8_t Sampl_que_len = 6 * Goertzel_SAMPLE_CNT;
//static QueueHandle_t Sampl_que;

SemaphoreHandle_t mutex;

/* the following 2 entries are for diagnostic capture of raw DMA ADC data*/
// int Smpl_buf[6 * Goertzel_SAMPLE_CNT];
// int Smpl_Cntr = 0;

int MutexbsyCnt = 0;
unsigned long SmpIntrl = 0;
unsigned long LstNw = 0;
unsigned long EvntStart = 0;
float Grtzl_Gain = 1.0;
/*the following 4 lines are needed when you want to synthesize an input tone of known freq & Magnitude*/
// float LclToneAngle = 0;
// float synthFreq = 750;
// int dwelCnt = 0;
// float AnglInc = 2*PI*synthFreq/SAMPLING_RATE;

/*Global ADC variables */
// #define Goertzel_SAMPLE_CNT   384 // @750Hz tone input & 48Khz sample rate = 64 samples per cycle & 6 cycle sample duration. i.e. 8ms
// #define EXAMPLE_ADC_CONV_MODE           ADC_CONV_SINGLE_UNIT_1
// #define EXAMPLE_ADC_USE_OUTPUT_TYPE1    1
// #define EXAMPLE_ADC_OUTPUT_TYPE         ADC_DIGI_OUTPUT_FORMAT_TYPE1
static adc_channel_t channel[1] = {ADC_CHANNEL_6};
adc_continuous_handle_t adc_handle = NULL;
static TaskHandle_t GoertzelTaskHandle;
static TaskHandle_t DsplUpDtTaskHandle = NULL;
static TaskHandle_t CWDecodeTaskHandle = NULL;
TaskHandle_t AdvParserTaskHandle = NULL;
static const char *TAG1 = "ADC_Config";
static const char *TAG2 = "PAIR_EVT";
uint32_t ret_num = 0;
uint8_t result[Goertzel_SAMPLE_CNT * SOC_ADC_DIGI_RESULT_BYTES] = {0};
/*tone freq calc variables */
uint32_t SmplCNt = 0;
uint32_t NoTnCntr = 0;
uint8_t PeriodCntr = 0;
int Oldk = 0; //used in the addsmpl() as part of the auto-tune/freq measurement process 
int DemodFreq = 0;
int DmodFrqOld = 0;
bool TstNegFlg = false;
bool CalGtxlParamFlg = false;


TaskHandle_t HKdotclkHndl;

TFTMsgBox tftmsgbx(&tft, StrdTxt);
CWSNDENGN CWsndengn(&DotClk_hndl, &tft, &tftmsgbx);
/*                           */

BTKeyboard bt_keyboard(&tftmsgbx, &DFault);
TxtNtryBox txtntrybox(&DotClk_hndl, &tft);
/* coredump crash test code */
/* typedef struct{
  int a;
  char *s;
} data_t;

void show_data(data_t *data){
  if(strlen(data->s) >10){
    printf("String too long");
    return;
  }
  printf("here's your string %s", data->s);
} */

/*ADC callback event; Fires when ADC buffer is full & ready for processing*/
bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // Notify GoertzelTaskHandle that the buffer is full.

  /* At this point GoertzelTaskHandle should not be NULL as a ADC conversion completed. */
  //configASSERT(GoertzelTaskHandle != NULL);
  EvntStart = pdTICKS_TO_MS(xTaskGetTickCount());
  vTaskNotifyGiveFromISR(GoertzelTaskHandle, &xHigherPriorityTaskWoken); // start Goertzel Task
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  
  /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
  should be performed to ensure the interrupt returns directly to the highest
  priority task.  The macro used for this purpose is dependent on the port in
  use and may be called portEND_SWITCHING_ISR(). */
  // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  return (xHigherPriorityTaskWoken == pdTRUE);
}
/* Setup & initialize DMA ADC process */
static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
  adc_continuous_handle_t handle = NULL;

  adc_continuous_handle_cfg_t adc_config = {
      .max_store_buf_size = (SOC_ADC_DIGI_RESULT_BYTES * Goertzel_SAMPLE_CNT)*8, //2048,
      .conv_frame_size = SOC_ADC_DIGI_RESULT_BYTES * Goertzel_SAMPLE_CNT, 
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));
  uint32_t freq = 120*1000;// actually yeilds something closer to 100Khz sample rate
  
  adc_continuous_config_t dig_cfg = {
      .sample_freq_hz = freq,
      .conv_mode = ADC_CONV_SINGLE_UNIT_1,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
  };
  /* uint32_t interval = APB_CLK_FREQ / (ADC_LL_CLKM_DIV_NUM_DEFAULT + ADC_LL_CLKM_DIV_A_DEFAULT / ADC_LL_CLKM_DIV_B_DEFAULT + 1) / 2 / freq;
  char buf[25];
  sprintf(buf, "interval: %d\n", (int)interval); */

  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
  dig_cfg.pattern_num = channel_num;
  for (int i = 0; i < channel_num; i++)
  {
    uint8_t unit = ADC_UNIT_1;
    uint8_t ch = channel[i] & 0x7;
    adc_pattern[i].atten = ADC_ATTEN_DB_11; // ADC_ATTEN_DB_0;
    adc_pattern[i].channel = ch;
    adc_pattern[i].unit = unit;
    adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    ESP_LOGI(TAG1, "adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
    ESP_LOGI(TAG1, "adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
    ESP_LOGI(TAG1, "adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
  }
  dig_cfg.adc_pattern = adc_pattern;
  ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

  *out_handle = handle;
}

static bool check_valid_data(const adc_digi_output_data_t *data)
{
  if (data->type1.channel >= SOC_ADC_CHANNEL_NUM(ADC_UNIT_1))
  {
    return false;
  }
  return true;
}
////////////////////////////////////////////
/* Add a new ADC sample to the Goertzel processing chain */
void addSmpl(int k, int i, int *pCntrA)
{
  /*The following is for diagnostic testing; it generates a psuesdo tone input of known freq & magnitude */
  // dwelCnt++;
  // if (dwelCnt>20*Goertzel_SAMPLE_CNT){ //Inc psuedo tone freq by 5 hz every 20 sample groups; i.e.~ every 80ms
  //   dwelCnt = 0;
  //   synthFreq +=5;
  //   if(synthFreq>950){
  //     synthFreq = 500;
  //   }
  //   AnglInc = 2*PI*synthFreq/SAMPLING_RATE;
  // }
  // LclToneAngle += AnglInc;
  // if(LclToneAngle >= 2*PI) LclToneAngle = LclToneAngle - 2*PI;
  // k = (int)(800*sin(LclToneAngle));

  /*Calculate Tone frequency*/
  /*the following is part of the auto tune process */
  const int ToneThrsHld = 50; // minimum usable peak tone value; Anything less is noise
  if (AutoTune)
  {
    /*if we have a usable signal; start counting the number of samples needed capture 40 periods of the incoming tone;
     i.e., a 500Hz tone used to send 2/3 a dah @ 50WPM */
    if ((SmplCNt == 0))
    {
      if (k > ToneThrsHld && Oldk <= ToneThrsHld)
      {
        /*armed with a positive going signal; ready to start new measuremnt; reset counters & wait for it to go negative*/
        TstNegFlg = true;
        NoTnCntr = 0; // set "No Tone Counter" to zero
        PeriodCntr = 0;
        SmplCNt++;
      }
    }
    else
    { // SmplCNt != 0; we're up and counting now
      SmplCNt++;
      NoTnCntr++; // increment "No Tone Counter"
      if (TstNegFlg)
      { //looking for the signal to go negative
        if ((k < -ToneThrsHld) && (Oldk >= -ToneThrsHld))
        { /*this one just went negative*/
          NoTnCntr = 0; // reset "No Tone Counter" to zero
          TstNegFlg = false;
        }
      }
      else
      {//  TstNegFlg = false ; Now looking for the signal to go positve
        if ((k > ToneThrsHld) && (Oldk <= ToneThrsHld))
        {/* this one just went positive*/
          PeriodCntr++; // we've lived one complete cycle of some unknown frequency
          NoTnCntr = 0; // reset "No Tone Counter" to zero
          TstNegFlg = true;
        }
      }
    }
    if (NoTnCntr == 200 || (PeriodCntr == 40 && (SmplCNt < 4020)))
    { /*We processed enough samples; But params are outside a usable frequency; Reset & try again*/
      SmplCNt = 0;
    }
    else if (PeriodCntr == 40)
    { /*if here, we have a usable signal; calculate its frequency*/
      DemodFreq = ((int)PeriodCntr * (int)SAMPLING_RATE) / (int)SmplCNt; // SAMPLING_RATE//100500
      if (DemodFreq > 450)
      {
        if (DemodFreq > DmodFrqOld - 50 && DemodFreq < DmodFrqOld + 50)
        {
          // sprintf(Title, "Tone: %d\t%d\n", DemodFreq, (int)NoTnCntr);
          // printf(Title);
          CalGtxlParamFlg = true;
          // CalcFrqParams((float)DemodFreq); // recalculate Goertzel parameters, for the newly selected target grequency
          // showSpeed();
        }
        DmodFrqOld = DemodFreq;
      }

      /*reset for next round of samples*/
      SmplCNt = 0;
    }
    Oldk = k;
  }
// 
  ProcessSample(k, i);
  /* uncomment for diagnostic testing; graph raw ADC samples*/
  // if ((*pCntrA < (6 * Goertzel_SAMPLE_CNT)) && UrTurn)
  // {
  //   if ((*pCntrA == Goertzel_SAMPLE_CNT) || (*pCntrA == 2 * Goertzel_SAMPLE_CNT) || (*pCntrA == 3 * Goertzel_SAMPLE_CNT) || (*pCntrA == 4 * Goertzel_SAMPLE_CNT) || (*pCntrA == 5 * Goertzel_SAMPLE_CNT))
  //   {
  //     Smpl_buf[*pCntrA] = k;// use this line if marker is NOT needed
  //     //Smpl_buf[*pCntrA] = 2000+(*pCntrA); // place a marker at the end of each group
  //   }
  //   else if(*pCntrA != 0)
  //     Smpl_buf[*pCntrA] = k;
  //   *pCntrA += 1;
  // }
  // if ((*pCntrA == (6 * Goertzel_SAMPLE_CNT)) || !UrTurn)
  // {
  //   UrTurn = false;
  //   *pCntrA = 0;
  //   //Smpl_buf[*pCntrA] = 2000; // place a marker at the the begining of the next set
  // }
  /* END code for diagnostic testing; graph raw ADC samples*/
}

/////////////////////////////////////////////
/* Goertzel Task; Process ADC buffer*/

void GoertzelHandler(void *param)
{
  static uint32_t thread_notification;
  static const char *TAG2 = "ADC_Read";
  uint16_t curclr = 0;
  uint16_t oldclr = 0;
  int k;
  int offset = 0;
  int BIAS = 1844+150; // based reading found when no signal applied to ESP32continuous_adc_init
  int Smpl_CntrA = 0;
  bool FrstPass = true;
  InitGoertzel(); // make sure the Goertzel Params are setup & ready to go
  /*The following is just for time/clock testing/veification */
  // unsigned long GrtzlStart = pdTICKS_TO_MS(xTaskGetTickCount());
  // unsigned long GrtzlDone = pdTICKS_TO_MS(xTaskGetTickCount());
  while (1)
  {
    /* Sleep until we are notified of a state change by an
     * interrupt handler. Note the first parameter is pdTRUE,
     * which has the effect of clearing the task's notification
     * value back to 0, making the notification value act like
     * a binary (rather than a counting) semaphore.  */

    thread_notification = ulTaskNotifyTake(pdTRUE,
                                           portMAX_DELAY);

    if (thread_notification)
    { // Goertzel data samples ready for processing
      /*1st do a little house keeping*/
      // if(!SlwFlg){
      //   FrstPass = true;
      // }
      ret = adc_continuous_read(adc_handle, result, Goertzel_SAMPLE_CNT * SOC_ADC_DIGI_RESULT_BYTES, &ret_num, 0);
      if (ret == ESP_OK)
      {
        if(SlwFlg) FrstPass = false; //20231231 added to support new 4ms sample interval, W/ 8ms dataset
        if(FrstPass) ResetGoertzel();
        if(SlwFlg && !FrstPass) offset = Goertzel_SAMPLE_CNT;
        else offset = 0;
        /*  ESP_LOGI("TASK_ADC", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);*/

        for (int i = 0; i < Goertzel_SAMPLE_CNT / 2; i++) // for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES)
        {
          /*Used this approach because I found the ADC data were being returned in alternating order.
          So by taking them a pair at a time,I could restore (& process them) in their true chronological order*/
          int pos0 = 2 * i;
          int pos1 = pos0 + 1;
          adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[pos0 * SOC_ADC_DIGI_RESULT_BYTES];
          adc_digi_output_data_t *p1 = (adc_digi_output_data_t *)&result[pos1 * SOC_ADC_DIGI_RESULT_BYTES];
          k = ((int)(p1->type1.data) - BIAS);
          addSmpl(k, pos0+offset, &Smpl_CntrA);//addSmpl(k, pos1, &Smpl_CntrA);
          k = ((int)(p->type1.data) - BIAS);
          addSmpl(k, pos1+offset, &Smpl_CntrA);//addSmpl(k, pos0, &Smpl_CntrA);
        }
        /*logic to manage the number of data samples to take before going on to compute goertzel magnitudes*/
        // if(SlwFlg && FrstPass){
        //   FrstPass = false;
        // } else if(SlwFlg){
        //   FrstPass = true;
        // } 
        if(SlwFlg && FrstPass){
          FrstPass = false;
        } else{
          FrstPass = true;
        }
        //if(CWsndengn.GetState() == 0){ //Check CW Send Engine sate/status. If Active (state !=0), dont pass this data set to the decoder
        if(!CWsndengn.IsActv()){
          if(FrstPass) ComputeMags(EvntStart);  //"EvntStart" is the time stamp for this dataset
          curclr = ToneClr();
        }//else printf("%d\n", CWsndengn.GetState());//just for diagnostic testing
        if (oldclr != curclr)
        {
          oldclr = curclr;
          tftmsgbx.ShwTone(curclr);
        }
        /*The following is just for time/clock testing/veification */
        // GrtzlDone = pdTICKS_TO_MS(xTaskGetTickCount());
        // uint16_t GrtzlIntrv = (uint16_t)(GrtzlDone-GrtzlStart);
        // GrtzlStart = GrtzlDone;
        // if(GrtzlIntrv > 5) printf("GrtzlIntrv: %d\n", GrtzlIntrv);
      }
      else if (ret == ESP_ERR_TIMEOUT)
      {
        // We try to read `ADC SAMPLEs/DATA` until API returns timeout, which means there's no available data
        /*JMH - This ocassionally happens; Why I'm not sure; But seems to recover & goes on with little fuss*/
        ESP_LOGI(TAG2, "BREAK from GoertzelHandler TASK");
        // break; //Commented out for esp32 decoder task
      }
      else
      {
        ESP_LOGI(TAG2, "NO ADC Data Returned");
      }
    }
  } // end while(1) loop
  /** JMH Added this to ESP32 version to handle random crashing with error,"Task Goertzel Task should not return, Aborting now" */
  vTaskDelete(NULL);
}
///////////////////////////////////////////////////////////////////////////////////
/* DisplayUpDt Task; Refresh/UpdateTFT Display*/
void DisplayUpDt(void *param)
{
  static uint32_t thread_notification;
  uint8_t state;
  while (1)
  {
    /* Sleep until we are notified of a state change by an
     * interrupt handler. Note the first parameter is pdTRUE,
     * which has the effect of clearing the task's notification
     * value back to 0, making the notification value act like
     * a binary (rather than a counting) semaphore.  */
    thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (thread_notification)
    {
      
      if (mutex != NULL)
      {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. 
        note:  The macro pdMS_TO_TICKS() converts milliseconds to ticks */
        if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
        {
          /* We were able to obtain the semaphore and can now access the
          shared resource. */
          mutexFLG = true;
          if(CWsndengn.UpDtWPM){
            CWsndengn.UpDtWPM = false;
            CWsndengn.RefreshWPM();
          } 
          tftmsgbx.dispMsg2();
          while (xQueueReceive(state_que, (void *)&state, (TickType_t)10) == pdTRUE)
          {
            tftmsgbx.IntrCrsr(state);
          }
          /* We have finished accessing the shared resource.  Release the
          semaphore. */
          xSemaphoreGive(mutex);
          mutexFLG = false;
        }
      }
    }
  } // end while(1) loop
  /** JMH Added this to ESP32 version to handle random crashing with error,"Task Goertzel Task should not return, Aborting now" */
  vTaskDelete(NULL);
}
///////////////////////////////////////////////////////////////////////////////////
/* CW Decoder Task; CW decoder main loop*/
void CWDecodeTask(void *param)
{
  static uint32_t thread_notification;
  static const char *TAG3 = "Decode Task";
  char Smpl[10];
  int sample;
  StartDecoder(&tftmsgbx);
  while (1)
  {
    /* Sleep until we are notified of a state change by an
     * interrupt handler. Note the first parameter is pdTRUE,
     * which has the effect of clearing the task's notification
     * value back to 0, making the notification value act like
     * a binary (rather than a counting) semaphore.  */

    thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (thread_notification)
    {
      //printf("CWDecodeTask CoreID: %d\n", xPortGetCoreID());
      Dcodeloop();
      vTaskDelay(1 / portTICK_PERIOD_MS); //this task under normal conditions runs all the time So insert a breather to allow Watchdog to reset
      //printf("CWDecodeTask complete\n");
      /* uncomment for diagnostic testing; graph ADC samples; Note: companion code in addSmpl(int k, int i, int *pCntrA) needs to be uncommented too */
      /* Pull accumulated "ADC sample" values from buffer & print to serial port*/
      // if(!UrTurn){
      //   /* uint32_t freq = 120*1000;//48*1000;
      //   uint32_t interval = APB_CLK_FREQ / (ADC_LL_CLKM_DIV_NUM_DEFAULT + ADC_LL_CLKM_DIV_A_DEFAULT / ADC_LL_CLKM_DIV_B_DEFAULT+1) /4 / freq;
      //   uint32_t Smpl_rate = 1000000/interval;
      //   char buf[40];
      //   sprintf(buf, "interval: %d; SmplRate: %d\n", (int)interval, (int)Smpl_rate);
      //   printf(buf); */
      //   for (int Smpl_CntrA = 0; Smpl_CntrA < 6 * Goertzel_SAMPLE_CNT; Smpl_CntrA++)
      //   {
      //     if(Smpl_buf[Smpl_CntrA] !=0){
      //       sprintf(Smpl, "%d\n", Smpl_buf[Smpl_CntrA]);
      //       printf(Smpl);
            
      //     }
      //   }
      //   UrTurn = true;
      // }
      /* END code for diagnostic testing; graph ADC samples; */
      
      xTaskNotifyGive(CWDecodeTaskHandle);
      
    }
    else if (ret == ESP_ERR_TIMEOUT)
    {
      ESP_LOGI(TAG3, "BREAK from CWDecodeTask TASK");
      break;
    }
    if(WokeFlg){ //report void IRAM_ATTR DotClk_ISR(void *arg) result
      WokeFlg = false;
      printf("!!WOKE!!\n");
    }
    if(QuequeFulFlg){ //report void IRAM_ATTR DotClk_ISR(void *arg) result
      QuequeFulFlg = false;
      char buf[40];
      sprintf(buf, "!!state QUEUE FULL!! %d\n", (int)QueFullstate);
      printf(buf);
      // printf("!!state QUEUE FULL!!\n");
    }
  }
}
///////////////////////////////////////////////////////////////////////////////////
/* AdvParserTask Task; Post Parser Code execution
*  Note: this task runs on core 1 
*/
void AdvParserTask(void *param)
{
  // static uint32_t thread_notification;
  // uint8_t state;
  // UBaseType_t uxHighWaterMark;
  //unsigned long AdvPStart = 0;
  while (1)
  {
    /* Sleep until instructed to resume from DcodeCW.cpp */
    //printf("AdvParserTask\n");
    //AdvPStart = pdTICKS_TO_MS(xTaskGetTickCount());
    advparser.EvalTimeData();

    /*Now compare Advparser decoded text to original text; If not the same,
    replace displayed with Advparser version*/
    bool same = true;
    bool Tst4Match = true;
    int i;
    int FmtchPtr; // now only used for debugging
    // printf("Start Scan\n");
    /*Scan/compare last word displayed w/ advpaser's version*/
    int NuMsgLen = advparser.GetMsgLen();
    int LtrPtr = advparser.LtrPtr;
    wrdbrkFtcr = advparser.wrdbrkFtcr ;
    // printf("NuMsgLen = %d; LtrPtr %d\n", NuMsgLen, LtrPtr);
    if (NuMsgLen > LtrPtr)
    { // if the advparser test string is longer, then delete the last word printed
      same = false;
      i = LtrPtr;
    }
    else
    {
      for (i = 0; i < LtrPtr; i++)
      {
        if (advparser.Msgbuf[i] == 0)
          Tst4Match = false;
        if ((advparser.LtrHoldr[i] != advparser.Msgbuf[i]) && Tst4Match)
        {
          FmtchPtr = i; // now only used for debugging
          same = false;
        }
        if (advparser.LtrHoldr[i] == 0)
          break;
      }
    }
    /*If they don't match, replace displayed text with AdvParser's version*/
    if (!same)
    {

      bool oldDltState = dletechar;
      int deletCnt = 0;
      char spacemarker = 'Y';
      uint8_t LstChr = 0;
      // printf("!Same\n");
      if (LtrPtr <= 11)
      {
        while (LstChr != 0x20)
        {
          vTaskDelay(5 / portTICK_PERIOD_MS);
          LstChr = tftmsgbx.GetLastChar();
        }
      }
      else
      {
        vTaskDelay(50 / portTICK_PERIOD_MS); // wait long enough for the TFT display to get updated
        LstChr = tftmsgbx.GetLastChar();     // get the last character posted via the DcodeCW process (To test below for a 'space' [0x20])
      }
      // printf("tftmsgbx.GetLastChar = %d\n", LstChr);
      if (LstChr == 0x20) // test to see if a word break space has been applied
      {
        deletCnt = 1 + i; // number of characters + space to be deleted
        // tftmsgbx.Delete(1+ i);// this caused the ESP32 to crash on WatchDog timer timeout
        // printf("MsgChrCnt[1] = %d\n", 1+ i);
      }
      else
      {
        spacemarker = 'N';
        deletCnt = i; // number of characters (No space) to be deleted
        // tftmsgbx.Delete(i);// this caused the ESP32 to crash on WatchDog timer timeout
        // printf("MsgChrCnt[1] = %d\n", i);
      }
      /*Configure DCodeCW.cpp to erase/delete original text*/
      MsgChrCnt[1] = deletCnt; // Load delete buffer w/ the number of characters to be deleted
      dletechar = true;
      /*Add word break space to post parsed text*/
      advparser.Msgbuf[NuMsgLen] = 0x20;
      advparser.Msgbuf[NuMsgLen + 1] = 0x0;
      if (advparser.Dbug)
        printf("old txt %s;  new txt %s delete cnt %d; advparser.LtrPtr %d ; new txt length %d; Space Corrected = %c/%d \n", advparser.LtrHoldr, advparser.Msgbuf, deletCnt, LtrPtr, NuMsgLen, spacemarker, LstChr);
      // printf("Pointer ERROR\n");/ printf("No Match @ %d; %d; %d\n", FmtchPtr, LtrHoldr[FmtchPtr], advparser.Msgbuf[FmtchPtr]);
      CptrTxt = false;
      // printf("Step Complete\n");
      dispMsg(advparser.Msgbuf); // send newly reparsed text over to DCodeCW.cpp to be displayed as replaced text
      CptrTxt = true;
      dletechar = oldDltState;
    } // else printf("Same\n");
    if (advparser.Dbug)
      printf("%s\n", advparser.LtrHoldr);
    // erase contents of LtrHoldr & reset its index pointer (LtrPtr)
    for (int i = 0; i < LtrPtr; i++)
      advparser.LtrHoldr[i] = 0;
    if (advparser.Dbug)
      printf("--------\n\n");
    //uint16_t AdvPIntrv = (uint16_t)(pdTICKS_TO_MS(xTaskGetTickCount())-AdvPStart);
    //printf("AdvPIntrv: %d\n", AdvPIntrv); 
    // uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    // printf("AdvParserTask stack: %d\n", (int)uxHighWaterMark);
    // printf("AdvParserTask CoreID: %d\n", xPortGetCoreID());
    // printf("Done\n");
    /* Sleep until instructed to resume from DcodeCW.cpp */
    vTaskSuspend(AdvParserTaskHandle);
  } // end while(1) loop
  /** JMH Added this to ESP32 version to handle random crashing with error,"Task Goertzel Task should not return, Aborting now" */
  vTaskDelete(NULL);
}

///////////////////////////////////////////////////////////////////////////////////

extern "C"
{
  void app_main();
}
/*Template for keyboard entry handler function detailed futrther down in this file*/
void ProcsKeyEntry(uint8_t keyVal);
/*        Display Update timer ISR Template        */
void DsplTmr_callback(TimerHandle_t xtimer);
/*        DotClk timer ISR Template                */
void IRAM_ATTR DotClk_ISR(void *arg);

void pairing_handler(uint32_t pid)
{
  sprintf(Title, "Please enter the following pairing code,\n");
  tftmsgbx.dispMsg(Title, TFT_WHITE);
  sprintf(Title, "followed with ENTER on your keyboard: \n");
  tftmsgbx.dispMsg(Title, TFT_WHITE);
  sprintf(Title, "%d \n", (int)pid);
  tftmsgbx.dispMsg(Title, TFT_BLUE);
  bt_keyboard.PairFlg = true;
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

void app_main()
{
  /*uncomment the following for diagnostic testing of host HID connection/pairing to keyboard*/
  // Exmpl_main();
  // while(1){
  //   vTaskDelay(1000 / portTICK_PERIOD_MS);
  // }
  ModeCnt = 0;
  static const char *TAG = "TASK Config";
  // Configure pin
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << KEY);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  digitalWrite(KEY, Key_Up); // key 'UP' state
  state_que = xQueueCreate(state_que_len, sizeof(uint8_t));
  /*create DisplayUpDate Task*/
  xTaskCreatePinnedToCore(DisplayUpDt, "DisplayUpDate Task", 8192, NULL, 3, &DsplUpDtTaskHandle, 0);
  vTaskSuspend( DsplUpDtTaskHandle);

  /*Selected 2092 based on results found by using uxTaskGetStackHighWaterMark( NULL ) in AdvParserTask*/
  xTaskCreatePinnedToCore(
      AdvParserTask, /* Function to implement the task */
      "AdvParser Task", /* Name of the task */
      4096,  /* Stack size in words */
      NULL,  /* Task input parameter */
      2,  /* Priority of the task */
      &AdvParserTaskHandle,  /* Task handle. */
      1); /* Core where the task should run */
  if (AdvParserTaskHandle == NULL)
    ESP_LOGI(TAG, "AdvParser Task handle FAILED");

  vTaskSuspend( AdvParserTaskHandle );
  
  xTaskCreatePinnedToCore(
      CWDecodeTask, /* Function to implement the task */
      "CW Decode Task", /* Name of the task */
      8192,  /* Stack size in words */
      NULL,  /* Task input parameter */
      4,  /* Priority of the task */
      &CWDecodeTaskHandle,  /* Task handle. */
      0); /*Core 0 or 1 */
  
  if (CWDecodeTaskHandle == NULL)
    ESP_LOGI(TAG, "CW Decoder Task handle FAILED");

  xTaskCreate(GoertzelHandler, "Goertzel Task", 8192, NULL, 5, &GoertzelTaskHandle); // priority used to be 3
  if (GoertzelTaskHandle == NULL)
    ESP_LOGI(TAG, "Goertzel Task Task handle FAILED");

  /*Setup Software Display Refresh/Update timer*/
  DisplayTmr = xTimerCreate(
      "Display-Refresh-Timer",
      33 / portTICK_PERIOD_MS,
      pdTRUE,
      (void *)1,
      DsplTmr_callback);

  memset(result, 0xcc, (SOC_ADC_DIGI_RESULT_BYTES * Goertzel_SAMPLE_CNT)); // 1 byte for the channel # & 1 byte for the data
  mutex = xSemaphoreCreateMutex();
  // ADC_smpl_mutex = xSemaphoreCreateMutex();

  /*setup Hardware DOT Clock timer & link to timer ISR*/
  const esp_timer_create_args_t DotClk_args = {
      .callback = &DotClk_ISR,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_ISR,
      .name = "DotClck"};

  ESP_ERROR_CHECK(esp_timer_create(&DotClk_args, &DotClk_hndl));

  
  /* The timer has been created but is not running yet */
intr_matrix_set(xPortGetCoreID(), ETS_TG0_T1_LEVEL_INTR_SOURCE, 26); // display

  // ESP_LOGI(TAG, "Start DotClk interrupt Config");
  // ESP_ERROR_CHECK(esp_intr_alloc(ETS_TG0_T1_LEVEL_INTR_SOURCE, ESP_INTR_FLAG_LEVEL3, DotClk_ISR, DotClk_args.arg, NULL));
  // ESP_LOGI(TAG, "End DotClk interrupt Config");

  ///////////////////////////////////////////////////////////////////////////////////////////////////////
 ESP_INTR_ENABLE(26); // display

  // /* Start the timer dotclock (timer)*/
  // ESP_ERROR_CHECK(esp_timer_start_periodic(DotClk_hndl, 60000)); // 20WPM or 60ms

  
  // To test the Pairing code entry, uncomment the following line as pairing info is
  // kept in the nvs. Pairing will then be required on every boot.
  // ret = nvs_flash_init();
  // ESP_ERROR_CHECK(nvs_flash_erase());
  ret = nvs_flash_init();
  if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND))
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  /*Initialize display and load splash screen*/
  tftmsgbx.InitDsplay();
  sprintf(Title, " ESP32 CW Decoder & Keyboard (%s)\n", RevDate); // sprintf(Title, "CPU CORE: %d\n", (int)xPortGetCoreID());
  tftmsgbx.dispMsg(Title, TFT_SKYBLUE);

  /* test/check NVS to see if user setting/param have been stored */
  uint8_t Rstat = Read_NVS_Str("MyCall", DFault.MyCall);
  if (Rstat != 1)
  {
    /*No settings found; load factory settings*/
    sprintf(DFault.MyCall, "%s", MyCall);
    sprintf(DFault.MemF2, "%s", MemF2);
    sprintf(DFault.MemF3, "%s", MemF3);
    sprintf(DFault.MemF4, "%s", MemF4);
    sprintf(DFault.MemF5, "%s", MemF5);
    DFault.DeBug = DeBug;
    DFault.WPM = CWsndengn.GetWPM();
    DFault.ModeCnt = ModeCnt;
    DFault.AutoTune = AutoTune;
    DFault.TRGT_FREQ = (int)TARGET_FREQUENCYC;
    DFault.Grtzl_Gain = Grtzl_Gain;
    DFault.SlwFlg = SlwFlg;
    DFault.NoisFlg = NoisFlg;
    sprintf(Title, "\n        No stored USER params Found\n   Using FACTORY values until params are\n   'Saved' via the Settings Screen\n");
    tftmsgbx.dispMsg(Title, TFT_ORANGE);
    bt_keyboard.trapFlg = false;
  }
  else
  {
    /*found 'mycall' stored setting, go get the other user settings */
    Rstat = Read_NVS_Str("MemF2", DFault.MemF2);
    Rstat = Read_NVS_Str("MemF3", DFault.MemF3);
    Rstat = Read_NVS_Str("MemF4", DFault.MemF4);
    Rstat = Read_NVS_Str("MemF5", DFault.MemF5);
    Rstat = Read_NVS_Val("DeBug", DFault.DeBug);
    Rstat = Read_NVS_Val("WPM", DFault.WPM);
    Rstat = Read_NVS_Val("ModeCnt", DFault.ModeCnt);
    Rstat = Read_NVS_Val("TRGT_FREQ", DFault.TRGT_FREQ);
    int intGainVal; // uint64_t intGainVal;
    Rstat = Read_NVS_Val("Grtzl_Gain", intGainVal);
    DFault.Grtzl_Gain = (float)intGainVal / 10000000.0;
    int strdAT;
    Rstat = Read_NVS_Val("AutoTune", strdAT);
    DFault.AutoTune = (bool)strdAT;
    Rstat = Read_NVS_Val("SlwFlg", strdAT);
    DFault.SlwFlg = (bool)strdAT;
    Rstat = Read_NVS_Val("NoisFlg", strdAT);
    DFault.NoisFlg = (bool)strdAT;
    /*pass the decoder setting(s) back to their global counterpart(s) */
    AutoTune = DFault.AutoTune;
    SlwFlg = DFault.SlwFlg;
    NoisFlg = DFault.NoisFlg;
    ModeCnt = DFault.ModeCnt;
    DeBug = DFault.DeBug;
    TARGET_FREQUENCYC = (float)DFault.TRGT_FREQ;
    Grtzl_Gain = DFault.Grtzl_Gain;
  }
  /* Start the Display timer */
  xTimerStart(DisplayTmr, portMAX_DELAY);
  vTaskResume( DsplUpDtTaskHandle);
  // CWsndengn.RfrshSpd = true;
  // CWsndengn.ShwWPM(DFault.WPM); // calling this method does NOT recalc/set the dotclock & show the WPM
  // CWsndengn.SetWPM(DFault.WPM); // 20230507 Added this seperate method call after changing how the dot clocktiming gets updated

  InitGoertzel();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  bt_keyboard.inPrgsFlg = false;
  /*start bluetooth pairing/linking process*/
  if (bt_keyboard.setup(pairing_handler))
  {                             // Must be called once
    printf("START SCAN\n");
    bt_keyboard.devices_scan(3); // Required to discover new keyboards and for pairing
                                // Default duration is 3 seconds
    printf("EXIT SCAN\n");                            
  }
  else
  {
    sprintf(Title, "\n*******  RESTART!  ******\n** NO KEYBOARD PAIRED ***");
    tftmsgbx.dispMsg(Title, TFT_RED);
    while (true)
    {
      /*do nothiing*/
      delay(20);
    }
  }
  /*initialize & start continuous DMA ADC conversion process*/
  continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &adc_handle);
  //xTaskCreate(GoertzelHandler, "Goertzel Task", 8192, NULL, 5, &GoertzelTaskHandle); // priority used to be 3

  adc_continuous_evt_cbs_t cbs = {
      .on_conv_done = s_conv_done_cb,
  };
  ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
  ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
  adcON = true;
  xTaskNotifyGive(CWDecodeTaskHandle);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  
  ESP_LOGI(TAG, "Start DotClk interrupt Config");
  ESP_ERROR_CHECK(esp_intr_alloc(ETS_TG0_T1_LEVEL_INTR_SOURCE, ESP_INTR_FLAG_LEVEL3, DotClk_ISR, DotClk_args.arg, NULL));
  ESP_LOGI(TAG, "End DotClk interrupt Config"); 
 /* Start the timer dotclock (timer)*/
  ESP_ERROR_CHECK(esp_timer_start_periodic(DotClk_hndl, 60000)); // 20WPM or 60ms
  CWsndengn.RfrshSpd = true;
  CWsndengn.ShwWPM(DFault.WPM); // calling this method does NOT recalc/set the dotclock & show the WPM
  CWsndengn.SetWPM(DFault.WPM); // 20230507 Added this seperate method call after changing how the dot clocktiming gets updated

  

  /* main CW keyboard loop*/
  
  while (true)
  {
#if 1 // 0 = scan codes retrieval, 1 = augmented ASCII retrieval
    /* note: this loop only completes when there is a key entery from a paired/connected Bluetooth Keyboard */

    vTaskDelay(1); // give the watchdogtimer a chance to reset

    if (setupFlg)
    /*if true, exit main loop and jump to "settings" screen */
    {
      bool IntSOTstate = CWsndengn.GetSOTflg();
      if (IntSOTstate)
        CWsndengn.SOTmode();   // do this to stop any outgoing txt that might currently be in progress before switching over to settings screen
      tftmsgbx.SaveSettings(); // save keyboard app's current display configuration; i.e., ringbuffer pointeres, etc. So that when the user closes the setting screen the keyboard app can conitnue fro where it left off
      /* if in decode mode 4, the adc DMA scan was never started*/
      if (ModeCnt != 4)
      {
        ESP_ERROR_CHECK(adc_continuous_stop(adc_handle)); // true; user has pressed Ctl+S key, & wants to configure default settings
        adcON = false;
        vTaskSuspend(GoertzelTaskHandle);
        ESP_LOGI(TAG1, "SUSPEND GoertzelHandler TASK");
        vTaskSuspend(CWDecodeTaskHandle);
        ESP_LOGI(TAG1, "SUSPEND CWDecodeTaskHandle TASK");
        vTaskDelay(20);
      }
      /*Now ready to jump to "settings" screen */
      setuploop(&tft, &CWsndengn, &tftmsgbx, &bt_keyboard, &DFault); // function defined in SetUpScrn.cpp/.h file
      // if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE)//pdMS_TO_TICKS()//portMAX_DELAY
      // {
      //   /* We were able to obtain the semaphore and can now access the
      //   shared resource. */
      //   mutexFLG = true;
      tftmsgbx.ReBldDsplay();
      // /* We have finished accessing the shared resource.  Release the semaphore. */
      //   xSemaphoreGive(mutex);
      //   mutexFLG = false;
      // }
      CWsndengn.RefreshWPM();
      if (IntSOTstate && !CWsndengn.GetSOTflg())
        CWsndengn.SOTmode(); // Send On Type was enabled when we went to 'settings' so re-enable it
      if (ModeCnt != 4)
      {
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
        adcON = true;
        ESP_LOGI(TAG1, "RESUME CWDecodeTaskHandle TASK");
        vTaskResume(CWDecodeTaskHandle);
        ESP_LOGI(TAG1, "RESUME GoertzelHandler TASK");
        vTaskResume(GoertzelTaskHandle);
        vTaskDelay(20);
      }
    }
    /*Added this to support 'open' paired BT keyboard event*/
    switch (bt_keyboard.Adc_Sw)
    {
    case 1: // need to shut down all other spi activities, so the pending BT event has exclusive access to SPI
      bt_keyboard.Adc_Sw = 0;
      if (adcON) // added "if" just to make sure we're not gonna do something that doesn't need doing
      {
        ESP_LOGI(TAG1, "!!!adc_continuous_stop!!!");
        ESP_ERROR_CHECK(adc_continuous_stop(adc_handle));
        adcON = false;
        vTaskSuspend(GoertzelTaskHandle);
        ESP_LOGI(TAG1, "SUSPEND GoertzelHandler TASK");
        vTaskSuspend(CWDecodeTaskHandle);
        ESP_LOGI(TAG1, "SUSPEND CWDecodeTaskHandle TASK");
        // vTaskSuspend(DsplUpDtTaskHandle);
      }
      break;

    case 2: // BT event complete restore/resume all other spi activities
      bt_keyboard.Adc_Sw = 0;
      if (!adcON) // added "if" just to make sure we're not gonna do something that doesn't need doing
      {
        ESP_LOGI(TAG1, "***adc_continuous_start***");
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
        adcON = true;
        ESP_LOGI(TAG1, "RESUME CWDecodeTaskHandle TASK");
        vTaskResume(CWDecodeTaskHandle);
        ESP_LOGI(TAG1, "RESUME GoertzelHandler TASK");
        vTaskResume(GoertzelTaskHandle);
        if (bt_keyboard.PairFlg && bt_keyboard.inPrgsFlg)
        {
          xTimerStart(DisplayTmr, portMAX_DELAY);
          //vTaskResume(DsplUpDtTaskHandle);
          bt_keyboard.PairFlg = false;
          bt_keyboard.inPrgsFlg = false;
          ESP_LOGI(TAG2, "Pairing Complete; Display ON");
        }
        break;

      default:
        break;
      }
    } // end switch
    if (bt_keyboard.PairFlg && !bt_keyboard.inPrgsFlg)
    {
      xTimerStop(DisplayTmr, portMAX_DELAY);
      ESP_LOGI(TAG2, "Pairing Start; Display OFF");
      bt_keyboard.inPrgsFlg = true;
    }
    uint8_t key = 0;
    if (0) // set to '1' for flag debugging
    {
      if (bt_keyboard.OpnEvntFlg)
        printf("OpnEvntFlgTRUE");
      else
        printf("OpnEvntFlgFALSE");
      if (bt_keyboard.trapFlg)
        printf("; trapFlgTRUE\n");
      else
        printf("; trapFlgFALSE\n");

      // else
      //   printf("; PairFlgFALSE\n");
    }
    if (bt_keyboard.OpnEvntFlg && !bt_keyboard.trapFlg) // this gets set to true when the K380 KB generates corrupt keystroke data
      key = bt_keyboard.wait_for_ascii_char();          // true  // by setting to "true" this task/loop will wait 'forever' for a keyboard key press

    /*test key entry & process as needed*/
    if (key != 0)
    {
      ProcsKeyEntry(key);
    }

#else
    BTKeyboard::KeyInfo inf;

    bt_keyboard.wait_for_low_event(inf);

    std::cout << "RECEIVED KEYBOARD EVENT: "
              << std::hex
              << "Mod: "
              << +(uint8_t)inf.modifier
              << ", Keys: "
              << +inf.keys[0] << ", "
              << +inf.keys[1] << ", "
              << +inf.keys[2] << std::endl;
#endif
  }
} /*End Main loop*/

/*Timer interrupt ISRs*/

void DsplTmr_callback(TimerHandle_t xtimer)
{
  // uint8_t state;
  // BaseType_t TaskWoke;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(DsplUpDtTaskHandle, &xHigherPriorityTaskWoken); // start DisplayUpDt Task
  if (xHigherPriorityTaskWoken == pdTRUE) portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/*                                          */
/* DotClk timer ISR                 */
void IRAM_ATTR DotClk_ISR(void *arg)
{
  BaseType_t Woke;
  uint8_t state =0;
  // if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) // pdMS_TO_TICKS()//portMAX_DELAY
  // {
  //   /* We were able to obtain the semaphore and can now access the
  //   shared resource. 
  //   this was placed here because CWsndengn.Intr() can cause the SHOWWPM function to write to the display*/
  //   mutexFLG = true;
    state = CWsndengn.Intr(); // check CW send Engine & process as code as needed
                                      /* We have finished accessing the shared resource.  Release the semaphore. */
  //   xSemaphoreGive(mutex);
  //   mutexFLG = false;
  // }
  if (state != 0)
  {
    vTaskSuspend(CWDecodeTaskHandle);
    clrbuf = true;
  }
  else
  {
    if (clrbuf)
    {
      clrbuf = false;
      CLrDCdValBuf(); // added this to ensure no DeCode Vals accumulate while the CWGen process is active; In other words the App doesn't decode itself
      vTaskResume(CWDecodeTaskHandle);
    }
  }
  /*Push returned "state" values on the que */
  if (xQueueSendFromISR(state_que, &state, &Woke) == pdFALSE)
  {
    QuequeFulFlg = true;
    QueFullstate = state;
  }
  if (Woke == pdTRUE)
    portYIELD_FROM_ISR(Woke);
  /*Woke == pdTRUE, if sending to the queue caused a task to unblock,
  and the unblocked task has a priority higher than the currently running task.
  If xQueueSendFromISR() sets this value to pdTRUE,
  then a context switch should be requested before the interrupt is exited.*/
  // WokeFlg = true;
}
///////////////////////////////////////////////////////////////////////////////////

/*This routine checks the current key entry; 1st looking for special key values that signify special handling
if none are found, it hands off the key entry to be treated as a standard CW character*/
void ProcsKeyEntry(uint8_t keyVal)
{
  bool addspce = false;
  char SpcChr = char(0x20);

  if (keyVal == 0x8)
  {                                  //"BACKSpace" key pressed
    int ChrCnt = CWsndengn.Delete(); // test to see if there's an "unsent" character that can be deleted
    if (ChrCnt > 0)
    {
      if (adcON) // added "if" just to make sure we're not gonna do something that doesn't need doing
      {
        // ESP_LOGI(TAG1, "!!!adc_continuous_stop!!!");
        ESP_ERROR_CHECK(adc_continuous_stop(adc_handle));
        adcON = false;
        vTaskSuspend(GoertzelTaskHandle);
        // ESP_LOGI(TAG1, "SUSPEND GoertzelHandler TASK");
        // vTaskSuspend(CWDecodeTaskHandle);
        // ESP_LOGI(TAG1, "SUSPEND CWDecodeTaskHandle TASK");
      }
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) // wait forever
      {
        /* We were able to obtain the semaphore and can now access the
        shared resource. */
        mutexFLG = true;
        tftmsgbx.Delete(ChrCnt);
        /* We have finished accessing the shared resource.  Release the
        semaphore. */
        xSemaphoreGive(mutex);
        mutexFLG = false;
      }
      if (!adcON) // added "if" just to make sure we're not gonna do something that doesn't need doing
      {
        // ESP_LOGI(TAG1, "***adc_continuous_start***");
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
        adcON = true;
        // ESP_LOGI(TAG1, "RESUME CWDecodeTaskHandle TASK");
        // vTaskResume(CWDecodeTaskHandle);
        // ESP_LOGI(TAG1, "RESUME GoertzelHandler TASK");
        vTaskResume(GoertzelTaskHandle);
      }
    }
    return;
  }
  else if (keyVal == 0x98)
  { // PG/arrow UP
    CWsndengn.IncWPM();
    DFault.WPM = CWsndengn.GetWPM();
    return;
  }
  else if (keyVal == 0x97)
  { // PG/arrow DOWN
    CWsndengn.DecWPM();
    DFault.WPM = CWsndengn.GetWPM();
    return;
  }
  else if (keyVal == 0x8C)
  { // F12 key (Alternate action SOT [Send On type])
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
      /* We were able to obtain the semaphore and can now access the
      shared resource. */
      mutexFLG = true;
      CWsndengn.SOTmode();
      /* We have finished accessing the shared resource.  Release the semaphore. */
      xSemaphoreGive(mutex);
      mutexFLG = false;
    }
    return;
  }
  else if (keyVal == 0x81)
  { // F1 key (store TEXT)
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
      /* We were able to obtain the semaphore and can now access the
      shared resource. */
      mutexFLG = true;
      CWsndengn.StrTxtmode();
      /* We have finished accessing the shared resource.  Release the semaphore. */
      xSemaphoreGive(mutex);
      mutexFLG = false;
    }
    return;
  }
  else if (keyVal == 0x82)
  { // F2 key (Send MemF2)
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(DFault.MemF2, sizeof(DFault.MemF2));
    return;
  }
  else if (keyVal == 0x83)
  { // F3 key (Send MemF3)
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(DFault.MemF3, sizeof(DFault.MemF3));
    return;
  }
  else if (keyVal == 0x84)
  { // F4 key (Send MemF4)
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(DFault.MemF4, sizeof(DFault.MemF4));
    return;
  }
  else if (keyVal == 0x85)
  { // F5 key (Send MemF5)
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(DFault.MemF5, sizeof(DFault.MemF5));
    return;
  }
  else if (keyVal == 0x95)
  { // Right Arrow Key (Alternate action SOT [Send On type])
  if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
      /* We were able to obtain the semaphore and can now access the
      shared resource. */
      mutexFLG = true;
    CWsndengn.SOTmode();
    /* We have finished accessing the shared resource.  Release the semaphore. */
      xSemaphoreGive(mutex);
      mutexFLG = false;
    }
    return;
  }
  else if (keyVal == 0x96)
  { // Left Arrow Key (store TEXT)
  if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
    {
      /* We were able to obtain the semaphore and can now access the
      shared resource. */
      mutexFLG = true;
    CWsndengn.StrTxtmode();
    /* We have finished accessing the shared resource.  Release the semaphore. */
      xSemaphoreGive(mutex);
      mutexFLG = false;
    }
    return;
  }
  else if (keyVal == 0x1B)
  { // ESC key (Kill Send)
    CWsndengn.AbortSnd();
    return;
  }
  else if ((keyVal == 0x9B))
  { // Cntrl+"T"
    CWsndengn.Tune();
    return;
  }
  else if ((keyVal == 0x9C))
  { // Cntrl+"S"
    setupFlg = !setupFlg;
    return;
  }
  else if ((keyVal == 0x9D))
  { // Cntrl+"F"; auto-tune/ freqLocked
    AutoTune = !AutoTune;
    DFault.AutoTune = AutoTune;
    vTaskDelay(20);
    showSpeed();
    vTaskDelay(250);
    return;
  }
  else if ((keyVal == 0xA1))
  { // Cntrl+"G"; Sample interval 4ms / 8ms
    // int GainCnt =0;
    // if(NoisFlg) GainCnt = 2;
    // if(SlwFlg && !NoisFlg) GainCnt = 1;
    //   GainCnt++;
    // if (GainCnt > 1)  GainCnt = 0;//20231029 decided that the 3rd gain mode was no longer needed, so locked it out
    //   switch(GainCnt){
    // case 0:
    //   SlwFlg = false;
    //   NoisFlg = false;
    //   break;
    // case 1:
    //   SlwFlg = true;
    //   NoisFlg = false;
    //   break;
    // case 2:
    //   SlwFlg = true;
    //   NoisFlg = true;
    //   break;
    /*20240123 Changed So that Ctrl+G now Enables/Disables Debug */
    if (DeBug)
    {
      DeBug = false;
      DFault.DeBug = false;
    }
    else
    {
      DeBug = true;
      DFault.DeBug = true;
    }
    // DFault.SlwFlg = SlwFlg;
    // DFault.NoisFlg = NoisFlg;
    // InitGoertzel();
    // vTaskDelay(20);
    // showSpeed();
    // vTaskDelay(250);
    return;
  }
  /*20240123 Disabled Ctrl+D function; replaced by AdvParser Class & methods*/
  // else if ((keyVal == 0x9E))
  // { // LEFT Cntrl+"D"; Decode Modef()

  //     /*Normal setup */
  //     ModeCnt++;
  //     if (ModeCnt > 3)
  //     ModeCnt = 0;
  //     DFault.ModeCnt = ModeCnt;
  //     SetModFlgs(ModeCnt);
  //     CurMdStng(ModeCnt);//added 20230104
  //     vTaskDelay(20);
  //     showSpeed();
  //     vTaskDelay(250);
  //     return;
  // }
  else if ((keyVal == 0xA0))
  { // RIGHT Cntrl+"D"; Decode Modef()

    /*Normal setup */
    // if (ModeCnt == 4)
    // {
    //   ESP_LOGI(TAG1, "RESUME CWDecodeTaskHandle TASK");
    //   vTaskResume(CWDecodeTaskHandle);
    //   ESP_LOGI(TAG1, "RESUME GoertzelHandler TASK");
    //   vTaskResume(GoertzelTaskHandle);
    //   ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
    //   adcON =true;
    //   vTaskDelay(20);
    // }
    ModeCnt--;
    if (ModeCnt < 0)
      ModeCnt = 3;
    DFault.ModeCnt = ModeCnt;
    SetModFlgs(ModeCnt);
    CurMdStng(ModeCnt); // added 20230104
    vTaskDelay(20);
    showSpeed();
    vTaskDelay(250);
    return;
  }
  else if ((keyVal == 0x9F))
  { // Cntrl+"P"; CW decode ADC plot Enable/Disable
    PlotFlg = !PlotFlg;
    // DFault.AutoTune = AutoTune;
    vTaskDelay(250);
    return;
  }
  else if ((keyVal == 0xD))
  { // "ENTER" Key send myCallSign
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(DFault.MyCall, sizeof(DFault.MyCall));
    return;
  }
  else if ((keyVal == 0x9A))
  { // "Cntrl+ENTER" send StrdTxt call
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(StrdTxt, 20);
    return;
  }
  else if ((keyVal == 0x99))
  { // "shift+ENTER" send both calls (StrdTxt & MyCall)
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    // char buf[20]="";
    sprintf(Title, "%s DE %s", StrdTxt, DFault.MyCall);
    CWsndengn.LdMsg(Title, 20);
    return;
  }
  // else if ((keyVal ==  0xA1))
  // { /* special test for Left ctr+'g'
  //   reduce Goertzel gain*/
  //   Grtzl_Gain = Grtzl_Gain/2;
  //   if (Grtzl_Gain < 0.00390625) Grtzl_Gain  = 0.00390625;
  //   DFault.Grtzl_Gain = Grtzl_Gain;
  //   return;
  // }
  // else if ((keyVal ==  0xA2))
  // { /* special test for Right ctr+'g'
  //   increase Goertzel gain*/
  //   Grtzl_Gain = 2* Grtzl_Gain;
  //   if (Grtzl_Gain > 1.0) Grtzl_Gain  = 1.0;
  //   DFault.Grtzl_Gain = Grtzl_Gain;
  //   return;
  // }
  if ((keyVal >= 97) & (keyVal <= 122))
  {
    keyVal = keyVal - 32;
  }
  char Ltr2Bsent = (char)keyVal;
  switch (Ltr2Bsent)
  {
  case '=':
    addspce = true; //<BT>
    break;
  case '+':
    addspce = true; //<KN>
    break;
  case '%':
    addspce = true;
    ; //<SK>
    break;
  case '>':
    addspce = true; //<BT>
    break;
  case '<':
    addspce = true; //<BT>
    break;
  }
  if (addspce && CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
    CWsndengn.AddNewChar(&SpcChr);
  CWsndengn.AddNewChar(&Ltr2Bsent);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* the following code handles the read from & write to NVS processes needed to handle the User CW prefences/settings  */

uint8_t Read_NVS_Str(const char *key, char *value)
/* read string data from NVS*/
{
  uint8_t stat = 0;
  // const uint16_t* p = (const uint16_t*)(const void*)&value;
  //  Handle will automatically close when going out of scope or when it's reset.
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &ret);
  if (ret != ESP_OK)
  {
    sprintf(Title, "Error (%s) opening READ NVS string handle for: %s!\n", esp_err_to_name(ret), key);
    tftmsgbx.dispMsg(Title, TFT_RED);
  }
  else
  {
    size_t required_size;
    ret = handle->get_item_size(nvs::ItemType::SZ, key, required_size);
    switch (ret)
    {
    case ESP_OK:

      if (required_size > 0 && required_size < 100)
      {
        char temp[100];
        ret = handle->get_string(key, temp, required_size);
        int i;
        for (i = 0; i < 100; i++)
        {
          value[i] = temp[i];
          if (temp[i] == 0)
            break;
        }
        if (DFault.DeBug)
        {
          sprintf(Title, "%d characters copied to %s\n", i, key);
          tftmsgbx.dispMsg(Title, TFT_BLUE);
        }
        stat = 1;
      }
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      // sprintf(Title, "%s: has not been initialized yet!\n", key);
      // tftmsgbx.dispMsg(Title, TFT_RED);
      break;
    default:
      sprintf(Title, "Error (%s) reading %s!\n", esp_err_to_name(ret), key);
      tftmsgbx.dispMsg(Title, TFT_RED);
      delay(3000);
    }
  }
  return stat;
}
/////////////////////////////////////////////////////////////
template <class T>
uint8_t Read_NVS_Val(const char *key, T &value)
/* read numeric data from NVS*/
{
  uint8_t stat = 0;
  // const uint16_t* p = (const uint16_t*)(const void*)&value;
  //  Handle will automatically close when going out of scope or when it's reset.
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &ret);
  if (ret != ESP_OK)
  {
    sprintf(Title, "Error (%s) opening READ NVS value handle for: %s!\n", esp_err_to_name(ret), key);
    tftmsgbx.dispMsg(Title, TFT_RED);
  }
  else
  {
    ret = handle->get_item(key, value);
    switch (ret)
    {
    case ESP_OK:
      stat = 1;
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      sprintf(Title, "%s value is not initialized yet!\n", key);
      tftmsgbx.dispMsg(Title, TFT_RED);
      break;
    default:
      sprintf(Title, "Error (%s) reading %s!\n", esp_err_to_name(ret), key);
      tftmsgbx.dispMsg(Title, TFT_RED);
      delay(3000);
    }
  }
  return stat;
}

/*Save data to NVS memory routines*/

uint8_t Write_NVS_Str(const char *key, char *value)
/* write string data to NVS */
{
  uint8_t stat = 0;
  //  Handle will automatically close when going out of scope or when it's reset.
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &ret);
  if (ret != ESP_OK)
  {
    // printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    sprintf(Title, "Error (%s) opening WRITE NVS string handle for: %s!\n", esp_err_to_name(ret), key);
    tftmsgbx.dispMsg(Title, TFT_RED);
  }
  else
  {

    ret = handle->set_string(key, value);
    switch (ret)
    {
    case ESP_OK:
      /* write operation worked; go ahead and /lock the data in */
      ret = handle->commit();
      if (ret != ESP_OK)
      {
        sprintf(Title, "Commit Failed (%s) on %s!\n", esp_err_to_name(ret), key);
        tftmsgbx.dispMsg(Title, TFT_RED);
      }
      else
      {
        /* exit point when everything works as it should */
        // sprintf(Title, "%s string saved %d characters\n", key, sizeof(&value));
        // tftmsgbx.dispMsg(Title, TFT_GREEN);
        // delay(3000);
        stat = 1;
        break;
      }
    default:
      sprintf(Title, "Error (%s) reading %s!\n", esp_err_to_name(ret), key);
      tftmsgbx.dispMsg(Title, TFT_RED);
      delay(3000);
    }
  }
  return stat;
}
/////////////////////////////////////////////////////////////

uint8_t Write_NVS_Val(const char *key, int value)
/* write numeric data to NVS; If the data is "stored",return with an exit status of "1" */
{
  uint8_t stat = 0;
  // const uint16_t* p = (const uint16_t*)(const void*)&value;
  //  Handle will automatically close when going out of scope or when it's reset.
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("storage", NVS_READWRITE, &ret);
  if (ret != ESP_OK)
  {
    sprintf(Title, "Error (%s) opening WRITE NVS value handle for: %s!\n", esp_err_to_name(ret), key);
    tftmsgbx.dispMsg(Title, TFT_RED);
  }
  else
  {
    ret = handle->set_item(key, value);
    switch (ret)
    {
    case ESP_OK:
      ret = handle->commit();
      if (ret != ESP_OK)
      {
        sprintf(Title, "Commit Failed (%s) on %s!\n", esp_err_to_name(ret), key);
        tftmsgbx.dispMsg(Title, TFT_RED);
      }
      else
        /* exit point when everything works as it should */
        stat = 1;
      break;
    default:
      sprintf(Title, "Error (%s) reading %s!\n", esp_err_to_name(ret), key);
      tftmsgbx.dispMsg(Title, TFT_RED);
      delay(3000);
    }
  }
  return stat;
}


