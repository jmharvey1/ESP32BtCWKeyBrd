#include "AdvParser.h"
#include "DcodeCW.h"

AdvParser::AdvParser(void) //TFT_eSPI *tft_ptr, char *StrdTxt
{
	// ptft = tft_ptr;
	// pStrdTxt = StrdTxt;
	// ToneColor = 0;
};

void AdvParser::EvalTimeData(uint16_t KeyUpIntrvls[50], uint16_t KeyDwnIntrvls[50], int KeyUpPtr, int KeyDwnPtr)
{
    KeyDwnBucktPtr = KeyUpBucktPtr = 0; // reset Bucket pntrs
    if(KeyDwnPtr< KeyUpPtr)TmpUpIntrvlsPtr = KeyDwnPtr; 
    else TmpUpIntrvlsPtr = KeyUpPtr;
    /*Print the "raw" capture tables*/
    for (int i = 0; i < TmpUpIntrvlsPtr; i++)
    {
        TmpDwnIntrvls[i] = KeyDwnIntrvls[i];
        TmpUpIntrvls[i] = KeyUpIntrvls[i];
        // printf("Dwn: %3d\t", KeyDwnIntrvls[i]);
        // if (i < KeyUpPtr)
        //     printf("Up: %3d\n", KeyUpIntrvls[i]);
        // else
        //     printf("Up: ???\n");
    }
    //printf("%d; %d\n\n", KeyDwnPtr, KeyUpPtr);
    /*Now sort the raw tables*/
    insertionSort(KeyDwnIntrvls, KeyDwnPtr);
    insertionSort(KeyUpIntrvls, KeyUpPtr);
    KeyUpBuckts[KeyUpBucktPtr] = KeyUpIntrvls[0];// At this point KeyUpBucktPtr = 0
    KeyDwnBuckts[KeyDwnBucktPtr] = KeyDwnIntrvls[0];// At this point KeyDwnBucktPtr = 0
    /*Build the Key down Bucket table*/
    for (int i = 1; i < KeyDwnPtr; i++)
    {
        bool match = false;
        if ((float)KeyDwnIntrvls[i] <= (4 + (1.2 * KeyDwnBuckts[KeyDwnBucktPtr])))
        {
            match = true;
        }
        if (!match)
        {
            KeyDwnBucktPtr++;
            if (KeyDwnBucktPtr >= 15)
                break;
            KeyDwnBuckts[KeyDwnBucktPtr] = KeyDwnIntrvls[i];
        }
    }
    /*Build the Key Up Bucket table*/
    for (int i = 1; i < KeyUpPtr; i++)
    {
        bool match = false;
        if ((float)KeyUpIntrvls[i] <= (4 + (1.2 * KeyUpBuckts[KeyUpBucktPtr])))
        {
            match = true;
        }
        if (!match)
        {
            KeyUpBucktPtr++;
            if (KeyUpBucktPtr >= 15){
                KeyUpBucktPtr = 14;
                break;
            }
            KeyUpBuckts[KeyUpBucktPtr] = KeyUpIntrvls[i];
        }
    }

    if (KeyDwnBucktPtr > 0 && KeyUpBucktPtr > 0)
    {
        for (int i = 0; i <= KeyDwnBucktPtr; i++)
        {
            printf(" KeyDwn: %3d/%3d\t", KeyDwnBuckts[i], (int)(4 + (1.2 * KeyDwnBuckts[i])));
        }
        printf("%d\n", 1 + KeyDwnBucktPtr);
        for (int i = 0; i <= KeyUpBucktPtr; i++)
        {
            printf(" KeyUp : %3d/%3d\t", KeyUpBuckts[i], (int)(4 + (1.2 * KeyUpBuckts[i])));
        }
        printf("%d\n", 1 + KeyUpBucktPtr);
        SetSpltPt(KeyDwnBuckts, KeyDwnBucktPtr);
        printf("SplitPoint:%3d\n", DitDahSplitVal);
    }
    /*OK; now its time to build a text string*/
    int n = 0;
    SymbSet = 1;
    this->Msgbuf[0] = 0;
    ExitPtr = 0;
    printf("LtrbrkHistory:\n");
    /*Rebuild/Parse this group of Key down/key Up times*/
    while(n < TmpUpIntrvlsPtr){
        printf("Dwn: %3d\t", TmpDwnIntrvls[n]);
        if (n < KeyUpPtr)
            printf("Up: %3d\t", TmpUpIntrvls[n]);
        else
            printf("Up: ???\t");
        SymbSet = SymbSet<<1;// append a new bit to the symbolset & default it to a 'Dit'
        if(TmpDwnIntrvls[n] > DitDahSplitVal)  SymbSet +=1;// Reset the new bit to a 'Dah'
        if(Tst4LtrBrk(n)){// now test if the follow on keyup time represents a letter break
           /*if true we have a complete symbol set; So find matching character(s)*/
           int IndxPtr = AdvSrch4Match(SymbSet, true);//try to convert the current symbol set to text & 
                                                      //and save/append the results to 'Msgbuf[]'
           SymbSet = 1;//start a new symbolset
        }
        
        printf("\tLBrkCd: %d", ExitPath[n]); 
        if(BrkFlg !=NULL) printf("+\n");
        else printf("\n"); 
        n++;
    }
    printf("%d; %d\n\n", KeyDwnPtr, KeyUpPtr);
    printf("AdvParse text: %s\n", this->Msgbuf);
    
    // for (int i = 0; i < LtrPtr; i++)
    //     LtrHoldr[i] = 0;
    // LtrPtr = 0;
    printf("\n--------\n\n");
};

