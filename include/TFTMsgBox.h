/*
 * TFTMsgBox.h
 *
 *  Created on: Oct 7, 2021
 *      Author: jim
 */

#ifndef INC_TFTMSGBOX_H_
#define INC_TFTMSGBOX_H_
#include "stdint.h"// need this to use char type
#include "TFT_eSPI.h" 
#include "esp32-hal-delay.h" //included to support the arduino function 'delay(int ms)'

#define HiRes //uncomment for 480x340 screens
#define RingBufSz 400

class TFTMsgBox
{
private:
	char RingbufChar[RingBufSz];
	uint16_t RingbufClr[RingBufSz];
	bool BGHilite;
	
	char Msgbuf[50];
	char LastStatus[50];
	uint16_t LastStatClr;
	int txtpos;
	int RingbufPntr1;
	int RingbufPntr2;
	int StrdRBufPntr1;
	int StrdRBufPntr2;
#ifdef HiRes	
	#define fontsize 2 //& 320x480 screen*/
	#define CPL 40 //number of characters each line that a 3.5" screen can contain
	#define row 15 //number of usable rows on a 3.5" screen
	#define ScrnHght  320
	#define ScrnWdth  480
	#define FONTW 12
	#define FONTH 20 //16
	const int StatusX = 100;
#else
	#define fontsize 1 & 240x320 screen*/	
    #define CPL  40 //number of characters each line that a 2.8" screen can contain
	#define row  15 //number of usable rows on a 2.8" screen
	#define ScrnHght  240
	#define ScrnWdth  320
	#define FONTW = 8
	#define FONTH 14
	const int StatusX = 80;
#endif
	const int fontH = FONTH;
	const int fontW = FONTW;
	const int StatusY = ScrnHght-20;
	const int displayW = ScrnWdth;	
	/* F1/F12 Status Box position */
	const int StusBxX = 10 ;
	const int StusBxY = ScrnHght-30 ;
	char Pgbuf[CPL*row];
	char SpdBuf[50];
	uint16_t PgbufColor[CPL*row];
	uint16_t ToneColor;
	uint16_t SpdClr;
	int CursorPntr;
	int cursorY;
	int cursorX;
	int cnt; //used in scrollpage routine
	int curRow;
	int offset;
	int StrdPntr;
	int StrdY;
	int StrdX;
	int Strdcnt; //used in scrollpage routine
	int StrdcurRow;
	int Strdoffset;
	int BlkStateVal[5];
	int BlkStateCntr;
	int oldstate; //jmh added just for debugging cw highlight cursor position
	bool BlkState;
	bool SOTFlg;
	bool StrTxtFlg;
	bool ToneFlg;
	bool SpdFlg;
	bool Bump;
	bool PgScrld; //flag to indicate whether that the 'scroll' routine has ever run; i.e initially false. but always true once the process has
	bool CWActv;// flag controlled by CWsendEngn; lets this class know when the sendengn is between letters/characters. used to block page scrolling while in the middle of a letter
	TFT_eSPI *ptft;
	char *pStrdTxt;
	void scrollpg(void);
	void HiLiteBG(void);
	void RestoreBG(void);
	void PosHiLiteCusr(void);

public:
	TFTMsgBox( TFT_eSPI *tft_ptr, char *StrdTxt);
	void InitDsplay(void);
	void ReBldDsplay(void);
	void KBentry(char Ascii);
	void Delete(int ChrCnt);
	void dispMsg(char Msgbuf[50], uint16_t Color);
	void dispMsg2(void);
	void DisplCrLf(void);
	void IntrCrsr(int state);
	void dispStat(char Msgbuf[50], uint16_t Color);
	void showSpeed(char Msgbuf[50], uint16_t Color);
	void setSOTFlg(bool flg);
	void setStrTxtFlg(bool flg);
	void SaveSettings(void);
	void ReStrSettings(void);
	void setCWActv(bool flg);
	bool getBGHilite(void);
	void ShwTone(uint16_t color);
	// void DelLastNtry(void);
};




#endif /* INC_TFTMSGBOX_H_ */
