/*
 * SetUpScrn.cpp
 *
 *  Created on: Jun 8, 2022
 *      Author: jim
 */

/*
 * 20220824 Fixed error in "Save" to EEPROM process to ensure all current settings actually saved
 / 20230313 added line of code to ensure text entries behave as WYSWYG entries;when it comes going from the settings screen back to the 'main' keyboard screen
 / 20230507 Reversed bavior of "Up" & "Down" Arrow keys
 */


/* Includes ------------------------------------------------------------------*/
// #include <eeprom.h>
#include "esp_timer.h"
#include "SetUpScrn.h"
#include "esp32-hal-delay.h"
#include "main.h"
//#include "Goertzel.h"
#include "DcodeCW.h"



uint16_t KBntry; // used by SetUpScrn.cpp to handle Keyboard entries while user is operating in settings mode
// int px = 0; //mapped 'x' display touch value
// int py = 0;
///*TODO "touch-screen" references need to go away*/
// const int XP=PB7, XM=PA6, YP=PA7, YM=PB6;
// const int TS_LEFT=634,TS_RT=399,TS_TOP=492,TS_BOT=522;
// TouchScreen_kbv ts(XP, YP, XM, YM, 300);
// TSPoint_kbv tp;//TODO Needs to go away
// int PrgmNdx = 0;//TODO Needs to go away

int16_t scrnHeight;
int16_t scrnWidth;

/*  Added for buttons support */
char textMsg[51];
int tchcnt = 0;			   // needed global variable for read/detect button support
bool BlkIntrpt = false;	   // needed global variable for read/detect button support
int btnPrsdCnt = 0;		   // needed global variable for read/detect button support
const int fontH = 16;	   // needed global variable for read/detect button support
const int fontW = 12;	   // needed global variable for read/detect button support
bool buttonEnabled = true; // needed global variable for read/detect button support
int value;				   // needed global variable for debugging button support
bool FrstTime = true;	   // needed global variable for readResistiveTouch() function
/* Begin Button Definitions */
int loopcnt;
int btnHght = 40;
int btnWdthS = 80;
int btnWdthL = 2 * btnWdthS;

char BtnCaptn[14];

const int paramCnt = 9; // Total number of parameters/settings that can be edited on this screen

BtnParams SetUpBtns[paramCnt]; // currently the Setup screen has 8 buttons

bool SetScrnActv;


/* create an array of eight ntry box class objects; i.e. the user number of settings that can be configured in this application*/
TxtNtryBox NtryBoxGrp[]{
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft),
	TxtNtryBox(&DotClk_hndl, &tft)};

CWSNDENGN *pcwsnd;
// TFT_eSPI  *ptft;
TFTMsgBox *ptftmsgbx;
BTKeyboard *pbt_keyboard;
DF_t *pDFault;

/* End Button Definitions */


