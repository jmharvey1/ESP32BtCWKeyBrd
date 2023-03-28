/*
 * TFTMsgBox.cpp
 *
 *  Created on: Oct 7, 2021
 *      Author: jim
 */
#include "TFTMsgBox.h"
// #include "Arduino.h"//needed to support "delay()" function call

TFTMsgBox::TFTMsgBox(TFT_eSPI *tft_ptr, char *StrdTxt)
{
	ptft = tft_ptr;
	pStrdTxt = StrdTxt;
	txtpos = 0;
	RingbufPntr1 = 0;
	RingbufPntr2 = 0;
	CursorPntr = 0;
	cursorY = 0;
	cursorX = 0;
	cnt = 0; // used in scrollpage routine
	curRow = 0;
	offset = 0;
	oldstate =0;
	BlkState = false;
	SOTFlg = true;
	StrTxtFlg = false;
	Bump = false;
	PgScrld = false;
	BGHilite = false;
};

void TFTMsgBox::InitDsplay(void)
{
	ptft->init(); // ptft->reset();
	ptft->setRotation(3); // valid values 1 or 3
	ptft->fillScreen(TFT_BLACK);
	ptft->fillScreen(TFT_BLACK); // have to call this twice, to ensure full erasure
	ptft->setCursor(cursorX, cursorY);
	ptft->setTextColor(TFT_WHITE); // tft.setTextColor(WHITE, BLACK);
	ptft->setTextSize(fontsize);		   // 2 for 320x480; 1 for 240x320
	ptft->setTextWrap(false);
};
/*Originally written to be called after returning from setup/settings screen*/
void TFTMsgBox::ReBldDsplay(void)
{
	ptft->fillScreen(TFT_BLACK);
	ReStrSettings();
	// char temp[50];
	// sprintf(temp, "%d;  %d;  %d;  %d; %d; %d", RingbufPntr1, RingbufPntr2, CursorPntr, cnt, curRow, offset);
	// printf(temp); // result of no entry: 35;  35;  40;  40; 1
	// printf("\n");
	//TODO this may need more thought
	RingbufPntr2 = 0;
	// if (!PgScrld) // the text entered has yet to fill the screen/display;	So we simply go back to the begining
	// {
	// 	RingbufPntr2 = 0;
	// }
	// else
	// {
	// 	RingbufPntr2 = RingbufPntr1 - RingBufSz;
	// 	if (RingbufPntr2 < 0)
	// 	{
	// 		int NegBuf2 = RingbufPntr2;
	// 		RingbufPntr2 = (RingBufSz + NegBuf2);
	// 	}
	// }
	/*reset pointers & counters*/
	cursorX = cursorY = curRow = cnt = offset = CursorPntr = 0;
	RingbufPntr1--;
	if(RingbufPntr1<0) RingbufPntr1 = RingBufSz-1;
	// sprintf(temp, "%d;  %d;  %d;  %d; %d", RingbufPntr1, RingbufPntr2, CursorPntr, cnt, curRow);
	// printf(temp); // result of no entry: 35;  35;  40;  40; 1
	// printf("\n");
	// delay(5000);
	Bump = true;
	bool curflg = SOTFlg;
	setSOTFlg(curflg); // one way to refresh the SOT/STR status box without having to reinvent the wheel
	while(RingbufPntr1 != RingbufPntr2){
		delay(1);
	}
	CursorPntr = StrdPntr;
}
/* normally called by the Keyboard parser process
 * the main feature of this entry point is to set
 * the cursor position pointer
 */