void AdvParser::insertionSort(uint16_t arr[], int n) {
    for (int i = 1; i < n; i++) {uint16_t key = arr[i]; int j = i - 1; while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;
    }
};
/*for this group of keydown intervals find the value where any shorter inteval is a "Dit"
 & all longer times are "Dahs"*/
void AdvParser::SetSpltPt(uint16_t arr[], int n){
    int i;
    uint16_t NewSpltVal;
    for (i = 1; i <= n; i++)
    {
        if (2 * arr[i - 1] <= arr[i])
            break;
    }
    if (i < n)
        /*Lets make sure we're not going to come up with an absurd value*/
        if(3*arr[i - 1]<= arr[i]) NewSpltVal = arr[i - 1] + (arr[i] - arr[i - 1]) / 3;
        else NewSpltVal = arr[i - 1] + (arr[i] - arr[i - 1]) / 2;
    else
         /*Lets make sure we're not going to come up with an absurd value*/
        if(3*arr[0]<= arr[n]) NewSpltVal =  arr[0] + (arr[n] - arr[0]) / 3;
        else NewSpltVal = arr[0] + (arr[n] - arr[0]) / 2;
    if(DitDahSplitVal == 0) DitDahSplitVal = NewSpltVal;
    else DitDahSplitVal = (3*DitDahSplitVal + NewSpltVal)/4;
};
/*for this group of keydown intervals(TmpDwnIntrvls[]) & the selected keyUp interval (TmpDwnIntrvls[n]), 
use the test set out within to decide if this keyup time represents a letter break
& return "true" if it does*/
bool AdvParser::Tst4LtrBrk(int n)
{
    bool ltrbrkFlg = false;
    BrkFlg = NULL;
    if (TmpUpIntrvls[n] >= KeyUpBuckts[KeyUpBucktPtr]){
        ExitPath[n] = 0;
        BrkFlg = '+';
        return true;
    }
    if (TmpUpIntrvls[n] <= 1.20 * KeyUpBuckts[0]){
        ExitPath[n] = 1;
        return false;
    }
    /*test that there is another keydown interval after this one*/
    if (n < TmpUpIntrvlsPtr-1)
    {
        /*test for 2 adjacent 'dahs'*/
        if ((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n+1] > DitDahSplitVal))
        {
            /*we have two adjcent dahs, set letter break if the key up interval is longer than the shortest of the 2 Dahs*/
            if ((TmpUpIntrvls[n] > TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > TmpDwnIntrvls[n + 1])){
                ExitPath[n] = 2;
                BrkFlg = '+';
                return true;
            }else{
                ExitPath[n] = 3;
                return false;
            }
        /*test for 2 adjacent 'dits'*/    
        }else if((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n+1] < DitDahSplitVal)){
            /*we have two adjcent dits, set letter break if the key up interval is 1.5x longer than the shortest of the 2 Dits*/
            if ((TmpUpIntrvls[n] > 1.5*TmpDwnIntrvls[n]) || (TmpUpIntrvls[n] > 1.5*TmpDwnIntrvls[n + 1])){
                ExitPath[n] = 4;
                BrkFlg = '+';
                return true;
            }else{
                ExitPath[n] = 5;
                return false;
            }
        /*test for dah to dit transistion*/    
        }else if((TmpDwnIntrvls[n] > DitDahSplitVal) && (TmpDwnIntrvls[n+1] < DitDahSplitVal)){
            /*We have Dah to dit transistion set letter break only if key up interval is > 2x the dit interval*/
            if ((TmpUpIntrvls[n] > 1.5*TmpDwnIntrvls[n+1]) ){
                ExitPath[n] = 6;
                BrkFlg = '+';
                return true;
            }else{
                ExitPath[n] = 7;
                return false;
            } 
        /*test for dit to dah transistion*/    
        }else if((TmpDwnIntrvls[n] < DitDahSplitVal) && (TmpDwnIntrvls[n+1] > DitDahSplitVal)){
            /*We have Dit to Dah transistion set letter break only if key up interval is > 2x the dit interval*/
            if ((TmpUpIntrvls[n] > 2*TmpDwnIntrvls[n]) ){
                ExitPath[n] = 8;
                BrkFlg = '+';
                return true;
            }else{
                ExitPath[n] = 9;
                return false;
            }    
        }else{
            printf("Error: NO letter brk test");
            ExitPath[n] = 10;
            return ltrbrkFlg;
        }        
    /*then this is the last keyup so letter brk has to be true*/
    } else{
        ExitPath[n] = 11;
        BrkFlg = '+';
        return true;
    }
    /*Should never Get Here*/
    ExitPath[n] = 12;
    printf("Error: NO letter brk test");
    return ltrbrkFlg;
};

int AdvParser::AdvSrch4Match(unsigned int decodeval, bool DpScan)
{
	int pos1 = linearSearchBreak(decodeval, CodeVal1, ARSIZE); // note: decodeval '255' returns SPACE character
	char TmpBufA[10];
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
			sprintf(this->Msgbuf,"%s%s", TmpBufA, "*");
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