/* Setup screen entry point & loop*/
void setuploop(TFT_eSPI *tft_ptr, CWSNDENGN *cwsnd_ptr, TFTMsgBox *msgbx_ptr, BTKeyboard *keyboard_ptr, DF_t *Dft_ptr)
{
	pDFault = Dft_ptr;
	pcwsnd = cwsnd_ptr;
	ptftmsgbx = msgbx_ptr;
	pbt_keyboard = keyboard_ptr;
	uint8_t CurFntSz = tft_ptr->textsize;
	scrnHeight = tft_ptr->height();
	scrnWidth = tft_ptr->width();
	tft_ptr->fillScreen(TFT_BLACK);
	tft_ptr->fillScreen(TFT_BLACK); // need to call this twice, to ensure full erasure of old display
									//	const int paramCnt = 6;// Total number of parameters/settings that can be edited on this screen
									//	TxtNtryBox NtryBoxGrp[paramCnt];
	//int BtnIndx = 0;
	const char CallBtnCaptn[9] = {'M', 'y', ' ', 'C', 'a', 'l', 'l', ':'};
	const char *CaptnPtr = CallBtnCaptn;
	struct BtnParams MycallBxsettings;
	int curRow = 0;
	const int MyCallIndx = 0;
	MycallBxsettings.BtnXpos = 0;	   // Button X position
	MycallBxsettings.BtnWdth = 80;	   // Button Width
	MycallBxsettings.BtnYpos = curRow; // Button X position
	MycallBxsettings.BtnHght = 15;	   // Button Height
	MycallBxsettings.Captn = CaptnPtr;
	MycallBxsettings.BtnClr = TFT_GREEN;
	MycallBxsettings.TxtClr = TFT_WHITE;
	MycallBxsettings.IsNumbr = false;
	MycallBxsettings.BxIndx = MyCallIndx;
	NtryBoxGrp[MycallBxsettings.BxIndx].InitBox(MycallBxsettings);
	NtryBoxGrp[MycallBxsettings.BxIndx].SetValue(pDFault->MyCall, sizeof(pDFault->MyCall));

	const char WPMBtnCaptn[5] = {'W', 'P', 'M', ':'};
	const char *WPMCaptnPtr = WPMBtnCaptn;
	struct BtnParams WPMBxsettings;
	curRow += fontH + 6;
	const int WPMIndx = 1;
	WPMBxsettings.BtnXpos = 0;		// Button X position
	WPMBxsettings.BtnWdth = 80;		// Button Width
	WPMBxsettings.BtnYpos = curRow; // Button X position
	WPMBxsettings.BtnHght = 15;		// Button Height
	WPMBxsettings.Captn = WPMCaptnPtr;
	WPMBxsettings.BtnClr = TFT_GREEN;
	WPMBxsettings.TxtClr = TFT_WHITE;
	WPMBxsettings.IsNumbr = true;
	WPMBxsettings.BxIndx = WPMIndx;

	/*Convert WPM int parameter to character*/
	char WPM_char[2 + sizeof(char)];
	sprintf(WPM_char, "%d", pDFault->WPM);
	NtryBoxGrp[WPMBxsettings.BxIndx].InitBox(WPMBxsettings);
	NtryBoxGrp[WPMBxsettings.BxIndx].SetValue(WPM_char, sizeof(WPM_char));

	const char MemF2BtnCaptn[11] = {'M', 'e', 'm', 'o', 'r', 'y', ' ', 'F', '2', ':'};
	const char *MF2CaptnPtr = MemF2BtnCaptn;
	struct BtnParams MemF2Bxsettings;
	curRow += fontH + 6;
	const int MemF2Indx = 2;
	MemF2Bxsettings.BtnXpos = 0;	  // Button X position
	MemF2Bxsettings.BtnWdth = 80;	  // Button Width
	MemF2Bxsettings.BtnYpos = curRow; // Button X position
	MemF2Bxsettings.BtnHght = 15;	  // Button Height
	MemF2Bxsettings.Captn = MF2CaptnPtr;
	MemF2Bxsettings.BtnClr = TFT_GREEN;
	MemF2Bxsettings.TxtClr = TFT_WHITE;
	MemF2Bxsettings.IsNumbr = false;
	MemF2Bxsettings.BxIndx = MemF2Indx;
	NtryBoxGrp[MemF2Bxsettings.BxIndx].InitBox(MemF2Bxsettings);
	NtryBoxGrp[MemF2Bxsettings.BxIndx].SetValue(pDFault->MemF2, sizeof(pDFault->MemF2));

	const char MemF3BtnCaptn[11] = {'M', 'e', 'm', 'o', 'r', 'y', ' ', 'F', '3', ':'};
	const char *MF3CaptnPtr = MemF3BtnCaptn;
	struct BtnParams MemF3Bxsettings;
	curRow += 2 * (fontH + 6);
	const int MemF3Indx = 3;
	MemF3Bxsettings.BtnXpos = 0;	  // Button X position
	MemF3Bxsettings.BtnWdth = 80;	  // Button Width
	MemF3Bxsettings.BtnYpos = curRow; // Button X position
	MemF3Bxsettings.BtnHght = 15;	  // Button Height
	MemF3Bxsettings.Captn = MF3CaptnPtr;
	MemF3Bxsettings.BtnClr = TFT_GREEN;
	MemF3Bxsettings.TxtClr = TFT_WHITE;
	MemF3Bxsettings.IsNumbr = false;
	MemF3Bxsettings.BxIndx = MemF3Indx;
	NtryBoxGrp[MemF3Bxsettings.BxIndx].InitBox(MemF3Bxsettings);
	NtryBoxGrp[MemF3Bxsettings.BxIndx].SetValue(pDFault->MemF3, sizeof(pDFault->MemF3));

	const char MemF4BtnCaptn[11] = {'M', 'e', 'm', 'o', 'r', 'y', ' ', 'F', '4', ':'};
	const char *MF4CaptnPtr = MemF4BtnCaptn;
	struct BtnParams MemF4Bxsettings;
	curRow += 2 * (fontH + 6);
	const int MemF4Indx = 4;
	MemF4Bxsettings.BtnXpos = 0;	  // Button X position
	MemF4Bxsettings.BtnWdth = 80;	  // Button Width
	MemF4Bxsettings.BtnYpos = curRow; // Button X position
	MemF4Bxsettings.BtnHght = 15;	  // Button Height
	MemF4Bxsettings.Captn = MF4CaptnPtr;
	MemF4Bxsettings.BtnClr = TFT_GREEN;
	MemF4Bxsettings.TxtClr = TFT_WHITE;
	MemF4Bxsettings.IsNumbr = false;
	MemF4Bxsettings.BxIndx = MemF4Indx;
	NtryBoxGrp[MemF4Bxsettings.BxIndx].InitBox(MemF4Bxsettings);
	NtryBoxGrp[MemF4Bxsettings.BxIndx].SetValue(pDFault->MemF4, sizeof(pDFault->MemF4));

	const char MemF5BtnCaptn[11] = {'M', 'e', 'm', 'o', 'r', 'y', ' ', 'F', '5', ':'};
	const char *MF5CaptnPtr = MemF5BtnCaptn;
	struct BtnParams MemF5Bxsettings;
	curRow += 2 * (fontH + 6);
	const int MemF5Indx = 5;
	MemF5Bxsettings.BtnXpos = 0;	  // Button X position
	MemF5Bxsettings.BtnWdth = 80;	  // Button Width
	MemF5Bxsettings.BtnYpos = curRow; // Button X position
	MemF5Bxsettings.BtnHght = 15;	  // Button Height
	MemF5Bxsettings.Captn = MF5CaptnPtr;
	MemF5Bxsettings.BtnClr = TFT_GREEN;
	MemF5Bxsettings.TxtClr = TFT_WHITE;
	MemF5Bxsettings.IsNumbr = false;
	MemF5Bxsettings.BxIndx = MemF5Indx;
	NtryBoxGrp[MemF5Bxsettings.BxIndx].InitBox(MemF5Bxsettings);
	NtryBoxGrp[MemF5Bxsettings.BxIndx].SetValue(pDFault->MemF5, sizeof(pDFault->MemF5));		

	const char DBugOFFBtnCaptn[10] = {'D', 'E', 'B', 'U', 'G', ' ', 'O', 'F', 'F'};
	const char DBugONBtnCaptn[10] = {'D', 'E', 'B', 'U', 'G', ' ', 'O', 'N'};
	const char *DBugCaptnPtr[2];
	DBugCaptnPtr[0] = DBugOFFBtnCaptn;
	DBugCaptnPtr[1] = DBugONBtnCaptn;
	struct BtnParams DBugBxsettings;
	curRow += 2 * (fontH + 6);
	int BtnWdth = 125;
	const int DbugIndx = 6;
	DBugBxsettings.BtnXpos = 0 * BtnWdth; // Button X position
	DBugBxsettings.BtnWdth = BtnWdth;	  // Button Width
	DBugBxsettings.BtnYpos = curRow;	  // Button X position
	DBugBxsettings.BtnHght = 38;		  // Button Height
	DBugBxsettings.Captn = DBugCaptnPtr[pDFault->DeBug];
	DBugBxsettings.BtnClr = TFT_GREEN;
	DBugBxsettings.TxtClr = TFT_WHITE;
	DBugBxsettings.IsNumbr = false;
	DBugBxsettings.BxIndx = DbugIndx;
	DBugBxsettings.Option = pDFault->DeBug;
	NtryBoxGrp[DBugBxsettings.BxIndx].InitBox(DBugBxsettings);
	// char Null[] = {' '};
	char Null[150]; // set a character place holder bigger than any text setting we will changing
	NtryBoxGrp[DBugBxsettings.BxIndx].SetValue(Null, 0);

	BtnWdth = 70;
	const char SaveBtnCaptn[5] = {'S', 'A', 'V', 'E'};
	const char *SaveCaptnPtr = SaveBtnCaptn;
	struct BtnParams SaveBxsettings;
	curRow += fontH + 24;
	const int SaveIndx = 7;
	SaveBxsettings.BtnXpos = 0 * BtnWdth; // Button X position
	SaveBxsettings.BtnWdth = BtnWdth;	  // Button Width
	SaveBxsettings.BtnYpos = curRow;	  // Button X position
	SaveBxsettings.BtnHght = 38;		  // Button Height
	SaveBxsettings.Captn = SaveCaptnPtr;
	SaveBxsettings.BtnClr = TFT_GREEN;
	SaveBxsettings.TxtClr = TFT_WHITE;
	SaveBxsettings.IsNumbr = false;
	SaveBxsettings.BxIndx = SaveIndx;
	NtryBoxGrp[SaveBxsettings.BxIndx].InitBox(SaveBxsettings);
	NtryBoxGrp[SaveBxsettings.BxIndx].SetValue(Null, 0);

	const char ExitBtnCaptn[5] = {'E', 'X', 'I', 'T'};
	const char *ExitCaptnPtr = ExitBtnCaptn;
	struct BtnParams ExitBxsettings;
	const int ExitIndx =  8;
	ExitBxsettings.BtnXpos = 1 * BtnWdth; // Button X position
	ExitBxsettings.BtnWdth = BtnWdth;	  // Button Width
	ExitBxsettings.BtnYpos = curRow;	  // Button X position
	ExitBxsettings.BtnHght = 38;		  // Button Height
	ExitBxsettings.Captn = ExitCaptnPtr;
	ExitBxsettings.BtnClr = TFT_GREEN;
	ExitBxsettings.TxtClr = TFT_WHITE;
	ExitBxsettings.IsNumbr = false;
	ExitBxsettings.BxIndx = ExitIndx;
	NtryBoxGrp[ExitBxsettings.BxIndx].InitBox(ExitBxsettings);
	NtryBoxGrp[ExitBxsettings.BxIndx].SetValue(Null, 0);
	SetScrnActv = true;
	int NdxPtr = 0;	  // pointer to the active character in the Value array for the parameter currently in "FOCUS"
	int paramPtr = 0; // Pointer to current parameter that has "FOCUS"
	/*highlight 1st setting*/
	NtryBoxGrp[paramPtr].ShwCsr(0);
	//NdxPtr = 0; // for text based settings move the pointer to the 1st character in the string
	bool FocusChngd = false;
	while (setupFlg)
	{ // run inside this loop until user exits setup mode
		uint8_t KBntry = pbt_keyboard->wait_for_ascii_char();
		/*Test if user wants to change/move the "focus" to another parameter/setting*/
		if ((KBntry == 0x9C))
		{ // Cntrl+"S"
			setupFlg = false;
			KBntry = 0;
		}
		else if (KBntry == 0x98)
		{ // Arrow UP
			KBntry = 0;
			NtryBoxGrp[paramPtr].KillCsr();
			paramPtr--;
			FocusChngd = true;
		}
		else if (KBntry == 0x97)
		{ // Arrow DOWN
			KBntry = 0;
			NtryBoxGrp[paramPtr].KillCsr();
			paramPtr++;
			FocusChngd = true;
		}
		if (paramPtr ==  paramCnt)
			paramPtr = 0;
		if (paramPtr < 0)
			paramPtr = paramCnt - 1;
		/*Now that we know the selected setting, configure things so that it can be manipulated*/
		int len = NtryBoxGrp[paramPtr].GetValLen();
		char *value = Null;
		if (len > 0)
			value = NtryBoxGrp[paramPtr].GetValue();

		if (FocusChngd)
		{
			FocusChngd = false;
			NtryBoxGrp[paramPtr].ShwCsr(0);
			NdxPtr = 0; // for text based settings move the pointer to the 1st character in the string
		}
		if (KBntry == 0x95)
		{ // Right Arrow Key => Move Cursor Right
			KBntry = 0;
			switch (NtryBoxGrp[paramPtr].BxIndx)
			{
			case DbugIndx:
				//"DeBug" button has Focus
				NtryBoxGrp[DBugBxsettings.BxIndx].Option = pDFault->DeBug;
				NtryBoxGrp[DBugBxsettings.BxIndx].UpDateCaptn((char *)DBugCaptnPtr[pDFault->DeBug]);
				tft_ptr->setTextSize(CurFntSz);
				break;
			case ExitIndx:
				break;
			case SaveIndx:
				break;
			default:
				NdxPtr++;
				/*Test to ensure we haven't gone beyond the text space allocated for this setting*/
				if (NdxPtr == NtryBoxGrp[paramPtr].GetValLen())
					NdxPtr--;
				/*the following two cases should never happen, but check/fix anyway */
				if (value[NdxPtr] == 0)
				{
					NdxPtr--;
				}
				if (NdxPtr < 0)
					NdxPtr = 0;
				NtryBoxGrp[paramPtr].ShwCsr(NdxPtr);
				break;
			}
		}
		else if (KBntry == 0x96)
		{ // Left Arrow Key  => Move Cursor Left
			KBntry = 0;
			if (NtryBoxGrp[paramPtr].BxIndx == DBugBxsettings.BxIndx)
			{ //"DeBug" button has Focus
				/*Set DeBug to OFF*/
				pDFault->DeBug = 0;
				NtryBoxGrp[DBugBxsettings.BxIndx].Option = pDFault->DeBug;
				NtryBoxGrp[DBugBxsettings.BxIndx].UpDateCaptn((char *)DBugCaptnPtr[pDFault->DeBug]);
				tft_ptr->setTextSize(CurFntSz);
			}
			else
			{
				NdxPtr--;
				if (NdxPtr < 0)
					NdxPtr++;
				NtryBoxGrp[paramPtr].ShwCsr(NdxPtr);
			}
		}
		else if ((KBntry == 0xD))
		{ // "ENTER" Key
			KBntry = 0;
			if (NtryBoxGrp[paramPtr].GetValLen() == 0)
			{ // len ==0; this a "choice" parameter & user wants to "select" the current "option"
				switch (NtryBoxGrp[paramPtr].BxIndx)
				{
				case DbugIndx: //"DeBug" button has Focus
					/*Swap DeBug state (ON to OFF; OFF to ON)*/
					if (pDFault->DeBug)
						pDFault->DeBug = 0;
					else
						pDFault->DeBug = 1;
					NtryBoxGrp[DBugBxsettings.BxIndx].Option = pDFault->DeBug;
					NtryBoxGrp[DBugBxsettings.BxIndx].UpDateCaptn((char *)DBugCaptnPtr[pDFault->DeBug]);
					tft_ptr->setTextSize(CurFntSz);
					break;
				case ExitIndx: //"Exit" button
					setupFlg = false;
					break;
				case SaveIndx: //"Save" button
					// Save current Parameters to EEPROM;
					uint8_t oldSZ = tft_ptr->textsize;
					NtryBoxGrp[paramPtr].ShwBGColor(TFT_BLUE);
					tft_ptr->setTextSize(CurFntSz);
					GatherCurSettings(MycallBxsettings, MemF2Bxsettings, MemF3Bxsettings, MemF4Bxsettings, MemF5Bxsettings, WPMBxsettings, DBugBxsettings);
					SaveUsrVals();
					delay(500);
					tft_ptr->setTextSize(oldSZ);
					NtryBoxGrp[paramPtr].ShwCsr(0);
					break;
				}
			}
			else
			{ // process "Enter" key as a "text" parameter input
				/*zero out (null out) any residual text beyond the currrent cursor position*/
				for (int i = NdxPtr; i < len; i++)
				{
					value[i] = 0; // MyCall[i] = 0;
				}
				NdxPtr = 0;
				NtryBoxGrp[paramPtr].SetValue(value, len); // reload/update text entered for this setting
				NtryBoxGrp[paramPtr].ShwCsr(NdxPtr);	   // refresh the selected setting on the settings screen, and place the cusror at the 1st character in the text
				tft_ptr->setTextSize(CurFntSz);
			}
		}
		else if ((KBntry == 0x8))
		{ // "BACKSpace" Key
			KBntry = 0;
			if (NdxPtr > 0)
			{ // Skip if Pointer already at 0 position
				NdxPtr--;
				/*replace character at the current position by moving the
				 * remaining characters right of this position one place left.*/
				for (int i = NdxPtr; i < (len - 1); i++)
				{
					value[i] = value[i + 1];
					NtryBoxGrp[paramPtr].UpDateValue(i, NdxPtr);
				}
				if (value[len - 1] != 0)
				{
					value[len - 1] = 0;
					NtryBoxGrp[paramPtr].UpDateValue(len - 1, NdxPtr);
				}
			}
		}
		else if ((KBntry == 0x2A))
		{ // "DELETE" Key
			KBntry = 0;

			for (int i = NdxPtr; i < (len - 1); i++)
			{ // delete character at the current position by moving the remaining characters right of this position one place left.
				value[i] = value[i + 1];
				NtryBoxGrp[paramPtr].UpDateValue(i, NdxPtr);
			}
			/*Now check test special case where array is completely full*/
			if (value[len - 1] != 0)
			{
				value[len - 1] = 0;
				NtryBoxGrp[paramPtr].UpDateValue(len - 1, NdxPtr);
			}
		}
		if (KBntry > 0)
		{ // looks like a text entry/update to one of the user settings
			if ((KBntry >= 97) & (KBntry <= 122))
			{
				KBntry = KBntry - 32;
			}
			char AsciiVal = (char)KBntry;
			/*test if current parameter is a numeric box/button*/
			if (NtryBoxGrp[paramPtr].IsNumbr)
			{
				if ((AsciiVal >= '0') && (AsciiVal <= '9'))
				{
					if (NdxPtr < NtryBoxGrp[paramPtr].ValueLen)
						value[NdxPtr] = AsciiVal;
					NdxPtr++;
					if (NdxPtr >= (NtryBoxGrp[paramPtr].ValueLen - 1))
					{
						NdxPtr--;
						NtryBoxGrp[paramPtr].UpDateValue(NdxPtr, NdxPtr);
					}
					else
						NtryBoxGrp[paramPtr].UpDateValue(NdxPtr - 1, NdxPtr);
				}
			}
			else //treat this entry as a text entry to a string (MyCall, F2 menory, F3 menory, F4 menory)
			{
				int charPos = NdxPtr;
				value[charPos] = AsciiVal; // insert new key entry into this parameter's value array
				NdxPtr++;
				if (NdxPtr == NtryBoxGrp[paramPtr].GetValLen())
					NdxPtr--;
				NtryBoxGrp[paramPtr].SetValue(value, NtryBoxGrp[paramPtr].GetValLen());//JMH 20230313 refresh the current "saved" entry	
				NtryBoxGrp[paramPtr].UpDateValue(charPos, NdxPtr); // refresh the settings screen
				
			}
			KBntry = 0; // need this here; Otherwise on next while loop pass "KBntry" could evaluate to a special character; i.e. "up arrow"
		}
	} // End While Loop
	tft_ptr->setTextSize(CurFntSz);
	GatherCurSettings(MycallBxsettings, MemF2Bxsettings, MemF3Bxsettings, MemF4Bxsettings, MemF5Bxsettings, WPMBxsettings, DBugBxsettings);

	return;
} // end of SetUp Loop

