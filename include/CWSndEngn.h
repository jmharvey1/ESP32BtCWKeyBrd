/*
 * CWSndEngn.h
 *
 *  Created on: Oct 5, 2021
 *      Author: jim
 * 20230418 expanded send buffer from 160 to 400 charater (4 line to 10 dispaly lines)
 * 20230502 changed RfrshSpd flag to public as partof the code changes to avoid TFT display crashes when dotclktiming is changed while buffered text is being sent.
 * * 20240204 added GetState method; primarily to notify decoder/Goertzl side to go into 'standby/sleep' mode while the send side is active.
 */
	/*
	 * Given:
	 * 	Global Variables:
	 * 		TmInrvlCnt, KeyState, SymblCnt, RingBufrPntr1, RingBufrPntr2,
	 * 		Symbl
	 *
	 * 1. Is Time interval (TmInrvlCnt) > 0
	 * 2. If "Yes" (we're actively sending), Dec TmInrvlCnt
	 * Is Is TmInrvlCnt now = 0?
	 * 		If "NO" return (wait for next Interrupt).
	 * 		If "YES", Were we sending a Keyup (Space) or a KeyDwn?.
	 * 			If KeyDwn then need to change to Keyup, and setup inter-character break (1 time interval)
	 * 			If Keyup, and are we actively processing a letter?
	 * 5. 			If "YES" (SymblCnt >0) continue to unpack (Goto SymblCnt >0)
	 * 6. 			If "NO" (SymblCnt =0) continue space interval to from a letter break
	 * 				(+2 time intervals for a total of 3 continuous Keyup periods)
	 * 				 Exit to wait for next interrupt.
	 *
	 * Then if here(KeyUp & TmIntrvlCnt = 0):
	 * Is there a symbol waiting to be processed?
	 * 7. 	If "NO" (RingBufrPntrs are =) Exit to wait for next interrupt.
	 * 8. 	If "YES" (RingBufrPntr1 != RingBufrPntr2), there's symbol to be processed,
	 *    	get it( inc RingBufrPntr2), set SymbCnt = 15.
	 *    	(DO While loop) shift "Symbl" left an dec SymbCnt to the 1st active bit
	 *    	(SymblCnt equals the number of symbols to be sent) (Goto SymblCnt >0)
	 *
	 *  SymblCnt >0:
	 *  	shift "Symbl" left
	 *      Is this symbol a "dit" (0) or a "dah" (1)?
	 *      	its a "dit" set TmInrvlCnt = 1
	 *      	its a "dah" set TmInrvlCnt = 3
	 *    Set KeyDwn Exit to wait for next interrupt.
	 */

///////////////////////////////////////////////////////////////////////////

#ifndef INC_CWSNDENGN_H_
#define INC_CWSNDENGN_H_
#include "stdint.h"
//#include "main.h"
#include "TFT_eSPI.h"
#include "TFTMsgBox.h"

#define KEY GPIO_NUM_13    // LED connected to GPIO2
#define Key_Dwn 1
#define Key_Up 0
class CWSNDENGN
{
private:
	esp_timer_handle_t *DotClk;
	TFT_eSPI *ptft;
	TFTMsgBox *pMsgBx;
	int TmInrvlCnt;
	bool KeyDwn;
	bool LtrBkFlg;
	bool ActvFlg;
	bool SpcFlg;
	bool SOTFlg;
	bool StrTxtFlg;
	bool TuneFlg;
	bool LstNtrySpcFlg;
	
	int curWPM;
	int state;
	int SymblCnt;
	int RingBufrPntr1;
	int RingBufrPntr2;
	int intcntr;//used for debugging
	unsigned int uSec;
	unsigned int SndWPM;
	uint16_t Symbl;
	char SndBuf[400];//JMH for ESP32 changed buffer size from 160 to 400 
	void ConfigKey(bool KeyState);
	int CalcARRval(int wpm);
	int AdvncPntr(int pntr);
	//void ShwWPM(int wpm);
	uint16_t ChrToSymb(char* CurChr);

public:
	bool RfrshSpd;
	CWSNDENGN(esp_timer_handle_t *Timr_Hndl, TFT_eSPI *tft_ptr, TFTMsgBox *pMsgBx_ptr);
	bool IsActv(void);
	bool LstNtrySpc(void);
	int Intr(void);
	void AddNewChar(char* Nuchr);
	void LdMsg(char *Msg, size_t len);
	void IncWPM(void);
	void DecWPM(void);
	void Tune(void);
	int Delete(void);
	void SOTmode(void);
	void StrTxtmode(void);
	void AbortSnd(void);
	void RefreshWPM(void);
	int GetWPM(void);
	int GetState(void);
	bool GetSOTflg(void);
	void SetWPM(int newWPM);
	void ShwWPM(int wpm);
};
#endif /* INC_CWSNDENGN_H_ */
