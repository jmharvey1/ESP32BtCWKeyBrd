/*
 * AdvParser.h
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */
/* 
 * 20240114 numerous adds to methods properties and constants to extend/enhance this class )
 */
#ifndef INC_ADVPARSER_H_
#define INC_ADVPARSER_H_
#include "stdint.h"// need this to use char type
#include <stdio.h>
#define IntrvlBufSize 200
#define MsgbufSize 50
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
    uint8_t ExitPath[IntrvlBufSize];
    uint8_t ExitPtr; 
	Buckt_t KeyUpBuckts[15];
    Buckt_t KeyDwnBuckts[15];
    int KeyUpBucktPtr = 0;
    int KeyDwnBucktPtr = 0;
    int TmpUpIntrvlsPtr = 0;
    int LstLtrPrntd = 0; //MsgBuf indx pointer to charter printed as Debug output
    uint16_t TmpUpIntrvls[IntrvlBufSize];
    uint16_t TmpDwnIntrvls[IntrvlBufSize];
    uint16_t DitDahSplitVal;
    unsigned int SymbSet;
    char BrkFlg;
    bool Tst4LtrBrk(int n);
    void insertionSort(uint16_t arr[], int n);
	void SetSpltPt(Buckt_t arr[], int n);
    int AdvSrch4Match(unsigned int decodeval, bool DpScan);
    void PrintThisChr(void);

public:
	AdvParser(void); //TFT_eSPI *tft_ptr, char *StrdTxt
    void EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr);
	char Msgbuf[MsgbufSize];
    /*controlls debug USB serial print*/
    bool Dbug = false;
    bool BgMode = false;//used for the display's status indicator("S" or "E")
    float AvgSmblDedSpc;
    int GetMsgLen(void);
    
	
};

#endif /* INC_ADVPARSER_H_ */