/*
 * TxtNtryBox.h
 *
 *  Created on: Jun 9, 2022
 *      Author: jim
 */

#ifndef INC_TXTNTRYBOX_H_
#define INC_TXTNTRYBOX_H_
#include "stdint.h"
#include "TFT_eSPI.h" // Graphics and font library for ILI9341 driver chip

//extern struct BtnParams btnparams;
extern esp_timer_handle_t DotClk_hndl;
extern TFT_eSPI tft;
//TxtNtryBox NtryBoxGrp[paramCnt];

struct BtnParams{ //type name
   int BtnXpos;  //Button X position
   int BtnWdth;  //Button Width
   int BtnYpos;  //Button X position
   int BtnHght;  //Button Height
   const char* Captn;
   unsigned int BtnClr;
   unsigned int TxtClr;
   bool IsNumbr;
   int BxIndx;
   int Option;
};
// struct DF_t {
// 	char MyCall[10];
// 	int WPM;
// 	int DeBug;
// };

//TxtNtryBox NtryBoxGrp[6];

class TxtNtryBox {
private:
	char BtnValue[80];//Can be up to 2 screen lines of characters
	esp_timer_handle_t *pDotClk;
	TFT_eSPI *ptft;
	int BtnXpos;  //Button X position
	int BtnWdth;  //Button Width
	int BtnYpos;  //Button X position
	int BtnHght;  //Button Height
	int ValXpos; //starting x location of the button's value
	const char* Captn;
	unsigned int BtnClr;//background color when button has "focus" (is selected)
	unsigned int TxtClr;
	const int fontH = 26;
	const int fontW = 12;
	const int ScrnW = 480;
	const int ScrnH = 320;
	void BldBtn(bool Hilight);//Rebuild button to show selected, or NOT selected

public:
	bool IsNumbr;
	int ValueLen;
	int BxIndx;
	int Option;// or mode; DeBug 0 => DeBug "off"; DeBug 1 => DeBug "on"
	TxtNtryBox(esp_timer_handle_t *Timr_Hndl, TFT_eSPI *tft_ptr);
	//TxtNtryBox(TIM_HandleTypeDef *TIM_ptr, MCUFRIEND_kbv *tft_ptr);
	virtual ~TxtNtryBox();
	void InitBox(BtnParams settings);
	void ShwData(void);
	void ShwCsr(int CsrPos);
	void ShwBGColor(unsigned int NewClr);
	void KillCsr(void);
	void SetValue(char* curval, int len);
	void UpDateValue(int UpDatePos, int CsrPos);
	void UpDateCaptn(char* NuCaptn);
	char* GetValue(void);
	int GetValLen(void);
};

#endif /* INC_TXTNTRYBOX_H_ */