void TFTMsgBox::KBentry(char Ascii)
{
	char buf[2];
	uint16_t color = TFT_WHITE;
	buf[0] = Ascii;
	buf[1] = 0;

	if (StrTxtFlg)
	{
		pStrdTxt[txtpos] = Ascii;
		pStrdTxt[txtpos + 1] = 0;
		txtpos++;
		if (txtpos >= 19)
			txtpos = 18;
		else
			color = TFT_YELLOW;
	}
	if (CursorPntr == 0)
		CursorPntr = cnt;
	dispMsg(buf, TFT_WHITE);
};
void TFTMsgBox::dispMsg(char Msgbuf[50], uint16_t Color)
{
	int msgpntr = 0;
	/* Add the contents of the Msgbuf to ringbuffer */

	while (Msgbuf[msgpntr] != 0)
	{
		if (RingbufPntr1 < RingBufSz - 1)
			RingbufChar[RingbufPntr1 + 1] = 0;
		else
			RingbufChar[0] = 0;
		RingbufChar[RingbufPntr1] = Msgbuf[msgpntr];
		RingbufClr[RingbufPntr1] = Color;
		RingbufPntr1++;
		if (RingbufPntr1 == RingBufSz)
			RingbufPntr1 = 0;
		msgpntr++;
	}
	/* Restore interrupts from MAX3421 */
	//	CPUCTLval = Usb.regRd(rCPUCTL);
	//	CPUCTLval = CPUCTLval | 0x01;
	//	Usb.regWr(rCPUCTL, CPUCTLval); //enable interrupt pin
};
/* timer interrupt driven method. drives the Ringbuffer pointer to ensure that
 * character(s) sent to TFT Display actually get printed (when time permits)
 */
