/*
 * SetUpScrn.h
 *
 *  Created on: Jun 8, 2022
 *      Author: jim
 */
/*
 * 20220824 void "GatherCurSettings()" routine to ensure all current settings actually saved
 */
#ifndef INC_SETUPSCRN_H_
#define INC_SETUPSCRN_H_

#include "TFT_eSPI.h"
#include "TxtNtryBox.h"
#include "CWSndEngn.h"
#include "bt_keyboard.hpp"
#include "main.h"

// #define BLACK   0x0000
// #define BLUE    0x001F
// #define RED     0xF800
// #define GREEN   0x07E0
// #define CYAN    0x07FF
// #define MAGENTA 0xF81F
// #define YELLOW  0xFFE0
// #define WHITE   0xFFFF

//#define paramCnt  6 // Total number of parameters/settings that can be edited on this screen

extern bool setupFlg;
extern int tchcnt;
extern int px; //mapped 'x' display touch value
extern int py;
extern int btnPrsdCnt;
//extern int DeBug;
extern bool buttonEnabled;
extern bool Test;
// extern bool TonPltFlg;
// extern bool AutoTune;
// extern bool NoiseSqlch;
extern bool SetScrnActv;
//extern char MyCall[];
//extern char MemF2[];

extern volatile unsigned long lastDit1;

void setuploop(TFT_eSPI *tft_ptr, CWSNDENGN *cwsnd_ptr, TFTMsgBox *msgbx_ptr, BTKeyboard *keyboard_ptr, DF_t *Dft_ptr); //Added for buttons support
void GatherCurSettings(struct BtnParams Mycallsettings, struct BtnParams MemF2settings, struct BtnParams MemF3settings, struct BtnParams MemF4settings, struct BtnParams MemF5settings,  struct BtnParams WPMsettings, struct BtnParams DBugsettings);
void ShwData(int MsgX, int MsgY, char* TxtMsg); //Added for buttons support
void BldBtn(int BtnNo, BtnParams Btns[]); //Added for buttons support
bool BtnActive(int BtnNo, BtnParams Btns[], int px, int py); //Added for buttons support
void enableDisplay(void); //JMH Needs fixing; old Arduino call
void PlotIfNeed2(void);
void readResistiveTouch(void);//Added for buttons support
void SetDBgCptn(char *textMsg); //Added for buttons support
void SetPrgmCptn(char *textMsg); //Added to tract/show program option
void RptActvBtn(char msg[]); //Added for buttons support
int ReadBtns(void);
void BldButtons(void);
void ShwUsrParams(void);
void LdFactryVals(void);
void SaveUsrVals(void);

#endif /* INC_SETUPSCRN_H_ */
