// Copyright (c) 2020 Guy Turcotte
//
// MIT License. Look at file licenses.txt for details.
/*20230310 added logitech K380 configure function keys command */
/*20230317 added Memory F3 & F4 function keys */
/*20230328 moved to GitHub*/
/*20230405 reconfigued project files to run both on Linux & Windows 10*/
#include "sdkconfig.h" //added for timer support
#include "globals.hpp"
#include "main.h"
#include "bt_keyboard.hpp"
#include <esp_log.h>
#include <iostream>

#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

//////////////////////////////
#include "TFT_eSPI.h" // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include <Stream.h>
#include "SetUpScrn.h" //added to support references to the setttings screen; used to configure CW keyboard user perferences
#include "TxtNtryBox.h"
// #define KEY GPIO_NUM_13    // LED connected to GPIO2
#define TFT_GREY 0x5AEB    // New colour

TFT_eSPI tft = TFT_eSPI(); // Invoke TFT Display library
//////////////////////////////
/*timer interrupt support*/
#include "esp_timer.h"
esp_timer_handle_t DotClk_hndl;
esp_timer_handle_t DsplTmr_hndl;
TimerHandle_t DisplayTmr;
/* END timer interrupt support          */
/**blackpill tftmsgbox spport*/
#include "TFTMsgBox.h"
#include "CWSndEngn.h"
/*Debug & backtrace analysis*/
uint8_t global_var;
/*To make these variables available to other files/section of this App, Moved the following to main.h*/

DF_t DFault;
int DeBug = 1; // Debug factory default setting; 0 => Debug "OFF"; 1 => Debug "ON"
char StrdTxt[20] = {'\0'};
/*Factory Default Settings*/
char RevDate[9] = "20230405";
char MyCall[10] = {'K', 'W', '4', 'K', 'D'};
char MemF2[80] = "VVV VVV TEST DE KW4KD";
char MemF3[80] = "CQ CQ CQ DE KW4KD KW4KD";
char MemF4[80] = "TU 73 ES GL";
esp_err_t ret;
char Title[120];
bool setupFlg = false;
bool EnDsplInt = true;
volatile int DotClkstate = 0;
volatile int CurHiWtrMrk = 0;
static const uint8_t state_que_len = 50;
static QueueHandle_t state_que;
TaskHandle_t HKdotclkHndl;
TFTMsgBox tftmsgbx(&tft, StrdTxt);
CWSNDENGN CWsndengn(&DotClk_hndl, &tft, &tftmsgbx);
/*                           */

BTKeyboard bt_keyboard(&tftmsgbx, &DFault);
TxtNtryBox txtntrybox(&DotClk_hndl, &tft);
extern "C"
{
  void app_main();
}
/*Template for keyboard entry handler function detailed futrther down in this file*/
void ProcsKeyEntry(uint8_t keyVal);
/*        Display Update timer ISR          */
//static void DsplTmr_callback(void *arg);
void DsplTmr_callback(TimerHandle_t  xtimer);
/*        DotClk timer ISR                 */
void IRAM_ATTR DotClk_ISR(void *arg);
/*low priority dotclock housekeeping task*/
//void HKcursor(void *parameter)
// {
//   while (1)
//   {
//     vTaskSuspend(NULL);
//     //xTimerStop(DisplayTmr, portMAX_DELAY);
//     //vTaskPrioritySet(NULL,5);
//     //tftmsgbx.IntrCrsr(DotClkstate); // Set Display Box CW Send Cursor as needed
//     //vTaskPrioritySet(NULL,1);
//     //xTimerStart(DisplayTmr,0);
//     // int HiWtrMrk = uxTaskGetStackHighWaterMark(NULL);
//     // if (HiWtrMrk > CurHiWtrMrk)
//     // {
//     //   CurHiWtrMrk = HiWtrMrk;
//     //   char temp[50];
//     //   sprintf(temp, "STACK: %d\n", CurHiWtrMrk);
//     //   printf(temp);
//     // }
//     // vTaskDelay(led_delay / portTICK_PERIOD_MS);
//   }
// }

void pairing_handler(uint32_t pid)
{
  sprintf(Title, "Please enter the following pairing code,\n");
  tftmsgbx.dispMsg(Title, TFT_WHITE);
  sprintf(Title, "followed with ENTER on your keyboard: \n");
  tftmsgbx.dispMsg(Title, TFT_WHITE);
  sprintf(Title, "%d \n", (int)pid);
  tftmsgbx.dispMsg(Title, TFT_BLUE);
}

