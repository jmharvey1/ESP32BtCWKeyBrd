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
 * 20240127 extended letter break test '2' to also look at a string/series of 'dits' and generate letter break on long "keyup" interval
 * 20240129 improved method for calculating DitIntrvlVal + other tweaks to bug parsing rules
 * 20240201 Added Straight Key Rule Set & Detection; Revised bug1 RS dit & dah runs parsing
 * 20240202 Added Bg1SplitPt & refined Bug1 "Dah run" code
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
void AdvParser::EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr, int curWPM)
{
    //this->Dbug = false;
    this->wpm = curWPM;
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

    if (KeyDwnBucktPtr >= 1 && KeyUpBucktPtr >= 1)
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
    // if (Dbug)
    // {
    //     printf("\nSplitPoint:%3d\tDitIntrvlVal:%d\t", DitDahSplitVal, DitIntrvlVal);
    // }
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
    UnitIntvrlx2r5 = (uint16_t)(2.4 * ((AvgSmblDedSpc + DitIntrvlVal) / 2));
    Bg1SplitPt = (uint16_t)((float)UnitIntvrlx2r5 * 0.6);
    /*OK; now its time to build a text string*/
    /*But 1st, decide which parsing rule set to use*/
    // if (Dbug)
    // {
    //     printf("AvgDedSpc:%0.1f\tUnitIntvrlx2r5:%d\n", AvgSmblDedSpc, UnitIntvrlx2r5);
    //     printf("\nKeyDwnBuckt Cnt: %d ", KeyDwnBucktPtr + 1);
    // }
    /*Figure out which Key type rule set to use. Paddle(BugKey = 0)/Bug(BugKey = 1)/Cootie(BugKey = 2)*/
    uint8_t bgPdlCd = 0;
    BugKey = 1; // start by assuming its a standard bug type key
    /*select 'cootie' key based on extreme short keyup timing relative to keydown time*/
    // printf("\nAvgSmblDedSpc:%d; KeyDwnBuckts[0].Intrvl:%d; Intrvl / 3: %0.1f\n", (int)AvgSmblDedSpc, KeyDwnBuckts[0].Intrvl, KeyDwnBuckts[0].Intrvl / 2.7);
    if ((AvgSmblDedSpc < KeyDwnBuckts[0].Intrvl / 2.7) && (KeyUpBucktPtr < 5))
    { // Cootie Type 1
        BugKey = 2;
        bgPdlCd = 50;
    }
    else if (3 * KeyDwnBuckts[0].Intrvl < AvgSmblDedSpc)
    { // Cootie Type 2
        BugKey = 3;
        bgPdlCd = 60;
    }
    else
    {
        /*the following tests chooses between paddle or bug*/
        switch (this->DitDahBugTst()) // returns 0, 1, or 5 for paddle, 2,3,& 4 for bug, &  6 for unknown;
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
        case 6:
            /*not enough info leave as is */
            bgPdlCd = 99;
            // BugKey = 0;
            break;
        case 7:
            /*Straight Key */
            bgPdlCd = 50;
            BugKey = 5;
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
                            BugKey = 0; /* its a paddle */
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
                            BugKey = 0; /* its a paddle */
                            bgPdlCd = 8;
                        }
                        else if (TmpUpIntrvlsPtr >= 8)
                        {
                            BugKey = 0; /* its a paddle */
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
    if (Dbug)
    {
        printf("\nSplitPoint:%3d\tBg1SplitPt:%d\tDitIntrvlVal:%d\t", DitDahSplitVal, Bg1SplitPt, DitIntrvlVal);
        printf("AvgDedSpc:%0.1f\tUnitIntvrlx2r5:%d\n", AvgSmblDedSpc, UnitIntvrlx2r5);
        printf("\nKeyDwnBuckt Cnt: %d ", KeyDwnBucktPtr + 1);
    }
    /*Now test for bug key type (1), and if true, decide which bug style (rule set to use)*/
    if (BugKey == 1)
    {
        if (DitIntrvlVal > 1.5 * AvgSmblDedSpc)
        {
            bgPdlCd += 100;
            BugKey = 4;
        }
    }
    KeyType = BugKey; // let the outside world know what mode/rule set being used
    switch (BugKey)
    {
    case 0:          // paddle/keyboard
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 1:          // Bug/straight key
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 2:          // cootie type A
        ModeCnt = 3; // DcodeCW.cpp use "cootie" settings/timing ; no glitch detection
        break;
    case 3:          // cootie typ B
        ModeCnt = 3; // DcodeCW.cpp use "cootie" settings/timing ; no glitch detection
        break;
    case 4:          // paddle/keyboard
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 5:          // Straight Key
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;    
    default:
        break;
    }
    SetModFlgs(ModeCnt); // DcodeCW.cpp routine; Update DcodeCW.cpp timing settings & ultimately update display status line
    CurMdStng(ModeCnt);  // convert to ModeVal for Goertzel.cpp to use (mainly glitch control)

    if (Dbug)
    {
        printf(" CD:%d\n", bgPdlCd);
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
            printf(" Bug1/Straight\n");
            break;
        case 2:
            printf(" Cootie\n");
            break;
        case 3:
            printf(" ShrtDits\n");
            break;
        case 4:
            printf(" Bug2\n");
            break;
        case 5:
            printf(" Str8Key\n");
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
        int curN = n + 1;
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
        else if (ExitPath[n] == 4 && BrkFlg == '%')
        { // found a run but ended W/o a letter break. So for Debug output,need to advance/resync pointers
            while (curN <= n)
            {
                if (Dbug)
                {
                    printf("\nDwn: %3d\tUp: %3d\t", TmpDwnIntrvls[curN], TmpUpIntrvls[curN]);
                }
                LstLtrBrkCnt++;
                curN++;
            }
        }
        else
            LstLtrBrkCnt++;

        if (Dbug)
        {
            if (BrkFlg == NULL)
                BrkFlg = ' ';
            printf("\tLBrkCd: %d%c", ExitPath[n], BrkFlg);
            if (BrkFlg == '+' || BrkFlg == '&')
            {
                // printf("\t");
                printf("\tSymbSet:%d\t", SymbSet);
                PrintThisChr();
                SymbSet = 1; // reset the symbolset for the next character
            }
            else
                printf("\n");
        }
        else if (BrkFlg == '+' || BrkFlg == '&')
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
    if (prntOvrRide)
    {
        this->Dbug = oldDbugState;
    }
};
/*Sort number array in ascending order*/
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
    int MaxDitPtr = 0;
    int  MaxDitCnt, DahCnt;
    uint16_t MaxIntrval = (3*(1500/wpm));//set the max interval to that of a dah @ 0.8*WPM
    uint16_t MaxDelta = 0;
    MaxDitCnt = DahCnt =0;
    for (i = 0; i <= (n - 1); i++)
    {
        if((arr[i+1].Intrvl > MaxIntrval)) break; // exclude/stop comparison when/if the interval exceeds that of a dah interval @ the current WPM 
        /*Test if the change interval between these keydwn groups is bigger than anything we've seen before (in this symbol set)*/
        if(arr[i+1].Intrvl - arr[i].Intrvl > MaxDelta ){
            if(n>1 && i+1 == n &&  arr[i+1].Cnt ==1 && !AllDit) break;// when there is more than 2 buckets skip the last one. Because for bugs this last one is likely an exaggerated daH
            MaxDelta = arr[i+1].Intrvl - arr[i].Intrvl;
            NuSpltVal = arr[i].Intrvl +(MaxDelta)/2;
            //if(NuSpltVal >100) printf("\nERROR NuSpltVal >100; lastDitPtr =%d;\n", i);
            /*now Update/recalculate the running average dit interval value (based on the last six dit intervals)*/
            if(arr[i].Intrvl<=DitDahSplitVal){
                if(arr[i].Cnt <=6){
                    DitIntrvlVal =(uint16_t) (((6-arr[i].Cnt)*(float)DitIntrvlVal) + (arr[i].Cnt * 1.1 * (float)arr[i].Intrvl))/6.0;//20240129 build a running average of the last 6 dits
                }
                else DitIntrvlVal = arr[i].Intrvl;
            }
        }
        /*Collect data for an alternative derivation of NuSpltVal, by identifying the bucket with the most dits*/
        if(arr[i].Cnt > MaxDitCnt){
            MaxDitPtr = i;
            MaxDitCnt = arr[i].Cnt;
        }

        if ((arr[i - 1].Intrvl < DitDahSplitVal) )
            AllDah = false;
        if ((arr[n -i].Intrvl > DitDahSplitVal) )
            AllDit = false;    
        if(lastDitPtr+1>= n-i) break;//make absolutey certian that we the dits dont cross over the dahs    
    }
    /*If a significant cluster of 'dits' were found (8), then test if their weighted interval value is lesss than the nusplitval calculated value above. 
    If true, then use their weighted value */
    if (MaxDitCnt >= 5){ //MaxDitCnt >= 8
        uint16_t WghtdDit = (1.5 * arr[MaxDitPtr].Intrvl);
        if(WghtdDit < NuSpltVal) NuSpltVal = WghtdDit;
    }
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
        return this->Bug1Rules(n);
        break;
    case 2:
        return this->CootyRules(n);
        break;
    case 3:
        return this->Cooty2Rules(n);
        break;
    case 4:
        return this->Bug2Rules(n);
        break;
    case 5:
        return this->SKRules(n);
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
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n] > (2.0 * TmpUpIntrvls[n + 1])+8))
    {
        ExitPath[n] = 101;
        BrkFlg = '+';
        return true;
    }
    if (NewSpltVal)
    {
        //if ((TmpUpIntrvls[n] >= 1.5 * 1.2 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl) &&
        //    (TmpUpIntrvls[n] > KeyDwnBuckts[0].Intrvl))
        // if ((TmpUpIntrvls[n] >= 1.5 * DitIntrvlVal) &&
        //     (TmpUpIntrvls[n] > KeyDwnBuckts[0].Intrvl))
        if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
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

bool AdvParser::Bug1Rules(int& n)
{
    /*Bug or Manual Key rules*/
    
    bool ltrbrkFlg = false;
    if (TmpUpIntrvls[n] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl)
    {
        ExitPath[n] = 0;
        BrkFlg = '+';
        return true;
    }
    // if (!AllDah && (TmpUpIntrvls[n] <= 1.20 * KeyUpBuckts[0].Intrvl)) // could be working with the word "TO" want to skip this check
    // {
    //     ExitPath[n] = 1;
    //     return false;
    // }
    /*test for N dahs in a row, & set ltrbrk based on longest dah in the group*/
    int maxdahIndx = 0;
    int mindahIndx = 0;
    int RunCnt = 0; // used to validate that there were at least two adjacent dahs or dits
    uint16_t maxdah = 0;
    uint16_t mindah = KeyDwnBuckts[KeyDwnBucktPtr].Intrvl;
    char ExtSmbl = ' ';
    for (int i = n; i <= TmpUpIntrvlsPtr; i++)
    {
        //if (TmpDwnIntrvls[i] + 8 < DitDahSplitVal)
        if (TmpDwnIntrvls[i] < Bg1SplitPt)
        {
            ExtSmbl = '@';
            break; // quit when a dit is detected
        }
        else // we have a dah;
        {
            RunCnt++;
            if (TmpDwnIntrvls[i] > maxdah)
            {
                maxdah = TmpDwnIntrvls[i];
                maxdahIndx = i;
            }
            if (TmpDwnIntrvls[i] < mindah)
            {
                mindah = TmpDwnIntrvls[i];
                mindahIndx = i;
            }
            if (i > n)
            {
                if ((TmpDwnIntrvls[i] > 1.5 * TmpDwnIntrvls[i - 1]) && (TmpUpIntrvls[i] > 1.5 * DitIntrvlVal))
                {
                    ExtSmbl = '#';
                    break; // quit now this Dah looks significantly streched compared to its predecessor
                }
            }
            if ((TmpUpIntrvls[i] > 1.4 * TmpDwnIntrvls[i]) || 
               ((TmpDwnIntrvls[i]>= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl) && (TmpUpIntrvls[i]> Bg1SplitPt)) || 
               (TmpUpIntrvls[i]>= KeyUpBuckts[KeyUpBucktPtr].Intrvl)) //(TmpUpIntrvls[i] > 1.1 * TmpDwnIntrvls[i])
            {
                ExtSmbl = '$';
                break; // quit, Looks like a clear intention to signal a letterbreak
            }
            
        }
    }
    /*Test that the long dah is significantly longer than its sisters,
      & it is also terminated/followed by a reasonable keyup interval*/
    // if (this->Dbug)
    // {
    //     if (RunCnt > 0)
    //         printf("RunCnt %d%c\t", RunCnt, ExtSmbl);
    //     else
    //         printf("\t\t");
    // }
    if (RunCnt > 1 && maxdahIndx > 0 && (maxdah > 1.3 * mindah) && (TmpUpIntrvls[maxdahIndx] > (DitIntrvlVal + 8)))
    {
        // if(maxdahIndx > 0 && (maxdah > 1.3*mindah) && (TmpUpIntrvls[maxdahIndx] > (0.8* DitDahSplitVal))){
        /*we have a run of dahs, so build/grow the SymbSet to mach the found run of dahs*/
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
        }
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
    } else if (RunCnt > 1 && (ExtSmbl == '@')){//there was a run of dahs but no letterbreak terminator found; So just append what was found to the current symbolset & continue looking
        while (RunCnt > 1)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
            RunCnt--;
        }
        ExitPath[n] = 4;//NOTE: there are two "ExitPath[n] = 4;"
        BrkFlg = '%';
        return false;
    }
    //else if (RunCnt > 1 && maxdah > 0 && (mindahIndx == maxdahIndx) && (TmpUpIntrvls[maxdahIndx] > TmpDwnIntrvls[maxdahIndx]))
    else if (RunCnt > 0 && ExtSmbl == '$' && n == maxdahIndx)
    { // the 1st dah seems to be a letter break
        // while (n < maxdahIndx)
        // {
        //     n++;
        //     SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        //     SymbSet += 1;
        // }
        ExitPath[n] = 23;
        BrkFlg = '+';
        return true;
    }
   
    /*Now do a similar test/logic for n dits in a row (to help better manage 'es' and the like parsing)*/
    /* set ltrbrk based on longest keyup interval in the group*/
    maxdahIndx = 0; // reuse this variable from Dah series test
    mindahIndx = 0; // reuse this variable from Dah series test
    maxdah = 0;     // reuse this variable from Dah series test
    RunCnt = 0;     // used to validate that there were at least two adjacent dahs or dits
    mindah = KeyUpBuckts[KeyDwnBucktPtr].Intrvl;
    ExtSmbl = ' ';
    for (int i = n; i <= TmpUpIntrvlsPtr; i++)
    {
        //if ((TmpDwnIntrvls[i] + 8 > DitDahSplitVal))
        if (TmpDwnIntrvls[i] > Bg1SplitPt)
        {
            ExtSmbl = '@';
            break; // quit, a dah was detected
        }
        else       // we have a dit;
        {
            RunCnt++;
            maxdahIndx = i;
            
            if ((TmpUpIntrvls[i] > UnitIntvrlx2r5)){
                    //maxdahIndx = i;
                    ExtSmbl = '#';
                    break; // quit an there's an obvious letter break
            }
        }
    }
    /*Test that there was a run of dits AND it was bounded/terminated by a letterbreak*/
    if (RunCnt > 0 && maxdahIndx > 0 && (ExtSmbl == '#'))
    {

        /*we have a run of dits, so build/grow the SymbSet to mach the found run of dits*/
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            
        }
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
    } else if (RunCnt > 1 && (ExtSmbl == '@')){//there was a run of dits but no letterbreak terminator found; So just append what was found to the current symbolset & continue looking
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            
        }
        ExitPath[n] = 4;//NOTE: there are two "ExitPath[n] = 4;"
        BrkFlg = '%';
        return false;
    }

    /*Middle keyup test to see this keyup is twice the length of the one just before it,
    And we are using a valid keyup reference interval & not just some random bit of noise.
    If it is then call this one a letter break*/
    if (n > 0 && (TmpUpIntrvls[n] > 2.4 * TmpUpIntrvls[n - 1]) && (TmpUpIntrvls[n - 1] >= DitIntrvlVal))
    {
        ExitPath[n] = 5;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one following it,
    AND greater than the one that proceeded it, provided the proceeding keyup was not a letterbrk.
    If all true, then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (n > 0))
    {
        if ((TmpUpIntrvls[n] > 2.4 * TmpUpIntrvls[n + 1]) && (TmpUpIntrvls[n] > 1.6 * TmpUpIntrvls[n - 1]) &&
            LstLtrBrkCnt > 0)
        {
            ExitPath[n] = 6;
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
            if ((TmpDwnIntrvls[n - 1] > Bg1SplitPt) &&
                (TmpDwnIntrvls[n] > Bg1SplitPt) &&
                (TmpDwnIntrvls[n + 1] > Bg1SplitPt))
            { // we are are surrounded by dahs
                if ((TmpUpIntrvls[n - 1] >= Bg1SplitPt) && (TmpUpIntrvls[n] > 1.2 * TmpUpIntrvls[n - 1]))
                {
                    ExitPath[n] = 7;
                    BrkFlg = '+';
                    return true;
                }
                else if ((TmpUpIntrvls[n] >= 0.8 * Bg1SplitPt) &&
                         (TmpUpIntrvls[n - 1] >= 0.8 * Bg1SplitPt) &&
                         (TmpDwnIntrvls[n] >= TmpDwnIntrvls[n - 1]) &&
                         (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n - 1]))
                { // we're the middle of a dah serries; but this one looks streched compared to its predecessor
                    ExitPath[n] = 8;
                    BrkFlg = '+';
                    return true;
                }
            }
        }
        /*test for the 1st of 2 adjacent 'dahs'*/
        if ((TmpDwnIntrvls[n] > Bg1SplitPt) && (TmpDwnIntrvls[n + 1] > Bg1SplitPt))
        {
            /*we have two adjcent dahs, set letter break if the key up interval is longer than the shortest of the 2 Dahs*/
            if ((TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n + 1]))
            {
                ExitPath[n] = 9;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= Bg1SplitPt) &&
                     (TmpUpIntrvls[n + 1] >= Bg1SplitPt) &&
                     (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n + 1]))
            { // this Dah has lot longer Keyup than the next dah; so this looks like a letter break
                ExitPath[n] = 10;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= Bg1SplitPt) &&
                     (TmpUpIntrvls[n] >= TmpUpIntrvls[n + 1]) &&
                     (TmpDwnIntrvls[n] >= 1.3 * TmpDwnIntrvls[n + 1]))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 11;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= 1.4 * Bg1SplitPt))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 12;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 13;
                return false;
            }
        }
        /*test for 2 adjacent 'dits'*/
        else if ((TmpDwnIntrvls[n] < Bg1SplitPt) && (TmpDwnIntrvls[n + 1] < Bg1SplitPt))
        {
            /*we have two adjcent dits, set letter break if the key up interval is 2.0x longer than the shortest of the 2 Dits*/
            if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n] + 8) || (TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n + 1] + 8))
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
        }
        /*test for dah to dit transition*/
        else if ((TmpDwnIntrvls[n] >= Bg1SplitPt) && (TmpDwnIntrvls[n + 1] < Bg1SplitPt))
        {
            /*We have Dah to dit transition set letter break only if key up interval is > 1.6x the dit interval
            And the keyup time is more than 0.6 the dah interval //20240120 added this 2nd qualifier
            20240128 reduced 2nd qualifier to 0.4 & added 3rd 'OR' qualifier, this dah is the longest in the group*/
            if ((TmpUpIntrvls[n] > 1.6 * TmpDwnIntrvls[n + 1]) && ((TmpUpIntrvls[n] > 0.4 * TmpDwnIntrvls[n]) || TmpDwnIntrvls[n] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl))
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
        /*test for dit to dah transition*/
        else if ((TmpDwnIntrvls[n] < Bg1SplitPt) && (TmpDwnIntrvls[n + 1] > Bg1SplitPt))
        {
            /*We have Dit to Dah transition set letter break only if key up interval is > 2x the dit interval*/
            // if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n]))
            if ((TmpUpIntrvls[n] > 2.1 * DitIntrvlVal)) // 20240128 changed it to use generic dit; because bug dits should all be equal
            {
                ExitPath[n] = 18;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 19;
                return false;
            }
        }
        else
        {
            printf("Error 2: NO letter brk test\n");
            ExitPath[n] = 20;
            return ltrbrkFlg;
        }
        /*then this is the last keyup so letter brk has to be true*/
    }
    else
    {
        ExitPath[n] = 21;
        BrkFlg = '+';
        return true;
    }
    /*Should never Get Here*/
    ExitPath[n] = 22;
    printf("Error 2: NO letter brk test\n");
    return ltrbrkFlg;
};

