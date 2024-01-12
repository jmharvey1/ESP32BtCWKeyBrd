/*
 * AdvParser.h
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */

#ifndef INC_ADVPARSER_H_
#define INC_ADVPARSER_H_
#include "stdint.h"// need this to use char type
#include <stdio.h>
//#define RingBufSz 400
struct Buckt_t
{
	uint16_t Intrvl;
	uint8_t Cnt;
};

class AdvParser
{
private:
    bool AllDah;
    bool NewSpltVal;
    bool BugKey;//controls whether Bug parsing rules or paddle parsing rules are used
    int MaxCntKyUpBcktPtr;
    uint8_t ExitPath[50];
    uint8_t ExitPtr; 
	Buckt_t KeyUpBuckts[15];
    Buckt_t KeyDwnBuckts[15];
    int KeyUpBucktPtr = 0;
    int KeyDwnBucktPtr = 0;
    int TmpUpIntrvlsPtr = 0;
    uint16_t TmpUpIntrvls[150];
    uint16_t TmpDwnIntrvls[150];
    uint16_t DitDahSplitVal;
    unsigned int SymbSet;
    //char Msgbuf[30];
    char BrkFlg;
    bool Tst4LtrBrk(int n);
    void insertionSort(uint16_t arr[], int n);
	void SetSpltPt(Buckt_t arr[], int n);
    int AdvSrch4Match(unsigned int decodeval, bool DpScan);

public:
	AdvParser(void); //TFT_eSPI *tft_ptr, char *StrdTxt
    void EvalTimeData(uint16_t KeyUpIntrvls[50], uint16_t KeyDwnIntrvls[50], int KeyUpPtr, int KeyDwnPtr);
	char Msgbuf[30];
    bool Dbug = true;//controlls debug USB serial print
	
};

#endif /* INC_ADVPARSER_H_ */