void app_main()
{
  // Configure pin
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << KEY);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  digitalWrite(KEY, Key_Up); //key 'UP' state
/*Setup Software Display Refresh/Update timer*/
  DisplayTmr = xTimerCreate(
    "Display-Refresh-Timer",
    33/portTICK_PERIOD_MS,
    pdTRUE,
    (void *)1,
    DsplTmr_callback
  );
  state_que = xQueueCreate(state_que_len, sizeof(uint8_t));

  /*Place Dot Clock housekeeping task in RTOS*/
  // xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
  //           HKcursor,      // Function to be called
  //           "HKcursor",   // Name of task
  //           2048,           // Stack size (bytes in ESP32, words in FreeRTOS)
  //           NULL,           // Parameter to pass
  //           6,              // Task priority
  //           &HKdotclkHndl,           // Task handle
  //           (BaseType_t)0);       // Run on one core for demo purposes (ESP32 only)

  
  /*setup Hardware DOT Clock timer & link to timer ISR*/
  const esp_timer_create_args_t DotClk_args = {
      .callback = &DotClk_ISR,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_ISR,
      .name = "DotClck"};

  /*setup Display Update timer & link to it's ISR*/
  // const esp_timer_create_args_t DsplTmr_args = {
  //     .callback = &DsplTmr_callback,
  //     .arg = NULL,
  //     .dispatch_method = ESP_TIMER_ISR,
  //     .name = "DisplayTimer"};

   ESP_ERROR_CHECK(esp_timer_create(&DotClk_args, &DotClk_hndl));

  /* The timer has been created but is not running yet */
   //ESP_ERROR_CHECK(esp_timer_create(&DsplTmr_args, &DsplTmr_hndl));

  /* The timer has been created but is not running yet */
  static const char *TAG = "TimrIntr";
  // intr_matrix_set(xPortGetCoreID(), ETS_TG0_T0_LEVEL_INTR_SOURCE, 27);//dotclck
  intr_matrix_set(xPortGetCoreID(), ETS_TG0_T1_LEVEL_INTR_SOURCE, 26);//display

  ESP_LOGI(TAG, "Start DotClk interrupt Config");
  ESP_ERROR_CHECK(esp_intr_alloc(ETS_TG0_T1_LEVEL_INTR_SOURCE, ESP_INTR_FLAG_LEVEL3, DotClk_ISR, DotClk_args.arg, NULL));
  ESP_LOGI(TAG, "End DotClk interrupt Config");
  // ESP_LOGI(TAG, "Start Display Update timer interrupt Config");
  // ESP_ERROR_CHECK(esp_intr_alloc(ETS_TG0_T1_LEVEL_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1, DsplTmr_callback, DsplTmr_args.arg, NULL));
  // ESP_LOGI(TAG, "End Display Update timer interrupt Config");


  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  // ESP_INTR_ENABLE(27);//dotclock
  ESP_INTR_ENABLE(26);//display
  // KB_timer_init(TIMER_GROUP_0, TIMER_0, true, 600); //dotclock @ 20WPM or 60ms
  // KB_timer_init(TIMER_GROUP_0, TIMER_1, true, 333); //displayRefersh @ ~30 frames/updates per second
  /* Start the timer dotclock (timer)*/
  ESP_ERROR_CHECK(esp_timer_start_periodic(DotClk_hndl, 60000)); // 20WPM or 60ms
  /* Start the Display timer */
  //ESP_ERROR_CHECK(esp_timer_start_periodic(DsplTmr_hndl, 33330)); //~30 frames/updates per second
  xTimerStart(DisplayTmr, portMAX_DELAY);

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
  sprintf(Title, "     ESP32 BT CW Keyboard (%s)\n", RevDate); // sprintf(Title, "CPU CORE: %d\n", (int)xPortGetCoreID());
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
    DFault.DeBug = DeBug;
    DFault.WPM = CWsndengn.GetWPM();
    sprintf(Title, "\n        No stored USER params Found\n   Using FACTORY values until params are\n   'Saved' via the Settings Screen\n");
    tftmsgbx.dispMsg(Title, TFT_ORANGE);
  }
  else
  {
    /*found 'mycall' stored setting, go get the other user settings */
    Rstat = Read_NVS_Str("MemF2", DFault.MemF2);
    Rstat = Read_NVS_Str("MemF3", DFault.MemF3);
    Rstat = Read_NVS_Str("MemF4", DFault.MemF4);
    Rstat = Read_NVS_Val("DeBug", DFault.DeBug);
    Rstat = Read_NVS_Val("WPM", DFault.WPM);
  }
  CWsndengn.ShwWPM(DFault.WPM); //calling this method does both recalc/set the dotclock & show the WPM
