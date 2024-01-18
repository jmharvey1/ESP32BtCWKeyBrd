/*
 * AdvParser.h
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */
/* 
 * 20240114 numerous adds to methods properties and constants to extend/enhance this class )
 * 20240117 added Dcode4Dahs() to class; 
 * */
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
    int BugKey;//controls wich parsibool AdvParser::DitDahBugTst(void)ng rules are used 0 = paddle; 1 = bug; 2 = cootie
    int MaxCntKyUpBcktPtr;

    uint8_t ExitPath[IntrvlBufSize];
    uint8_t ExitPtr; 
	Buckt_t KeyUpBuckts[15];
    Buckt_t KeyDwnBuckts[15];
    uint8_t SameSymblWrdCnt = 0;
    int KeyUpBucktPtr = 0;
    int KeyDwnBucktPtr = 0;
    int TmpUpIntrvlsPtr = 0;
    int LstLtrPrntd = 0; //MsgBuf indx pointer to charter printed as Debug output
    uint16_t TmpUpIntrvls[IntrvlBufSize];
    uint16_t TmpDwnIntrvls[IntrvlBufSize];
    uint16_t DitDahSplitVal;
    unsigned int SymbSet;
    unsigned int LstLtrBrkCnt = 0;
    char BrkFlg;
    bool Tst4LtrBrk(int n);
    bool PadlRules(int n);
    bool BugRules(int n);
    bool CootyRules(int n);
    bool Cooty2Rules(int n);
    void insertionSort(uint16_t arr[], int n);
	void SetSpltPt(Buckt_t arr[], int n);
    int AdvSrch4Match(unsigned int decodeval, bool DpScan);
    void PrintThisChr(void);
    int DitDahBugTst(void); //returns 2 for unknown; 0 for paddle; 1 for bug
    void Dcode4Dahs(int n);

public:
	AdvParser(void); //TFT_eSPI *tft_ptr, char *StrdTxt
    void EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr);
	char Msgbuf[MsgbufSize];
    /*controlls debug USB serial print*/
    bool Dbug = false;
    int KeyType = 0;//used for the display's status indicator("S" or "E")
    float AvgSmblDedSpc;
    int GetMsgLen(void);
    
	
};

#endif /* INC_ADVPARSER_H_ */