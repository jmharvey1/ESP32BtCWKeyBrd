/*
 * TxtNtryBox.cpp
 *
 *  Created on: Jun 9, 2022
 *      Author: jim
 */

#include <TxtNtryBox.h>
//#include "KeyBoardR01.h"

BtnParams btnparams ; //object name

TxtNtryBox::TxtNtryBox(esp_timer_handle_t *Timr_Hndl, TFT_eSPI *tft_ptr){
	pDotClk = Timr_Hndl;
	ptft = tft_ptr;
	//pDotClk = &htim5;//TIM_ptr;
	//ptft = &tft;//tft_ptr;

}

void TxtNtryBox::InitBox(BtnParams settings){
	BtnXpos = settings.BtnXpos;  //Button X position
	BtnWdth = settings.BtnWdth;  //Button Width
	BtnYpos = settings.BtnYpos;  //Button X position
	BtnHght = settings.BtnHght;  //Button Height
	Captn = settings.Captn;
	BtnClr = settings.BtnClr;
	TxtClr = settings.TxtClr;
	BxIndx = settings.BxIndx;
	IsNumbr = settings.IsNumbr;
	Option = settings.Option;
	ShwData();// refresh this button's name


}
/*ShwData does two things:
 * 1. display the button's caption/name;
 * 2. Position the Value's 'X' cursor to its 'starting' screen location */
void TxtNtryBox::ShwData(void){
	int msgpntr = 0;
	int MsgX = 0;
	ptft->setTextColor(TxtClr);
	ptft->setCursor(BtnXpos, BtnYpos);
	while (Captn[msgpntr] != 0) {
		char curChar =Captn[msgpntr];
		ptft->print(curChar);
		msgpntr++;
		MsgX += (fontW);
		ptft->setCursor(BtnXpos+MsgX, BtnYpos);
	}
	ValXpos = BtnXpos+MsgX+ fontW;
}

TxtNtryBox::~TxtNtryBox() {
	// TODO Auto-generated destructor stub
}
/*Normally used to update the current cursor position
 * when "left" & "right" arrow keys are used to reposition the cursor.
 * Also comes into play when a new letter, or number, has been added,
 * to the 'value' arrary*/
void TxtNtryBox::ShwCsr(int CsrPos){

	if(ValueLen == 0){
		BldBtn(true);
	}else{
		unsigned int tmpClr = TxtClr;
		TxtClr = BtnClr;
		ShwData();// refresh this button's name using the button's HighLight Color
		TxtClr = tmpClr;
		int msgptr = 0;
		int curow = BtnYpos + 4;//needs to be a bit lower than where the letter will actually appear
		int MsgX = 0;
		ptft->setTextColor(TxtClr);
		ptft->setCursor(ValXpos, curow);
		/*now refresh the entire value array screen space,
		 * just in case a past update got missed */
		for(msgptr = 0; msgptr < ValueLen ; msgptr++){
			if(msgptr == CsrPos){
				ptft->setTextColor(TxtClr);
				ptft->print("_"); //Generate Cursor Marker"
			}else{
				ptft->setTextColor(TFT_BLACK);
				ptft->print("_");//erase any old cursor markers
			}
			/*Now check to see if its time to move to the next row*/
			if((ValXpos+ MsgX +fontW)>=ScrnW){
				curow += fontH;
				MsgX =0;
				ptft->setCursor(ValXpos, curow);//reset cursor to beginning of new row
			}else MsgX += fontW;

		}
	}
}
void TxtNtryBox::ShwBGColor(unsigned int NewClr){
	unsigned int tmpClr = BtnClr;
	BtnClr = NewClr;
	ShwCsr(0);
	BtnClr = tmpClr;

}
void TxtNtryBox::KillCsr(void){
	if(ValueLen == 0){
		BldBtn(false);
	} else{
		/*ShwCsr, by default, wants to Highlight the Button "caption"/"name" text
		 * So we need to hide the normal button color, and replace it with
		 * its text color*/
		unsigned int tmpClr = BtnClr;
		BtnClr = TxtClr;
		ShwCsr(ValueLen);//refresh the button's caption/name
		BtnClr = tmpClr;// restore original button color
	}
}
void TxtNtryBox::SetValue(char* curval, int len){
	int msgptr = 0;
	int MsgX = 0;
	ValueLen = len;
	if(len == 0){
		BldBtn(false);
		return;
	}
	/* show value of 'text' based parameters */
	ptft->setTextColor(TxtClr);
	ptft->setCursor(ValXpos, BtnYpos);
	int curow = BtnYpos;
	for(msgptr = 0; msgptr<len ; msgptr++){
		char curChar = curval[msgptr];
		ptft->fillRect(ValXpos+MsgX, curow, fontW, (fontH), TFT_BLACK);//erase old invalid character
		BtnValue[msgptr] = curChar;
		if(curChar !=0) ptft->print(curChar); //Update Display entry for this position
		MsgX += fontW;
		if((ValXpos+ MsgX +fontW)>ScrnW){//time to move to next row
			curow += fontH;
			MsgX =0;
		}
		ptft->setCursor(ValXpos+MsgX, curow);
	}
	BtnValue[msgptr] = 0;//make sure new text entry terminates with null value
}
/*After a new key entry has been inserted in this paramter's "value" array,
 * this function is called, to update/refresh the settings screen,
 * with the new entry, and show where the cursor is now positioned*/
void TxtNtryBox::UpDateValue(int UpDatePos, int CsrPos){//typically the "UpDatePos" will be "CsrPos -1"
	int curow = BtnYpos;
	int MsgX = 0;
	for(int i =0; i<UpDatePos; i++){
		if((ValXpos+ MsgX +fontW)>=ScrnW){//time to move to next row
			curow += fontH;
			MsgX =0;
		}else MsgX += fontW;
	}
	ptft->setTextColor(TxtClr);
	ptft->fillRect(ValXpos+MsgX, curow, fontW, (fontH + 10), TFT_BLACK);//erase old invalid character
	ptft->setCursor(ValXpos+MsgX, curow);
	ptft->print(BtnValue[UpDatePos]); //Update Display with new changed entry for this position
	if (CsrPos < ValueLen) ShwCsr(CsrPos);
}
/*OverWrite current Button Caption with new Caption,
 * which may, or may NOT, be of the same length*/
void TxtNtryBox::UpDateCaptn(char* NuCaptn){
	unsigned int tmpClr = TxtClr;
	TxtClr = BtnClr;
	BldBtn(true); //Blank out old caption using same color as button background
	TxtClr = tmpClr;
	Captn = NuCaptn; // change pointer to new caption array
	BldBtn(true);// Reconstruct Button with new caption and "focus" background color
}

char* TxtNtryBox::GetValue(void){
	char *VlaPtr = BtnValue;
	return VlaPtr;
}
int TxtNtryBox::GetValLen(void){
	return ValueLen;
}
/* Rebuild button to show it is "selected", or NOT "selected" */
void TxtNtryBox::BldBtn(bool Hilight){
	if(!Hilight)ptft->fillRect(BtnXpos, BtnYpos, BtnWdth, BtnHght, TFT_BLACK);//fill button space
	else ptft->fillRect(BtnXpos, BtnYpos, BtnWdth, BtnHght, BtnClr);//Hi-Light button
	ptft->drawRect(BtnXpos, BtnYpos, BtnWdth, BtnHght, TFT_WHITE); //frame button space
	ptft->setCursor(BtnXpos + 11, BtnYpos + 12);
	ptft->setTextColor(TxtClr);
	ptft->setTextSize(2);
	ptft->print(Captn);
}

