/*
 * CWSndEngn.cpp
 *
 *  Created on: Oct 5, 2021
 *      Author: jim
 * 20220827 minor tweek to CWSNDENGN::Intr(void) code to allow F12 (SOT) to always stop key at the end of a letter
 * 20230429 Added code to set character timing speed to a minimum of 15wpm
 * 20230430 fixed crash issue related to changing speed while buffered code is being sent.
 * 20230502 reworked WPM screen refresh code & updates to dotclk timing interval to avoid TFT display crashes. 
 */
/*
 * Given:
 * 	Global Variables:
 * 		TmInrvlCnt, KeyState, SymblCnt, RingBufrPntr1, RingBufrPntr2,
 * 		Symbl
 *
 * 1. Is Time interval Count (TmInrvlCnt) > 0
 * 2. If "Yes" (we're actively sending), Dec TmInrvlCnt
 * Is TmInrvlCnt now = 0?
 * 		If "NO" return (wait for next Interrupt).
 * 		If "YES", Were we sending a Keyup (Space) or a KeyDwn?.
 * 			If KeyDwn then need to change to Keyup, and setup inter-character break (1 time interval)
 * 			If Keyup, are we actively processing a letter?
 * 5. 			If "YES" (SymblCnt >0) continue to unpack (Goto SymblCnt >0)
 * 6. 			If "NO" (SymblCnt =0) continue space interval to form a letter break
 * 				(+2 time intervals for a total of 3 continuous Keyup periods)
 * 				 Exit to wait for next interrupt.
 *
 * Then if here; i.e.,(KeyUp & TmIntrvlCnt = 0):
 * Is there a symbol waiting to be processed?
 * 7. 	If "NO" (RingBufrPntrs are =) Exit to wait for next interrupt.
 * 8. 	If "YES" (RingBufrPntr1 != RingBufrPntr2), there's a character to be processed.
 *    	Get it (INC RingBufrPntr2), & set SymblCnt = 15.
 *    	[DO While loop] shift "Symbl" left, & DEC SymblCnt to the 1st active bit is found
 *    	(SymblCnt equals the number of symbols to be sent) (Goto SymblCnt >0)
 *
 *  SymblCnt >0:
 *  	shift "Symbl" left
 *      Is this symbol a "dit" (0) or a "dah" (1)?
 *      	its a "dit" set TmInrvlCnt = 1
 *      	its a "dah" set TmInrvlCnt = 3
 *    Set KeyDwn Exit to wait for next interrupt.
 */
#include "TFT_eSPI.h"
#include "CWSndEngn.h"
#include "TFTMsgBox.h"
#include "DcodeCW.h"
#include "esp_timer.h"
#include "sdkconfig.h" //added for timer support
#include <esp_log.h>


CWSNDENGN::CWSNDENGN(esp_timer_handle_t *Timr_Hndl, TFT_eSPI *tft_ptr, TFTMsgBox *MsgBx_ptr){
	DotClk = Timr_Hndl;
	pMsgBx = MsgBx_ptr;
	ptft = tft_ptr;
	TmInrvlCnt = 0;
	KeyDwn = false;
	SymblCnt = 0;
	RingBufrPntr1 = 0;
	RingBufrPntr2 = 0;
	Symbl = 0;
	LtrBkFlg = false;
	ActvFlg = false;
	SpcFlg = false;
	TuneFlg = false;
	SOTFlg = true;
	StrTxtFlg = false;
	LstNtrySpcFlg =false;
	RfrshSpd = true;
	ConfigKey(KeyDwn);
	uSec = 80000;
	SndWPM = 20;
	curWPM =0;
	intcntr = 0;
};

bool CWSNDENGN::IsActv(void){
	return ActvFlg;
};
bool CWSNDENGN::LstNtrySpc(void){
	return LstNtrySpcFlg;
};
/* executes each time the "DOT" timer initiates an interrupt *
 * i.e. if the WPM setting is 15, then this interrupt occurs every 80ms (1200ms/15)
 */
/* state codes
		0 No activity
		1 Processing
		2 Letter Complete(Kill BG HiLite)
		3 Start Space(Set BG HiLite)
		4 Start Character(Set BG HiLite)
		5 End Space & Start Next Character(Kill old BG HiLite & Hilite Next Position)
 */
