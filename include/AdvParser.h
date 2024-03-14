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
 * 20240206 added StrchdDah property
 * 20240313 added SrchRplc_stuct & SrchRplcDict[]
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

struct SrchRplc_stuct
{
  char srchTerm[10];
  char NuTerm[10];
};
class AdvParser
{
private:
    /* Properties */
    SrchRplc_stuct SrchRplcDict[70] ={
        {"PD", "AND"}, //0
        {"PY", "ANY"}, //1
        {"PT", "ANT"}, //2
        {"CP", "CAN"}, //3
        {"QY", "MAY"}, //4
        {"S2", "SUM"}, //5
        {"WHW", "WHAT"}, //6
        {"UAN", "UP"}, //7
        {"WJS", "WATTS"}, //8
        {"KNS", "YES"}, //9
        {"PEK", "WEEK"}, //10
        {"NAG", "NAME"}, //11
        {"SAG", "SAME"}, //12
        {"TIG", "TIME"}, //13
        {"QLK", "TALK"}, //14
        {"TB3", "73"}, //15
        {"SO9", "SOON"}, //16
        {"MPY", "MANY"}, //17
        {"SI6", "SIDE"}, //18
        {"QDE", "MADE"}, //19
        {"LWE", "LATE"}, //20
        {"THW", "THAT"}, //21
        {"THP", "THAN"}, //22
        {"TMN", "ON"}, //23
        {"PLL", "WELL"}, //24
        {"SJE", "SAME"}, //25
        {"CPT", "CANT"}, //26
        {"0VE", "MOVE"}, //27
        {"RLN", "RAIN"}, //28
        {"D9T", "DONT"}, //28
        {"TNN", "GN"}, //30
        {"TNO", "GO"}, //31
        {"SOG", "SOME"}, //32
        {"D9T", "DONT"}, //33
        {"CHW", "CHAT"}, //34
        {"WPT", "WANT"}, //35
        {"W5N", "WHEN"}, //36
        {"PNT", "WENT"}, //37
        {"6IS", "THIS"}, //38
        {"PEK", "WEEK"}, //39
        {"THJ", "THAT"}, //40
        {"9LY", "ONLY"}, //41
        {"WXST", "JUST"}, //42
        {"TNET", "GET"}, //43
        {"EAEA", "REA"}, //44
        {"DAKT", "DAY"}, //45
        {"TDNG", "TTING"}, //46
        {"CETN", "CAN"}, //47
        {"QSMT", "QSO"}, //48
        {"INTN", "ING"}, //49
        {"SINT", "SUN"}, //50
        {"MMMK", "OOK"}, //51
        {"GMTT", "GOT"}, //52
        {"TTTN", "ON"}, //53
        {"WEUT", "PUT"}, //54
        {"TBVT", "73"}, //55
        {"INME", "ING"}, //56
        {"EZNG", "ETTING"}, //57
        {"DTYL", "XYL"}, //58
        {"GAEE", "GRE"}, //59
        {"NKEE", "NCE"}, //60
        {"ARKT", "ARY"}, //61
        {"SNOAT", "SNOW"}, //62
        {"TELAI", "TELL"}, //63
        {"TTTAN", "OP"}, //64
        {"TTEAE", "GRE"}, //65
        {"KTES", "YES"}, //66
        {"C9DX", "CONDX"}, //67
        {"MKT", "MY"}, //68
        {"EXCA", "EXTRA"}, //69
    };
    bool AllDah;
    bool NewSpltVal;
    bool StrchdDah; //used mainly to steer which rules to apply within the Bug1 rule set (when long dahs are detected certain simple rules are bypassed)
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
    int LstLtrPrntd = 0; //MsgBuf indx pointer to character printed as Debug output
    //int wpm =0; //upated from DcodeCW.cpp through this class method EvalTimeData()
    float DahVarPrcnt;
    uint16_t TmpUpIntrvls[IntrvlBufSize];
    uint16_t TmpDwnIntrvls[IntrvlBufSize];
    uint16_t DitDahSplitVal;
    uint16_t NuSpltVal = 0;
    uint16_t DitIntrvlVal; //used as sanity test/check in 'bug' letterbrk rule set; 20240129 running average of the last 6 dits
    uint16_t WrdBrkVal; // serves in post parser as the value to insert a space in the reconstructed character string
    unsigned int SymbSet;
    unsigned int LstLtrBrkCnt = 0;//track the number of keyevents since last letterbreak.
    uint16_t UnitIntvrlx2r5; //basic universal symbol interval; i.e. a standard dit X 2.4; used in b1 rule set to find letter breaks
    uint16_t Bg1SplitPt; //bug1 rule set dit/dah decision value; derived from UnitIntvrlx2r5
    char BrkFlg;
    /* Methods */
    bool Tst4LtrBrk(int& n);
    bool PadlRules(int& n);
    bool Bug1Rules(int& n);
    bool Bug2Rules(int& n);
    bool CootyRules(int& n);
    bool Cooty2Rules(int& n);
    bool SloppyBgRules(int& n);
    bool SKRules(int& n); //true straight key(i.e. J38)
    void insertionSort(uint16_t arr[], int n);
	void SetSpltPt(Buckt_t arr[], int n);
    int AdvSrch4Match(int n, unsigned int decodeval, bool DpScan);
    bool SrchAgn(int n);
    int BldCodeVal(int Start, int LtrBrk);
    int FindLtrBrk(int Start, int End);
    void SyncTmpBufA(void);
    void PrintThisChr(void);
    int DitDahBugTst(void); //returns 2 for unknown; 0 for paddle; 1 for bug
    void Dcode4Dahs(int n);
    void FixClassicErrors(void);
    int SrchEsReplace(int MsgBufIndx, char srchTerm[10], char NuTerm[10]);
    char TmpBufA[MsgbufSize - 5];

public:
	AdvParser(void); //TFT_eSPI *tft_ptr, char *StrdTxt
    //void EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr, int curWPM);
	void EvalTimeData(void);
    char Msgbuf[MsgbufSize];
    char LtrHoldr[30];
    int LtrPtr = 0;
    /*controls debug USB serial print*/
    bool Dbug = false;
    int KeyType = 0;//used for the display's status indicator("S" or "E")
    float AvgSmblDedSpc;
    int GetMsgLen(void);
    uint16_t KeyUpIntrvls[IntrvlBufSize];
    uint16_t KeyDwnIntrvls[IntrvlBufSize];
    int KeyUpPtr = 0;
    int KeyDwnPtr = 0;
    int wpm =0; //upated from DcodeCW.cpp
	
};

#endif /* INC_ADVPARSER_H_ */