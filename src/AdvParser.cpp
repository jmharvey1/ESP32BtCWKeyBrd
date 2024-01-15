/*
 * AdvParser.cpp
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */

/* 
 * 20240114 numerous changes/adds to bug and paddle letter break rule set "Tst4LtrBrk(int n)")
 */
#include "AdvParser.h"
#include "DcodeCW.h"

AdvParser::AdvParser(void) // TFT_eSPI *tft_ptr, char *StrdTxt
    {
        AvgSmblDedSpc = 1200/30;
        // ptft = tft_ptr;
        // pStrdTxt = StrdTxt;
        // ToneColor = 0;
    };
/*Main entry point to post process the key time intervals used to create the current word*/
void AdvParser::EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr)
{
    KeyDwnBucktPtr = KeyUpBucktPtr = 0; // reset Bucket pntrs
    BugKey = true;
    NewSpltVal = false;
    LstLtrPrntd = 0;
    if (KeyDwnPtr < KeyUpPtr) // this now should never happen
    {
        /*Houston, We Have a Problem*/
        // uint16_t shrttime =1000;
        int runtenrty = 2;
        for (int i = runtenrty; i < KeyUpPtr - 1; i++)
        {
            KeyUpIntrvls[i] = KeyUpIntrvls[i + 1];
        }
    }
    /*Copy the 2 timing array to local arrays*/
    TmpUpIntrvlsPtr = KeyDwnPtr;
    for (int i = 0; i < TmpUpIntrvlsPtr; i++)
    {
        TmpDwnIntrvls[i] = KeyDwnIntrvls[i];
        TmpUpIntrvls[i] = KeyUpIntrvls[i];
    }
    /*Now sort the raw tables*/
    insertionSort(KeyDwnIntrvls, KeyDwnPtr);
    insertionSort(KeyUpIntrvls, KeyUpPtr);
    KeyUpBuckts[KeyUpBucktPtr].Intrvl = KeyUpIntrvls[0]; // At this point KeyUpBucktPtr = 0
    KeyUpBuckts[KeyUpBucktPtr].Cnt = 1;
    KeyDwnBuckts[KeyDwnBucktPtr].Intrvl = KeyDwnIntrvls[0]; // At this point KeyDwnBucktPtr = 0
    KeyDwnBuckts[KeyDwnBucktPtr].Cnt = 1;
    /*Build the Key down Bucket table*/
    for (int i = 1; i < KeyDwnPtr; i++)
    {
        bool match = false;
        if ((float)KeyDwnIntrvls[i] <= (4 + (1.2 * KeyDwnBuckts[KeyDwnBucktPtr].Intrvl)))
        {
            KeyDwnBuckts[KeyDwnBucktPtr].Cnt++;
            match = true;
        }
        if (!match)
        {
            KeyDwnBucktPtr++;
            if (KeyDwnBucktPtr >= 15)
            {
                KeyDwnBucktPtr = 14;
                break;
            }
            KeyDwnBuckts[KeyDwnBucktPtr].Intrvl = KeyDwnIntrvls[i];
            KeyDwnBuckts[KeyDwnBucktPtr].Cnt = 1;
        }
    }
    /*Build the Key Up Bucket table*/
    for (int i = 1; i < KeyUpPtr; i++)
    {
        bool match = false;
        if ((float)KeyUpIntrvls[i] <= (4 + (1.2 * KeyUpBuckts[KeyUpBucktPtr].Intrvl)))
        {
            KeyUpBuckts[KeyUpBucktPtr].Cnt++;
            match = true;
        }
        if (!match)
        {
            KeyUpBucktPtr++;
            if (KeyUpBucktPtr >= 15)
            {
                KeyUpBucktPtr = 14;
                break;
            }
            KeyUpBuckts[KeyUpBucktPtr].Intrvl = KeyUpIntrvls[i];
            KeyUpBuckts[KeyUpBucktPtr].Cnt = 1;
        }
    }

    if (KeyDwnBucktPtr > 0 && KeyUpBucktPtr > 0)
    {
        if (Dbug)
        {
            for (int i = 0; i <= KeyDwnBucktPtr; i++)
            {
                printf(" KeyDwn: %3d/%3d; Cnt:%d\t", KeyDwnBuckts[i].Intrvl, (int)(4 + (1.2 * KeyDwnBuckts[i].Intrvl)), KeyDwnBuckts[i].Cnt);
            }
            printf("%d\n", 1 + KeyDwnBucktPtr);
            for (int i = 0; i <= KeyUpBucktPtr; i++)
            {
                printf(" KeyUp : %3d/%3d; Cnt:%d\t", KeyUpBuckts[i].Intrvl, (int)(4 + (1.2 * KeyUpBuckts[i].Intrvl)), KeyUpBuckts[i].Cnt);
            }
            printf("%d\n", 1 + KeyUpBucktPtr);
        }
        SetSpltPt(KeyDwnBuckts, KeyDwnBucktPtr);
        NewSpltVal = true;
    }
    if (Dbug)
    {
        printf("\nSplitPoint:%3d\t", DitDahSplitVal);
    }
    /*Set the "MaxCntKyUpBcktPtr" property with Key Up Bucket index with the most intervals*/
    uint8_t maxCnt = 0;
    for (int i = 0; i <= KeyUpBucktPtr; i++)
    {
        if (KeyUpBuckts[i].Cnt > maxCnt)
        {
            maxCnt = KeyUpBuckts[i].Cnt;
            MaxCntKyUpBcktPtr = i;
        }
    }
    /*Update/recalculat avg inter symbol key up time*/
    for(int i =0; i< KeyUpBuckts[0].Cnt; i++){
        AvgSmblDedSpc = (4*AvgSmblDedSpc +KeyUpBuckts[0].Intrvl)/5;
    }
    
    /*OK; now its time to build a text string*/
    /*1st decide which parsing rule set to use*/
    if (Dbug)
    {
        printf("AvgDedSpc:%0.1f\n", AvgSmblDedSpc);
        printf("\nKeyDwnBuckt Cnt: %d ", KeyDwnBucktPtr + 1);
    }
    uint8_t bgPdlCd = 0;
    /**/
    if (KeyDwnBucktPtr + 1 <= 2)
    {
        bgPdlCd = 1;
        if (KeyUpBucktPtr + 1 < 5)
        {
            if (MaxCntKyUpBcktPtr < KeyUpBucktPtr)
            {
                bgPdlCd = 2;
                if (1.5 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl < KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl)
                {
                    BugKey = false;
                    bgPdlCd = 3;
                }
                else if (TmpUpIntrvlsPtr >= 7)
                {
                    BugKey = false;
                    bgPdlCd = 4;
                }
                else
                    bgPdlCd = 5;
            }
            else
                bgPdlCd = 6;
        }
        else
        {
            if (MaxCntKyUpBcktPtr < KeyUpBucktPtr)
            {
                bgPdlCd = 7;
                if (1.5 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl < KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl)
                {
                    BugKey = false;
                    bgPdlCd = 8;
                }
                else if (TmpUpIntrvlsPtr >= 8)
                {
                    BugKey = false;
                    bgPdlCd = 9;
                }
                else
                    bgPdlCd = 10;
            }
            else
                bgPdlCd = 11;
        }
    }
    if (KeyDwnBucktPtr + 1 > 2)
    {
        bgPdlCd = 12;
        if (MaxCntKyUpBcktPtr < KeyUpBucktPtr)
        {
            bgPdlCd = 13;
            if (TmpUpIntrvlsPtr >= 8 && KeyDwnBucktPtr + 1 == 3)
            {
                if (KeyUpBucktPtr < 6)
                {
                    BugKey = false;
                    bgPdlCd = 14;
                }
                else
                    bgPdlCd = 15;
            }
            else
                bgPdlCd = 16;
            // if (1.75 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl < KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl){
            //     BugKey = false;
            //     bgPdlCd = 6;
            // }
        }
        else
            bgPdlCd = 17;
        float ratio1 = (float)(KeyUpBuckts[MaxCntKyUpBcktPtr].Cnt) / (float)(TmpUpIntrvlsPtr);
        if (ratio1 > 0.68)
        {
            BugKey = false;
            bgPdlCd = 18;
        }
        /*Hi Speed Code test & if above 35wpm use keyboard/paddle rule set*/
        if (KeyDwnBuckts[KeyDwnBucktPtr].Intrvl < 103)
        {
            BugKey = false;
            bgPdlCd = 19;
        }
        if (Dbug)
        {
            printf("Ratio %d/%d = %0.2f", (KeyUpBuckts[MaxCntKyUpBcktPtr].Cnt), (TmpUpIntrvlsPtr), ratio1);
        }
    }
    if (Dbug)
    {
        printf(" CD:%d\n", bgPdlCd);
    }
    if (!BugKey)
    {
        /*figure out inter element symbol time by finding the keyUpbucket with the most times*/
    }
    BgMode = BugKey; //let the outside world know what mode/rule set being used
    int n = 0;
    SymbSet = 1;
    /*Reset the string buffer (Msgbuf)*/
    this->Msgbuf[0] = 0;
    ExitPtr = 0;
    if (Dbug)
    {
        printf("LtrbrkHistory:");
        if (!BugKey)
            printf(" Paddle/KeyBoard\n");
        else
            printf(" Bug/Straight\n");
    }
    /*Rebuild/Parse this group of Key down/key Up times*/
    while (n < TmpUpIntrvlsPtr)
    {
        if (Dbug)
        {
            printf("Dwn: %3d\t", TmpDwnIntrvls[n]);
            if (n < KeyUpPtr)
                printf("Up: %3d\t", TmpUpIntrvls[n]);
            else
                printf("Up: ???\t");
        }
        SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        if (TmpDwnIntrvls[n] +8 > DitDahSplitVal) //if within *ms of the split value, its a 'dah'
            SymbSet += 1; // Reset the new bit to a 'Dah'
        
        if (Tst4LtrBrk(n))
        { // now test if the follow on keyup time represents a letter break
            /*if true we have a complete symbol set; So find matching character(s)*/
            int IndxPtr = AdvSrch4Match(SymbSet, true); // try to convert the current symbol set to text &
                                                        // and save/append the results to 'Msgbuf[]'
            //SymbSet = 1;                                // start a new symbolset
        }
        if (Dbug)
        {
            printf("\tLBrkCd: %d", ExitPath[n]);
            if (BrkFlg != NULL){
                printf("+\t");
                printf("SymbSet:%d\t", SymbSet);
                PrintThisChr();
                SymbSet = 1;//reset the symbolset for the next character 
            }else
                printf("\n");
               
        } else if (BrkFlg != NULL){
            PrintThisChr();
            SymbSet = 1;//reset the symbolset for the next character
        }
        n++;
    }
    /*Text string Analysis complete*/
    if (Dbug)
    {
        printf("%d; %d\n\n", KeyDwnPtr, KeyUpPtr);
        printf("AdvParse text: %s\n", this->Msgbuf);
        printf("\n--------\n\n");
    }
};

