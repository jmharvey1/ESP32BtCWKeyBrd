/*
 * AdvParser.cpp
 *
 *  Created on: Jan 7, 2024
 *      Author: jim (KW4KD)
 */
/*
 * NOTE: Under current code (defined in DcodeCW.cpp) this code/class is NOT envoked for single letters, and/or
 * CW speeds below 14 WPM.
 */

/*
 * 20240114 numerous changes/adds to bug and paddle letter break rule set "Tst4LtrBrk(int n)")
 * 20240116 added test to ensure msgbuf doesn't overflow
 * 20240117 added Dcode4Dahs() to class; parses 4 dahs into "TO" or "OT", "MM" also a possible result 
 * 20240119 added test for paddle/keybrd by finding all dahs have the same interval; added subparsing to AdvSrch4Match() method
 * 20240119 also revised SetSpltPt() method to better find the dit dah decision point
 * 20240120 reworked DitDahBugTst() method to better detect bug vs paddle/keyboard signals plus minor tweaks to bug rule set
 * 20240122 revised DitDahSplitVal averaging algorithm to be based on last 30 symbol set elements
 * 20240123 Added letterbrk test "2" to look for lttrbk based on "long" dah 
 * 20240124 Modified SetSpltPt method for finding the Dit/dah split point. Added new private class variable DitIntrvlVal
 * */