////////////////////////////////////////////////////////

bool AdvParser::Bug2Rules(int& n)
{
    /*Bug2 rules*/
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
    int RunCnt = 0; // used to validate that there were at least two adjacent dahs or dits
    uint16_t maxdah = 0;
    uint16_t mindah = KeyDwnBuckts[KeyDwnBucktPtr].Intrvl;
    char ExtSmbl = ' ';
    for (int i = n; i <= TmpUpIntrvlsPtr; i++)
    {
        if (TmpDwnIntrvls[i] < DitDahSplitVal)
        {
            ExtSmbl = '@';
            break; // quit when a dit is detected
        }
        else // we have a dah;
        {
            RunCnt++;
            if (TmpDwnIntrvls[i] > maxdah)
            {
                maxdah = TmpDwnIntrvls[i];
                maxdahIndx = i;
            }
            if (TmpDwnIntrvls[i] < mindah)
            {
                mindah = TmpDwnIntrvls[i];
                mindahIndx = i;
            }
            if (i > n)
            {
                if ((TmpDwnIntrvls[i] > 1.5 * TmpDwnIntrvls[i - 1]) && (TmpUpIntrvls[i] > 1.5 * DitIntrvlVal))
                {
                    ExtSmbl = '#';
                    break; // quit now this Dah looks significantly streched compared to its predecessor
                }
            }
            if (TmpUpIntrvls[i] > 1.1 * TmpDwnIntrvls[i])
            {
                ExtSmbl = '$';
                break; // quit, Looks like a clear intention to signal a letterbreak
            }
        }
    }
    /*Test that the long dah is significantly longer than its sisters,
      & it is also terminated/followed by a reasonable keyup interval*/
    // if (this->Dbug)
    // {
    //     if (RunCnt > 0)
    //         printf("RunCnt %d%c\t", RunCnt, ExtSmbl);
    //     else
    //         printf("\t\t");
    // }
    if (RunCnt > 1 && maxdahIndx > 0 && (maxdah > 1.3 * mindah) && (TmpUpIntrvls[maxdahIndx] > (DitIntrvlVal + 8)))
    {
        // if(maxdahIndx > 0 && (maxdah > 1.3*mindah) && (TmpUpIntrvls[maxdahIndx] > (0.8* DitDahSplitVal))){
        /*we have a run of dahs, so build/grow the SymbSet to mach the found run of dahs*/
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
        }
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
    }
    else if (RunCnt > 1 && maxdah > 0 && (mindahIndx == maxdahIndx) && (TmpUpIntrvls[maxdahIndx] > TmpDwnIntrvls[maxdahIndx]))
    { // the 1st dah seems to be a letter break
        ExitPath[n] = 21;
        BrkFlg = '+';
        return true;
    }
    /*Now do a similar test/logic for run of 'n' dits in a row (to help better manage 'es' and the like parsing)*/
    /* set ltrbrk based on longest keyup interval in the group*/
    maxdahIndx = 0; // reuse this variable from Dah series test
    mindahIndx = 0; // reuse this variable from Dah series test
    maxdah = 0;     // reuse this variable from Dah series test
    RunCnt = 0;     // used to validate that there were at least two adjacent dahs or dits
    mindah = KeyUpBuckts[KeyDwnBucktPtr].Intrvl;
    for (int i = n; i <= TmpUpIntrvlsPtr; i++)
    {
        if ((TmpDwnIntrvls[i] + 8 > DitDahSplitVal)){
            ExtSmbl = '@';
            break; // quit when a dah is detected or an obvious letter break
        }
        else       // we have a dit;
        {
            RunCnt++;
            if (TmpUpIntrvls[i] > maxdah)
            {
                maxdah = TmpUpIntrvls[i];
                maxdahIndx = i;
            }
            if (TmpUpIntrvls[i] < mindah)
            {
                mindah = TmpDwnIntrvls[i];
                mindahIndx = i;
            }
            if (i > n)
            {
                // if((TmpUpIntrvls[i]> 1.5* TmpUpIntrvls[i-1]) &&(TmpUpIntrvls[i]> 1.5* DitIntrvlVal) ) break; //quit now this Dit looks significantly streched compared to its predecessor
                if ((TmpUpIntrvls[i] > 1.5 * TmpUpIntrvls[i - 1])){
                    ExtSmbl = '#';
                    break; // quit when an obvious letter break is detected
                }
            }
            if ((TmpUpIntrvls[i] > DitDahSplitVal)){
                ExtSmbl = '$';
                break; // quit an there's an obvious letter break
            } 
        }
    }
    /*Test that the long dit keyup is significantly longer than its sisters,
      & it's also terminated/followed by a reasonable keyup interval*/
    // if (this->Dbug)
    // {
    //     if (RunCnt > 0)
    //         printf("RunCnt %d%c\t", RunCnt, ExtSmbl);
    //     else
    //         printf("\t\t");
    // }
    //if (RunCnt > 1 && maxdahIndx > 0 && (TmpUpIntrvls[maxdahIndx] > (1.5 * TmpDwnIntrvls[maxdahIndx])))
    if (RunCnt > 1 && maxdahIndx > 0 && (ExtSmbl == '#'))
    {
        /*we have a run of dits terminated w/ a letterbreak keyup interval, 
        so build/grow the SymbSet to match the found run of dits*/
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            // SymbSet += 1;
        }
        ExitPath[n] = 2;
        BrkFlg = '&';
        return true;
    } else if(RunCnt == 1 && (TmpUpIntrvls[n] > DitDahSplitVal)){
        // quit an there's an obvious letter break
        ExitPath[n] = 3;
        BrkFlg = '+';
        return true;
    }

    /*Middle keyup test to see this keyup is twice the length of the one just before it,
    And we are using a valid keyup reference interval & not just some random bit of noise.
    If it is then call this one a letter break*/
    if (n > 0 && (TmpUpIntrvls[n] > 2.4 * TmpUpIntrvls[n - 1]) && (TmpUpIntrvls[n - 1] >= DitIntrvlVal))
    {
        ExitPath[n] = 4;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the lenght of the one following it,
    AND greater than the one that proceeded it, provided the proceeding keyup was not a letterbrk.
    If all true, then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (n > 0))
    {
        if ((TmpUpIntrvls[n] > 3.4 * TmpUpIntrvls[n + 1]) && (TmpUpIntrvls[n] > TmpUpIntrvls[n - 1]) &&
            LstLtrBrkCnt > 0)
        {
            ExitPath[n] = 5;
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
            // if ((TmpDwnIntrvls[n - 1] > DitDahSplitVal) &&
            //     (TmpDwnIntrvls[n] > DitDahSplitVal) &&
            //     (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
            if ((SymbSet & 0b1) &&
                (TmpDwnIntrvls[n] > DitDahSplitVal) &&
                (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
            { // we are are surrounded by dahs
                if ((TmpUpIntrvls[n - 1] >= DitDahSplitVal) && (TmpUpIntrvls[n] > 1.2 * TmpUpIntrvls[n - 1]))
                {
                    
                    ExitPath[n] = 6;
                    BrkFlg = '+';
                    return true;
                }
                else if ((TmpUpIntrvls[n] >= 0.8 * DitDahSplitVal) &&
                         (TmpUpIntrvls[n - 1] >= 0.8 * DitDahSplitVal) &&
                         (TmpDwnIntrvls[n] >= TmpDwnIntrvls[n - 1]) &&
                         (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n - 1]))
                { // we're the middle of a dah serries; but this one looks streched compared to its predecessor
                    ExitPath[n] = 7;
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
                ExitPath[n] = 8;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n + 1] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n + 1]))
            { // this Dah has lot longer Keyup than the next dah; so this looks like a letter break
                ExitPath[n] = 9;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= TmpUpIntrvls[n + 1]) &&
                     (TmpDwnIntrvls[n] >= 1.3 * TmpDwnIntrvls[n + 1]))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 10;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= 1.4 * DitDahSplitVal))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 11;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 12;
                return false;
            }
        }
        /*test for 2 adjacent 'dits'*/
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*we have two adjcent dits, set letter break if the key up interval is 2.0x longer than the shortest of the 2 Dits*/
            if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n] + 8) || (TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n + 1] + 8))
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
        }
        /*test for dah to dit transition*/
        else if ((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*We have Dah to dit transition; set letter break only if key up interval is > 1.6x the dit interval
            And the keyup time is more than 0.6 the dah interval //20240120 added this 2nd qualifier
            20240128 reduced 2nd qualifier to 0.4 & added 3rd 'OR' qualifier, this dah is the longest in the group*/
            //if ((TmpUpIntrvls[n] > 1.6 * TmpDwnIntrvls[n + 1]) && ((TmpUpIntrvls[n] > 0.4 * TmpDwnIntrvls[n]) || TmpDwnIntrvls[n] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl))
            if ((TmpUpIntrvls[n] > 4.0 * AvgSmblDedSpc) && ((TmpUpIntrvls[n] > 0.4 * TmpDwnIntrvls[n]) || TmpDwnIntrvls[n] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl))
            
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
            /*test for dit to dah transition*/
        }
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
        {
            /*We have Dit to Dah transition set letter break only if key up interval is > 2x the dit interval*/
            // if ((TmpUpIntrvls[n] > 2 * TmpDwnIntrvls[n]))
            if ((TmpUpIntrvls[n] > 2.1 * DitIntrvlVal)) // 20240128 changed it to use generic dit; because bug dits should all be equal
            {
                ExitPath[n] = 17;
                BrkFlg = '+';
                return true;
            }
            else
            {
                ExitPath[n] = 18;
                return false;
            }
        }
        else
        {
            printf("Error 1: NO letter brk test\n");
            ExitPath[n] = 19;
            return ltrbrkFlg;
        }
        /*then this is the last keyup so letter brk has to be true*/
    }
    else
    {
        ExitPath[n] = 20;
        BrkFlg = '+';
        return true;
    }
    /*Should never Get Here*/
    ExitPath[n] = 21;
    printf("Error 2: NO letter brk test\n");
    return ltrbrkFlg;
};
////////////////////////////////////////////////////////
bool AdvParser::SKRules(int& n)
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
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n] > (2.0 * TmpUpIntrvls[n + 1])+8))
    {
        ExitPath[n] = 101;
        BrkFlg = '+';
        return true;
    }
    if (NewSpltVal)
    {
        //if ((TmpUpIntrvls[n] >= 1.5 * 1.2 * KeyUpBuckts[MaxCntKyUpBcktPtr].Intrvl) &&
        //    (TmpUpIntrvls[n] > KeyDwnBuckts[0].Intrvl))
        // if ((TmpUpIntrvls[n] >= 1.5 * DitIntrvlVal) &&
        //     (TmpUpIntrvls[n] > KeyDwnBuckts[0].Intrvl))
        if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
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
/* Key or 'Fist' style test 
 * looks for electronic key by finding constant dah intervals.
 * Bug test by noting the keyup interval is consistantly shorter between the dits vs dahs
 * returns 0, 1, or 5 for paddle; 2,3,& 4 for bug; 6 for unknown; 7 for straight Key.
 */
int AdvParser::DitDahBugTst(void)
{
    int ditcnt;
    int dahDwncnt;
    int dahcnt = ditcnt = dahDwncnt = 0;
    uint16_t dahInterval;
    uint16_t dahDwnInterval;
    uint16_t ditDwnInterval;
    uint16_t ditDwncnt;
    uint16_t ditInterval = dahInterval = dahDwnInterval = ditDwnInterval = ditDwncnt= 0;
    int stop = TmpUpIntrvlsPtr - 1;
    bool same;
    for (int n = 0; n < stop; n++)
    {
        
        /*Made the define the average dah, less restrictive; because paddle generated dahs should all have the same interval */
        if (n > 0 && (TmpDwnIntrvls[n] > DitDahSplitVal))
        {
            dahDwnInterval += TmpDwnIntrvls[n];
            dahDwncnt++;
        }
        else if (n > 0 && (TmpDwnIntrvls[n] < DitDahSplitVal)){
            ditDwnInterval += TmpDwnIntrvls[n];
            ditDwncnt++;
        }
        

        /*Sum the KeyUp numbers for any non letterbreak terminated dah*/
        if ((TmpDwnIntrvls[n] > DitDahSplitVal) &&
            (TmpUpIntrvls[n] < 1.25 * DitDahSplitVal))
        {
            dahInterval += TmpUpIntrvls[n];
            dahcnt++;
        }
        /*test adjcent dits KeyUp timing. But only include if there's not a letter break between them */
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) &&
                 (TmpDwnIntrvls[n + 1] < DitDahSplitVal) &&
                 (TmpUpIntrvls[n] < DitDahSplitVal))
        {
            ditInterval += TmpUpIntrvls[n];
            ditcnt++;
        }
    }
    /*Test/check for Straight key, by dits with varying intervals*/
    if (ditDwncnt >= 4)
    {
        /* average/normalize dit interval */
        ditDwnInterval /= ditDwncnt;
        int Delta = (int)((0.15 * (float)ditDwnInterval));//0.1*
        int NegDelta = -1*Delta;
        //same = true;
        ditDwncnt = 0;
        int GudDitCnt = 0;// could use this as part of a ratio test
        for (int n = 1; n <= this->TmpUpIntrvlsPtr; n++) // skip the 1st key down event because testing showed the timing of the 1st event is often shorter than the rest in the group
        {
            if (TmpDwnIntrvls[n] < DitDahSplitVal)
            {
                ditDwncnt++;
                int error = TmpDwnIntrvls[n] - ditDwnInterval; //at this point, 'ditDwnInterval' is the average dit interval for this word group
                if ((error <  Delta) && (error > NegDelta))
                    GudDitCnt++;
                // else
                //     same = false;
            }
        }
        float Score = (float)GudDitCnt/(float)ditDwncnt;
        //printf("ditDwncnt: %d; \tGudDitCnt: %d; ditDwnInterval: %d; Delta: %d Score: %0.1f\n", ditDwncnt, GudDitCnt, ditDwnInterval, Delta, Score);
        if(Score <= 0.70){
            //this->Dbug = true; //override current Dbug state
            return 7;
        }
    }
    /*Test/check for electronic keys by constant dah intervals*/
    if (dahDwncnt > 1)
    {
        /* average/normalize results */
        dahDwnInterval /= dahDwncnt;
        /*test1*/
        /*if all dah intervals are the same, its a paddle/keyboard */
        same = true;
        dahDwncnt = 0;
        int GudDahCnt = 0;
        uint16_t Tolrenc = (uint16_t)(0.1 * (float)dahDwnInterval);
        // printf("\nTolrenc %d; dahDwnInterval %d\n", Tolrenc, dahDwnInterval);
        for (int n = 1; n < stop; n++) // skip the 1st key down event because testing showed the timing of the 1st event is often shorter than the rest in the group
        {
            if (TmpDwnIntrvls[n] > DitDahSplitVal)
            {
                dahDwncnt++;
                if ((TmpDwnIntrvls[n] < (dahDwnInterval + Tolrenc)) && (TmpDwnIntrvls[n] > (dahDwnInterval - Tolrenc)))
                    GudDahCnt++;
                else
                    same = false;
            }
        }
        /*test2*/
        if (same && (dahDwncnt > 1))
        {
            if (ditcnt > 0 && dahcnt > 0)
            {
                /* average/normalize the keyup invervals results */
                dahInterval /= dahcnt;
                ditInterval /= ditcnt;
                // printf("\nditcnt:%d; dahcnt:%d; ditInterval: %d; dahInterval: %d\n", ditcnt, dahcnt, ditInterval, dahInterval);
                if (dahInterval > ditInterval + 8)
                    return 2; // bug (80)
                else
                    return 1; // paddle/krybrd
            }
            else
                return 0; // paddle/krybrd
        }
        else if (((float)GudDahCnt / (float)dahDwncnt > 0.8))
                return 5; // paddle/krybrd
        else if (dahDwncnt > 1){
            //printf("\nGudDahCnt:%d; dahDwncnt:%d\n",GudDahCnt, dahDwncnt);
            return 3; // bug
        }
    }
    else
    {
        return 6;// not enough info to decide

    }
    return 7;// not enough info to decide



    // printf("\nditcnt:%d; dahcnt:%d; interval cnt: %d\n", ditcnt, dahcnt, stop);
    // if (ditcnt > 0 && dahcnt > 0)
    // {
    //     /* average/normalize results */
    //     dahInterval /= dahcnt;
    //     ditInterval /= ditcnt;
    //     // printf("\nditcnt:%d; dahcnt:%d; ditInterval: %d; dahInterval: %d\n", ditcnt, dahcnt, ditInterval, dahInterval);
    //     if (dahInterval > ditInterval + 8)
    //         return 4; // bug
    //     else
    //         return 5; // paddle/krybrd
    // }
    // else
    //     return 7; // not enough info to decide
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