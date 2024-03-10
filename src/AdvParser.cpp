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
 * 20240205 added WrdBrkVal property to insert space character in post parsed character string + changes to Bug1 & DitDahBugTst
 * 20240206 added StrchdDah property, to better devine which rules apply in bug1 rule set.
 * 20240207 ammended 'AdvSrch4Match' method for setting up 'follow on' search, when 1st searches don't find a match
 * 20240208 reworked 'LstLtrBrkCnt' management to better track the number of keyevents since last letterbreak event
 * 20240210 created new class method 'SrchAgn()' to handle deep dive search for Codevals/morse text, when normal codeval to text conversion fails
 * 20240215 More minor tweaks to Bug1 Rule set & DitDahBugTst
 * 20240216 Added 'FixClassicErrors()' method
 * 20240220 added 'SloppyBgRules()' method. Plus numerous changes to integrate this rule set into the existing code
 * 20240223 numerous 'tweaks' to this file, mostly to better delineate between different key types
 * 20240225 more 'tweaks' to bug2 & sloppybug rule sets; bug1 ruleset is getting little use due to current 'DitDahBugTst()' code
 * 20240227 reworked GetMsgLen(void), FixClassicErrors(void), & SloppyBgRules(int& n)
 * 20240228 reworked SetSpltP() to better ignore noise & return/set DitIntrvlVal & NuSpltVal
 * 20240301 Changed BG2 dah run to detect letterbreak on UnitIntvrlx2r5; chages to SetSpltPt()
 * 20240307 Added SrchEsReplace() function & and simplified the code in FixClassicErrors()
 * 20240309 Moved straight key test ahead of sloppy test;
 * */
// #include "freertos/task.h"
// #include "freertos/semphr.h"
#include "AdvParser.h"
#include "DcodeCW.h"
#include "Goertzel.h"
#include "main.h"

TaskHandle_t AdvParserTaskHandle = NULL;
// /*AdvParserTask Task; Post Parser Code execution*/
// void AdvParserTask(void *param)
// {
//   static uint32_t thread_notification;
//   //uint8_t state;
//   while (1)
//   {
//     /* Sleep until we are notified of a state change by an
//      * interrupt handler. Note the first parameter is pdTRUE,
//      * which has the effect of clearing the task's notification
//      * value back to 0, making the notification value act like
//      * a binary (rather than a counting) semaphore.  */
//     thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//     if (thread_notification)
//     {
//         advparser.EvalTimeData();
//         /*Now compare Advparser decoded text to original text; If not the same,
// 				replace displayed with Advparser version*/
// 				bool same = true;
// 				bool Tst4Match = true;
// 				int i;
// 				int FmtchPtr;