void AdvParser::insertionSort(uint16_t arr[], int n)
{
    for (int i = 1; i < n; i++)
    {
        uint16_t key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key)
        {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;
    }
};
/*for this group of keydown intervals find the value where any shorter inteval is a "Dit"
 & all longer times are "Dahs"
 Note: 'n' points to the last keyDwnbucket in this group*/
void AdvParser::SetSpltPt(Buckt_t arr[], int n)
{
    int i;
    uint16_t NewSpltVal;
    AllDah = true; // made this a class property so it could be used later in the "Tst4LtrBrk()" method
    bool AllDit = true;
    for (i = 1; i <= n; i++)
    {
        if ((arr[i - 1].Intrvl < DitDahSplitVal) || (arr[i].Intrvl < DitDahSplitVal))
            AllDah = false;
        if ((arr[i - 1].Intrvl > DitDahSplitVal) || (arr[i].Intrvl > DitDahSplitVal))
            AllDit = false;
        if (2 * arr[i - 1].Intrvl <= arr[i].Intrvl)
            break;
    }
    /*if this group of key down intervals is either All Dits or All dahs,
    then its pointless to reevaluate the "DitDahSplitVal"
    So abort this routine*/
    if ((AllDit || AllDah) && (DitDahSplitVal != 0))
        return;
    if (i < n)
        /*Lets make sure we're not going to come up with an absurd value*/
        if (3 * arr[i - 1].Intrvl <= arr[i].Intrvl)
            NewSpltVal = arr[i - 1].Intrvl + (arr[i].Intrvl - arr[i - 1].Intrvl) / 3;
        else
            NewSpltVal = arr[i - 1].Intrvl + (arr[i].Intrvl - arr[i - 1].Intrvl) / 2;
    else
        /*Lets make sure we're not going to come up with an absurd value*/
        if (3 * arr[0].Intrvl <= arr[n].Intrvl)
            NewSpltVal = arr[0].Intrvl + (arr[n].Intrvl - arr[0].Intrvl) / 3;
        else
            NewSpltVal = arr[0].Intrvl + (arr[n].Intrvl - arr[0].Intrvl) / 2;
    if (DitDahSplitVal == 0)
        DitDahSplitVal = NewSpltVal;
    else
        DitDahSplitVal = (3 * DitDahSplitVal + NewSpltVal) / 4;
};
/*for this group of keydown intervals(TmpDwnIntrvls[]) & the selected keyUp interval (TmpDwnIntrvls[n]),
use the test set out within to decide if this keyup time represents a letter break
& return "true" if it does*/
bool AdvParser::Tst4LtrBrk(int n)
{
    bool ltrbrkFlg = false;
    BrkFlg = NULL;
    /*Paddle Rule Set*/
    if (!BugKey)
    {
    /*Paddle or Keyboard rules*/
    /*Middle keyup test to see this keyup is twice the lenght of the one just before it, 
    If it is then call this one a letter break*/
    if (n > 0 && (TmpUpIntrvls[n]> 2.0*TmpUpIntrvls[n-1]) )
    {
        ExitPath[n] = 100;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one following it, 
    If it is then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n]> 2.0*TmpUpIntrvls[n+1]) )
    {
        ExitPath[n] = 101;
        BrkFlg = '+';
        return true;
    }  
        if (NewSpltVal)
        {
            if ((TmpUpIntrvls[n] >= 1.5 * 1.2 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl) &&
            (TmpUpIntrvls[n] > KeyDwnBuckts[0].Intrvl))
            {
                BrkFlg = '+';
                ExitPath[n] = 102;
                return true;
            } else ExitPath[n] = 103;
            if (MaxCntKyUpBcktPtr < KeyUpBucktPtr) // safety check, before trying the real test
            {
                // if (TmpUpIntrvls[n] >= KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl)
                if (TmpUpIntrvls[n] >= DitDahSplitVal)
                {
                    BrkFlg = '+';
                    ExitPath[n] = 104;
                    return true;
                }
            } else ExitPath[n] = 105;
        }
        else
        {
            if (TmpUpIntrvls[n] >= DitDahSplitVal)
            {
                BrkFlg = '+';
                ExitPath[n] = 106;
                return true;
            } else ExitPath[n] = 107;
        }
        return false;
    }
    /*Bug or Manual Key rules*/
    if (TmpUpIntrvls[n] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl)
    {
        ExitPath[n] = 0;
        BrkFlg = '+';
        return true;
    }
    if (!AllDah && (TmpUpIntrvls[n] <= 1.20 * KeyUpBuckts[0].Intrvl)) // could be working with the word "TO" want to skip this check
    {
        ExitPath[n] = 1;
        return false;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one just before it, 
    If it is then call this one a letter break*/
     if (n > 0 && (TmpUpIntrvls[n]> 2.4*TmpUpIntrvls[n-1]) )
    {
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one following it, 
    If it is then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n]> 2.4*TmpUpIntrvls[n+1]) )
    {
        ExitPath[n] = 3;
        BrkFlg = '+';
        return true;
    }    
    /*test that there is another keydown interval after this one*/
    if (n < TmpUpIntrvlsPtr - 1)
    {
        /*test for middle of 3 adjacent 'dahs'*/
        if (n >= 1)
        {
            if ((TmpDwnIntrvls[n - 1] > DitDahSplitVal) &&
                (TmpDwnIntrvls[n] > DitDahSplitVal) &&
                (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
            { // we are are surrounded by dahs
                if ((TmpUpIntrvls[n - 1] >= DitDahSplitVal) && (TmpUpIntrvls[n] > 1.2 * TmpUpIntrvls[n - 1]))
                {
                    ExitPath[n] = 4;
                    BrkFlg = '+';
                    return true;
                }
                else if ((TmpUpIntrvls[n] >= 0.8 * DitDahSplitVal) &&
                         (TmpUpIntrvls[n - 1] >= 0.8 * DitDahSplitVal) &&
                         (TmpDwnIntrvls[n] >= TmpDwnIntrvls[n - 1]) &&
                         (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n - 1]))
                { // we're the middle of a dah serries; but this one looks streched compared to its predecessor
                    ExitPath[n] = 5;
                    BrkFlg = '+';
                    return true;
                }
            }
        }
        /*test for the 1st of 2 adjacent 'dahs'*/
        if ((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
        {
            /*we have two adjcent dahs, set letter break if the key up interval is longer than the shortest of the 2 Dahs*/
            if ((TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n + 1]))
            {
                ExitPath[n] = 6;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n + 1] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n + 1]))
            { // this Dah has lot longer Keyup than the next dah; so this looks like a letter break
                ExitPath[n] = 7;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= TmpUpIntrvls[n + 1]) &&
                     (TmpDwnIntrvls[n] >= 1.3 * TmpDwnIntrvls[n + 1]))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 8;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= 1.4 * DitDahSplitVal))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 9;
                BrkFlg = '+';
                return true;
            }

            else
            {
                ExitPath[n] = 10;
                return false;
            }
            /*test for 2 adjacent 'dits'*/
        }
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*we have two adjcent dits, set letter break if the key up interval is 2.0x longer than the shortest of the 2 Dits*/
            if ((TmpUpIntrvls[n] > 1.5 * TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n + 1]))
            {
                ExitPath[n] = 11;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 12;
                return false;
            }
            /*test for dah to dit transistion*/
        }
        else if ((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*We have Dah to dit transistion set letter break only if key up interval is > 1.6x the dit interval*/
            if ((TmpUpIntrvls[n] > 1.6 * TmpDwnIntrvls[n + 1]))
            {
                ExitPath[n] = 13;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 14;
                return false;
            }
            /*test for dit to dah transistion*/
        }
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
        {
            /*We have Dit to Dah transistion set letter break only if key up interval is > 2x the dit interval*/
            if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n]))
            {
                ExitPath[n] = 15;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 16;
                return false;
            }
        }
        else
        {
            printf("Error: NO letter brk test");
            ExitPath[n] = 17;
            return ltrbrkFlg;
        }
        /*then this is the last keyup so letter brk has to be true*/
    }
    else
    {
        ExitPath[n] = 18;
        BrkFlg = '+';
        return true;
    }
    /*Should never Get Here*/
    ExitPath[n] = 19;
    printf("Error: NO letter brk test");
    return ltrbrkFlg;
};