int CWSNDENGN::Intr(void){
	//static constexpr char const * Tag = "DotClk-Int";
	//bool UpdateTiming = false;
	//20230430 moved changing dot clock timing to here to ensure timing changes are inserted at the begining of a new timing interval 
	
	if(!ActvFlg || (!SOTFlg && !SymblCnt && !KeyDwn)){ //added "&& !!SymblCnt" to make F12 stop at letter end
		intcntr = 0;
		TmInrvlCnt = 0;
		if(SndWPM != curWPM){
			ShwWPM(SndWPM);
			curWPM = SndWPM;
			uSec = CalcARRval(curWPM);
			ESP_ERROR_CHECK(esp_timer_restart(*DotClk, uSec));
		}
		return 0;//no activity
	}
	state = 1;// processing
	intcntr++;
	if(TmInrvlCnt > 0){
		TmInrvlCnt--;
		if(TmInrvlCnt > 0) return state;// processing
		else{//TmInrvlCnt now =0
			if(KeyDwn){
				KeyDwn = false;
				TmInrvlCnt = 1;
				ConfigKey(KeyDwn);
				if(SymblCnt == 0){
					LtrBkFlg = true;
					pMsgBx->setCWActv(false);//clear the bG cursor highlight
					bool chngSpd = false;
					if(SndWPM != curWPM){
						ShwWPM(SndWPM);
						curWPM = SndWPM;
						uSec = CalcARRval(curWPM);
						chngSpd = true;
					}
					if(curWPM<15){//restore dotclock to this <15wpm timing
						uSec = CalcARRval(curWPM);
						chngSpd = true;
					}
					if(chngSpd) ESP_ERROR_CHECK(esp_timer_restart(*DotClk, uSec));
				}
				return state;// processing
			} else{ //Key Up
				if((SymblCnt == 0) && LtrBkFlg && !SpcFlg){
					LtrBkFlg = false;
					TmInrvlCnt = 2; // nothing left in this letter; extend time to create a full letter break interval
					state =2;
					return state;//letter complete
				}else if(LtrBkFlg && SpcFlg){
					LtrBkFlg = false;
					//					SpcFlg = false;
					TmInrvlCnt = 3;
					state =5;
				}
				else if(!LtrBkFlg && SpcFlg){
					state =5;//need to both restore old space & set next space background
				}
				//else ;//(Done with last symbol & "follow on" interval; Check for more to send; Goto SymblCnt >0)
			}
		}

	}
	if(SymblCnt== 0){ //Check to see if there's another character to be sent
		/*But 1st check TmInrvlCnt, if !=0, we're still procesing a space */
		if(TmInrvlCnt != 0) return 1;// processing
		if(RingBufrPntr1 == RingBufrPntr2){
			ActvFlg = false;
			if(SpcFlg){
				SpcFlg = false;
				state = 2;//Letter Complete (restore background)
				if(curWPM<15){//restore dotclock to this <15wpm timing
					if(SndWPM != curWPM){
						ShwWPM(SndWPM);
						curWPM = SndWPM;
					}
					uSec = CalcARRval(curWPM);
					ESP_ERROR_CHECK(esp_timer_restart(*DotClk, uSec));
				} else if(SndWPM != curWPM){
					ShwWPM(SndWPM);
					curWPM = SndWPM;
					uSec = CalcARRval(curWPM);
					ESP_ERROR_CHECK(esp_timer_restart(*DotClk, uSec));
				}
			}
			return state;
		}
		else{ // Yes, another character is waiting to be sent. Go get it
			char curChar = SndBuf[RingBufrPntr2];

			RingBufrPntr2 = AdvncPntr(RingBufrPntr2);
			if(curChar == 32){ // "Space" key found
				/*A pause of 3 intervals has already occurred,
				 * So need to wait 4 more for a 7 interval word break */
				ActvFlg = true;
				intcntr = 0;
				SpcFlg = true;
				LtrBkFlg = true;
				TmInrvlCnt += 4;
				if(state!=5) state = 3;//start space
				return state;
			}
			else{
				SpcFlg = false;
				Symbl = ChrToSymb(&curChar);
				intcntr = 0;
				ActvFlg = true;
				SymblCnt = 15;
				while(!(Symbl& 0b1000000000000000)){
					Symbl = Symbl<<1;
					SymblCnt--;
				}
				if(SndWPM != curWPM){
					ShwWPM(SndWPM);
					curWPM = SndWPM;
				}	
				if(curWPM<15){// set/force dotclock to send character with 15wpm timing
					uSec = CalcARRval(15);
					ESP_ERROR_CHECK(esp_timer_restart(*DotClk, uSec));
				}
			}
		}
	}
	// Now determine whether the next symbol is a Dit or a Dah
	KeyDwn = true;
	Symbl = Symbl<<1;
	SymblCnt--;
	if(Symbl & 0b1000000000000000) TmInrvlCnt = 3; // its a Dah
	else TmInrvlCnt = 1; // its a Dit
	ConfigKey(KeyDwn);
	pMsgBx->setCWActv(true);
	if(state!=5){
		state = 4;//start letter
	}
	return state;
};