//////////////////////////////////////////////////////////////////////////
void GatherCurSettings(struct BtnParams Mycallsettings,
	 struct BtnParams MemF2settings,
	 struct BtnParams MemF3settings,
	 struct BtnParams MemF4settings,
	 struct BtnParams MemF5settings,
	 struct BtnParams WPMsettings,
	 struct BtnParams DBugsettings)
{
	// Transfer modified button values back to their respective variables/parameters
	/* Collect current MyCall value from Parameters/setting Screen */
	// char buf[80];
	/* Collect current F2Mem value from Parameters/setting Screen */
	char *value = NtryBoxGrp[MemF2settings.BxIndx].GetValue();
	int len = NtryBoxGrp[MemF2settings.BxIndx].GetValLen();
	int i;
	bool zeroFlg = false;
	/*Xfer current MemF2 setting to main 'default' & zero/null out all residual characters beyond the current cursor position*/
	for (i = 0; i < len; i++)
	{
		if (value[i] == 0)
			zeroFlg = true;
		else
			pDFault->MemF2[i] = value[i];
		if (zeroFlg)
		{
			value[i] = 0;
			pDFault->MemF2[i] = 0;
		}
	}
	/* Collect current F3Mem value from Parameters/setting Screen */
	value = NtryBoxGrp[MemF3settings.BxIndx].GetValue();
	len = NtryBoxGrp[MemF3settings.BxIndx].GetValLen();
	//int i;
	zeroFlg = false;
	/*Xfer current MemF3 setting to main 'default' & zero/null out all residual characters beyond the current cursor position*/
	for (i = 0; i < len; i++)
	{
		if (value[i] == 0)
			zeroFlg = true;
		else
			pDFault->MemF3[i] = value[i];
		if (zeroFlg)
		{
			value[i] = 0;
			pDFault->MemF3[i] = 0;
		}
	}