int AdvParser::AdvSrch4Match(unsigned int decodeval, bool DpScan)
{
    int pos1 = linearSearchBreak(decodeval, CodeVal1, ARSIZE); // note: decodeval '255' returns SPACE character
    char TmpBufA[25];
    for (int i = 0; i < sizeof(TmpBufA); i++)
    {
        TmpBufA[i] = this->Msgbuf[i];
        if (this->Msgbuf[i] == 0)
            break;
    }

    if (pos1 < 0 && DpScan)
    { // did not find a match in the standard Morse table. So go check the extended dictionary
        pos1 = linearSearchBreak(decodeval, CodeVal2, ARSIZE2);
        if (pos1 < 0)
            sprintf(this->Msgbuf, "%s%s", TmpBufA, "*");
        else
        {

            sprintf(this->Msgbuf, "%s%s", TmpBufA, DicTbl2[pos1]);
        }
    }
    else
        sprintf(this->Msgbuf, "%s%s", TmpBufA, DicTbl1[pos1]); // sprintf( Msgbuf, "%s%s", Msgbuf, DicTbl1[pos1] );
    return pos1;
};
//////////////////////////////////////////////////////////////////////
void AdvParser::PrintThisChr(void){
    int curEnd = LstLtrPrntd;
    while(Msgbuf[curEnd] !=0){
        if (Dbug) printf("%c", this->Msgbuf[curEnd]);
        curEnd++;
    }
    if (Dbug) printf("\n");
    LstLtrPrntd = curEnd;
};
///////////////////////////////////////////////////////////////////////
/*Return the current string length of the AdvParser MsgBuf*/
int AdvParser::GetMsgLen(void){
    return this->LstLtrPrntd;
};