// 				/*Scan/compare last word displayed w/ advpaser's version*/
// 				if (advparser.GetMsgLen() > LtrPtr)
// 				{ // if the advparser verson is longer, then delete the last word printed
// 					same = false;
// 					i = LtrPtr;
// 				}
// 				else
// 				{
// 					for (i = 0; i < LtrPtr; i++)
// 					{
// 						if (advparser.Msgbuf[i] == 0)
// 							Tst4Match = false;
// 						if ((LtrHoldr[i] != advparser.Msgbuf[i]) && Tst4Match)
// 						{
// 							FmtchPtr = i;
// 							same = false;
// 						}
// 						if (LtrHoldr[i] == 0)
// 							break;
// 					}
// 				}
// 				/*If they don't match, replace displayed text with AdvParser's version*/
// 				if (!same)
// 				{
// 					bool oldDltState = dletechar;
// 					dletechar = true;
// 					MsgChrCnt[1] = i; // Load delete buffer w/ the number of characters to be deleted from the display
// 					// printf("Pointer ERROR\n");/ printf("No Match @ %d; %d; %d\n", FmtchPtr, LtrHoldr[FmtchPtr], advparser.Msgbuf[FmtchPtr]);
// 					CptrTxt = false;
// 					dispMsg(advparser.Msgbuf);
// 					CptrTxt = true;
// 					dletechar = oldDltState;
// 				} // else printf("Match\n");
// 	}
//   } // end while(1) loop
//   /** JMH Added this to ESP32 version to handle random crashing with error,"Task Goertzel Task should not return, Aborting now" */
//   vTaskDelete(NULL);
// }
AdvParser::AdvParser(void) // TFT_eSPI *tft_ptr, char *StrdTxt
{
    this->AvgSmblDedSpc = 1200 / 30;
    this->DitDahSplitVal = 106;
    this->AvgSmblDedSpc = 45.0;
    this->Bg1SplitPt = 67;
    this->UnitIntvrlx2r5 = 113;
    this->DitIntrvlVal = 50;
    this->BugKey = 0; // paddle rules
    // xTaskCreate(AdvParserTask, "AdvParserTask Task", 8192, NULL, 2, &AdvParserTaskHandle);

    // ptft = tft_ptr;
    // pStrdTxt = StrdTxt;
    // ToneColor = 0;
};
/*Main entry point to post process the key time intervals used to create the current word*/
// void AdvParser::EvalTimeData(uint16_t KeyUpIntrvls[IntrvlBufSize], uint16_t KeyDwnIntrvls[IntrvlBufSize], int KeyUpPtr, int KeyDwnPtr, int curWPM)
void AdvParser::EvalTimeData(void)
{
    // this->wpm = curWPM;
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
        TmpDwnIntrvls[i] = this->KeyDwnIntrvls[i];
        TmpUpIntrvls[i] = this->KeyUpIntrvls[i];
    }
    /*Now sort the referenced timing arrays*/
    insertionSort(this->KeyDwnIntrvls, KeyDwnPtr);
    insertionSort(this->KeyUpIntrvls, KeyUpPtr);

    KeyDwnBuckts[KeyDwnBucktPtr].Intrvl = this->KeyDwnIntrvls[0]; // At this point KeyDwnBucktPtr = 0
    KeyDwnBuckts[KeyDwnBucktPtr].Cnt = 1;
    /*Build the Key down Bucket table*/
    uint16_t BucktAvg = this->KeyDwnIntrvls[0];
    ;
    for (int i = 1; i < KeyDwnPtr; i++)
    {
        bool match = false;
        // if ((float)KeyDwnIntrvls[i] <= (4 + (1.2 * KeyDwnBuckts[KeyDwnBucktPtr].Intrvl)))
        if ((float)this->KeyDwnIntrvls[i] <= (16 + KeyDwnBuckts[KeyDwnBucktPtr].Intrvl)) // 20240220 step buckets by timing error
        {
            BucktAvg += this->KeyDwnIntrvls[i];
            KeyDwnBuckts[KeyDwnBucktPtr].Cnt++;
            match = true;
        }
        else
        {
            /*time to calc this bucket's average*/
            KeyDwnBuckts[KeyDwnBucktPtr].Intrvl = BucktAvg / KeyDwnBuckts[KeyDwnBucktPtr].Cnt;
        }
        if (!match) // if not a match, then time to create a new bucket group
        {
            KeyDwnBucktPtr++;
            if (KeyDwnBucktPtr >= 15)
            {
                KeyDwnBucktPtr = 14;
                break;
            }
            /*start a new average*/
            BucktAvg = this->KeyDwnIntrvls[i];
            KeyDwnBuckts[KeyDwnBucktPtr].Intrvl = BucktAvg;
            KeyDwnBuckts[KeyDwnBucktPtr].Cnt = 1;
        }
    }
    /*cleanup by finding the average of the last group*/
    KeyDwnBuckts[KeyDwnBucktPtr].Intrvl = BucktAvg / KeyDwnBuckts[KeyDwnBucktPtr].Cnt;
    /*Build the Key Up Bucket table*/
    KeyUpBuckts[KeyUpBucktPtr].Intrvl = this->KeyUpIntrvls[0]; // At this point KeyUpBucktPtr = 0
    KeyUpBuckts[KeyUpBucktPtr].Cnt = 1;
    for (int i = 1; i < KeyUpPtr; i++)
    {
        bool match = false;
        if ((float)this->KeyUpIntrvls[i] <= (4 + (1.2 * KeyUpBuckts[KeyUpBucktPtr].Intrvl)))
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
            KeyUpBuckts[KeyUpBucktPtr].Intrvl = this->KeyUpIntrvls[i];
            KeyUpBuckts[KeyUpBucktPtr].Cnt = 1;
        }
    }

    if (KeyDwnBucktPtr >= 1 && KeyUpBucktPtr >= 1)
    {
        if (Dbug)
        {
            for (int i = 0; i <= KeyDwnBucktPtr; i++)
            {
                printf(" KeyDwn: %3d; Cnt:%d\t", KeyDwnBuckts[i].Intrvl, KeyDwnBuckts[i].Cnt);
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
    uint16_t OldIntvrlx2r5 = UnitIntvrlx2r5;
    UnitIntvrlx2r5 = (uint16_t)(2.4 * ((AvgSmblDedSpc + DitIntrvlVal) / 2));
    Bg1SplitPt = (uint16_t)((float)UnitIntvrlx2r5 * 0.726);
    // this->WrdBrkVal = (uint16_t)(5 * ((AvgSmblDedSpc + DitIntrvlVal) / 2));
    this->WrdBrkVal = (uint16_t)(6 * ((AvgSmblDedSpc + DitIntrvlVal) / 2)); // 20240220 - made wrd break a little longer, to reduce the frequency of un-needed word breaks
    /*OK; before we can build a text string*/
    /*Need to 1st, decide which parsing rule set to use*/
    // if (Dbug)
    // {
    //     printf("AvgDedSpc:%0.1f\tUnitIntvrlx2r5:%d\n", AvgSmblDedSpc, UnitIntvrlx2r5);
    //     printf("\nKeyDwnBuckt Cnt: %d ", KeyDwnBucktPtr + 1);
    // }
    // printf("KeyDwnBuckt Cnt: %d\tKeyUpBuckt Cnt: %d \n", KeyDwnBucktPtr, KeyUpBucktPtr);
    uint8_t bgPdlCd = 0;
    if ((UnitIntvrlx2r5 >= OldIntvrlx2r5 - 5) && (UnitIntvrlx2r5 <= OldIntvrlx2r5 + 5) && (TmpUpIntrvlsPtr <= 7))
    {
        // propbablly not enough data to make a good decision; So stick with the old key type
        /*not enough info leave as is */
        if (Dbug)
            printf("Use old key type (Cd 99)\n\n\n\n\n\n");
        bgPdlCd = 99;
    }
    else
    {
        // printf("\nAvgSmblDedSpc:%d; KeyDwnBuckts[0].Intrvl:%d; Intrvl / 3: %0.1f\n", (int)AvgSmblDedSpc, KeyDwnBuckts[0].Intrvl, KeyDwnBuckts[0].Intrvl / 2.7);
        /*select 'cootie' key based on extreme short keyup timing relative to keydown time*/
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
        // else if(KeyDwnBucktPtr >= 2 && KeyUpBucktPtr >= 4)
        // { //Sloppy Bug
        //     BugKey = 6;
        //     bgPdlCd = 40;
        //     this->WrdBrkVal = (uint16_t) 3.0 * this->UnitIntvrlx2r5;
        // }
        else
        {
            /*the following test chooses between paddle or bug*/
            /* returns 0, 1, or 5 for paddle, 2,3,& 4 for bug, &  6 for unknown; sloppy bug = 10 */
            int DitDahBugTstCd = this->DitDahBugTst();
            if (Dbug)
                printf("DitDahBugTst(): %d\n", DitDahBugTstCd);
            /*UnComent the next 2 lines for "locked" rule set testing*/
            // DitDahBugTstCd = 2;
            // this->StrchdDah = true;
            switch (DitDahBugTstCd)
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
                /*not enough info leave as is */
                bgPdlCd = 98;
                // BugKey = 1;
                break;
            case 5:
                /* its a paddle */
                bgPdlCd = 72;
                BugKey = 0;
                break;
            case 6:
                /*not enough info leave as is */
                bgPdlCd = 99;
                break;
            case 7:
                /*Straight Key */
                bgPdlCd = 50;
                BugKey = 5;
                break;
            case 8:
                /* its a bug */
                bgPdlCd = 82;
                BugKey = 1;
                break;
            case 9:
                /* its a bug */
                bgPdlCd = 83;
                BugKey = 1;
                break;
            case 10:
                // Sloppy Bug
                // printf("sloppy bug\n");
                BugKey = 6;
                bgPdlCd = 40;
                this->WrdBrkVal = (uint16_t)3.0 * this->UnitIntvrlx2r5;
                break;
            default:
                /*if we are here, start by assuming BugKey value equals 1; i.e., "bug1"*/
                BugKey = 1; //
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
    }
    /*End of select Key type (BugKey) code*/
    if (Dbug)
    {
        printf("\nSplitPoint:%3d\tBg1SplitPt:%d\tDitIntrvlVal:%d\t", DitDahSplitVal, Bg1SplitPt, DitIntrvlVal);
        printf("AvgDedSpc:%0.1f\tUnitIntvrlx2r5:%d\tWrdBrkVal:%d\n", AvgSmblDedSpc, UnitIntvrlx2r5, WrdBrkVal);
        printf("\nKeyDwnBuckt Cnt: %d\n", KeyDwnBucktPtr + 1);
    }
    /*Now for bug key type (1), test which bug style (rule set to use)
    But if bgPdlCd = 82 or 83 (found stretched dahs) stick with bug1 rule set*/
    if (BugKey == 1 && bgPdlCd != 99)
    {
        // if ((DitIntrvlVal > 1.5 * AvgSmblDedSpc)  && bgPdlCd == 80)
        if (!this->StrchdDah)
        { /*This symbol set does not appear to have "Exagerated dahs so use bug2 rule set"*/
            bgPdlCd += 100;
            BugKey = 4; // Bug2
        }
    }
    KeyType = BugKey; // let the outside world know what mode/rule set being used
    switch (BugKey)
    {
    case 0:          // paddle/keyboard
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 1:          // Bug1
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 2:          // cootie type A
        ModeCnt = 3; // DcodeCW.cpp use "cootie" settings/timing ; no glitch detection
        break;
    case 3:          // cootie typ B
        ModeCnt = 3; // DcodeCW.cpp use "cootie" settings/timing ; no glitch detection
        break;
    case 4:          // Bug2
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 5:          // Straight Key
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    case 6:          // Sloppy Bug
        ModeCnt = 0; // DcodeCW.cpp use "Normal" timing
        break;
    default:
        break;
    }
    SetModFlgs(ModeCnt); // DcodeCW.cpp routine; Update DcodeCW.cpp timing settings & ultimately update display status line
    CurMdStng(ModeCnt);  // convert to ModeVal for Goertzel.cpp to use (mainly glitch control)

    // if (Dbug)
    // {
    //     printf(" CD:%d\n", bgPdlCd);
    // }
    int n = 0;
    SymbSet = 1;
    /*Reset the string buffer (Msgbuf)*/
    this->Msgbuf[0] = 0;
    ExitPtr = 0;
    if (Dbug)
    {
        char txtbuf[16];
        // printf("Key Type:");
        switch (BugKey)
        {
        case 0:
            sprintf(txtbuf, "Paddle/KeyBoard");
            // printf(" Paddle/KeyBoard\t");
            break;
        case 1:
            sprintf(txtbuf, "Bug1");
            // printf(" Bug1\t");
            break;
        case 2:
            sprintf(txtbuf, "Cootie");
            // printf(" Cootie\t");
            break;
        case 3:
            sprintf(txtbuf, "ShrtDits");
            // printf(" ShrtDits\t");
            break;
        case 4:
            sprintf(txtbuf, "Bug2");
            // printf(" Bug2\t");
            break;
        case 5:
            sprintf(txtbuf, "Str8Key");
            // printf(" Str8Key\t");
            break;
        case 6:
            sprintf(txtbuf, "SloppyBug");
            // printf(" SloppyBug\t");
            break;
        default:
            sprintf(txtbuf, "???");
            // printf(" ???\t");
            break;
        }
        printf("Key Type: %s; CD:%d\n", txtbuf, bgPdlCd);
    }

    /*Now have everything needed to rebuild/parse this group of Key Down/Up times*/
    this->LstLtrBrkCnt = 0;
    while (n < TmpUpIntrvlsPtr) // ''n' initially is 0
    {
        if (Dbug)
        {
            printf("%2d. DWn: %3d\t", n, TmpDwnIntrvls[n]);
            if (n < KeyUpPtr)
                printf("Up: %3d\t", TmpUpIntrvls[n]);
            else
                printf("Up: ???\t");
        }
        SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        /*TODO - need to change the following 'if' to 'Switch' based which ruleset/keytype will be used later
        to aviod keydown interval from bieng mis-classified as a dah, where later it might be recognized a dit */
        if (BugKey == 6)
        {                                           // Sloppy Bug
            if (TmpDwnIntrvls[n] >= DitDahSplitVal) // if true, its a 'dah'
                SymbSet += 1;                       // Set the new bit to a 'Dah'
        }
        else if (BugKey == 0)
        {                                           // Paddle/KeyBoard
            if (TmpDwnIntrvls[n] >= DitDahSplitVal) // AKA 'SplitPoint'; if true, its a 'dah'
                SymbSet += 1;                       // Set the new bit to a 'Dah'
        }
        else
        {
            if (TmpDwnIntrvls[n] >= Bg1SplitPt) // if true, its a 'dah'
                SymbSet += 1;                   // Set the new bit to a 'Dah'
        }
        int curN = n + 1;
        /* now test if the follow on keyup time represents a letter break */
        if (Tst4LtrBrk(n))
        { /*if true we have a complete symbol set; So find matching character(s)*/
            /*But 1st, need to check if the letterbrk was based on exit-code 2,
            if so & in debug mode, need to do some logging cleanup/catchup work  */
            if (ExitPath[n] == 2 || ExitPath[n] == 24 || ExitPath[n] == 25 || ExitPath[n] == 26)
            {
                while (curN <= n)
                {
                    if (Dbug)
                    {
                        printf("\n%2d. D0n: %3d\t", curN, TmpDwnIntrvls[curN]);
                        if (curN < KeyUpPtr)
                            printf("Up: %3d\t", TmpUpIntrvls[curN]);
                        else
                            printf("Up: ???\t");
                    }
                    curN++;
                    this->LstLtrBrkCnt++;
                }
                // printf("n:%d; LstLtrBrkCnt: %d \n", n, this->LstLtrBrkCnt);
            }
            /*Now, if the symbol set = 31 (4 dits in a row), we need to figure out where the biggest key up interval is
            and subdivide this into something that can be decoded*/
            if ((SymbSet == 31))
                Dcode4Dahs(n);
            else
                int IndxPtr = AdvSrch4Match(n, SymbSet, true); // try to convert the current symbol set to text &
                                                               // and save/append the results to 'Msgbuf[]'
                                                               // start a new symbolset
            /*We found a letter, but maybe its also a word; test by testing the keyup interval*/
            if (TmpUpIntrvls[n] > this->WrdBrkVal && (n < this->TmpUpIntrvlsPtr - 1))
            { // yes, it looks like a word break
                // add " " (space) to reparsed string
                this->AdvSrch4Match(n, 255, false);
            }
            this->LstLtrBrkCnt = 0;
        }
        else if (ExitPath[n] == 4 && BrkFlg == '%')
        { // found a run but ended W/o a letter break. So for Debug output,need to advance/resync pointers
            while (curN <= n)
            {
                if (Dbug)
                {
                    printf("\n%2d. Dwn: %3d\tUp: %3d\t", curN, TmpDwnIntrvls[curN], TmpUpIntrvls[curN]);
                }
                this->LstLtrBrkCnt++;
                curN++;
            }
            this->LstLtrBrkCnt++;
            // printf("n:%d; LstLtrBrkCnt: %d \n", n, this->LstLtrBrkCnt);
        }
        else
        {
            this->LstLtrBrkCnt++;
            // printf("n:%d; LstLtrBrkCnt: %d \n", n, this->LstLtrBrkCnt);
        }
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
        // printf("n:%d; LstLtrBrkCnt: %d \n", n, this->LstLtrBrkCnt);
    }
    /*Text string Analysis complete*/
    this->FixClassicErrors(); // now do a final check to look & correct classic parsing errors
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
 Note: 'n' points to the last keydown bucket (interval/cnt) in the array
 */
void AdvParser::SetSpltPt(Buckt_t arr[], int n)
{
    int i;
    uint16_t OldSpltVal = this->DitDahSplitVal;
    this->NuSpltVal = 0;
    this->AllDah = true; // made this a class property so it could be used later in the "Tst4LtrBrk()" method
    bool AllDit = true;
    bool SpltCalc = true;
    int lastDitPtr = 0;
    int MaxDitPtr = 0;
    int MaxDitCnt, DahCnt;
    uint16_t MaxIntrval = (3 * (1500 / wpm)); // set the max interval to that of a dah @ 0.8*WPM
    uint16_t MaxDelta = 0;
    MaxDitCnt = DahCnt = 0;
    // if (Dbug) printf("\nSetSpltPt() - ");
    for (i = 0; i < n; i++)
    {
        if ((arr[i + 1].Intrvl > MaxIntrval))
            break; // exclude/stop comparison when/if the interval exceeds that of a dah interval @ the current WPM
        /*Test if the change interval between these keydwn groups is bigger than anything we've seen before (in this symbol set)*/
        if (arr[i].Intrvl > 34)
        { /*only consider intevals that represent keying below 35 wpm. Anything faster is likely just noise*/
            if (arr[i + 1].Intrvl - arr[i].Intrvl > MaxDelta && SpltCalc)
            {
                /* when there is more than 2 buckets, skip the last one.
                Because, for bugs, the last one is likely an exaggerated daH */
                if (n > 1 && i + 1 == n && arr[i + 1].Cnt == 1 && !AllDit)
                    break;
                MaxDelta = arr[i + 1].Intrvl - arr[i].Intrvl;
                this->NuSpltVal = arr[i].Intrvl + (MaxDelta) / 2;
                lastDitPtr = i;

                /*now Update/recalculate the running average dit interval value (based on the last six dit intervals)*/
                if (arr[i].Intrvl <= DitDahSplitVal)
                {
                    /*20240228 new method*/
                    int LpCntr = 0;
                    while (LpCntr < arr[i].Cnt)
                    {
                        this->DitIntrvlVal = (uint16_t)((5 * (float)this->DitIntrvlVal) + (float)arr[i].Intrvl) / 6.0;
                        LpCntr++;
                    }
                    // if(arr[i].Cnt <=6){
                    //     this->DitIntrvlVal =(uint16_t) (((6-arr[i].Cnt)*(float)DitIntrvlVal) + (arr[i].Cnt * 1.1 * (float)arr[i].Intrvl))/6.0;//20240129 build a running average of the last 6 dits
                    // }
                    // else this->DitIntrvlVal = arr[i].Intrvl;
                }
                if (arr[i + 1].Intrvl > 1.5 * arr[i].Intrvl && (arr[i].Cnt > 2))
                { // 1.7 * arr[i].Intrvl
                    // if (Dbug) printf("SpltCalc = false\n");
                    SpltCalc = false; // found an abrupt increase in keydown interval; likely reprents the dit/dah boundry, so look no further
                }
            }
            /*Collect data for an alternative derivation of NuSpltVal, by identifying the bucket with the most dits*/
            if (arr[i].Cnt > MaxDitCnt)
            {
                MaxDitPtr = i;
                MaxDitCnt = arr[i].Cnt;
            }
        }
        if ((arr[(n - 1) - i].Intrvl < DitDahSplitVal))
            AllDit = AllDah = false;
        if ((arr[i].Intrvl > DitDahSplitVal))
            AllDit = AllDah = false;
        if (lastDitPtr + 1 >= n - i)
            break; // make absolutey certian that the dits dont cross over the dahs
        /*check if the interval is a dit at less than 35wpm*/
        if ((arr[i].Intrvl > 34) && (arr[i + 1].Intrvl > (1.45 * arr[i].Intrvl)) && arr[i].Cnt > 3)
        {
            // if (Dbug) printf("BREAK\n");
            break; // since we are working our way up the interval sequence, its appears that we have a cluster of dits & the next step up (because the gap is 1.45 x this cluster) should be treated as a 'dah'. So its safe to quit looking
        }
    }
    /* If a significant cluster of 'dits' were found (5), then test if their weighted interval
       value is greater than the nusplitval value found above.
       If true, then use this weighted value & recalculate the DitIntrvlVal*/
    // printf("MaxDitCnt %d; MaxDitPtr %d; lastDitPtr %d\n", MaxDitCnt, MaxDitPtr, lastDitPtr);
    if (MaxDitCnt >= 5)
    { // && MaxDitPtr > lastDitPtr
        uint16_t WghtdDit = (1.5 * arr[MaxDitPtr].Intrvl);
        // printf("WghtdDit %d; NuSpltVal %d\n", WghtdDit, NuSpltVal);
        //  if(WghtdDit > NuSpltVal)
        //  {

        this->NuSpltVal = WghtdDit;
        lastDitPtr = MaxDitPtr;
        AllDit = AllDah = false;
        if (Dbug)
            printf("Using Alternate Calc NuSpltVal Method: %d\n", this->NuSpltVal);
        int LpCntr = 0;
        while (LpCntr < arr[lastDitPtr].Cnt)
        {
            this->DitIntrvlVal = (uint16_t)((5 * (float)this->DitIntrvlVal) + (float)arr[lastDitPtr].Intrvl) / 6.0;
            LpCntr++;
        }
        // }
    }
    // printf("\nlastDitPtr =%d; lastDahPtr =%d; ditVal:%d; dahVal:%d\n", lastDitPtr, lastDahPtr, arr[lastDitPtr].Intrvl, arr[lastDahPtr].Intrvl);

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
        {
            if (Dbug)
                printf("(AllDit || AllDah) ABORT\n");
            return;
        }
    }
    else
        SameSymblWrdCnt = 0;

    if (this->NuSpltVal != 0)
    {
        if (Dbug)
            printf("\nNuSpltVal: %d; Dit Ndx: %d\n", this->NuSpltVal, lastDitPtr);
        if (DitDahSplitVal == 0)
            DitDahSplitVal = this->NuSpltVal;
        else
            /*New Method for weighting the NuSpltVal against previous DitDahSplitVal*/
            /*Bottom line, if this word symbol set has more than 30 elements forget the old split value & use the one just found*/
            /*if less than 30, weight/avg the new result proportionally to the last 30*/
            if (TmpUpIntrvlsPtr >= 30)
                DitDahSplitVal = this->NuSpltVal;
            else
            {
                int OldWght = 30 - TmpUpIntrvlsPtr;
                DitDahSplitVal = ((OldWght * DitDahSplitVal) + (TmpUpIntrvlsPtr * this->NuSpltVal)) / 30;
            }
        // DitDahSplitVal = (3 * DitDahSplitVal + NuSpltVal) / 4;
    }
    else
    {
        if (this->DitDahSplitVal == 0)
            this->DitDahSplitVal = OldSpltVal;
        this->NuSpltVal = DitDahSplitVal; // make sure that NuSpltVal !=0
    }
};

/*for this group of keydown intervals(TmpDwnIntrvls[]) & the selected keyUp interval (TmpDwnIntrvls[n]),
use the test set out within to decide if this keyup time represents a letter break
& return "true" if it does*/
bool AdvParser::Tst4LtrBrk(int &n)
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
    case 6:
        return this->SloppyBgRules(n);
        break;
    default:
        return false;
        break;
    }
};
/////////////////////////////////////////////////////////
/*Rule set used to parse Sloppy Bug (B3); i.e., an "O" sounds like, "dot, dot, dah"
main rule here is key down interval plus key up interval > 2.5*UnitIntvrlx2r5 is a letter break */
bool AdvParser::SloppyBgRules(int &n)
{
    bool ltrbrkFlg = false;
    if (TmpUpIntrvls[n] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl)
    {
        ExitPath[n] = 0;
        BrkFlg = '+';
        return true;
    }

    /*test for N dahs in a row, & set ltrbrk based on longest dah in the group*/
    int maxdahIndx = 0;
    int mindahIndx = 0;
    int RunCnt = 0; // used to validate that there were at least two adjacent dahs or dits
    uint16_t maxdah = 0;
    uint16_t mindah = KeyDwnBuckts[KeyDwnBucktPtr].Intrvl;
    char ExtSmbl = ' ';
    if (SymbSet & 1)                                                                                                   // if the current symbol is a dah, do the following tests
    {                                                                                                                  /*Current keydwn state represents a dah */
        if (TmpDwnIntrvls[n] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl && this->StrchdDah && RunCnt != 0 && SymbSet == 3) // dont consider strected dah on very 1st sybmol
        {
            ExitPath[n] = 25;
            BrkFlg = '+';
            return true;
        }
        /*Lead off by a quick test/check for an 'O'*/
        if (n + 2 <= TmpUpIntrvlsPtr)
        { /*We have enough keydown intervals to test for an 'O'*/
            if (TmpDwnIntrvls[n] >= DitDahSplitVal && TmpDwnIntrvls[n + 1] >= DitDahSplitVal && TmpDwnIntrvls[n + 2] >= DitDahSplitVal)
            { /*We have 3 consectutive dahs*/
                /*Now make sure the last dah is the best letter break*/
                uint16_t ThrdCombo = TmpDwnIntrvls[n + 2] + TmpUpIntrvls[n + 2];
                if (ThrdCombo >= 2.5 * this->UnitIntvrlx2r5 && (ThrdCombo) > (TmpDwnIntrvls[n + 1] + TmpUpIntrvls[n + 1]) && (ThrdCombo) > (TmpDwnIntrvls[n] + TmpUpIntrvls[n]))
                {
                    /*We have a clear letterbreak*/
                    int STOP = n + 2;
                    while (n < STOP)
                    {
                        n++;
                        SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
                        SymbSet += 1;
                    }
                    ExitPath[n] = 24;
                    BrkFlg = '+';
                    return true;
                }
                /*Alternate test for "O"; Look at next keydown event, & if it too is a dah,
                but its keyup is substasntually shorter than the current keyup intervsal,
                then treat this keyup as a letterbreak*/
                if (n + 3 <= TmpUpIntrvlsPtr)
                { /*there are enough time intervals to make this test */
                    uint16_t ThrdCombo = TmpDwnIntrvls[n + 2] + TmpUpIntrvls[n + 2];
                    if (TmpDwnIntrvls[n + 3] >= this->DitDahSplitVal && TmpUpIntrvls[n + 2] >= 1.8 * TmpUpIntrvls[n + 3] && ThrdCombo > 2.4 * this->UnitIntvrlx2r5) //&& TmpUpIntrvls[n + 2] > this->DitDahSplitVal
                    {
                        int STOP = n + 2;
                        while (n < STOP)
                        {
                            n++;
                            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
                            SymbSet += 1;
                        }
                        ExitPath[n] = 26;
                        BrkFlg = '+';
                        return true;
                    }
                }
            }
        }
        for (int i = n; i < TmpUpIntrvlsPtr; i++)
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
                    /*20240301 removed the middle term based & raised >UnitIntvrlx2r5 multpilier from 2.3 to 4*/
                    if ((TmpDwnIntrvls[i] + TmpUpIntrvls[i] >= 2.4 * this->UnitIntvrlx2r5) && RunCnt != 3) // && ((float)TmpUpIntrvls[i] > (1.2 * (float)this->DitIntrvlVal))
                    /*don't consider this out on 3rd time, because we have already done "O" checks*/
                    {
                        if (this->Dbug)
                        {
                            printf("EXIT A %d\t", RunCnt);
                        }
                        ExtSmbl = '#';
                        break; // quit now this Dah looks significantly stretched compared to its predecessor
                    }
                    if (RunCnt == 3)
                    { /* we have 3 dahs; & it didn't pass the earlier "o" checks, but it might be part of a 'J' */
                        if ((TmpDwnIntrvls[i] >= 1.25 * TmpDwnIntrvls[i - 1]) && (TmpDwnIntrvls[i] >= TmpDwnIntrvls[i - 2]) && (TmpDwnIntrvls[i] >= TmpDwnIntrvls[i + 1]))
                        { /*the 3rd dah was the longest in this serries of 3 & its longer than the next keydown interval*/
                            if (this->Dbug)
                            {
                                printf("EXIT H %d\t", RunCnt);
                            }
                            ExtSmbl = '#';
                            break; // quit now this Dah looks significantly stretched compared to its predecessor
                        }
                    }
                    // if ((TmpDwnIntrvls[i] > 1.5 * TmpDwnIntrvls[i - 1]) && (TmpUpIntrvls[i] > 1.5 * DitIntrvlVal))
                    // {
                    //     if (this->Dbug)
                    //     {
                    //         printf("EXIT A\t");
                    //     }
                    //     ExtSmbl = '#';
                    //     break; // quit now this Dah looks significantly stretched compared to its predecessor
                    // }
                    // else if ((TmpUpIntrvls[i] > 1.4 * TmpDwnIntrvls[i]) ||
                    //          ((TmpUpIntrvls[i] >= UnitIntvrlx2r5) && ((float)TmpUpIntrvls[i] > 0.8*(float)TmpDwnIntrvls[i]))) //20240214-Replaced the following commented out line with this
                    //          //(TmpUpIntrvls[i] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl))
                    // {
                    //     if (this->Dbug)
                    //     {
                    //         printf("EXIT B\t");
                    //     }
                    //     ExtSmbl = '#'; //(exit code 2)
                    //     break;         // quit, Looks like a clear intention to signal a letterbreak
                    // }
                    /*Only consider the following "out", if we're not working with a collection of symbols that
                    contains "Stretched" Dahs*/
                    // else if (TmpUpIntrvls[i] >= this->UnitIntvrlx2r5 && !this->StrchdDah)
                    // {
                    //     if (this->Dbug)
                    //     {
                    //         printf("EXIT C %d; %d; %d\t", TmpUpIntrvls[i], i, RunCnt);
                    //     }

                    //     ExtSmbl = '#'; //(exit code 2)
                    //     break;         // quit, Looks like a clear intention to signal a letterbreak
                    // }
                    // else if ((TmpDwnIntrvls[i] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl) && (TmpUpIntrvls[i] > UnitIntvrlx2r5))
                    // {
                    //     if (this->Dbug)
                    //     {
                    //         printf("EXIT D\t");
                    //     }
                    //     ExtSmbl = '#'; //(exit code 2)
                    //     break;         // quit, Looks like a clear intention to signal a letterbreak
                    // }
                }
                else if ((TmpDwnIntrvls[i] + TmpUpIntrvls[i] >= 2.5 * this->UnitIntvrlx2r5) && (TmpUpIntrvls[i] > this->DitDahSplitVal)) // 2.4 *// 2.3* //2.5 * this->UnitIntvrlx2r5
                {
                    if (this->Dbug)
                    {
                        printf("EXIT B\t");
                    }
                    ExtSmbl = '$';
                    break; // quit now this Dah looks significantly stretched compared to its predecessor
                }
                else if (i == TmpUpIntrvlsPtr - 1)
                {
                    if (this->Dbug)
                    {
                        printf("EXIT C %d; TmpUpIntrvlsPtr: %d\t", RunCnt, TmpUpIntrvlsPtr);
                    }
                    ExtSmbl = '#';
                    break; // quit now this Dah is the last symbolset entry, so has to be a letter break
                }
                // else if ((TmpUpIntrvls[i] > 1.4 * TmpDwnIntrvls[i]) ||
                //          ((TmpUpIntrvls[i] >= UnitIntvrlx2r5) && (TmpUpIntrvls[i] > TmpDwnIntrvls[i]))) //20240214-Replaced the following commented out line with this
                //         //(TmpUpIntrvls[i] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl))
                // {
                //     if (this->Dbug)
                //     {
                //         printf("EXIT E\t");
                //     }
                //     ExtSmbl = '$'; //(exit code 23)
                //     break;         // quit, Looks like a clear intention to signal a letterbreak
                // }
                // /*Only consider the following "out", if we're not working with a collection of symbols that contains "Stretched" Dahs*/
                // else if (TmpUpIntrvls[i] >= this->UnitIntvrlx2r5 && !this->StrchdDah)
                // {
                //     if (this->Dbug)
                //     {
                //         printf("EXIT F %d; %d; %d\t", TmpUpIntrvls[i], i, RunCnt);
                //     }
                //     ExtSmbl = '$'; //(exit code 23)
                //     break;         // quit, Looks like a clear intention to signal a letterbreak
                // }
                // else if ((TmpDwnIntrvls[i] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl) && (TmpUpIntrvls[i] > UnitIntvrlx2r5))
                // {
                //     if (this->Dbug)
                //     {
                //         printf("EXIT G\t");
                //     }
                //     ExtSmbl = '$'; //(exit code 23)
                //     break;         // quit, Looks like a clear intention to signal a letterbreak
                // }
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
        if (RunCnt > 1 && ExtSmbl == '#')
        {
            /*we have a run of dahs, so build/grow the SymbSet to match the found run of dahs; remember since the 1st dah has already been added,
            we can skip that one*/
            int STOP = n + (RunCnt - 1);
            while (n < STOP)
            {
                n++;
                SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
                SymbSet += 1;
            }
            ExitPath[n] = 2;
            BrkFlg = '+';
            return true;
        }
        else if (RunCnt > 1 && (ExtSmbl == '@'))
        { // there was a run of dahs but no letterbreak terminator found; So just append what was found to the current symbolset & continue looking
            while (RunCnt > 1)
            {
                n++;
                SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
                SymbSet += 1;
                RunCnt--;
            }
            // printf("A RunCnt %d; n %d; SymbSet %d; ExtSmbl %c\t", RunCnt, n, SymbSet, ExtSmbl);
            ExitPath[n] = 4; // NOTE: there are two "ExitPath[n] = 4;"
            BrkFlg = '%';
            return false;
        }
        // else if (RunCnt > 1 && maxdah > 0 && (mindahIndx == maxdahIndx) && (TmpUpIntrvls[maxdahIndx] > TmpDwnIntrvls[maxdahIndx]))
        else if (RunCnt > 0 && ExtSmbl == '$' && n == maxdahIndx)
        { // the 1st dah seems to be a letter break
            ExitPath[n] = 23;
            BrkFlg = '+';
            return true;
        }
    }
    /*Now do a similar test/logic for n dits in a row (to help better manage 'es' and the like parsing)*/
    /* set ltrbrk based on longest keyup interval in the group*/
    maxdahIndx = 0; // reuse this variable from Dah series test
    mindahIndx = 0; // reuse this variable from Dah series test
    maxdah = 0;     // reuse this variable from Dah series test
    RunCnt = 0;     // used to validate that there were at least two adjacent dahs or dits
    mindah = KeyUpBuckts[KeyDwnBucktPtr].Intrvl;
    ExtSmbl = ' ';
    /*use which ever retuns the smaller inteval*/
    uint16_t bstLtrBrk = this->DitDahSplitVal;
    if (this->Bg1SplitPt < bstLtrBrk)
        bstLtrBrk = this->Bg1SplitPt;
    for (int i = n; i < TmpUpIntrvlsPtr; i++)
    {
        // if ((TmpDwnIntrvls[i] + 8 > DitDahSplitVal))
        if (TmpDwnIntrvls[i] >= DitDahSplitVal)
        {
            ExtSmbl = '@';
            break; // quit, a dah was detected
        }
        else // we have a dit;
        {
            RunCnt++;
            maxdahIndx = i;
            if (n > 0 && RunCnt == 1)
            { /*we are in the middle of a letter */
                if (TmpUpIntrvls[i] > 2 * TmpUpIntrvls[i - 1])
                {
                    if ((TmpUpIntrvls[i] > UnitIntvrlx2r5))
                    { /*we are in a run of dits, & this keyup interval is significantly longer
                        than its predecessor,& and it exceed the typical letterbreak interval */
                        if (this->Dbug)
                        {
                            printf("EXIT D\t");
                        }
                        ExtSmbl = '#';
                        break; // quit an there's an obvious letter break
                    }
                }
            }
            // if (TmpDwnIntrvls[i] + TmpUpIntrvls[i] >= 2.5 * this->UnitIntvrlx2r5)
            // if (TmpUpIntrvls[i] >= 1.3 * this->UnitIntvrlx2r5)
            /*20240228 change based on recorded noisyA test this date*/
            if (TmpUpIntrvls[i] >= this->UnitIntvrlx2r5)
            {
                if (this->Dbug)
                {
                    printf("EXIT E %d\t", RunCnt);
                }
                ExtSmbl = '#';
                break; // quit now this Dit looks significantly stretched compared to its predecessor
            }
            if (RunCnt > 1 && TmpUpIntrvls[i] >= 2 * TmpUpIntrvls[i - 1])
            {
                if (this->Dbug)
                {
                    printf("EXIT F %d\t", RunCnt);
                }
                ExtSmbl = '#';
                break; // quit now this Dit up interval is significantly stretched compared to its predecessor
            }
            if (i == TmpUpIntrvlsPtr - 1)
            {
                if (this->Dbug)
                {
                    printf("EXIT G %d; %d\t", RunCnt, TmpUpIntrvlsPtr);
                }
                ExtSmbl = '#';
                break; // quit now this Dit is the last symbolset entry, so has to be a letter break
            }
        }
    }
    /*Test that there was a run of dits AND it was bounded/terminated by a letterbreak*/
    if (RunCnt > 1 && maxdahIndx > 0 && (ExtSmbl == '#'))
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
    }
    else if (RunCnt > 1 && (ExtSmbl == '@'))
    { // there was a run of dits but no letterbreak terminator found; So just append what was found to the current symbolset & continue looking
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        }
        // printf("B RunCnt %d; n %d; ExtSmbl %c\t", RunCnt, n, ExtSmbl);
        ExitPath[n] = 4; // NOTE: there are two "ExitPath[n] = 4;"
        BrkFlg = '%';
        return false;
    }
    else if (RunCnt == 1 && (ExtSmbl == '#'))
    {
        /* quit and set letter break flag, there's an obvious letter break on the 1st dit*/
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
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
    /*Middle keyup test to see this keyup is twice the length of the one following it,
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
            if ((TmpDwnIntrvls[n - 1] > DitDahSplitVal) &&
                (TmpDwnIntrvls[n] > DitDahSplitVal) &&
                (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
            { // we are are surrounded by dahs
                if ((TmpUpIntrvls[n - 1] >= DitDahSplitVal) && (TmpUpIntrvls[n] > 1.2 * TmpUpIntrvls[n - 1]))
                {
                    ExitPath[n] = 7;
                    BrkFlg = '+';
                    return true;
                }
                else if ((TmpUpIntrvls[n] >= 0.8 * DitDahSplitVal) &&
                         (TmpUpIntrvls[n - 1] >= 0.8 * DitDahSplitVal) &&
                         (TmpDwnIntrvls[n] >= TmpDwnIntrvls[n - 1]) &&
                         (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n - 1]))
                { // we're the middle of a dah serries; but this one looks stretched compared to its predecessor
                    ExitPath[n] = 8;
                    BrkFlg = '+';
                    return true;
                }
            }
        }
        /*test for the 1st of 2 adjacent 'dahs'*/
        if ((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
        {
            /*we have two adjcent dahs*/
            /* set letter break only if it meets the default sloppy bug letter break test*/
            // if ((TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n + 1]))
            if (TmpDwnIntrvls[n] + TmpUpIntrvls[n] >= 2.5 * this->UnitIntvrlx2r5)
            {
                ExitPath[n] = 9;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n + 1] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= 1.2 * TmpUpIntrvls[n + 1]))
            { // this Dah has lot longer Keyup than the next dah; so this looks like a letter break
                ExitPath[n] = 10;
                BrkFlg = '+';
                return true;
            }
            else if ((TmpUpIntrvls[n] >= DitDahSplitVal) &&
                     (TmpUpIntrvls[n] >= TmpUpIntrvls[n + 1]) &&
                     (TmpDwnIntrvls[n] >= 1.3 * TmpDwnIntrvls[n + 1]))
            { // this Dah is a lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
                ExitPath[n] = 11;
                BrkFlg = '+';
                return true;
            }
            // else if ((TmpUpIntrvls[n] >= 1.4 * DitDahSplitVal))
            else if (TmpDwnIntrvls[n] + TmpUpIntrvls[n] >= 2.5 * this->UnitIntvrlx2r5)
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
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
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
        else if ((TmpDwnIntrvls[n] >= DitDahSplitVal) && (TmpDwnIntrvls[n + 1] < DitDahSplitVal))
        {
            /*We have Dah to dit transition set letter break only if key up interval is > 1.6x the dit interval
            And the keyup time is more than 0.6 the dah interval //20240120 added this 2nd qualifier
            20240128 reduced 2nd qualifier to 0.4 & added 3rd 'OR' qualifier, this dah is the longest in the group*/
            /* 20240224 added 2nd qualifier back in for bg2 types that occacionally, due to strected dah, land in bg3 group */
            // if ((TmpUpIntrvls[n] > 1.6 * TmpDwnIntrvls[n + 1]) && ((TmpUpIntrvls[n] > 0.4 * TmpDwnIntrvls[n])
            //     || TmpDwnIntrvls[n] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl))

            if (((TmpDwnIntrvls[n] + TmpUpIntrvls[n]) >= 2.5 * this->UnitIntvrlx2r5) && (TmpUpIntrvls[n] > bstLtrBrk))
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
        else if ((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n + 1] > DitDahSplitVal))
        {
            /*We have Dit to Dah transition set letter break only if it meets the default sloppy bug letter break test*/
            // if ((TmpUpIntrvls[n] > 2.1 * DitIntrvlVal)) // 20240128 changed it to use generic dit; because bug dits should all be equal
            if (TmpDwnIntrvls[n] + TmpUpIntrvls[n] >= 1.6 * this->UnitIntvrlx2r5) // 2.5 * this->UnitIntvrlx2r5
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
bool AdvParser::Cooty2Rules(int &n)
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
bool AdvParser::CootyRules(int &n)
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
bool AdvParser::PadlRules(int &n)
{
    /*Paddle or Keyboard rules*/
    /*Middle keyup test to see this keyup is twice the length of the one just before it,
    If it is then call this one a letter break*/
    // if (n > 0 && (TmpUpIntrvls[n] > 2.0 * TmpUpIntrvls[n - 1]))
    if (n > 0 && (TmpUpIntrvls[n] > this->UnitIntvrlx2r5))
    {
        ExitPath[n] = 100;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the length of the one following it,
    If it is then call this one a letter break*/
    // if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n] > (2.0 * TmpUpIntrvls[n + 1])+8))
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n] > UnitIntvrlx2r5))
    {
        ExitPath[n] = 101;
        BrkFlg = '+';
        return true;
    }
    if (NewSpltVal)
    {
        // if (TmpUpIntrvls[n] >= DitDahSplitVal) // //AKA 'SplitPoint';
        if (TmpUpIntrvls[n] >= Bg1SplitPt)
        {
            BrkFlg = '+';
            ExitPath[n] = 102;
            return true;
        }
        else
            ExitPath[n] = 103;
        if (MaxCntKyUpBcktPtr < KeyUpBucktPtr) // safety check, before trying the real test
        {
            // if (TmpUpIntrvls[n] >= DitDahSplitVal)
            if (TmpUpIntrvls[n] >= UnitIntvrlx2r5)
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
        // if (TmpUpIntrvls[n] >= DitDahSplitVal)
        if (TmpUpIntrvls[n] >= UnitIntvrlx2r5)
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
/*Bug1 Key rule Set*/
bool AdvParser::Bug1Rules(int &n)
{
    bool ltrbrkFlg = false;
    if (TmpUpIntrvls[n] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl)
    {
        ExitPath[n] = 0;
        BrkFlg = '+';
        return true;
    }

    /*test for N dahs in a row, & set ltrbrk based on longest dah in the group*/
    int maxdahIndx = 0;
    int mindahIndx = 0;
    int RunCnt = 0; // used to validate that there were at least two adjacent dahs or dits
    uint16_t maxdah = 0;
    uint16_t mindah = KeyDwnBuckts[KeyDwnBucktPtr].Intrvl;
    char ExtSmbl = ' ';
    for (int i = n; i < TmpUpIntrvlsPtr; i++)
    {
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
                    if (this->Dbug)
                    {
                        printf("EXIT E\t");
                    }
                    ExtSmbl = '#';
                    break; // quit now this Dah looks significantly stretched compared to its predecessor
                }
                else if ((TmpUpIntrvls[i] > 1.4 * TmpDwnIntrvls[i]) ||
                         ((TmpUpIntrvls[i] >= UnitIntvrlx2r5) && ((float)TmpUpIntrvls[i] > 0.8 * (float)TmpDwnIntrvls[i]))) // 20240214-Replaced the following commented out line with this
                //(TmpUpIntrvls[i] >= KeyUpBuckts[KeyUpBucktPtr].Intrvl))
                {
                    if (this->Dbug)
                    {
                        printf("EXIT F\t");
                    }
                    ExtSmbl = '#'; //(exit code 2)
                    break;         // quit, Looks like a clear intention to signal a letterbreak
                }

                else if (TmpUpIntrvls[i] >= this->UnitIntvrlx2r5)
                {
                    if (this->Dbug)
                    {
                        printf("EXIT G %d; %d; %d\t", TmpUpIntrvls[i], i, RunCnt);
                    }

                    ExtSmbl = '#'; //(exit code 2)
                    break;         // quit, Looks like a clear intention to signal a letterbreak
                }
                else if ((TmpDwnIntrvls[i] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl) && (TmpUpIntrvls[i] > UnitIntvrlx2r5))
                {
                    if (this->Dbug)
                    {
                        printf("EXIT H\t");
                    }
                    ExtSmbl = '#'; //(exit code 2)
                    break;         // quit, Looks like a clear intention to signal a letterbreak
                }
            }
            else if ((TmpUpIntrvls[i] > 1.4 * TmpDwnIntrvls[i]) ||
                     ((TmpUpIntrvls[i] >= UnitIntvrlx2r5) && (TmpUpIntrvls[i] > TmpDwnIntrvls[i])))
            { // looks like a valid letter break, but 1st double check we don't have an even stronger letterbreak following this one
                if ((i < TmpUpIntrvlsPtr) &&
                    TmpUpIntrvls[i + 1] > (TmpUpIntrvls[i] &&
                                           TmpDwnIntrvls[i + 1] > this->Bg1SplitPt))
                {
                    // we do have a better one so skip this one
                    if (this->Dbug)
                    {
                        printf("EXIT E Aborted\t");
                    }
                }
                else
                {
                    if (this->Dbug)
                    {
                        printf("EXIT E\t");
                    }
                    ExtSmbl = '$'; //(exit code 23)
                    break;         // quit, Looks like a clear intention to signal a letterbreak
                }
            }
            /*Only consider the following "out", if we're not working with a collection of symbols that contains "Stretched" Dahs*/
            else if (TmpUpIntrvls[i] >= this->UnitIntvrlx2r5 && !this->StrchdDah)
            {
                if (this->Dbug)
                {
                    printf("EXIT F %d; %d; %d\t", TmpUpIntrvls[i], i, RunCnt);
                }
                ExtSmbl = '$'; //(exit code 23)
                break;         // quit, Looks like a clear intention to signal a letterbreak
            }
            else if ((TmpDwnIntrvls[i] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl) && (TmpUpIntrvls[i] > UnitIntvrlx2r5))
            {
                if (this->Dbug)
                {
                    printf("EXIT G\t");
                }
                ExtSmbl = '$'; //(exit code 23)
                break;         // quit, Looks like a clear intention to signal a letterbreak
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
    if (RunCnt > 1 && ExtSmbl == '#')
    {
        /*we have a run of dahs, so build/grow the SymbSet to match the found run of dahs; remember since the 1st dah has already been added,
        we can skip that one*/
        int STOP = n + (RunCnt - 1);
        while (n < STOP)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
        }
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
    }
    else if (RunCnt > 1 && (ExtSmbl == '@'))
    { // there was a run of dahs but no letterbreak terminator found; So just append what was found to the current symbolset & continue looking
        while (RunCnt > 1)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
            RunCnt--;
        }
        // printf("A RunCnt %d; n %d; SymbSet %d; ExtSmbl %c\t", RunCnt, n, SymbSet, ExtSmbl);
        ExitPath[n] = 4; // NOTE: there are two "ExitPath[n] = 4;"
        BrkFlg = '%';
        return false;
    }
    // else if (RunCnt > 1 && maxdah > 0 && (mindahIndx == maxdahIndx) && (TmpUpIntrvls[maxdahIndx] > TmpDwnIntrvls[maxdahIndx]))
    else if (RunCnt > 0 && ExtSmbl == '$' && n == maxdahIndx)
    { // the 1st dah seems to be a letter break
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
    for (int i = n; i < TmpUpIntrvlsPtr; i++)
    {
        // if ((TmpDwnIntrvls[i] + 8 > DitDahSplitVal))
        if (TmpDwnIntrvls[i] >= Bg1SplitPt)
        {
            ExtSmbl = '@';
            break; // quit, a dah was detected
        }
        else // we have a dit;
        {
            RunCnt++;
            maxdahIndx = i;

            // if ((TmpUpIntrvls[i] > UnitIntvrlx2r5) || (TmpUpIntrvls[i] > 2* this->AvgSmblDedSpc)){
            if ((TmpUpIntrvls[i] >= Bg1SplitPt) || (TmpUpIntrvls[i] > 2 * this->AvgSmblDedSpc))
            {
                // maxdahIndx = i;
                ExtSmbl = '#';
                break; // quit an there's an obvious letter break
            }
        }
    }
    /*Test that there was a run of dits AND it was bounded/terminated by a letterbreak*/
    if (RunCnt > 0 && maxdahIndx > 0 && (ExtSmbl == '#'))
    {

        /*we have a run of dits, so build/grow the SymbSet to match the found run of dits*/
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        }
        ExitPath[n] = 2;
        BrkFlg = '+';
        return true;
    }
    else if (RunCnt > 1 && (ExtSmbl == '@'))
    { // there was a run of dits but no letterbreak terminator found; So just append what was found to the current symbolset & continue looking
        while (n < maxdahIndx)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        }
        // printf("B RunCnt %d; n %d; ExtSmbl %c\t", RunCnt, n, ExtSmbl);
        ExitPath[n] = 4; // NOTE: there are two "ExitPath[n] = 4;"
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
    /*Middle keyup test to see this keyup is twice the length of the one following it,
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
                { // we're the middle of a dah serries; but this one looks stretched compared to its predecessor
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

/*Bug 2 rule set, differs from other bug rule sets, in that it doesn't look for long dahs to find letter breaks
And relies primarily on key up interval timing */
bool AdvParser::Bug2Rules(int &n)
{
    /*Bug2 rules*/
    bool ltrbrkFlg = false;
    // DitIntrvlVal:98	AvgDedSpc:48.6
    uint16_t Bg2LtrBrk = Bg1SplitPt;
    // if(AvgSmblDedSpc < 0.6*(float)DitIntrvlVal) Bg2LtrBrk = (uint16_t)2*AvgSmblDedSpc;
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
    for (int i = n; i < TmpUpIntrvlsPtr; i++)
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
            if (TmpUpIntrvls[i] > UnitIntvrlx2r5) // 20240301 (TmpUpIntrvls[i] > Bg1SplitPt)//20240222 (TmpUpIntrvls[i] >DitDahSplitVal)
            {                                     /*Looks like we have a letterbreak signal, but need to check further*/
                if (i + 1 <= TmpUpIntrvlsPtr)
                { // more key states follow this one
                    if (TmpDwnIntrvls[i + 1] > UnitIntvrlx2r5)
                    { /*the next keydown in this group is also a dah*/
                        if (TmpUpIntrvls[i + 1] < Bg1SplitPt)
                        { /*but its shorter, or has no letterbreak */
                            if (this->Dbug)
                                printf("EXIT A\t");
                            ExtSmbl = '$';
                            break; // quit, Looks like a clear intention to signal a letterbreak
                        }
                    }
                    else
                    {
                        if (this->Dbug)
                            printf("EXIT B\t");
                        ExtSmbl = '$';
                        break; // quit, Looks like a clear intention to signal a letterbreak
                    }
                }
                else
                { // no more key states to consider, so can exit with letterbreak
                    if (this->Dbug)
                        printf("EXIT C\t");
                    ExtSmbl = '$';
                    break; // quit, Looks like a clear intention to signal a letterbreak
                }
            }
        }
    }
    /*Test that the long dah is significantly longer than its sisters,
      & it is also terminated/followed by a reasonable keyup interval*/

    if (ExtSmbl != '@')
    {
        // if (RunCnt > 1 && maxdahIndx > 0 && (maxdah > 1.3 * mindah) && (TmpUpIntrvls[maxdahIndx] > (DitIntrvlVal + 8)))
        // {
        //     // if(maxdahIndx > 0 && (maxdah > 1.3*mindah) && (TmpUpIntrvls[maxdahIndx] > (0.8* DitDahSplitVal))){
        //     /*we have a run of dahs, so build/grow the SymbSet to mach the found run of dahs*/
        //     while (n < maxdahIndx)
        //     {
        //         n++;
        //         SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
        //         SymbSet += 1;
        //     }
        //     ExitPath[n] = 2; //note for this rule set there are two ExitPath[n] = 2's; this is the 1st one
        //     BrkFlg = '+';
        //     return true;
        // }
        // else if (RunCnt > 1 && maxdah > 0 && (mindahIndx == maxdahIndx) && (TmpUpIntrvls[maxdahIndx] > TmpDwnIntrvls[maxdahIndx]))
        // { // the 1st dah seems to be a letter break
        //     ExitPath[n] = 21;
        //     BrkFlg = '+';
        //     return true;
        // }
        if (RunCnt > 1 && ExtSmbl == '$')
        {
            // printf("ExtSmbl: %c; RunCnt: %d\n", ExtSmbl, RunCnt);
            /*we have a run of dahs terminated with a letter break, so build/grow the SymbSet to mach the found run of dahs*/
            while (RunCnt > 1)
            {
                n++;
                SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
                SymbSet += 1;
                RunCnt--;
            }
            ExitPath[n] = 2; // note for this rule set there are two ExitPath[n] = 2's; this is the 2nd one
            BrkFlg = '+';
            return true;
        }
    }
    else if (RunCnt > 1)
    {
        while (RunCnt > 1)
        {
            n++;
            SymbSet = SymbSet << 1; // append a new bit to the symbolset & default it to a 'Dit'
            SymbSet += 1;
            RunCnt--;
        }
        // printf("A RunCnt %d; n %d; SymbSet %d; ExtSmbl %c\t", RunCnt, n, SymbSet, ExtSmbl);
        ExitPath[n] = 4; // NOTE: there are two "ExitPath[n] = 4;"
        BrkFlg = '%';
        return false;
    }
    /*Now do a similar test/logic for run of 'n' dits in a row (to help better manage 'es' and the like parsing)*/
    /* set ltrbrk based on longest keyup interval in the group*/
    maxdahIndx = 0; // reuse this variable from Dah series test
    mindahIndx = 0; // reuse this variable from Dah series test
    maxdah = 0;     // reuse this variable from Dah series test
    RunCnt = 0;     // used to validate that there were at least two adjacent dahs or dits
    mindah = KeyUpBuckts[KeyDwnBucktPtr].Intrvl;
    for (int i = n; i < TmpUpIntrvlsPtr; i++)
    {
        if ((TmpDwnIntrvls[i] >= DitDahSplitVal))
        {
            ExtSmbl = '@';
            break; // quit when a dah is detected or an obvious letter break
        }
        else // we have a dit;
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
                // if ((TmpUpIntrvls[i] > 1.5 * TmpUpIntrvls[i - 1]))
                if ((TmpUpIntrvls[i] >= DitDahSplitVal)) // >= Bg1SplitPt
                {
                    ExtSmbl = '#';
                    break; // quit when an obvious letter break is detected
                }
            }
            if ((TmpUpIntrvls[i] > DitDahSplitVal))
            {
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
    // if (RunCnt > 1 && maxdahIndx > 0 && (TmpUpIntrvls[maxdahIndx] > (1.5 * TmpDwnIntrvls[maxdahIndx])))
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
    }
    else if (RunCnt == 1 && (TmpUpIntrvls[n] > DitDahSplitVal))
    {
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
    /*Middle keyup test to see this keyup is twice the length of the one following it,
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
                { // we're the middle of a dah serries; but this one looks stretched compared to its predecessor
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
            // if ((TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > 5 + TmpDwnIntrvls[n + 1]))
            /*we have two adjcent dahs, but for bug2 use simple letterbreak test */
            if (TmpUpIntrvls[n] > UnitIntvrlx2r5)
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
            // else if (((float)TmpUpIntrvls[n] >= (float)0.85*this->UnitIntvrlx2r5))//20240222 changed to UnitIntvrlx2r5 ///Bg1SplitPt 20240211 going to this to avoid adjacent letters ending with a dah & starting with dah to get treated as a single symblset
            // { // this Dah's keyup is lot longer than the next dah & it also has a longer Keyup time; so this looks like a letter break
            //     ExitPath[n] = 11;
            //     BrkFlg = '+';
            //     return true;
            // }
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

            20240128 reduced 2nd qualifier to 0.4 & added 3rd 'OR' qualifier, this dah is the longest in the group*/
            /*20240128 deleted or qualifier since bg2 rule set no longer uses streched dahs as a letter break marker */
            // if ((TmpUpIntrvls[n] > Bg2LtrBrk) || (TmpDwnIntrvls[n] >= KeyDwnBuckts[KeyDwnBucktPtr].Intrvl))
            if ((TmpUpIntrvls[n] > Bg2LtrBrk))
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
bool AdvParser::SKRules(int &n)
{
    /*Straight key Rules - Originally model form Paddle or Keyboard rules*/
    /*Middle keyup test to see this keyup is greater than 'UnitIntvrlx2r5',
    If it is then call this one a letter break*/
    // if (n > 0 && (TmpUpIntrvls[n] > 2.0 * TmpUpIntrvls[n - 1]))
    if (n > 0 && (TmpUpIntrvls[n] > UnitIntvrlx2r5))
    {
        ExitPath[n] = 100;
        BrkFlg = '+';
        return true;
    }
    /*Middle keyup test to see this keyup is twice the length of the one following it,
    If it is then call this one a letter break*/
    if ((n < TmpUpIntrvlsPtr - 1) && (TmpUpIntrvls[n] > (2.0 * TmpUpIntrvls[n + 1]) + 8))
    {
        ExitPath[n] = 101;
        BrkFlg = '+';
        return true;
    }
    if (NewSpltVal)
    {
        if (TmpUpIntrvls[n] >= UnitIntvrlx2r5)
        {
            BrkFlg = '+';
            ExitPath[n] = 102;
            return true;
        }
        else
            ExitPath[n] = 103;

        // if (MaxCntKyUpBcktPtr < KeyUpBucktPtr) // safety check, before trying the real test
        // {
        //     // if (TmpUpIntrvls[n] >= KeyUpBuckts[MaxCntKyUpBcktPtr + 1].Intrvl)
        //     if (TmpUpIntrvls[n] >= DitDahSplitVal)
        //     {
        //         BrkFlg = '+';
        //         ExitPath[n] = 104;
        //         return true;
        //     }
        // }
        // else
        //     ExitPath[n] = 105;
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
    // char TmpBufA[MsgbufSize - 5];
    for (int i = 0; i < sizeof(this->TmpBufA); i++)
    {
        this->TmpBufA[i] = this->Msgbuf[i];
        if (this->Msgbuf[i] == 0)
            break;
    }
    int pos1 = linearSearchBreak(decodeval, CodeVal1, ARSIZE); // note: decodeval '255' returns SPACE character

    if (pos1 < 0 && DpScan)
    { // did not find a match in the standard Morse table. So go check the extended dictionary
        pos1 = linearSearchBreak(decodeval, CodeVal2, ARSIZE2);
        if (pos1 < 0)
        { /*Still no match. Go back and sub divide this group timing intervals into two smaller sets to tese out embedded text codes*/
            // printf("  SrchAgn  ");
            // if(!this->SrchAgn(n)) printf("  SEARCHED FAILED  ");
            this->SrchAgn(n);
        }
        else
        {

            sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl2[pos1]);
        }
    }
    else
        sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]); // sprintf( Msgbuf, "%s%s", Msgbuf, DicTbl1[pos1] );
    return pos1;
};
//////////////////////////////////////////////////////////////////////
/*Returns 'true'*/
/*Uses user supplied pointer 'n' & class property 'LstLtrBrkCnt' as boundry points to
to do a recursive search for Codevals embedded in the key down/up arrays, & append as text to the class
property 'MsgBuf'. Returns with Success flag set to 'true', if scan seem to work*/
bool AdvParser::SrchAgn(int n)
{
    /*Build new symbsets & try to decode them*/
    /*find the longest keyup time in this interval group*/
    int NuLtrBrk = 0;
    int pos1;
    bool SrchWrkd = true;
    unsigned int Symbl1;
    int Start = n - (this->LstLtrBrkCnt);
    /*Look for most obvious letter break */
    NuLtrBrk = this->FindLtrBrk(Start, n);
    /** Build 1st symbol set based on start of the the original group & the found longest interval*/
    Symbl1 = this->BldCodeVal(Start, NuLtrBrk);

    /*Now find character matches for this new symbol set,
     and append their results to the message buffer*/
    pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
    if (pos1 >= 0)
    {
        // printf("A");
        sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]);
        /*make another copy of the current message buffer */
        this->SyncTmpBufA();
        // for (int i = 0; i < sizeof(this->TmpBufA); i++)
        // {
        //     this->TmpBufA[i] = this->Msgbuf[i];
        //     if (this->Msgbuf[i] == 0)
        //         break;
        // }
    }
    else
    {
        /*1st remember where the previous letter brk */
        int NuLtrBrk1 = NuLtrBrk;
        /*Look for most obvious letter break */
        NuLtrBrk1 = this->FindLtrBrk(Start, NuLtrBrk1);
        /** Build 1st symbol set based on start of the the original group & the found longest interval*/
        Symbl1 = this->BldCodeVal(Start, NuLtrBrk1);
        /*Now find character match for this new symbol set,
         and append the results to the message buffer*/
        pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
        if (pos1 >= 0)
        {
            // printf("B: %s; %d; %d", DicTbl1[pos1], NuLtrBrk1+1, NuLtrBrk );
            /*append the results to the message buffer*/
            sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]);
            /*make another copy of the current message buffer */
            this->SyncTmpBufA();
            /** Build 1st symbol set based on start of the the original group & the found longest interval*/
            Symbl1 = this->BldCodeVal(NuLtrBrk1 + 1, NuLtrBrk);
            /*Now find character match for this symbol set,
            and append their results to the message buffer*/
            pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
            if (pos1 >= 0)
            {
                // printf(" C: %s ", DicTbl1[pos1]);
                /*append the results to the message buffer*/
                sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]);
                /*make another copy of the current message buffer */
                this->SyncTmpBufA();
            }
            else
                SrchWrkd = false;
        }
        else
            SrchWrkd = false;
    }
    /*Finished 1st half of the original split; Now try to resolve parse the 2nd half*/
    Start = NuLtrBrk + 1;
    Symbl1 = this->BldCodeVal(Start, n);
    pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
    if (pos1 >= 0)
    {
        // printf("D: %s;", DicTbl1[pos1]);
        sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]);
        /*make another copy of the current message buffer */
        this->SyncTmpBufA(); // not really needed here, because your done with this symbol set
    }
    else
    {
        /*1st remember where the previous letter brk */
        int NuLtrBrk1 = NuLtrBrk;
        /*Look for the next most obvious letter break */
        NuLtrBrk1 = this->FindLtrBrk(Start, NuLtrBrk1);
        /** Build 1st symbol set based on start of the the original group & the found longest interval*/
        Symbl1 = this->BldCodeVal(Start, NuLtrBrk1);
        /*Now find character matches for this new symbol set,
          and append their results to the message buffer*/
        pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
        if (pos1 >= 0)
        {
            // printf(" E: %s; %d; %d", DicTbl1[pos1], NuLtrBrk1+1, n );
            sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]);
            /*make another copy of the current message buffer */
            this->SyncTmpBufA();
            /** Build 1st symbol set based on start of the the original group & the found longest interval*/
            Symbl1 = this->BldCodeVal(NuLtrBrk1 + 1, n);
            /*Now find character matches for this new symbol set,
            and append their results to the message buffer*/
            pos1 = linearSearchBreak(Symbl1, CodeVal1, ARSIZE);
            if (pos1 >= 0)
            {
                // printf(" F: %s; ", DicTbl1[pos1]);
                sprintf(this->Msgbuf, "%s%s", this->TmpBufA, DicTbl1[pos1]);
            }
            else
                SrchWrkd = false;
        }
        else
        {
            SrchWrkd = false;
        }
    }
    return SrchWrkd;
};
///////////////////////////////////////////////////////////////////////
/*Build New CodeVal Based on current KeyDown Array & Using the supplied Start & LtrBrk pointers*/
int AdvParser::BldCodeVal(int Start, int LtrBrk)
{
    /** Build 1st symbol set based on start of the the original group & the found longest interval*/
    int CodeVal = 1;
    for (int i = Start; i <= LtrBrk; i++)
    {
        CodeVal = CodeVal << 1;                    // append a new bit to the symbolset & default it to a 'Dit'
        if (TmpDwnIntrvls[i] + 8 > DitDahSplitVal) // if within *ms of the split value, its a 'dah'
            CodeVal += 1;
    }
    // printf("!: CodeVal %d; %d; %d ", CodeVal, Start, LtrBrk);
    return CodeVal;
};
///////////////////////////////////////////////////////////////////////
/*using the Start & End pointer values, return the index pointer with the longest keyup interval  */
int AdvParser::FindLtrBrk(int Start, int End)
{
    int NuLtrBrk = Start;
    uint16_t LongestKeyUptime = 0;
    for (int i = Start; i < End; i++) // stop 1 short of the original letter break
    {
        if (TmpUpIntrvls[i] > LongestKeyUptime)
        {
            NuLtrBrk = i;
            LongestKeyUptime = TmpUpIntrvls[i];
        }
    }
    return NuLtrBrk;
};
///////////////////////////////////////////////////////////////////////
void AdvParser::SyncTmpBufA(void)
{
    for (int i = 0; i < sizeof(this->TmpBufA); i++)
    {
        this->TmpBufA[i] = this->Msgbuf[i];
        if (this->Msgbuf[i] == 0)
            break;
    }
};
///////////////////////////////////////////////////////////////////////
/*This function finds the Msgbuf current length regardless of Dbug's state */
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
    this->LstLtrPrntd = 0;
    while (Msgbuf[this->LstLtrPrntd] != 0)
    {
        this->LstLtrPrntd++;
    }
    // this->LstLtrPrntd--;
    return this->LstLtrPrntd;
};
//////////////////////////////////////////////////////////////////////////////
/* Key or 'Fist' style test
 * looks for electronic key by finding constant dah intervals.
 * Bug test by noting the keyup interval is consistantly shorter between the dits vs dahs
 * returns 0, 1, or 5 for paddle; 2,3,& 4 for bug; 6 for unknown; 7 for straight Key; 10 for sloppy bug.
 */
