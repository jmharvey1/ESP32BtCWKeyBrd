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
#define SrchDictSize 160
struct Buckt_t
{
	uint16_t Intrvl;
	uint8_t Cnt;
};

struct SrchRplc_stuct
{
  const char srchTerm[10];
  const char NuTerm[10];
  int ChrCnt;
  int Rule;
};
class AdvParser
{
private:
    /* Properties */
    const SrchRplc_stuct SrchRplcDict[SrchDictSize] ={
        {"PD", "AND", 2, 1}, //0
        {"PY", "ANY", 2, 1}, //1 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"PT", "ANT", 2, 1}, //2 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"CP", "CAN", 2, 3}, //3 if (this->StrLength == 1)
        {"QY", "MAY", 2, 2}, //4 if (NdxPtr == 0 || (NdxPtr > 0 && this->Msgbuf[NdxPtr - 1] != 'C'))
        {"S2", "SUM", 2, 0}, //5
        {"WHW", "WHAT", 3, 0}, //6
        {"UAN", "UP", 3, 0}, //7
        {"WJS", "WATTS", 3, 0}, //8
        {"KNS", "YES", 3, 0}, //9
        {"PEK", "WEEK", 3, 0}, //10
        {"NAG", "NAME", 3, 1}, //11 /*search term & msgbuf size are the same*/
        {"SAG", "SAME", 3, 5}, //12
        {"TIG", "TIME", 3, 0}, //13
        {"QLK", "TALK", 3, 0}, //14
        {"TB3", "73", 3, 0}, //15
        {"SO9", "SOON", 3, 0}, //16
        {"MPY", "MANY", 3, 0}, //17
        {"SI6", "SIDE", 3, 0}, //18
        {"QDE", "MADE", 3, 2}, //19 if (NdxPtr == 0 || (NdxPtr > 0 && this->Msgbuf[NdxPtr - 1] != 'C'))
        {"LWE", "LATE", 3, 0}, //20
        {"THW", "THAT", 3, 0}, //21
        {"THP", "THAN", 3, 0}, //22
        {"TMN", "ON", 3, 0}, //23
        {"PLL", "WELL", 3, 0}, //24
        {"SJE", "SAME", 3, 0}, //25
        {"CPT", "CANT", 3, 0}, //26
        {"0VE", "MOVE", 3, 0}, //27
        {"RLN", "RAIN", 3, 0}, //28
        {"D9T", "DONT", 3, 0}, //28
        {"TNN", "GN", 3, 0}, //30
        {"TNO", "GO", 3, 7}, //31
        {"SOG", "SOME", 3, 1}, //32 /*search term & msgbuf size are the same*/
        {"D9T", "DONT", 3, 0}, //33
        {"CHW", "CHAT", 3, 0}, //34
        {"WPT", "WANT", 3, 0}, //35
        {"W5N", "WHEN", 3, 0}, //36
        {"PNT", "WENT", 3, 0}, //37
        {"6IS", "THIS", 3, 0}, //38
        {"PEK", "WEEK", 3, 0}, //39"
        {"THJ", "THAT", 3, 0}, //40
        {"9LY", "ONLY", 3, 0}, //41
        {"WXST", "JUST", 4, 0}, //42
        {"TNET", "GET", 4, 0}, //43
        {"EAEA", "REA", 4, 0}, //44
        {"DAKT", "DAY", 4, 0}, //45
        {"TDNG", "TTING", 4, 0}, //46
        {"CETN", "CAN", 4, 0}, //47
        {"QSMT", "QSO", 4, 0}, //48
        {"INTN", "ING", 4, 0}, //49
        {"SINT", "SUN", 4, 0}, //50
        {"MMMK", "OOK", 4, 0}, //51
        {"GMTT", "GOT", 4, 0}, //52
        {"TTTN", "ON", 4, 0}, //53
        {"WEUT", "PUT", 4, 0}, //54
        {"TBVT", "73", 4, 0}, //55
        {"INME", "ING", 4, 0}, //56", "PUM", 4,0}, //91      
        {"EZNG", "ETTING", 4, 0}, //57
        {"DTYL", "XYL", 4, 0}, //58
        {"GAEE", "GRE", 4, 0}, //59
        {"NKEE", "NCE", 4, 0}, //60
        {"ARKT", "ARY", 4, 0}, //61
        {"SNOAT", "SNOW", 5, 0}, //62
        {"TELAI", "TELL", 5, 0}, //63
        {"TTTAN", "OP", 5, 0}, //64
        {"TTEAE", "GRE", 5, 0}, //65
        {"KTES", "YES", 4, 0}, //66
        {"C9DX", "CONDX", 4, 0}, //67
        {"MKT", "MY", 3, 1}, //68 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"EXCA", "EXTRA", 4, 0}, //69
        {"AP", "AGE", 2, 1}, //70 if(this->StrLength + 1 == this->SrchRplcDict[STptr].ChrCnt){ /*search term & msgbuf size are the same*/
        {"C9S" , "CONS", 3, 0}, //71
        {"DNG", "TING", 3, 0}, //72
        {"LEP", "LEAN", 3, 0}, //73
        {"MEOT", "GOT", 4, 0}, //74
        {"6E", "THE", 2, 0}, //75
        {"6A", "THA", 2, 0}, //765
        {"VFG", "VING", 3, 0}, //77
        {"HEWH", "HEATH", 4, 0}, //78
        {"GEMIN", "GETTIN", 5,0}, //79
        {"QKING", "MAKING", 5,0}, //80
        {"HJ", "HAM", 2, 1}, //81 /*search term & msgbuf size are the same*/
        {"M<AR>", "QR", 5,0}, //82
        {"NTX", "WX", 3, 1}, //83 /*search term & msgbuf size are the same*/
        {"SMAU", "SQR", 4,0}, //84
        {"PST", "WEST", 3, 1}, //85 /*search term & msgbuf size are the same*/
        {"S0E", "SOME", 3, 0}, //861
        {"TWAE", "TAKE", 4,0}, //87
        {"LFUX", "LINUX", 4,0}, //88
        {"ANE2", "ABO", 4,0}, //89
        {"WERO", "PRO", 4,0}, //90
        {"WEUM", "PUM", 4,0}, //91
        {"STOAN", "STOP", 5, 0}, //92
        {"T0", "TOM", 2, 1}, //93 /*search term & msgbuf size are the same*/
        {"9E", "ONE", 2, 1}, //94 /*search term & msgbuf size are the same*/
        {"TTAK", "MAK", 4,0}, //95
        {"WWER", "WATER", 4,0}, //96
        {"TKT", "QT", 3, 0}, //97
        {"AQG", "AMAG", 3, 0}, //98
        {"GRWS", "GRATS", 4, 0}, //99
        {"POTH", "ANOTH", 4, 6}, //100
        {"TKSO", "QSO", 4, 0}, //101
        {"W9D", "WOND", 4, 0}, //102
        {"I<KN>", "ING", 5, 0}, //103
        {"TNL", "GL", 3, 0}, //104
        {"ATENT", "WENT", 5, 0}, //105
        {"NTT", "Y", 3, 0}, //106
        {"AIO", "LO", 3, 0}, //107
        {"HPG", "HANG", 3, 0}, //108
        {"TKS", "QS", 3, 8}, //109
        {"FTS", "FB", 3, 0}, //110
        {"CP", "CAN", 2, 4}, //111
        {"BTET", "BK", 4, 0}, //112
        {"CEP", "KEEP", 3, 0}, //113
        {"PDT", "ANDT", 3, 0}, //114
        {"PDT", "ANDT", 3, 0}, //115
        {"DOAG", "DOWN", 4, 0}, //116
        {"TOUH", "TO USE", 4, 0}, //117
        {"ANAD", "TO PAD", 4, 9}, //118
        {"NNK", "CK", 3, 0}, //119
        {"ATPE", "ANGE", 4, 0}, //120
        {"TEKE", "NC", 4, 0}, //121
        {"ZKE", "MIKE", 3, 0}, //122
        {"DFG", "DING", 3, 0}, //123
        {"NTTE", "NG", 4, 0}, //124
        {"AAES", "ARS", 4, 0}, //125
        {"AAI", "AL", 3, 0}, //126
        {"TDKE", "MIKE", 4, 0}, //127
        {"KTE", "YE", 3, 0}, //128
        {"RTMB", "ROB", 4, 0}, //129
        {"DAEI", "DRI", 4, 0}, //130
        {"HADMI", "HW?", 5, 0}, //131
        {"7MS", "73", 3, 0}, //132
        {"TSUT", "BUT", 4, 0}, //133
        {"IBS", "ITS", 3, 0}, //134
        {"ADLL", "WILL", 4, 0}, //135
        {"TTTAID", "OLD", 6, 0}, //136
        {"SKRT", "START", 4, 0}, //137
        {"COG", "COME", 3, 1}, //138
        {"Q6", "QTH", 2, 0}, //139
        {"ORI9", "ORION", 4, 0}, //140
        {"HPD", "HAND", 3, 0}, //141
        {"G?", "GUD", 2, 0}, //142
        {"L9G", "LONG", 3, 0}, //143
        {"<KN>AR", "YEAR", 6, 0}, //144
        {"BPD", "BAND", 3, 0}, //145
        {"SQLL", "SMALL", 4, 0}, //146
        {"AKS", "WAS", 3, 1}, //147
        {"ERLEA", "PLEA", 5, 0}, //148
        {"AGMT", "AGO", 4, 0}, //148
        {"6ST", "BEST", 3, 0}, //149
        {"HETVE", "HAVE", 5, 0}, //150
        {"I9", "ION", 5, 0}, //150
        {"NTEQ", "CQ", 4, 0}, //151
        {"NNQ", "CQ", 3, 0}, //152
        {"TYH", "QTH", 3, 0}, //153
        {"LTURN", "RETURN", 5, 0}, //154
        {"WAMS", "WATTS", 4, 0}, //155

    };
    bool AllDah;
    bool AllDit;
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
    int StrLength = 0; //MsgBuf indx pointer to character printed as Debug output
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
    int SrchEsReplace(int MsgBufIndx, int STptr, const char srchTerm[10], const char NuTerm[10]);
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