	/* Collect current F4Mem value from Parameters/setting Screen */
	value = NtryBoxGrp[MemF4settings.BxIndx].GetValue();
	len = NtryBoxGrp[MemF4settings.BxIndx].GetValLen();
	//int i;
	zeroFlg = false;
	/*Xfer current MemF4 setting to main 'default' & zero/null out all residual characters beyond the current cursor position*/
	for (i = 0; i < len; i++)
	{
		if (value[i] == 0)
			zeroFlg = true;
		else
			pDFault->MemF4[i] = value[i];
		if (zeroFlg)
		{
			value[i] = 0;
			pDFault->MemF4[i] = 0;
		}
	}

	/* Collect current F5Mem value from Parameters/setting Screen */
	value = NtryBoxGrp[MemF5settings.BxIndx].GetValue();
	len = NtryBoxGrp[MemF5settings.BxIndx].GetValLen();
	//int i;
	zeroFlg = false;
	/*Xfer current MemF5 setting to main 'default' & zero/null out all residual characters beyond the current cursor position*/
	for (i = 0; i < len; i++)
	{
		if (value[i] == 0)
			zeroFlg = true;
		else
			pDFault->MemF5[i] = value[i];
		if (zeroFlg)
		{
			value[i] = 0;
			pDFault->MemF5[i] = 0;
		}
	}