void TFTMsgBox::dispMsg2(void)
{
	char buf[2];
	/*For debugging */
	// buf[1] = 0;
	// if(RingbufPntr2 != RingbufPntr1) printf( "**");
	while (RingbufPntr2 != RingbufPntr1)
	{
		/*test if this next entry is going to generate a scroll page event
		& are we in the middle of sending a letter; if true, skip this update.
		This was done to fix an issue with the esp32 & not being able to give the dotclock ISR 
		priority over the display ISR */
		if(CWActv && (((cnt+1) - offset) * fontW >= displayW) && (curRow + 1 == row) ){
			// char buf[50];
			// sprintf(buf, "currow: %d; row: %d", curRow, row);
			// dispStat(buf, TFT_GREENYELLOW);//update status line
			// setSOTFlg(false);//changes status square to yellow
			break;
		} 
		ptft->setCursor(cursorX, cursorY);
		char curChar = RingbufChar[RingbufPntr2];

		/*For debugging */
		// buf[0] = RingbufChar[RingbufPntr2];
		
		// if(curChar !=0 ) printf( "%s", buf);
		
		uint16_t Color = RingbufClr[RingbufPntr2];
		if (curChar != 10)
		{ // test for "line feed" character
			ptft->setTextColor(RingbufClr[RingbufPntr2]);
			// sprintf(buf, "%s",curChar);
			// printf(buf);
			ptft->print(curChar);
			/* If needed add this character to the Pgbuf */
			if (curRow > 0)
			{
				Pgbuf[cnt - CPL] = curChar;
				Pgbuf[cnt - (CPL - 1)] = 0;
				PgbufColor[cnt - CPL] = Color;
			}
			cnt++;
			//printf("cnt:%d; ", cnt);
			if ((cnt - offset) * fontW >= displayW)
			{
				curRow++;
				cursorX = 0;
				cursorY = curRow * (fontH);
				offset = cnt;
				ptft->setCursor(cursorX, cursorY);
				if (curRow + 1 > row)
				{
					scrollpg();
				}
			}
			else
			{
				cursorX = (cnt - offset) * fontW;
			}
		}
		else
		{
			DisplCrLf();
			//printf("cnt:%d; \n", cnt);
		}
		RingbufPntr2++;
		if (RingbufPntr2 == RingBufSz)
			RingbufPntr2 = 0;
	} // End while loop
	if (Bump)
	{
		Bump = false;
		RingbufPntr1++;
		if (RingbufPntr1 == RingBufSz)
			RingbufPntr1 = 0;
		dispStat(LastStatus, LastStatClr);
	}
	int blks = BlkStateCntr;
	while (BlkStateCntr > 0)
	{
		IntrCrsr(BlkStateVal[blks - BlkStateCntr]);
		BlkStateCntr--;
	}
};
//////////////////////////////////////////////////////////////////////
void TFTMsgBox::Delete(int ChrCnt)
{
	while (ChrCnt != 0)
	{ // delete display of ever how many characters were printed in the last decodeval (may be more than one letter generated)
		// first,buzz thru the pgbuf array until we find the the last character (delete the last character in the pgbuf)
		int TmpPntr = 0;
		while (Pgbuf[TmpPntr] != 0)
			TmpPntr++;
		if (TmpPntr > 0)
			Pgbuf[TmpPntr - 1] = 0; // delete last character in the array by replacing it with a "0"
		cnt--;
		int xoffset = cnt;
		// use xoffset to locate the character position (on the display's x axis)
		curRow = 0;
		while (xoffset >= CPL)
		{
			xoffset -= CPL;
			curRow++;
		}
		cursorX = xoffset * (fontW);
		cursorY = curRow * (fontH);
		if (xoffset == (CPL - 1))
			offset = offset - CPL;										 // we just jump back to last letter in the previous line, So we need setup to properly calculate what display row we will be on, come the next character
		ptft->fillRect(cursorX, cursorY, fontW + 4, (fontH), TFT_BLACK); // black out/erase last displayed character
		ptft->setCursor(cursorX, cursorY);
		--ChrCnt;
		/*Now if we are also storing characters (via "F1" command) need to remove last entry from that buffer too */
		if (StrTxtFlg && (txtpos > 0))
		{
			txtpos--;
			pStrdTxt[txtpos] = 0;
		}
	}
};
//////////////////////////////////////////////////////////////////////
void TFTMsgBox::DisplCrLf(void)
{
	/* Pad the remainder of the line with space */
	if ((cnt - offset) == 0)
		return;
	int curOS = offset;
	ptft->setTextColor(TFT_BLACK);
	while ((cnt - curOS) * fontW <= displayW)
	{
		ptft->print(" "); // ASCII "Space"
		/* If needed add this character to the Pgbuf */
		if (curRow > 0)
		{
			Pgbuf[cnt - CPL] = 32;
			Pgbuf[cnt - (CPL - 1)] = 0;
			PgbufColor[cnt - CPL] = TFT_BLACK;
		}
		cnt++;
		if ((cnt - offset) * fontW >= displayW)
		{
			curRow++;
			cursorX = 0;
			cursorY = curRow * (fontH);
			offset = cnt;
			ptft->setCursor(cursorX, cursorY);
			if (curRow + 1 > row)
			{
				scrollpg();
				return;
			}
		}
		else
		{
			cursorX = (cnt - offset) * fontW;
		}
		if (((curOS + CPL) - cnt) == 0)
			break;
	}
};
//////////////////////////////////////////////////////////////////////
void TFTMsgBox::scrollpg()
{
	// buttonEnabled =false;
	// unsigned long Tstrt= millis();
	BlkState = true;
	BlkStateCntr = 0;
	cursorX = 0;
	cnt = 0;
	cursorY = 0;
	curRow = 0;
	offset = 0;
	PgScrld = true;
	bool PrintFlg = true;
	int curptr = RingbufPntr1;
	if (RingbufPntr2 > RingbufPntr1)
		curptr = RingbufPntr1 + RingBufSz;
	if ((curptr - RingbufPntr2) > CPL - 1)
		PrintFlg = false;
	//  enableDisplay(); //this is for touch screen support
	if (PrintFlg)
	{
		ptft->fillRect(cursorX, cursorY, displayW, row * (fontH), TFT_BLACK); // erase current page of text
		ptft->setCursor(cursorX, cursorY);
	}
	while (Pgbuf[cnt] != 0 && curRow + 1 < row)
	{ // print current page buffer and move current text up one line
		if (PrintFlg)
		{
			ptft->setTextColor(PgbufColor[cnt]);
			ptft->print(Pgbuf[cnt]);
		}
		Pgbuf[cnt] = Pgbuf[cnt + CPL]; // shift existing text character forward by one line
		PgbufColor[cnt] = PgbufColor[cnt + CPL];
		cnt++;
		if (((cnt)-offset) * fontW >= displayW)
		{
			curRow++;
			offset = cnt;
			cursorX = 0;
			cursorY = curRow * (fontH);
			if (PrintFlg)
				ptft->setCursor(cursorX, cursorY);
		}
		else
		{
			cursorX = (cnt - offset) * (fontW);
			ptft->setCursor(cursorX, cursorY);
		}

	} // end While Loop
	if (!PrintFlg)
	{																	// clear last line of text
		ptft->fillRect(cursorX, cursorY, displayW, (fontH), TFT_BLACK); // erase current page of text
		ptft->setCursor(cursorX, cursorY);
	}
	/* And if needed, move the CursorPntr up on line*/
	if (CursorPntr - CPL > 0)
		CursorPntr = CursorPntr - CPL;
	// char temp[50];
	// int ScrollTime = int(millis() - Tstrt);
	// sprintf(temp,"SCROLPG Time: %d\n", ScrollTime);
	// printf(temp);	
	BlkState = false;
	/* now, if a usable state change happened while scrolling the text,
	 * apply that state value now */
	// int blks = BlkStateCntr;
	// while (BlkStateCntr > 0)
	// {
	// 	IntrCrsr(BlkStateVal[blks - BlkStateCntr]);
	// 	BlkStateCntr--;
	// }
};
/*Manage highlighting the CW character currently being sent*/
/*this routine get executed everytime the dotclockgenerates an interrupt*/
/*The stat value that gets passed to it, controls how the character & its background is colorized*/
void TFTMsgBox::IntrCrsr(int state)
{
	/* state codes
		0 No activity
		1 Processing
		2 Letter Complete
		3 Start Space
		4 Start Character
		5 End Space & Start Next Character
	 */
	/*needed just for debugging cursorptr issue*/
	// char temp[50];
	// if (oldstate !=0 && state == 0) printf("**************\n");
	// oldstate = state;
	// if(state !=0){
	// sprintf(temp,"STATE: %d; CsrPtr: %d; cntr: %d\n", state, CursorPntr, cnt );
	// printf(temp);
	// }
	/*Don't need this with the latest queue method for managing the timer interrurpts*/
	// if (BlkState)
	// {
	// 	BlkStateVal[BlkStateCntr] = state; // save this state, so it can be processed after the scroll page routine completes
	// 	BlkStateCntr++;
	// 	return;
	// }
	switch (state)
	{
	case 0: // No activity
		// if (SOTFlg)
		// 	CursorPntr = cnt;
		break;		 // do nothing
	case 1:			 // Processing
		if(CursorPntr > cnt) CursorPntr = cnt;
		break;		 // do nothing
	case 2:			 // Letter Complete
		RestoreBG(); // restore normal Background
		break;
	case 3:			// Start Space
		HiLiteBG(); // Highlight Background
		break;
	case 4:			// Start Letter
		if(CursorPntr >= cnt) CursorPntr = cnt -1; 
		HiLiteBG(); // Highlight Background
		break;
	case 5: // restore Backgound & Highlight Next Character or Space
		if (CursorPntr + 1 >= cnt)
			CursorPntr = cnt - 1; // test, just in case something got screwed up
		RestoreBG();			  // restore normal Background
		HiLiteBG();				  // Highlight Background
		break;
	case 6:			 // Abort restore Backgound & Highlight Next Character or Space
		RestoreBG(); // restore normal Background
		CursorPntr = cnt;
		//		if(CursorPntr+1 >=cnt) CursorPntr = cnt-1;//test, just in case something got screwed up
		break;
	}
	return;
};
void TFTMsgBox::HiLiteBG(void)
{
	PosHiLiteCusr();
	ptft->setTextColor(PgbufColor[CursorPntr - CPL], TFT_RED);
	ptft->print(Pgbuf[CursorPntr - CPL]);
	ptft->setTextColor(TFT_WHITE, TFT_BLACK); // restore normal color scheme
											  // dont need to put Cursor back, other calls to print will do that
	BGHilite = true;										  

};
void TFTMsgBox::RestoreBG(void)
{
	PosHiLiteCusr();
	ptft->setTextColor(PgbufColor[CursorPntr - CPL], TFT_BLACK);
	ptft->print(Pgbuf[CursorPntr - CPL]);
	CursorPntr++;
	BGHilite = false;
	// if(CursorPntr == cnt) CursorPntr = 0;
};
void TFTMsgBox::PosHiLiteCusr(void)
{
	/* figure out where the HighLight Y cursor needs to be set */
	int HLY = 0;
	int HLX = 0;
	int tmppntr = 0;
	int tmprow = 1;
	while (tmppntr + CPL <= CursorPntr)
	{
		tmprow++;
		tmppntr += CPL;
	}
	HLY = (tmprow - 1) * (fontH);
	HLX = (CursorPntr - ((tmprow - 1) * CPL)) * fontW;
	ptft->setCursor(HLX, HLY);
};