#include "AdvParser.h"
#include "DcodeCW.h"
#include "Goertzel.h"
AdvParser::AdvParser(void) // TFT_eSPI *tft_ptr, char *StrdTxt
{
    AvgSmblDedSpc = 1200 / 30;
    // ptft = tft_ptr;
    // pStrdTxt = StrdTxt;
    // ToneColor = 0;
};
/*Main entry point to post process the key time intervals used to create the current word*/
void AdvParser::EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr)
{
    KeyDwnBucktPtr = KeyUpBucktPtr = 0; // reset Bucket pntrs
    bool prntOvrRide = false;
    bool oldDbugState = false;
    NewSpltVal = false;
    LstLtrPrntd = 0;
    if (KeyDwnPtr != KeyUpPtr) // this now should never happen
    {
        /*Houston, We Have a Problem*/
        printf("\n!! ERROR KeyUP KeyDwn size MisMatch !!\n");
        printf("   KeyDwnPtr:%d KeyUpPtr:%d\n", KeyDwnPtr, KeyUpPtr);
        // prntOvrRide = true;
        // oldDbugState = this->Dbug;
        // this->Dbug = true;
        
    }
    /*Copy the 2 referenced timing arrays to local arrays*/
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

    if (KeyDwnBucktPtr >= 2 && KeyUpBucktPtr >= 2)
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
    /*Update/recalculate avg inter symbol key up time*/
    for (int i = 0; i < KeyUpBuckts[0].Cnt; i++)
    {
        AvgSmblDedSpc = (4 * AvgSmblDedSpc + KeyUpBuckts[0].Intrvl) / 5;
    }

    /*OK; now its time to build a text string*/
    /*But 1st, decide which parsing rule set to use*/
    if (Dbug)
    {
        printf("AvgDedSpc:%0.1f\n", AvgSmblDedSpc);
        printf("\nKeyDwnBuckt Cnt: %d ", KeyDwnBucktPtr + 1);
    }
    /*Figure out which Key type rule set to use. Paddle(BugKey = 0)/Bug(BugKey = 1)/Cootie(BugKey = 2)*/
    uint8_t bgPdlCd = 0;
    BugKey = 1; // start by assuming its a standard bug type key
    /*select 'cootie' key based on extreme short keyup timing relative to keydown time*/
    //printf("\nAvgSmblDedSpc:%d; KeyDwnBuckts[0].Intrvl:%d; Intrvl / 3: %0.1f\n", (int)AvgSmblDedSpc, KeyDwnBuckts[0].Intrvl, KeyDwnBuckts[0].Intrvl / 2.7);
    if (AvgSmblDedSpc < KeyDwnBuckts[0].Intrvl / 2.7)
    {
        BugKey = 2;
        bgPdlCd = 50;
    }
    else if (3 * KeyDwnBuckts[0].Intrvl < AvgSmblDedSpc)
    {
        BugKey = 3;
        bgPdlCd = 60;
    }
    else
    {
        /*the following tests choose between paddle or bug*/
        switch (this->DitDahBugTst()) //returns 0, 1, or 5 for paddle, 2,3,& 4 for bug, &  6 for unknown;
        {
        case 0:
            /* its a paddle */
            bgPdlCd = 70; // because all dahs have the same duration 
            BugKey = 0;
            break;
        case 1:
            /* its a paddle */
            bgPdlCd = 71;
            BugKey = 0;
            break;
        case 2:
            /* its a bug */
            bgPdlCd = 80;
            BugKey = 1;
            break;
        case 3:
            /* its a bug */
            bgPdlCd = 81;
            BugKey = 1;
            break;
        case 4:
            /* its a bug */
            bgPdlCd = 82;
            BugKey = 1;
            break;         
        case 5:
            /* its a paddle */
            bgPdlCd = 72;
            BugKey = 0;
            break;       

        default:
            /*if we are here BugKey value equals 1; i.e., "bug/straight"*/
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
                            BugKey = 0; /* its a paddle */
                            bgPdlCd = 3;
                        }
                        else if (TmpUpIntrvlsPtr >= 7)
                        {
                            BugKey = 0;/* its a paddle */
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
                            BugKey = 0;/* its a paddle */
                            bgPdlCd = 8;
                        }
                        else if (TmpUpIntrvlsPtr >= 8)
                        {
                            BugKey = 0;/* its a paddle */
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
                            BugKey = 0;
                            bgPdlCd = 14;
                        }
                        else
                            bgPdlCd = 15;
                    }
                    else
                        bgPdlCd = 16;
                    // if (1.75 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl < KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl){
                    //     BugKey = 0;
                    //     bgPdlCd = 6;
                    // }
                }
                else
                    bgPdlCd = 17;
                float ratio1 = (float)(KeyUpBuckts[MaxCntKyUpBcktPtr].Cnt) / (float)(TmpUpIntrvlsPtr);
                if (ratio1 > 0.68)
                {
                    BugKey = 0;
                    bgPdlCd = 18;
                }
                /*Hi Speed Code test & if above 35wpm use keyboard/paddle rule set*/
                if (KeyDwnBuckts[KeyDwnBucktPtr].Intrvl < 103)
                {
                    BugKey = 0;
                    bgPdlCd = 19;
                }
                if (Dbug)
                {
                    printf("Ratio %d/%d = %0.2f", (KeyUpBuckts[MaxCntKyUpBcktPtr].Cnt), (TmpUpIntrvlsPtr), ratio1);
                }
            }
            break;
        }
    }
    /*End of select Key type (BugKey) code*/
    KeyType = BugKey; // let the outside world know what mode/rule set being used
    switch (BugKey)
    {
    case 0: // paddle/keyboard
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 1: // Bug/straight key
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 2: // cootie type A
        ModeCnt = 3; // DcodeCW.cpp use "cootie" settings/timing ; no glitch detection
        break;
    case 3: // cootie typ B
        ModeCnt = 3;  // DcodeCW.cpp use "cootie" settings/timing ; no glitch detection
        break;

    default:
        break;
    }
    SetModFlgs(ModeCnt);//DcodeCW.cpp routine; Update DcodeCW.cpp timing settings & ultimately update display status line 
    CurMdStng(ModeCnt);// convert to ModeVal for Goertzel.cpp to use (mainly glitch control)

    if (Dbug)
    {
        printf(" CD:%d\n", bgPdlCd);
    }
    if (BugKey == 0)
    {
        /*figure out inter element symbol time by finding the keyUpbucket with the most times*/
    }

    int n = 0;
    SymbSet = 1;
    /*Reset the string buffer (Msgbuf)*/
    this->Msgbuf[0] = 0;
    ExitPtr = 0;
    if (Dbug)
    {
        printf("Key Type:");
        switch (BugKey)
        {
        case 0:
            printf(" Paddle/KeyBoard\n");
            break;
        case 1:
            printf(" Bug/Straight\n");
            break;
        case 2:
            printf(" Cootie\n");
            break;
        case 3:
            printf(" ShrtDits\n");
            break;
        default:
            printf(" ???\n");
            break;
        }
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
        SymbSet = SymbSet << 1;                    // append a new bit to the symbolset & default it to a 'Dit'
        if (TmpDwnIntrvls[n] + 8 > DitDahSplitVal) // if within *ms of the split value, its a 'dah'
            SymbSet += 1;                          // Reset the new bit to a 'Dah'
        int curN = n+1;
        /* now test if the follow on keyup time represents a letter break */
        if (Tst4LtrBrk(n))
        { /*if true we have a complete symbol set; So find matching character(s)*/
            /*But 1st, need to check if the letterbrk was based on exit-code 2,
            if so & in debug mode, need to do some logging cleanup/catchup work  */
            if (ExitPath[n] == 2)
            {
                while (curN <= n)
                {
                    if (Dbug)
                    {
                        printf("\nDwn: %3d\t", TmpDwnIntrvls[curN]);
                        if (curN < KeyUpPtr)
                            printf("Up: %3d\t", TmpUpIntrvls[curN]);
                        else
                            printf("Up: ???\t");
                    }
                    curN++;
                }
            }
            /*Now, if the symbol set = 31 (4 dits in a row), we need to figure out where the biggest key up interval is
            and subdivide this into something that can be decoded*/
            if ((SymbSet == 31))
                Dcode4Dahs(n);
            else
                int IndxPtr = AdvSrch4Match(n, SymbSet, true); // try to convert the current symbol set to text &
                                                               // and save/append the results to 'Msgbuf[]'
                                                               // start a new symbolset
            LstLtrBrkCnt = 0;
        }
        else
            LstLtrBrkCnt++;
        if (Dbug)
        {
            printf("\tLBrkCd: %d", ExitPath[n]);
            if (BrkFlg != NULL)
            {
                printf("+\t");
                printf("SymbSet:%d\t", SymbSet);
                PrintThisChr();
                SymbSet = 1; // reset the symbolset for the next character
            }
            else
                printf("\n");
        }
        else if (BrkFlg != NULL)
        {
            PrintThisChr();
            SymbSet = 1; // reset the symbolset for the next character
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
    if(prntOvrRide){
        this->Dbug = oldDbugState;
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
/*for this group of sorted keydown intervals find the value where any shorter interval is a "Dit"
 & all longer times are "Dahs"
 Note: 'n' points to the last keyDwnbucket in this group
 */
void AdvParser::SetSpltPt(Buckt_t arr[], int n)
{
    int i;
    uint16_t NuSpltVal = 0;
    AllDah = true; // made this a class property so it could be used later in the "Tst4LtrBrk()" method
    bool AllDit = true;
    int lastDitPtr = 0;
    int lastDahPtr = n;
    /*start with shortest & longest intervals ()1st & last entries) and work toward the middle*/
    // for (i = 0; i <= (n + 1) / 2; i++)
    // {
    //     if ((2 * arr[i].Intrvl) < arr[lastDahPtr-i].Intrvl)
    //     {
    //         lastDitPtr = i;
    //     }
    //     if ((1.5 * arr[lastDitPtr].Intrvl) < arr[n - i].Intrvl)
    //     {
    //         lastDahPtr = n-i;
    //     }
    //     NuSpltVal = arr[lastDitPtr].Intrvl + (arr[lastDahPtr].Intrvl - arr[lastDitPtr].Intrvl) / 2;
    //     if ((arr[i - 1].Intrvl < DitDahSplitVal) )
    //         AllDah = false;
    //     if ((arr[n -i].Intrvl > DitDahSplitVal) )
    //         AllDit = false;    
    // }
    int  DitCnt, DahCnt;
    DitCnt = DahCnt =0;
    for (i = 0; i <= (n + 1) / 2; i++)
    {
        if(i >= n-i) break;//make absolutey certian that we the dits dont cross over the dahs
        if ((arr[i].Cnt) >= DitCnt)
        {
            DitCnt = arr[i].Cnt;
            lastDitPtr = i;
        }
        if (arr[n - i].Cnt >= DahCnt)
        {
            DahCnt = arr[n - i].Cnt;
            lastDahPtr = n-i;
        }
        NuSpltVal = arr[lastDitPtr].Intrvl + (arr[lastDahPtr].Intrvl - arr[lastDitPtr].Intrvl) / 2;
        if ((arr[i - 1].Intrvl < DitDahSplitVal) )
            AllDah = false;
        if ((arr[n -i].Intrvl > DitDahSplitVal) )
            AllDit = false;    
    }
    if((lastDitPtr== lastDahPtr) && (lastDahPtr < n)) lastDahPtr++;//another check/fix to ensure the dit & dah buckets are not the same bucket
    DitIntrvlVal = arr[lastDitPtr].Intrvl;//use this later in bug letter break test as a sanity check
    //printf("\nlastDitPtr =%d; lastDahPtr =%d; ditVal:%d; dahVal:%d\n", lastDitPtr, lastDahPtr, arr[lastDitPtr].Intrvl, arr[lastDahPtr].Intrvl);
    
    /*if this group of key down intervals is either All Dits or All dahs,
    then its pointless to reevaluate the "DitDahSplitVal"
    So abort this routine*/
    if ((AllDit || AllDah) && (DitDahSplitVal != 0))
    {
        SameSymblWrdCnt++;
        if (SameSymblWrdCnt > 2)
        { // had 3 all dits/dahs in a row; somethings not right, Do a hard reset
            SameSymblWrdCnt = 0;
            DitDahSplitVal = 0;
        }
        else
            return;
    }
    else
        SameSymblWrdCnt = 0;
    
    if (NuSpltVal != 0)
    {
        if (Dbug) printf("\nNuSpltVal: %d\n", NuSpltVal);
        if (DitDahSplitVal == 0)
            DitDahSplitVal = NuSpltVal;
        else
        /*New Method for weighting the NuSpltVal against previous DitDahSplitVal*/
        /*Bottom line, if this word symbol set has more than 30 elements forget the old split value & use the one just found*/
        /*if less than 30, weight/avg the new result proportionally to the last 30*/
            if(TmpUpIntrvlsPtr>=30) DitDahSplitVal = NuSpltVal;
            else{
                int OldWght = 30 - TmpUpIntrvlsPtr;
                DitDahSplitVal = ((OldWght * DitDahSplitVal) + (TmpUpIntrvlsPtr * NuSpltVal)) / 30;
            }
            // DitDahSplitVal = (3 * DitDahSplitVal + NuSpltVal) / 4;
    }
};
/*for this group of keydown intervals(TmpDwnIntrvls[]) & the selected keyUp interval (TmpDwnIntrvls[n]),
use the test set out within to decide if this keyup time represents a letter break
& return "true" if it does*/
bool AdvParser::Tst4LtrBrk(int& n)
{
    BrkFlg = NULL;
    /*Paddle Rule Set*/
    switch (BugKey)
    {
    case 0:
        return this->PadlRules(n);
        break;
    case 1:
        return this->BugRules(n);
        break;
    case 2:
        return this->CootyRules(n);
        break;
    case 3:
        return this->Cooty2Rules(n);
        break;
    default:
        return false;
        break;
    }
};
////////////////////////////////////////////////////////
bool AdvParser::Cooty2Rules(int& n)
{
    ExitPath[n] = 150;
    if (TmpUpIntrvls[n] >= 2.5 * AvgSmblDedSpc)
    {
        BrkFlg = '+';
        ExitPath[n] = 151;
        return true;
    }
    return false;
};
////////////////////////////////////////////////////////
bool AdvParser::CootyRules(int& n)
{
    ExitPath[n] = 200;
    if (TmpUpIntrvls[n] >= 2.5 * AvgSmblDedSpc)
    {
        BrkFlg = '+';
        ExitPath[n] = 201;
        return true;
    }
    return false;
};
////////////////////////////////////////////////////////
bool AdvParser::PadlRules(int& n)
{
    /*Paddle or Keyboard rules*/
    /*Middle keyup test to see this keyup is twice the lenght of the one just before it,
    If it is then call this one a letter break*/
    if (n > 0 && (TmpUpIntrvls[n] > 2.0 * TmpUpIntrvls[n - 1]))
    {
        ExitPath[n] = 100;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one following it,
    If it is then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n] > 2.0 * TmpUpIntrvls[n + 1]))
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
        }
        else
            ExitPath[n] = 103;
        if (MaxCntKyUpBcktPtr < KeyUpBucktPtr) // safety check, before trying the real test
        {
            // if (TmpUpIntrvls[n] >= KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl)
            if (TmpUpIntrvls[n] >= DitDahSplitVal)
            {
                BrkFlg = '+';
                ExitPath[n] = 104;
                return true;
            }
        }
        else
            ExitPath[n] = 105;
    }
    else
    {
        if (TmpUpIntrvls[n] >= DitDahSplitVal)
        {
            BrkFlg = '+';
            ExitPath[n] = 106;
            return true;
        }
        else
            ExitPath[n] = 107;
    }
    return false;
};
////////////////////////////////////////////////////////