	/* Collect current MyCall setting/value from Parameters/setting Screen */
	value = NtryBoxGrp[Mycallsettings.BxIndx].GetValue();
	len = NtryBoxGrp[Mycallsettings.BxIndx].GetValLen();
	zeroFlg = false;
	/*Xfer current MyCall setting to main "default' & zero/null out all residual characters beyond the current cursor position*/
	for (i = 0; i < len; i++)
	{
		if (value[i] == 0)
			zeroFlg = true;
		else
			pDFault->MyCall[i] = value[i];
		if (zeroFlg)
		{
			value[i] = 0;
			pDFault->MyCall[i] = 0;
		}
	}

	/* Collect current WPM value from Parameters/setting Screen */
	value = NtryBoxGrp[WPMsettings.BxIndx].GetValue();
	len = NtryBoxGrp[WPMsettings.BxIndx].GetValLen();
	/*recover WPM value (an integer from a character array*/
	int intVal = 0;
	for (int i = 0; i < len; i++)
	{
		if (value[i] > 0)
			intVal = (i * 10 * intVal) + (value[i] - '0');
	}
	pDFault->WPM = intVal;
	pcwsnd->SetWPM(pDFault->WPM);

	/* Collect current Debug option/mode from Parameters/setting Screen */
	pDFault->DeBug = NtryBoxGrp[DBugsettings.BxIndx].Option;
	DeBug = pDFault->DeBug;
	return;
}
//////////////////////////////////////////////////////////////////////////
void LdFactryVals(void)
{
	for (int i = 0; i < sizeof(MyCall); i++)
	{
		pDFault->MyCall[i] = MyCall[i];
	}
	pDFault->WPM = pcwsnd->GetWPM();
	pDFault->DeBug = DeBug;
}
//////////////////////////////////////////////////////////////////////////
void SaveUsrVals(void)
{
	/* Unlock the Flash Program Erase controller */
	char buf[30];
	bool GudFlg = true;
	char call[10];
	char mem[80];
	int i;
	uint8_t Rstat;
	bool zeroFlg = false;
	/*Set up to save MyCall*/
	for (i = 0; i < sizeof(call); i++)
	{
		if (zeroFlg)
		{
			call[i] = 0;
		}
		else
		{
			if (pDFault->MyCall[i] == 0)
			{
				zeroFlg = true;
				call[i] = 0;
			}
			else
				call[i] = pDFault->MyCall[i];
		}
	}
	Rstat = Write_NVS_Str("MyCall", call);
	if (Rstat != 1)
		GudFlg = false;
	/*Set up to save MemF2*/	
	zeroFlg = false;
	for (i = 0; i < sizeof(mem); i++)
	{
		if (zeroFlg)
		{
			mem[i] = 0;
		}
		else
		{
			if (pDFault->MemF2[i] == 0)
			{
				zeroFlg = true;
				mem[i] = 0;
			}
			else
				mem[i] = pDFault->MemF2[i];
		}
	}
	Rstat = Write_NVS_Str("MemF2", mem);
	if (Rstat != 1)
		GudFlg = false;
	/*Set up to save MemF3*/
	zeroFlg = false;
	for (i = 0; i < sizeof(mem); i++)
	{
		if (zeroFlg)
		{
			mem[i] = 0;
		}
		else
		{
			if (pDFault->MemF3[i] == 0)
			{
				zeroFlg = true;
				mem[i] = 0;
			}
			else
				mem[i] = pDFault->MemF3[i];
		}
	}
	Rstat = Write_NVS_Str("MemF3", mem);
	if (Rstat != 1)
		GudFlg = false;

	/*Set up to save MemF4*/
	zeroFlg = false;
	for (i = 0; i < sizeof(mem); i++)
	{
		if (zeroFlg)
		{
			mem[i] = 0;
		}
		else
		{
			if (pDFault->MemF4[i] == 0)
			{
				zeroFlg = true;
				mem[i] = 0;
			}
			else
				mem[i] = pDFault->MemF4[i];
		}
	}
	Rstat = Write_NVS_Str("MemF4", mem);
	if (Rstat != 1)
		GudFlg = false;

	/*Set up to save MemF5*/
	zeroFlg = false;
	for (i = 0; i < sizeof(mem); i++)
	{
		if (zeroFlg)
		{
			mem[i] = 0;
		}
		else
		{
			if (pDFault->MemF5[i] == 0)
			{
				zeroFlg = true;
				mem[i] = 0;
			}
			else
				mem[i] = pDFault->MemF5[i];
		}
	}
	Rstat = Write_NVS_Str("MemF5", mem);
	if (Rstat != 1)
		GudFlg = false;

	/*Save Debug Setting*/
	Rstat = Write_NVS_Val("DeBug", pDFault->DeBug);
	if (Rstat != 1)
		GudFlg = false;
	/*Save WPM Setting*/	
	Rstat = Write_NVS_Val("WPM", pDFault->WPM);
	if (Rstat != 1)
		GudFlg = false;
	/* Save current Decoder ModeCnt value */
	Rstat = Write_NVS_Val("ModeCnt", pDFault->ModeCnt);
	if (Rstat != 1)
		GudFlg = false;
    /* Save current Decoder AutoTune value */
	Rstat = Write_NVS_Val("AutoTune", (int)pDFault->AutoTune);
	if (Rstat != 1)
		GudFlg = false;
	/* Save current Decoder SlwFlg value */
	Rstat = Write_NVS_Val("SlwFlg", (int)pDFault->SlwFlg);
	if (Rstat != 1)
		GudFlg = false;
	/* Save current Decoder TARGET_FREQUENCYC value; Note pDFault->TRGT_FREQ was last updated in DcodeCW.showSpeed(void)  */
	Rstat = Write_NVS_Val("TRGT_FREQ", pDFault->TRGT_FREQ);
	if (Rstat != 1)
		GudFlg = false;
	/* Save current Grtzl_Gain value; Note this is an unsigned factional (1.0 or smaller) float value. But NVS library can't handle floats,
	so 1st convert to unsigned 64bit int    */
	//uint64_t intGainVal = uint64_t(10000000 * pDFault->Grtzl_Gain);
	int intGainVal = (int)(10000000 * pDFault->Grtzl_Gain);
    Rstat = Write_NVS_Val("Grtzl_Gain", intGainVal);
	if (Rstat != 1)
		GudFlg = false;
	if (GudFlg)
	{
		sprintf(buf, "User Params SAVED");
		ptftmsgbx->dispStat(buf, TFT_GREEN);
	}
	else
	{
		sprintf(buf, "SAVE FAILED");
		ptftmsgbx->dispStat(buf, TFT_RED);
	}
}