void TFTMsgBox::dispStat(char Msgbuf[50], uint16_t Color)
{
	int i;
	LastStatClr = Color;
	int LdSpaceCnt = 0;
	//ptft->setCursor(StatusX, StatusY);//old way
	ptft->setCursor(StusBxX+30, StatusY);//new way, based on where the sot status box ends
	ptft->setTextColor(Color, TFT_BLACK);
	/* 1st figure out how many leading spaces to print to "center"
	 * the Status message
	 */

	//int availspace = int(((ScrnWdth - 60) - StatusX) / fontW);
	int availspace = int(((ScrnWdth -(FONTW*6)) - (StusBxX+30)) / fontW);
	i = 0;
	while (Msgbuf[i] != 0)
		i++;
	if (i < availspace)
	{
		LdSpaceCnt = int((availspace - i) / 2);
		if (LdSpaceCnt > 0)
		{
			for (i = 0; i < LdSpaceCnt; i++)
			{
				ptft->print(" ");
			}
		}
	}
	for (i = 0; i < availspace; i++)
	{
		LastStatus[i] = Msgbuf[i]; // copy current status to last status buffer; In case needed later to rebuild main screen
		if (Msgbuf[i] == 0)
			break;
		ptft->print(Msgbuf[i]);
	}
	/*finish out remainder of line with blank space */
	//	ptft->setTextColor(TFT_BLACK);
	if (LdSpaceCnt > 0)
	{
		for (i = 0; i < LdSpaceCnt; i++)
		{
			ptft->print(" ");
		}
	}
};