int AdvParser::DitDahBugTst(void)
{
    int ditcnt;
    int dahDwncnt;
    int Longdahcnt;
    int dahcnt = ditcnt = dahDwncnt = Longdahcnt = 0;
    uint16_t dahInterval;
    uint16_t MindahInterval;
    uint16_t DahVariance = 0;
    uint16_t MaxdahInterval;
    uint16_t dahDwnInterval;
    uint16_t ditDwnInterval;
    uint16_t ditDwncnt;
    uint16_t ditInterval = dahInterval = dahDwnInterval = ditDwnInterval = ditDwncnt = MaxdahInterval = 0;
    MindahInterval = 1000;
    int stop = TmpUpIntrvlsPtr - 1;
    bool same;
    this->DahVarPrcnt = 0.0;
    this->StrchdDah = false;
    for (int n = 0; n < stop; n++)
    {
        /*Made the define the average dah, less restrictive; because paddle generated dahs should all have the same interval */
        /*ALSO do NOT consider the 1st entry in the symbol set; its interval value may be truncated*/
        if (n > 0 && (TmpDwnIntrvls[n] >= this->DitDahSplitVal))
        {
            dahDwnInterval += TmpDwnIntrvls[n];
            dahDwncnt++;
            if (TmpDwnIntrvls[n] > UnitIntvrlx2r5)
                Longdahcnt++;
            if (TmpDwnIntrvls[n] > MaxdahInterval)
                MaxdahInterval = TmpDwnIntrvls[n];
            if (TmpDwnIntrvls[n] < MindahInterval)
                MindahInterval = TmpDwnIntrvls[n];
        }
        else if (n > 0 && (TmpDwnIntrvls[n] < this->DitDahSplitVal))
        {
            ditDwnInterval += TmpDwnIntrvls[n];
            ditDwncnt++;
        }

        /*Sum the KeyUp numbers for any non letter-break terminated dahs*/
        if ((TmpDwnIntrvls[n] > DitDahSplitVal) &&
            (TmpUpIntrvls[n] + 8 < Bg1SplitPt))
        /*Commented the following out based on results using Paddle_NoWrdbrksending202410.mp3*/
        // if ((TmpDwnIntrvls[n] > DitDahSplitVal) &&
        //     (TmpUpIntrvls[n] < UnitIntvrlx2r5))
        {
            dahInterval += TmpUpIntrvls[n];
            dahcnt++;
            // printf("%d\n", n);
        }
        /*test adjcent dits KeyUp timing. But only include if there's not a letter break between them */
        else if ((TmpDwnIntrvls[n] < this->DitDahSplitVal) &&
                 (TmpDwnIntrvls[n + 1] < this->DitDahSplitVal) &&
                 (TmpUpIntrvls[n] < this->DitDahSplitVal))
        {
            ditInterval += TmpUpIntrvls[n];
            ditcnt++;
            // printf("\t%d\n", n);
        }
    }
    if (dahDwncnt > 1)
    {
        // printf("\tMindahInterval: %d\tMaxdahInterval: %d;\tDitDahSplitVal %d\n", MindahInterval, MaxdahInterval, this->DitDahSplitVal);
        DahVariance = MaxdahInterval - MindahInterval;
        this->DahVarPrcnt = (float)DahVariance / (float)MindahInterval; // used later to determine if sender is using strected dahs as a way of signaling letter breaks
    }
    // if(Longdahcnt > 0) this->StrchdDah = true;

    if (this->DahVarPrcnt > 0.8)
        this->StrchdDah = true;
    if (Dbug)
    {
        if (this->StrchdDah)
            printf("StrchdDah = true\n");
        else
            printf("StrchdDah = false\n");
    }

    if (ditDwncnt == 0 || dahDwncnt == 0) // we have all dits or all dahs -
    {
        if (Dbug)
            printf("code 99; ditDwncnt: %d; dahDwncnt: %d\n", ditDwncnt, dahDwncnt);
        return 6; // unknown (99) - Stick with whatever bug type was in play w/ last symbol set
    }
    /*20240309 Moved straight key test ahead of sloppy test; used to be after it & found long stright key runs
    were getting 'typed' as sloppy bugs & the wrong rule set would be applied */
    /*Test/check for Straight key, by dits with varying intervals*/
    if (ditDwncnt >= 4)
    {
        /* average/normalize dit interval */
        ditDwnInterval /= ditDwncnt;
        int Delta = (int)((0.15 * (float)ditDwnInterval)); // 0.1*
        int NegDelta = -1 * Delta;
        // same = true;
        ditDwncnt = 0;
        int GudDitCnt = 0;                               // could use this as part of a ratio test
        for (int n = 1; n <= this->TmpUpIntrvlsPtr; n++) // skip the 1st key down event because testing showed the timing of the 1st event is often shorter than the rest in the group
        {
            if (TmpDwnIntrvls[n] < DitDahSplitVal)
            {
                ditDwncnt++;
                int error = TmpDwnIntrvls[n] - ditDwnInterval; // at this point, 'ditDwnInterval' is the average dit interval for this word group
                if ((error < Delta) && (error > NegDelta))
                    GudDitCnt++;
                // else
                //     same = false;
            }
        }
        float Score = (float)GudDitCnt / (float)ditDwncnt;
        // printf("ditDwncnt: %d; \tGudDitCnt: %d; ditDwnInterval: %d; Delta: %d Score: %0.1f\n", ditDwncnt, GudDitCnt, ditDwnInterval, Delta, Score);
        if (Score <= 0.70)
        {
            return 7; // straight key; (50)
        }
        /* 1st, test for Sloppy Bug */
        float Dit2SpltRatio = (float)this->NuSpltVal / this->DitIntrvlVal;
        if (Dbug)
        {
            printf("\ndahDwncnt: %d\tDahVariance: %d\tDahVarPrcnt: %4.2f\n", dahDwncnt, DahVariance, this->DahVarPrcnt);
            printf("Sloppy: %4.2f\n", Dit2SpltRatio);
        }

        if (Dit2SpltRatio < 2)
        {
            char DbgTxt[16] = {"NOT Sloppy"};
            // if (Dbug) sprintf(DbgTxt, "NOT Sloppy");
            int RetrnCD = 0;
            if ((this->KeyDwnBucktPtr >= 4 && this->KeyUpBucktPtr >= 3 && this->StrchdDah)) // 20240222 added this->StrchdDah qualifier
            {
                // its a Sloppy Bug
                if (Dbug)
                    sprintf(DbgTxt, "SLOPPY EXIT A");
                RetrnCD = 10; // Sloppy Bug (40)
            }
            else if (this->DahVarPrcnt > 0.60) // this->DahVarPrcnt > 0.90
            {
                // its a Sloppy Bug
                if (Dbug)
                    sprintf(DbgTxt, "SLOPPY EXIT B");
                RetrnCD = 10; // Sloppy Bug (40)
            }
            if (Dbug)
                printf("\n%s\n", DbgTxt);
            if (RetrnCD != 0)
                return RetrnCD;
        }
        /*  A very simple test for paddle keyboard sent code */
        if ((dahDwncnt > 1) && (this->DahVarPrcnt < 0.10))
        {
            // its a paddle or keyboard
            return 0; // paddle/krybrd (70)
        }

        // /*Test/check for Straight key, by dits with varying intervals*/
        // if (ditDwncnt >= 4)
        // {
        //     /* average/normalize dit interval */
        //     ditDwnInterval /= ditDwncnt;
        //     int Delta = (int)((0.15 * (float)ditDwnInterval)); // 0.1*
        //     int NegDelta = -1 * Delta;
        //     // same = true;
        //     ditDwncnt = 0;
        //     int GudDitCnt = 0;                               // could use this as part of a ratio test
        //     for (int n = 1; n <= this->TmpUpIntrvlsPtr; n++) // skip the 1st key down event because testing showed the timing of the 1st event is often shorter than the rest in the group
        //     {
        //         if (TmpDwnIntrvls[n] < DitDahSplitVal)
        //         {
        //             ditDwncnt++;
        //             int error = TmpDwnIntrvls[n] - ditDwnInterval; // at this point, 'ditDwnInterval' is the average dit interval for this word group
        //             if ((error < Delta) && (error > NegDelta))
        //                 GudDitCnt++;
        //             // else
        //             //     same = false;
        //         }
        //     }
        //     float Score = (float)GudDitCnt / (float)ditDwncnt;
        //     printf("ditDwncnt: %d; \tGudDitCnt: %d; ditDwnInterval: %d; Delta: %d Score: %0.1f\n", ditDwncnt, GudDitCnt, ditDwnInterval, Delta, Score);
        //     if (Score <= 0.70)
        //     {
        //         return 7; // straight key; (50)
        //     }
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
                /* average/normalize the Dit & Dah keyup invervals results */
                dahInterval /= dahcnt;
                ditInterval /= ditcnt;
                if (Dbug)
                {
                    printf("\nditcnt:%d; ditInterval: %d\ndahcnt:%d; dahInterval: %d\n", ditcnt, ditInterval, dahcnt, dahInterval);
                }
                /*If both keyup intervals are essentially the same, then its likely electronic keying
                while bug generated code almost allways has longer dah keyup intervals*/
                if ((dahInterval > ditInterval + 9) || (DahVariance > 10))
                    if (Longdahcnt > 0 && this->DahVarPrcnt > 0.08) // DahVarPrcnt > 0.25  // found stretched dahs; need to use bug1 ruleset
                        return 9;                                   // bug (83)
                    else
                        return 2; // bug (80)
                else
                    return 0; // paddle/krybrd (70)
            }
            else
                return 4; // not enough info to decide
        }
        else if (Longdahcnt > 0 && this->DahVarPrcnt > 0.25) // found stretched dahs; need to use bug1 ruleset
            return 8;                                        // bug (82)
        else if (((float)GudDahCnt / (float)dahDwncnt > 0.8))
            return 5; // paddle/krybrd (72)
        else if (dahDwncnt > 1)
        {
            // printf("\nGudDahCnt:%d; dahDwncnt:%d\n",GudDahCnt, dahDwncnt);
            return 3; // bug
        }
    }
    else
    {
        if (Dbug)
            printf("&!!*\n");
        return 6; // bug(99)// not enough info to decide
    }
    return 11; // not enough info to decide
    // printf("\nditcnt:%d; dahcnt:%d; interval cnt: %d\n", ditcnt, dahcnt, stop);
};
/////////////////////////////////////////////
void AdvParser::Dcode4Dahs(int n)
{
    int NuLtrBrk = 0;
    uint16_t LongestKeyUptime = 0;
    unsigned int Symbl1, Symbl2;
    /*find the longest keyup time in the preceeding 3 intervals*/
    for (int i = n - 3; i < n; i++)
    {
        if (TmpUpIntrvls[i] > LongestKeyUptime)
        {
            NuLtrBrk = i - (n - 3);
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
//////////////////////////////////////////////
/* A final check to look for, & correct classic parsing errors*/
void AdvParser::FixClassicErrors(void)
{                                             // No longer need to worry about if we have enough decoded characters evaluate the following sloppy strings this->Msgbuf now has enough data, to test for special character combos often found with sloppy sending
    int lstCharPos = (this->LstLtrPrntd) - 1; // sizeof(this->Msgbuf) - 2;
    char SrchTerm[10];
    char RplaceTerm[10];
    // printf("MemAddr %#08X; this->Msgbuf: %s \n", (int)&this->Msgbuf, this->Msgbuf);
    int NdxPtr = 0;
    //printf("LstLtrPrntd: %d; Msgbuf: %s \n", this->LstLtrPrntd, this->Msgbuf);
    for (NdxPtr = 0; NdxPtr < this->LstLtrPrntd - 1; NdxPtr++)
    {
        if (NdxPtr + 1 < this->LstLtrPrntd)
        { // i.e. this search term group has a maxium 2 characters
            /*Look for embedded character sequence 'PD', if found replace with 'AND'*/
            sprintf(SrchTerm, "PD");
            sprintf(RplaceTerm, "AND");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'S2', if found replace with 'SUM'*/
            sprintf(SrchTerm, "S2");
            sprintf(RplaceTerm, "SUM");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);
        }
        if (NdxPtr + 2 < this->LstLtrPrntd) // i.e. search term has 3 characters
        {
            sprintf(SrchTerm, "TB3");
            sprintf(RplaceTerm, "73");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'SO9', if found replace with 'SOON'*/
            sprintf(SrchTerm, "SO9");
            sprintf(RplaceTerm, "SOON");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'MPY', if found replace with 'MANY'*/
            sprintf(SrchTerm, "MPY");
            sprintf(RplaceTerm, "MANY");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'SI6', if found replace with 'SIDE'*/
            sprintf(SrchTerm, "SI6");
            sprintf(RplaceTerm, "SIDE");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*if this is not part of a "CQ",
            look for embedded character sequence 'QDE', if found replace with 'MADE'*/
            if (NdxPtr == 0 || (NdxPtr > 0 && this->Msgbuf[NdxPtr - 1] != 'C'))
            {
                sprintf(SrchTerm, "QDE");
                sprintf(RplaceTerm, "MADE");
                NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);
            }

            /*Look for embedded character sequence 'LWE', if found replace with 'LATE'*/
            sprintf(SrchTerm, "LWE");
            sprintf(RplaceTerm, "LATE");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'THW', if found replace with 'THAT'*/
            sprintf(SrchTerm, "THW");
            sprintf(RplaceTerm, "THAT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'THP', if found replace with 'THAN'*/
            sprintf(SrchTerm, "THP");
            sprintf(RplaceTerm, "THAN");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'PLL', if found replace with 'WELL'*/
            sprintf(SrchTerm, "PLL");
            sprintf(RplaceTerm, "WELL");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'SJE', if found replace with 'SAME'*/
            sprintf(SrchTerm, "SJE");
            sprintf(RplaceTerm, "SAME");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'CPT', if found replace with 'CANT'*/
            sprintf(SrchTerm, "CPT");
            sprintf(RplaceTerm, "CANT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence '0VE', if found replace with 'MOVE'*/
            sprintf(SrchTerm, "0VE");
            sprintf(RplaceTerm, "MOVE");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'RLN', if found replace with 'RAIN'*/
            sprintf(SrchTerm, "RLN");
            sprintf(RplaceTerm, "RAIN");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'D9T', if found replace with 'DONT'*/
            sprintf(SrchTerm, "D9T");
            sprintf(RplaceTerm, "DONT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'TNN', if found replace with 'GN'*/
            sprintf(SrchTerm, "TNN");
            sprintf(RplaceTerm, "GN");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'SOG', if found replace with 'SOME'*/
            sprintf(SrchTerm, "SOG");
            sprintf(RplaceTerm, "SOME");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'D9T', if found replace with 'DONT'*/
            sprintf(SrchTerm, "D9T");
            sprintf(RplaceTerm, "DONT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'CHW', if found replace with 'CHAT'*/
            sprintf(SrchTerm, "CHW");
            sprintf(RplaceTerm, "CHAT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'WPT', if found replace with 'WANT'*/
            sprintf(SrchTerm, "WPT");
            sprintf(RplaceTerm, "WANT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'PNT', if found replace with 'WENT'*/
            sprintf(SrchTerm, "PNT");
            sprintf(RplaceTerm, "WENT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'PEK', if found replace with 'WEEK'*/
            sprintf(SrchTerm, "PEK");
            sprintf(RplaceTerm, "WEEK");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'THJ', if found replace with 'THAT'*/
            sprintf(SrchTerm, "THJ");
            sprintf(RplaceTerm, "THAT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);
        }
        if (NdxPtr + 3 < this->LstLtrPrntd)
        { // this search term group has a maxium of 4 characters
            /*Look for embedded character sequence 'WXST', if found replace with 'JUST'*/
            sprintf(SrchTerm, "WXST");
            sprintf(RplaceTerm, "JUST");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'MMMK', if found replace with 'OOK'*/
            sprintf(SrchTerm, "MMMK");
            sprintf(RplaceTerm, "OOK");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'GMTT', if found replace with 'GOT'*/
            sprintf(SrchTerm, "GMTT");
            sprintf(RplaceTerm, "GOT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'TTTN', if found replace with 'ON'*/
            sprintf(SrchTerm, "TTTN");
            sprintf(RplaceTerm, "ON");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'WEUT', if found replace with 'PUT'*/
            sprintf(SrchTerm, "WEUT");
            sprintf(RplaceTerm, "PUT");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);

            /*Look for embedded character sequence 'TBVT', if found replace with '73'*/
            sprintf(SrchTerm, "TBVT");
            sprintf(RplaceTerm, "73");
            NdxPtr = this->SrchEsReplace(NdxPtr, SrchTerm, RplaceTerm);
        }
        //printf("NdxPtr: %d Msgbuf: %s \n", NdxPtr, this->Msgbuf);
    }
    // Msgbufaddress = this->Msgbuf;
    // printf("MemAddr %#08X; this->Msgbuf: %s \n",  (int)&this->Msgbuf, this->Msgbuf);
    //  printf("%c%c%c\n", this->Msgbuf[lstCharPos - 2],  this->Msgbuf[lstCharPos - 1], this->Msgbuf[lstCharPos] );// for debugging sloppy strings only
    if (this->Msgbuf[lstCharPos - 1] == '@' && this->Msgbuf[lstCharPos] == 'D')
    {
        sprintf(Msgbuf, " (%c%s", this->Msgbuf[lstCharPos - 2], "AC)"); // test for "@" (%c%s", this->Msgbuf[lstCharPos - 2], "AC)"); //"true"; Insert preceeding character plus correction "AC"
    }
    if (lstCharPos >= 4)
    {
        if (this->Msgbuf[lstCharPos - 4] == '5' && this->Msgbuf[lstCharPos - 3] == 'O' && this->Msgbuf[lstCharPos - 2] == 'N' && this->Msgbuf[lstCharPos - 1] == 'O' && this->Msgbuf[lstCharPos] == 'N')
        {
            sprintf(Msgbuf, "5NN");
        }
    }
    //  if (this->Msgbuf[lstCharPos - 1] == 'P' && this->Msgbuf[lstCharPos] == 'D')
    //  {
    //     // test for "PD"
    //      sprintf(Msgbuf, " (%c%s", this->Msgbuf[lstCharPos - 2], "AND)"); //"true"; Insert preceeding character plus correction "AND"
    //  }

    if (this->Msgbuf[lstCharPos - 1] == '6' && this->Msgbuf[lstCharPos] == 'E')
    {                                                                    // test for "6E"
        sprintf(Msgbuf, " (%c%s", this->Msgbuf[lstCharPos - 2], "THE)"); //"true"; Insert preceeding character plus correction "THE"
    }
    if (this->Msgbuf[lstCharPos - 1] == '6' && this->Msgbuf[lstCharPos] == 'A')
    {                                                                    // test for "6A"
        sprintf(Msgbuf, " (%c%s", this->Msgbuf[lstCharPos - 2], "THA)"); //"true"; Insert preceeding character plus correction "THA"
    }
    //  if (this->Msgbuf[lstCharPos - 1] == '9' && this->Msgbuf[lstCharPos] == 'E')
    //  {                              // test for "9E"
    //      sprintf(Msgbuf, " (ONE)"); //"true"; Insert correction "ONE"
    //  }
    //  if (this->Msgbuf[lstCharPos - 2] == 'P' && this->Msgbuf[lstCharPos - 1] == 'L' && this->Msgbuf[lstCharPos] == 'L')
    //  {                               // test for "PLL"
    //      sprintf(Msgbuf, " (WELL)"); //"true"; Insert correction "WELL"
    //  }
    /*20240228 took this out; otherwise it gets corrected twice*/
    //  if ((this->Msgbuf[lstCharPos - 2] == 'N' || this->Msgbuf[lstCharPos - 2] == 'L') && this->Msgbuf[lstCharPos - 1] == 'M' && this->Msgbuf[lstCharPos] == 'Y')
    //  {                                                                   // test for "NMY/LMY"
    //      sprintf(Msgbuf, " (%c%s", this->Msgbuf[lstCharPos - 2], "OW)"); //"true"; Insert correction "NOW"/"LOW"
    //  }
    //  if (this->Msgbuf[lstCharPos - 2] == 'T' && this->Msgbuf[lstCharPos - 1] == 'T' && this->Msgbuf[lstCharPos] == 'O')
    //  {                             // test for "TTO"
    //      sprintf(Msgbuf, "  (0)"); //"true"; Insert correction "TTO" = "0"
    //  }
};

/*A text search & replace routine. That examines the current contents of the Msgbuf
 starting at the MsgBufIndx pointer, and tests for a match to the srchTerm
 & if found, replaces the srchTerm sequence with sequence contained in NuTerm */
int AdvParser::SrchEsReplace(int MsgBufIndx, char srchTerm[10], char NuTerm[10])
{
    bool match = true;
    int i = 0;
    int RplcLtrCnt = 0;
    // printf("%s\n", srchTerm);
    while (srchTerm[i] != 0)
    {
        if (srchTerm[i] != this->Msgbuf[MsgBufIndx + i])
        {
            match = false;
            break;
        }
        i++;
    }
    if (!match)
        return MsgBufIndx; // No match found

    /*if here, we have a match & now need to replace with NuTerm letter sequence*/
    /*But 1st, copy everything past the search term, in the Msgbuf, to a 2nd temp buffer */
    int j;
    // i++;
    for (j = 0; j < sizeof(this->TmpBufA) - 1; j++)
    {
        this->TmpBufA[j] = this->Msgbuf[MsgBufIndx + i + j];
        this->TmpBufA[j + 1] = 0; // just doing this to be extra cautious
        if (this->Msgbuf[MsgBufIndx + j + i] == 0)
            break;
    }
    // printf("TmpBufA: %s \n", TmpBufA);
    /*now starting at MsgBufIndx pointer append the new character sequence to the Msgbuf;*/
    while (NuTerm[RplcLtrCnt] != 0)
    {
        this->Msgbuf[MsgBufIndx + RplcLtrCnt] = NuTerm[RplcLtrCnt];
        RplcLtrCnt++;
    }
    /*finish off by adding back/appending the contents fo the temp buffer (TmpBufA) */
    j = 0;
    while (this->TmpBufA[j] != 0)
    {
        this->Msgbuf[MsgBufIndx + RplcLtrCnt + j] = this->TmpBufA[j];
        j++;
        if ((MsgBufIndx + RplcLtrCnt + j) == MsgbufSize)
        {
            this->Msgbuf[MsgBufIndx + RplcLtrCnt + j - 1] = 0;
            break; // abort, Msgbuf length exceeded
        }
    }
    /*make sure the character sequence is NULL terminated
     and update LstLtrPrntd */
    if ((MsgBufIndx + RplcLtrCnt + j) < MsgbufSize)
    {
        this->Msgbuf[MsgBufIndx + RplcLtrCnt + j] = 0;
        this->LstLtrPrntd = MsgBufIndx + RplcLtrCnt + j;
    }

    if (j > 0)
        MsgBufIndx += RplcLtrCnt;
    return MsgBufIndx;
};