//  KB_timer_event_t evt;
//  bool wait4event = true;
//  int cnt = 0;
//  while(wait4event && (cnt<20)){
//    wait4event = !xQueueReceive(s_timer_queue, &evt, portMAX_DELAY);
//    printf("DISPLAY\n");
//    cnt++;
//  }
  /*start bluetooth pairing/linking process*/
  if (bt_keyboard.setup(pairing_handler))
  {                             // Must be called once
    bt_keyboard.devices_scan(); // Required to discover new keyboards and for pairing
                                // Default duration is 5 seconds
    // sprintf(Title, "READY...\n");
    // tftmsgbx.dispMsg(Title, TFT_GREEN);
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
  /* main CW keyboard loop*/
  while (true)
  {
#if 1 // 0 = scan codes retrieval, 1 = augmented ASCII retrieval
    if (setupFlg)
    /*if true, exit main loop and jump to "settings" screen */
    {
      bool IntSOTstate = CWsndengn.GetSOTflg();
      if(IntSOTstate) CWsndengn.SOTmode();// do this to stop any outgoing txt that might currently be in progress before switching over to settings screen
      tftmsgbx.SaveSettings();                                       // true; user has pressed Ctl+S key, & wants to configure default settings
      setuploop(&tft, &CWsndengn, &tftmsgbx, &bt_keyboard, &DFault); // function defined in SetUpScrn.cpp/.h file
      tftmsgbx.ReBldDsplay();
      CWsndengn.RefreshWPM();
      if(IntSOTstate && !CWsndengn.GetSOTflg()) CWsndengn.SOTmode(); //Send On Type was enabled when we went to 'settings' so re-enable it 
    }
    
    uint8_t key = bt_keyboard.wait_for_ascii_char();
    EnDsplInt = false; //no longer need timer driven display refresh; As its now being handled in the wait for BT_keybrd entry loop "wait_for_low_event()"
        
    //ESP_ERROR_CHECK(esp_timer_delete(DsplTmr_hndl));
    // uint8_t key = bt_keyboard.get_ascii_char(); // Without waiting
    /*test key entry & process as needed*/
    if (key != 0)
      ProcsKeyEntry(key);

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
//static void DsplTmr_callback(void *arg)
void DsplTmr_callback(TimerHandle_t  xtimer)
{
  uint8_t state;
  BaseType_t TaskWoke;
  tftmsgbx.dispMsg2();
  //if(EnDsplInt)tftmsgbx.dispMsg2(); // update display
  while(xQueueReceiveFromISR(state_que,(void *)&state, &TaskWoke) ==pdTRUE) tftmsgbx.IntrCrsr(state);
  
}
/*                                          */
/* DotClk timer ISR                 */
void IRAM_ATTR DotClk_ISR(void *arg)
{
  BaseType_t Woke;
  uint8_t state = CWsndengn.Intr(); // check CW send Engine & process as code as needed
  if(xQueueSendFromISR(state_que, &state, &Woke) == pdFALSE)  printf("!!state QUEUE FULL!!");  ;
  if(Woke == pdTRUE) printf("!!WOKE!!");
  
}
///////////////////////////////////////////////////////////////////////////////////
// static bool IRAM_ATTR DotClock_isr_callback(void *args)
// {
//   BaseType_t high_task_awoken = pdFALSE;
//   KB_timer_info_t *Info = (KB_timer_info_t *)args;

//   uint64_t timer_counter_value =0;// = timer_group_get_counter_value_in_isr(Info->timer_group, Info->timer_idx);

//   /* Prepare basic event data that will be then sent back to task */
//   KB_timer_event_t evt;
//   // = {
//   evt.info.timer_group = Info->timer_group;
//   evt.info.timer_idx = Info->timer_idx;
//   evt.info.auto_reload = Info->auto_reload;
//   evt.info.alarm_interval = Info->alarm_interval;
//   evt.timer_counter_value = timer_counter_value;
//   //};

//   if (!Info->auto_reload)
//   {
//     timer_counter_value += Info->alarm_interval * TIMER_SCALE;
// //    timer_group_set_alarm_value_in_isr(Info->timer_group, Info->timer_idx, timer_counter_value);
//   }
//   // int state = CWsndengn.Intr(); // check CW send Engine & process as code as needed
//   // tftmsgbx.IntrCrsr(state);
//    printf("DOTCLCK\n");
//   /* Now just send the event data back to the main program task */
//   xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);

//   return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
// }

// static bool IRAM_ATTR Display_isr_callback(void *args)
// {
//     BaseType_t high_task_awoken = pdFALSE;
//     KB_timer_info_t *Info = (KB_timer_info_t *) args;

//     uint64_t timer_counter_value =0;// = timer_group_get_counter_value_in_isr(Info->timer_group, Info->timer_idx);

//     /* Prepare basic event data that will be then sent back to task */
//     KB_timer_event_t evt;
//     // = {
//         evt.info.timer_group = Info->timer_group;
//         evt.info.timer_idx = Info->timer_idx;
//         evt.info.auto_reload = Info->auto_reload;
//         evt.info.alarm_interval = Info->alarm_interval;
//         evt.timer_counter_value = timer_counter_value;
//     //};

//     if (!Info->auto_reload) {
//         timer_counter_value += Info->alarm_interval * TIMER_SCALE;
// //        timer_group_set_alarm_value_in_isr(Info->timer_group, Info->timer_idx, timer_counter_value);
//     }

//     // tftmsgbx.dispMsg2();
//      printf("DISPLAY\n");

//     /* Now just send the event data back to the main program task */
//     xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);

//     return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
// }


///////////////////////////////////////////////////////////////////////////////////
/*This routine checks the current key entry; 1st looking for special key values that signify special handling
if none are found, it hands off the key entry to be treated as a standard CW character*/
void ProcsKeyEntry(uint8_t keyVal)
{
  bool addspce = false;
  char SpcChr = char(0x20);
  // sprintf(Title, "%d\n", keyVal);
  // tftmsgbx.dispMsg(Title, TFT_GOLD);
  // return;
  if (keyVal == 0x8)
  { //"BACKSpace" key pressed
    // Serial.print(" Delete ");
    int ChrCnt = CWsndengn.Delete();
    if (ChrCnt > 0)
      tftmsgbx.Delete(ChrCnt);
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
    CWsndengn.SOTmode();
    return;
  }
  else if (keyVal == 0x81)
  { // F1 key (store TEXT)
    CWsndengn.StrTxtmode();
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
  { // F4 key (Send MemF3)
    if (CWsndengn.IsActv() && !CWsndengn.LstNtrySpc())
      CWsndengn.AddNewChar(&SpcChr);
    CWsndengn.LdMsg(DFault.MemF4, sizeof(DFault.MemF4));
    return;
  }
  else if (keyVal == 0x95)
  { // Right Arrow Key (Alternate action SOT [Send On type])
    CWsndengn.SOTmode();
    return;
  }
  else if (keyVal == 0x96)
  { // Left Arrow Key (store TEXT)
    CWsndengn.StrTxtmode();
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
  // const uint16_t* p = (const uint16_t*)(const void*)&value;
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
// template <class T>
// uint8_t Write_NVS_Val(const char *key, T& value)
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

// static void KB_timer_init(timer_group_t group, timer_idx_t timer, bool auto_reload, int timer_interval_sec)
// {
//     /* Select and initialize basic parameters of the timer */
//     timer_config_t config; // = {
//         config.divider = TIMER_DIVIDER;
//         config.counter_dir = TIMER_COUNT_UP;
//         config.counter_en = TIMER_PAUSE;
//         config.alarm_en = TIMER_ALARM_EN;
//         config.auto_reload = (timer_autoreload_t)auto_reload;
//         config.intr_type = TIMER_INTR_LEVEL;
//         config.clk_src = TIMER_SRC_CLK_APB;//GPTIMER_CLK_SRC_APB; //(80MHz)
//     //}; 
//     timer_init(group, timer, &config);

//     /* Timer's counter will initially start from value below.
//        Also, if auto_reload is set, this value will be automatically reload on alarm */
//     timer_set_counter_value(group, timer, 0);

//     /* Configure the alarm value and the interrupt on alarm. */
//     timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
//     timer_enable_intr(group, timer);

//     KB_timer_info_t timer_info; 
//     timer_info.timer_group = group;
//     timer_info.timer_idx = timer;
//     timer_info.auto_reload = auto_reload;
//     timer_info.alarm_interval = timer_interval_sec;
//     calloc(1, sizeof(KB_timer_info_t ));
//     if(timer ==0 ) timer_isr_callback_add(group, timer, DotClock_isr_callback, &timer_info, ESP_INTR_FLAG_LEVEL2 );
//     else timer_isr_callback_add(group, timer, Display_isr_callback, &timer_info, ESP_INTR_FLAG_LEVEL1 );
//     timer_start(group, timer);
// }