bool AdvParser::BugRules(int& n)
{
    /*Bug or Manual Key rules*/
    bool ltrbrkFlg = false;
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
    /*test for N dahs in a row, & set ltrbrk based on longest dah in the group*/
    int maxdahIndx = 0;
    int mindahIndx = 0;
    uint16_t maxdah = 0;
    uint16_t mindah = KeyDwnBuckts[KeyDwnBucktPtr].Intrvl;
    for (int i = n; i <= TmpUpIntrvlsPtr; i++){
        if(TmpDwnIntrvls[i] +8 < DitDahSplitVal) break; //quit when a dit is detected
        else //we have a dah;  
        {
            if(TmpDwnIntrvls[i] > maxdah){
                maxdah = TmpDwnIntrvls[i];
                maxdahIndx = i;
            }
            if(TmpDwnIntrvls[i] < mindah){
                mindah = TmpDwnIntrvls[i];
                mindahIndx = i;
            }

        }
    }
    /*Test that the long dah is significantly longer than its sisters,
      & it is also terminated/followed by a reasonable keyup interval*/
    if(maxdahIndx > 0 && (maxdah > 1.3*mindah) && (TmpUpIntrvls[maxdahIndx] > (0.8* DitDahSplitVal))){
        /*we have a run of dahs, so build/grow the SymbSet to mach the found run of dahs*/
        while(n< maxdahIndx){
            n++;
            SymbSet = SymbSet << 1;                    // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
        }
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;

    }
    /*Middle keyup test to see this keyup is twice the length of the one just before it,
    And we are using a valid keyup reference interval & not just some random bit of noise.
    If it is then call this one a letter break*/
    if (n > 0 && (TmpUpIntrvls[n] > 2.4 * TmpUpIntrvls[n - 1]) && (TmpUpIntrvls[n - 1] >= DitIntrvlVal))
    {
        ExitPath[n] = 3;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one following it,
    AND greater than the one that proceeded it, provided the proceeding keyup was not a letterbrk.
    If all true, then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (n > 0))
    {
        if ((TmpUpIntrvls[n] > 2.4 * TmpUpIntrvls[n + 1]) && (TmpUpIntrvls[n] > TmpUpIntrvls[n - 1]) &&
        LstLtrBrkCnt > 0)
        {
            ExitPath[n] = 4;
            BrkFlg = '+';
            return true;
        }
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
                    ExitPath[n] = 5;
                    BrkFlg = '+';
                    return true;
                }
                else if ((TmpUpIntrvls[n] >= 0.8 * DitDahSplitVal) &&
                         (TmpUpIntrvls[n - 1] >= 0.8 * DitDahSplitVal) &&
                         (TmpDwnIntrvls[n] >= TmpDwnIntrvls[n - 1]) &&
                         (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n - 1]))
                { // we're the middle of a dah serries; but this one looks streched compared to its predecessor
                    ExitPath[n] = 6;
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
                ExitPath[n] = 7;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n + 1] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n + 1]))
            { // this Dah has lot longer Keyup than the next dah; so this looks like a letter break
                ExitPath[n] = 8;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= TmpUpIntrvls[n + 1]) &&
                     (TmpDwnIntrvls[n] >= 1.3 * TmpDwnIntrvls[n + 1]))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 9;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= 1.4 * DitDahSplitVal))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 10;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 11;
                return false;
            }
        }
        /*test for 2 adjacent 'dits'*/
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*we have two adjcent dits, set letter break if the key up interval is 2.0x longer than the shortest of the 2 Dits*/
            if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n]+8) || (TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n + 1]+8))
            {
                ExitPath[n] = 12;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 13;
                return false;
            }
            /*test for dah to dit transistion*/
        }
        else if ((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*We have Dah to dit transistion set letter break only if key up interval is > 1.6x the dit interval
            And the keyup time is more than 0.6 the dah interval*/ //20240120 added this 2nd qualifier
            if ((TmpUpIntrvls[n] > 1.6 * TmpDwnIntrvls[n + 1])&& (TmpUpIntrvls[n] > 0.6 * TmpDwnIntrvls[n]))
            {
                ExitPath[n] = 14;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 15;
                return false;
            }
            /*test for dit to dah transistion*/
        }
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
        {
            /*We have Dit to Dah transistion set letter break only if key up interval is > 2x the dit interval*/
            if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n]))
            {
                ExitPath[n] = 16;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 17;
                return false;
            }
        }
        else
        {
            printf("Error 1: NO letter brk test\n");
            ExitPath[n] = 18;
            return ltrbrkFlg;
        }
        /*then this is the last keyup so letter brk has to be true*/
    }
    else
    {
        ExitPath[n] = 19;
        BrkFlg = '+';
        return true;
    }
    /*Should never Get Here*/
    ExitPath[n] = 20;
    printf("Error 2: NO letter brk test\n");
    return ltrbrkFlg;
};
////////////////////////////////////////////////////////
int AdvParser::AdvSrch4Match(int n, unsigned int decodeval, bool DpScan)
{
    /*1st test, & confirm, there's sufficient space to add search results to the 'Msgbuf'*/
    if (LstLtrPrntd >= (MsgbufSize - 5))
        return 0;

    /*make a copy of the current message buffer */
    char TmpBufA[MsgbufSize - 5];
    for (int i = 0; i < sizeof(TmpBufA); i++)
    {
        TmpBufA[i] = this->Msgbuf[i];
        if (this->Msgbuf[i] == 0)
            break;
    }
    int pos1 = linearSearchBreak(decodeval, CodeVal1, ARSIZE); // note: decodeval '255' returns SPACE character

    if (pos1 < 0 && DpScan)
    { // did not find a match in the standard Morse table. So go check the extended dictionary
        pos1 = linearSearchBreak(decodeval, CodeVal2, ARSIZE2);
        if (pos1 < 0)
        { /*Still no match. Go back and sub divide this group timing inervals into two smaller set & look again*/
            // sprintf(this->Msgbuf, "%s%s", TmpBufA, "*");
            /*Build 2 new symbsets & try to decode them*/
            /*find the longest keyup time in this interval group*/
            int NuLtrBrk = 0;
            uint16_t LongestKeyUptime = 0;
            unsigned int Symbl1, Symbl2;
            int Start = n - this->LstLtrBrkCnt;
            //printf("\nLstLtrBrkCnt: %d; offset: %d; ",LstLtrBrkCnt, Start);
            for (int i = Start; i < n; i++)//stop 1 short of the original letter break
            {
                if (TmpUpIntrvls[i] > LongestKeyUptime)
                {
                    NuLtrBrk = i;
                    LongestKeyUptime = TmpUpIntrvls[i];
                }
            }
            /** Build 1st symbol set based on start of the the original group & the found longest interval*/
            Symbl1 = 1;
            for (int i = Start; i <= NuLtrBrk; i++)
            {
                Symbl1 = Symbl1 << 1;                      // append a new bit to the symbolset & default it to a 'Dit'
                if (TmpDwnIntrvls[i] + 8 > DitDahSplitVal) // if within *ms of the split value, its a 'dah'
                    Symbl1 += 1;
            }
            //printf("\t1st Start:%d; NuLtrBrk:%d; Symbl1: %d; ", Start, NuLtrBrk, Symbl1);
            Start = NuLtrBrk + 1;
            Symbl2 = 1;
            for (int i = Start; i <= n; i++)
            {
                Symbl2 = Symbl2 << 1;                      // append a new bit to the symbolset & default it to a 'Dit'
                if (TmpDwnIntrvls[i] + 8 > DitDahSplitVal) // if within *ms of the split value, its a 'dah'
                    Symbl2 += 1;
            }
            //printf("\t2nd Start:%d; n:%d; Symbl2: %d\n", Start, n, Symbl2);
            /*Now find character matches for these two new symbol sets,
             and append their results to the message buffer*/
            pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
            if(pos1 >0){
            sprintf(this->Msgbuf, "%s%s", TmpBufA, DicTbl1[pos1]);
            /*make another copy of the current message buffer */
            for (int i = 0; i < sizeof(TmpBufA); i++)
            {
                TmpBufA[i] = this->Msgbuf[i];
                if (this->Msgbuf[i] == 0)
                    break;
            }
            }
            pos1 = linearSearchBreak(Symbl2, CodeVal1, ARSIZE);
            if(pos1 >0) sprintf(this->Msgbuf, "%s%s", TmpBufA, DicTbl1[pos1]);
        }
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
/*This function finds the Msgbuf current lenght regardless of Dbug's state */
void AdvParser::PrintThisChr(void)
{
    int curEnd = LstLtrPrntd;
    while (Msgbuf[curEnd] != 0)
    {
        if (Dbug)
            printf("%c", this->Msgbuf[curEnd]);
        curEnd++;
    }
    if (Dbug)
        printf("\n");
    LstLtrPrntd = curEnd;
};
///////////////////////////////////////////////////////////////////////
/*Return the current string length of the AdvParser MsgBuf*/
int AdvParser::GetMsgLen(void)
{
    return this->LstLtrPrntd;
};
//////////////////////////////////////////////////////////////////////////////
/* Paddle/keybrd test by finding constant dah intervals.
 * Bug test by noting the keyup interval is consistantly shorter between the dits vs dahs
 * returns 0, 1, or 5 for paddle, 2,3,& 4 for bug, &  6 for unknown;.
 */
int AdvParser::DitDahBugTst(void)
{
    int ditcnt;
    int dahDwncnt;
    int dahcnt = ditcnt = dahDwncnt = 0;
    uint16_t dahInterval;
    uint16_t dahDwnInterval;
    uint16_t ditInterval = dahInterval = dahDwnInterval = 0;
    int stop = TmpUpIntrvlsPtr - 1;
    for (int n = 0; n < stop; n++)
    {
        /*Sum the numbers for any non letterbreak terminated dah*/
        if (n > 0 && (TmpDwnIntrvls[n] > DitDahSplitVal) &&
            (TmpUpIntrvls[n] < 1.25 * DitDahSplitVal))
        {
            dahDwnInterval += TmpDwnIntrvls[n];
            dahDwncnt++;
        }
        /*test for 2 adjcent dahs. But only include if there's not an apparent letter break between them */
        if ((TmpDwnIntrvls[n] > DitDahSplitVal) &&
            (TmpDwnIntrvls[n + 1] > DitDahSplitVal) &&
            (TmpUpIntrvls[n] < 1.25 * DitDahSplitVal))
        {
            dahInterval += TmpUpIntrvls[n];
            dahcnt++;
        }
        /*test for 2 adjcent dits. But only include if there's not a letter break between them */
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) &&
                 (TmpDwnIntrvls[n + 1] < DitDahSplitVal) &&
                 (TmpUpIntrvls[n] < DitDahSplitVal))
        {
            ditInterval += TmpUpIntrvls[n];
            ditcnt++;
        }
    }
    /*Test/check for constant dah intervals*/
    if (dahDwncnt > 1)
    {
        /* average/normalize results */
        dahDwnInterval /= dahDwncnt;
        /*test1*/
        /*if all dah intervals are the same, its a paddle/keyboard */
        bool same = true;
        dahDwncnt = 0;
        uint16_t Tolrenc = (uint16_t)(0.08*(float)dahDwnInterval);
        //printf("\nTolrenc %d\n", Tolrenc);
        for (int n = 1; n < stop; n++) // skip the 1st key down event because testing showed the timing of the 1st event is often shorter than the rest in the group
        {
            if ((TmpDwnIntrvls[n] > DitDahSplitVal) &&
                (TmpUpIntrvls[n] < 1.25 * DitDahSplitVal))
            {
                dahDwncnt++;
                if ((TmpDwnIntrvls[n] > (dahDwnInterval + Tolrenc)) || (TmpDwnIntrvls[n] < (dahDwnInterval - Tolrenc)))
                    same = false;
            }
        }
        /*test2*/
        if (same && (dahDwncnt > 1))
        {
            if (ditcnt > 0 && dahcnt > 0)
            {
                /* average/normalize results */
                dahInterval /= dahcnt;
                ditInterval /= ditcnt;
                // printf("\nditcnt:%d; dahcnt:%d; ditInterval: %d; dahInterval: %d\n", ditcnt, dahcnt, ditInterval, dahInterval);
                if (dahInterval > ditInterval + 8)
                    return 2; // bug
                else
                    return 1; // paddle/krybrd
            }
            else
                return 0; // paddle/krybrd
        } else if (dahDwncnt > 1) return 3; // bug
    }
    // printf("\nditcnt:%d; dahcnt:%d; interval cnt: %d\n", ditcnt, dahcnt, stop);
    if (ditcnt > 0 && dahcnt > 0)
    {
        /* average/normalize results */
        dahInterval /= dahcnt;
        ditInterval /= ditcnt;
        // printf("\nditcnt:%d; dahcnt:%d; ditInterval: %d; dahInterval: %d\n", ditcnt, dahcnt, ditInterval, dahInterval);
        if (dahInterval > ditInterval + 8)
            return 4; // bug
        else
            return 5; // paddle/krybrd
    }
    else
        return 6; // not enough info to decide
};
/////////////////////////////////////////////
void AdvParser::Dcode4Dahs(int n)
{
    int NuLtrBrk = 0;
    uint16_t LongestKeyUptime = 0;
    unsigned int Symbl1, Symbl2;
    /*find the longest keyup time in the preceeding 3 intervals*/
    for(int i =n-3; i < n; i++){
        if(TmpUpIntrvls[i]> LongestKeyUptime){
            NuLtrBrk = i -(n-3);
            LongestKeyUptime = TmpUpIntrvls[i];
        }
    }
    switch (NuLtrBrk)
    {
    case 0:
        Symbl1 = 0b11;
        Symbl2 = 0b1111;
        break;
    case 1:
        Symbl1 = 0b111;
        Symbl2 = 0b111;
        break;
    case 2:
        Symbl1 = 0b1111;
        Symbl2 = 0b11;
        break;        
    
    default:
        Symbl1 = 76; //"?"
        Symbl2 = 76; //"?"
        break;
    }
    this->AdvSrch4Match(0, Symbl1, false);
    this->AdvSrch4Match(0, Symbl2, false);
 };