/* */
void CWSNDENGN::LdMsg(char *Msg, size_t len){
	for(unsigned int i = 0; i<len; i++){
		if(Msg[i] == 0) break;
		AddNewChar(&Msg[i]);
		//		tftmsgbx.KBentry(Msg[i]);
	}
};
/* Add a single (ASCII) character to Send Buffer */
void CWSNDENGN::AddNewChar(char* Nuchr){
	if(StrTxtFlg && (*Nuchr == ' ')){ //auto stop store
		StrTxtFlg = false;
		pMsgBx->setStrTxtFlg(StrTxtFlg);
	}
	if(*Nuchr == ' ') LstNtrySpcFlg =true;
	else LstNtrySpcFlg =false;
	SndBuf[RingBufrPntr1] = *Nuchr;
	pMsgBx->KBentry(*Nuchr);
	RingBufrPntr1 = AdvncPntr(RingBufrPntr1);
	ActvFlg = true;
};
int CWSNDENGN::Delete(void){
	if(RingBufrPntr1 == RingBufrPntr2) return 0;
	RingBufrPntr1--;
	if(RingBufrPntr1 < 0) RingBufrPntr1 = sizeof(SndBuf)-1;
	//TODO work out how many printed Characters this deleted CW character created
	// for the moment assume 1
	return 1;
};
/* uses the CW Decoder table (in reverse) to convert ASCII character to a uint16_t
 * bit pattern representing dot/dash pattern of the ASCII character */
uint16_t CWSNDENGN::ChrToSymb(char* CurChr){
	int i;
	for( i= 0; i < ARSIZE; i++){
		if(DicTbl1[i][0] == *CurChr) break;
	}
	if(i == ARSIZE){ //test for special CW codes
		uint16_t spclCd;
		switch(*CurChr){
		case '=' :
			spclCd = uint16_t(0b110001);//<BT>
			break;
		case '+' :
			spclCd = uint16_t(0b110110);//<KN>
			break;
		case '%' :
			spclCd = uint16_t(0b1000101);//<SK>
			break;
		case '>' :
			spclCd = uint16_t(0b101010);//<AR>
			break;
		case '<' :
			spclCd = uint16_t(0b101000);//<WAIT>
			break;
		default:
			spclCd = uint16_t(0b1000000);//CW error code
		}
		return spclCd;
	}
	else return uint16_t(CodeVal1[i]);
};

int CWSNDENGN::AdvncPntr(int pntr){
	pntr++;
	if(pntr >= sizeof(SndBuf)) pntr= 0;
	return pntr;
};

void CWSNDENGN::ConfigKey(bool KeyState){
	if(KeyState){
		//Do what it takes to create key closed condition
		//PIN_LOW(LED_GPIO_Port, LED_Pin);
		//PIN_HIGH(LED_GPIO_Port, TONE_Pin);
		digitalWrite(KEY, Key_Dwn); 
	}
	else{
		//Do what it takes to create key open condition
		//PIN_HIGH(LED_GPIO_Port, LED_Pin);
		//PIN_LOW(LED_GPIO_Port, TONE_Pin);
		digitalWrite(KEY, Key_Up);
		TuneFlg = false;
	}
};

void CWSNDENGN::IncWPM(void){
	SndWPM += 1;
	if(SndWPM>55){
		SndWPM = 55;
		return;
	}
	//ShwWPM(SndWPM);

};
void CWSNDENGN::DecWPM(void){
	SndWPM -= 1;
	if(SndWPM < 5){
		SndWPM = 5;
		return;
	}
	//ShwWPM(SndWPM);
};
int CWSNDENGN::CalcARRval(int wpm){
	int NuVal = 1200000/wpm;
	return NuVal;
};

