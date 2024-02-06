/*
 * AdvParser.h
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */
/* 
 * 20240114 numerous adds to methods properties and constants to extend/enhance this class )
 * 20240117 added Dcode4Dahs() to class; 
 * 20240205 added WrdBrkVal
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
    int BugKey;//controls wich parsing rules are used 0 = paddle; 1 = bug; 2 = cootie
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
    int wpm =0; //upated from DcodeCW.cpp through this class method EvalTimeData()
    uint16_t TmpUpIntrvls[IntrvlBufSize];
    uint16_t TmpDwnIntrvls[IntrvlBufSize];
    uint16_t DitDahSplitVal;
    uint16_t DitIntrvlVal; //used as sanity test/check in 'bug' letterbrk rule set; 20240129 running average of the last 6 dits
    uint16_t WrdBrkVal; // serves in post parser as the value to insert a space in the reconstructed character string
    unsigned int SymbSet;
    unsigned int LstLtrBrkCnt = 0;
    uint16_t UnitIntvrlx2r5; //basic universal symbol interval; i.e. a standard dit X 2.4; used in b1 rule set to find letter breaks
    uint16_t Bg1SplitPt; //bug1 rule set dit/dah decision value; derived from UnitIntvrlx2r5
    char BrkFlg;
    bool Tst4LtrBrk(int& n);
    bool PadlRules(int& n);
    bool Bug1Rules(int& n);
    bool Bug2Rules(int& n);
    bool CootyRules(int& n);
    bool Cooty2Rules(int& n);
    bool SKRules(int& n); //true straight key(i.e. J38)
    void insertionSort(uint16_t arr[], int n);
	void SetSpltPt(Buckt_t arr[], int n);
    int AdvSrch4Match(int n, unsigned int decodeval, bool DpScan);
    void PrintThisChr(void);
    int DitDahBugTst(void); //returns 2 for unknown; 0 for paddle; 1 for bug
    void Dcode4Dahs(int n);

public:
	AdvParser(void); //TFT_eSPI *tft_ptr, char *StrdTxt
    void EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr, int curWPM);
	char Msgbuf[MsgbufSize];
    /*controls debug USB serial print*/
    bool Dbug = false;
    int KeyType = 0;//used for the display's status indicator("S" or "E")
    float AvgSmblDedSpc;
    int GetMsgLen(void);
    
	
};

#endif /* INC_ADVPARSER_H_ */