void TFTMsgBox::setSOTFlg(bool flg)
{
	SOTFlg = flg;
	/*Now Use the max3421 interrupt box to show "Send On Text State"(SOT) mode */
	uint16_t color;
	int Xpos = StusBxX;
	int Ypos = StusBxY;
	int Wdth = 30;
	int Hght = 30;
	if (SOTFlg && !StrTxtFlg)
		color = TFT_GREEN;
	else if (SOTFlg && StrTxtFlg)
		color = TFT_WHITE;
	else
		color = TFT_YELLOW;
	ptft->fillRect(Xpos, Ypos, Wdth, Hght, color);
};
void TFTMsgBox::setStrTxtFlg(bool flg)
{
	StrTxtFlg = flg;
	/*Now Use the max3421 interrupt box to show "Store Text" mode */
	uint16_t color;
	int Xpos = StusBxX;
	int Ypos = StusBxY;
	int Wdth = 30;
	int Hght = 30;
	if (!StrTxtFlg && SOTFlg)
		color = TFT_GREEN;
	else if (!StrTxtFlg && !SOTFlg)
		color = TFT_YELLOW;
	else
	{
		color = TFT_WHITE;
		pStrdTxt[0] = 0;
		txtpos = 0;
	}
	ptft->fillRect(Xpos, Ypos, Wdth, Hght, color);
};
void TFTMsgBox::SaveSettings(void)
{
	StrdPntr = CursorPntr;
	StrdY = cursorY;
	StrdX = cursorX;
	Strdcnt = cnt; // used in scrollpage routine
	StrdcurRow = curRow;
	Strdoffset = offset;
	StrdRBufPntr1 = RingbufPntr1;
	StrdRBufPntr2 = RingbufPntr2;
	// printf("Ringbuf:");
	// char temp[2 * CPL];
	// for (int i = 0; i < 2 * CPL; i++)
	// {
	// 	temp[i] = RingbufChar[i];
	// }
	// printf(temp);
	// printf("\n");
};
void TFTMsgBox::ReStrSettings(void)
{
	CursorPntr = StrdPntr;
	cursorY = StrdY;
	cursorX = StrdX;
	cnt = Strdcnt; // used in scrollpage routine
	curRow = StrdcurRow;
	offset = Strdoffset;
	RingbufPntr1 = StrdRBufPntr1;
	RingbufPntr2 = StrdRBufPntr2;
	// printf("Ringbuf:");
	// char temp[2 * CPL];
	// for (int i = 0; i < 2 * CPL; i++)
	// {
	// 	temp[i] = RingbufChar[i];
	// }
	// printf(temp);
	// printf("\n");
};

void TFTMsgBox::setCWActv(bool flg){
	//if(flg) setSOTFlg(true);// just needed for debugging
	CWActv = flg;
};

bool TFTMsgBox::getBGHilite(void){
	return BGHilite;
};