void CWSNDENGN::ShwWPM(int wpm)
{
	char buf[10];
	char Curchar;
	int Xpos = ScrnWdth -(FONTW*6);//410;
	int Ypos = ScrnHght -20;//300;
	int Wdth = (FONTW*6);//80;
	int Hght = 30;
	if(wpm == curWPM && !RfrshSpd) return;
	RfrshSpd = false;
	sprintf(buf, "%d WPM", wpm);
	ptft->setTextColor(TFT_WHITE);
	ptft->fillRect(Xpos, Ypos, Wdth, Hght, TFT_BLACK); //erase old wpm value
	ptft->setCursor(Xpos, Ypos);
	for(int i=0; i<sizeof(buf); i++){
		if(buf[i] ==0) break;
		Curchar = buf[i];
		ptft->print(Curchar);
	}

};
void CWSNDENGN::Tune(void)
{
	TuneFlg = !TuneFlg;
	if(TuneFlg){
		//Do what it takes to create key closed condition
		//PIN_LOW(LED_GPIO_Port, LED_Pin);
		//PIN_HIGH(LED_GPIO_Port, TONE_Pin);
		digitalWrite(KEY, Key_Dwn);
	}
	else{
		//Do what it takes to create key open condition
		//PIN_HIGH(LED_GPIO_Port, LED_Pin);
		//PIN_LOW(LED_GPIO_Port, TONE_Pin);
		digitalWrite(KEY, Key_Up);
	}
	return;
};

void CWSNDENGN::SOTmode(void)
{
	/*Test/check for CW in progres*/
	//printf("State: %d\n",state);
	if (SOTFlg && (state >0))
	{	
		// char buf[20];
		// sprintf(buf, "WHILE LOOP S:%d", state);
		// pMsgBx->dispMsg(buf, TFT_YELLOW);
		/*something is going on, so wait for the current character to complete*/
		bool BGHilite = pMsgBx->getBGHilite();
		//int cnt = 0;
		//printf("Cnt %d\n",cnt);
		//while ((SymblCnt != 0) || KeyDwn || (state != 2) || BGHilite) // if any of the conditions are true, we're in the middle of sending a Morse character
		while (BGHilite)
		{
			//delay(1);
			//cnt++;
			//printf("Cnt %d\n",cnt);
			BGHilite = pMsgBx->getBGHilite();
			
		}
	}
	/* SOTFlg = true: incoming key strokes will be sent immediately */
	SOTFlg = !SOTFlg;
	if (SOTFlg && StrTxtFlg)
		StrTxtmode();		   // if true, then we need to also stop storing incoming entries;
	pMsgBx->setSOTFlg(SOTFlg); // set the color of the display's status indicator
};

void CWSNDENGN::StrTxtmode(void)
{
	StrTxtFlg = !StrTxtFlg;
	pMsgBx->setStrTxtFlg(StrTxtFlg);
};

void CWSNDENGN::AbortSnd(void)
{
	KeyDwn = false;
	RingBufrPntr2 = RingBufrPntr1;
	Symbl = 0;
	TmInrvlCnt =0;
	intcntr = 0;
	LtrBkFlg = false;
	ActvFlg = false;
	SpcFlg = false;
	TuneFlg = false;
	KeyDwn = false;
	LstNtrySpcFlg =false;
	ConfigKey(KeyDwn);
	pMsgBx->IntrCrsr(6);//abort state
};
void CWSNDENGN::RefreshWPM(void){
	int wpm = curWPM;
	RfrshSpd = true;
	ShwWPM(wpm);
};
int CWSNDENGN::GetWPM(void){
	return SndWPM;
};
bool CWSNDENGN::GetSOTflg(void){
	return SOTFlg;
};
void  CWSNDENGN::SetWPM(int newWPM){
	SndWPM = newWPM;
	if(SndWPM>55){
		SndWPM = 55;
	}
	if(SndWPM<5){
		SndWPM = 5;
	}
	curWPM = SndWPM;
	uSec = CalcARRval(SndWPM);
	ESP_ERROR_CHECK(esp_timer_stop(*DotClk));
    ESP_ERROR_CHECK(esp_timer_start_periodic(*DotClk, uSec));

};
