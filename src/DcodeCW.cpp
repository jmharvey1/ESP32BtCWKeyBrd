/*
 * DcodeCW.cpp
 *
 *  Created on: Mar 28, 2021
 *      Author: jim
 * 20221020 changed the interval measuring clock from HAL_GetTick() timer5, for improved timing measurements
 * 20221025 made changes to timing/parsing routines to improve Bug3 (cootie) decoding
 * 20230609 Added code to the KEYISR to improve recovery/WPM tracking from slow to fast CW
 * 20230617 To support advanced DcodeCW "Bug" processes, reworked dispMsg(char Msgbuf[50]) to handle "delete" character condition
 * 20230711 minor tweaks to concatenate processes to imporve delete character managment
 */

//#include <SetUpCwDecoder.h>
//#include "main.h" // removed this reference for ESP32 version
#include "DcodeCW.h"
#include "Goertzel.h"
//#include "BtnSuprt.h"
//#include "TchScrnCal.h"
#include "Arduino.h" //For ESP32, may not need this reterence, since timr5 count calls are replaced with ESP32 equivalent 
//#include "SerialClass.h"

int ShrtBrk[10];
int charCnt = 0;
int shwltrBrk = 0;
int msgcntr = 0;
int badCodeCnt = 0;
int dahcnt =0;
int MsgChrCnt[2];
int ltrCmplt = -2200; //letter complete false;  used in plot mode, to show where/when letter breaks are detected
volatile int TimeDat[16];
int Bitpos = 0;
int badKeyEvnt = 0;
int DeleteID = 0;

//int px = 0; //mapped 'x' display touch value
//int py = 0; //mapped 'y' display touch value
int displayW = 320;
int LineLen = 27; //max number of characters to be displayed on one line
//int btnPrsdCnt = 0;
int statsMode = 0;
int textSrtX = 0;
int textSrtY = 0;
int cnt = 0; //used in scrollpage routine
int curRow = 0;
int offset = 0;
int LtrCntr =0;
int OldLtrPntr =0;
int unsigned ClrBtnCnt = 0;
int unsigned ModeBtnCnt = 0;
int ModeCnt = 0; // used to cycle thru the 4 decode modes (norm, bug1, bug2, bug3)

int TEcnt = 0;
int BadDedSpceCnt =0;


int ylo = 295; //button touch reading 3.5" screen
int yhi = 320; //button touch reading 3.5" screen
int Stxl = 0; //Status touch reading 3.5" screen
int Stxh = 140;//150; //Status touch reading 3.5" screen
int bxlo = 155; //button touch reading 3.5" screen
int bxhi = 240; //button touch reading 3.5" screen
int gxlo  = 240; //button touch reading 3.5" screen
int gxhi  = 325; //button touch reading 3.5" screen
/*Tone Speed Button */
int Sbxlo  = 326; //button touch reading 3.5" screen
int Sbxhi  = 400; //button touch reading 3.5" screen
//int CPL = 40; //number of characters each line that a 3.5" screen can contain
//int row = 10; //number of uable rows on a 3.5" screen
int fontH = 16;
int fontW = 12;
int cursorY = 0;
int cursorX = 0;
int wpm = 0;
int lastWPM = 0;
int state = 0;
//int RfshFlg = 0; //no longer needed here; now done in Goertzel.cpp
//int DeBug = 0;
int TDBptr = 0;

volatile int TimeDatBuf[24];

char newLine = '\n';
char DeBugMsg[150];
char Pgbuf[448];
char Msgbuf[50];
char LstPstdchar[2];// used for diagnostic testing only
char P[13];
char PrntBuf[150];
//char RevDate[9] = "20221115";
//char DahMthd[150];//used for diagnostic testing only
//char TmpMthd[150];//used for diagnostic testing only
volatile bool valid = LOW;
volatile bool mark = LOW;
bool dataRdy = LOW;
bool Bug2 = false;
bool Bug3 = false;
bool badLtrBrk = false;
bool dletechar = false;
bool ConcatSymbl = false;
bool Test = false; //if "true", use ide Serial Monitor to see output/results
bool SCD = false; //false;//true; //Sloppy Code Debugging ('Bg1'); if "true", use ide Serial Monitor to see output/results
bool FrstSymbl = false;
bool chkStatsMode = true;
bool NrmMode = true;
bool CalcAvgdeadSpace = false;
bool CalcDitFlg = false;
bool LtrBrkFlg = true;
bool newLineFlag = false;
bool NuWrd = true;
bool NuWrdflg = false;
bool SetRtnflg = false;
bool SwMode = false;
bool TonPltFlg = false; //added here to satisfy ESP32 configuration
//bool buttonEnabled = true;
bool BugMode = false;
bool XspctHi = false;
bool XspctLo = true;
bool Prtflg = false; //aded for diagnostic keyISR testing
int exitCD = 0; //aded for diagnostic keyISR testing
int DitDahCD = 0; //aded for diagnostic keyISR testing
int glitchCnt =0; //added to recover from slow CW shifting to high speed cw ({~>25WPM})
float LtBrkSF = 0.3; //letter Break Scale factor. Used to catch sloppy sending; typical values //(0.66*ltrBrk)  //(0.15*ltrBrk) //(0.5*ltrBrk)
int unsigned ModeCntRef = 0;
unsigned long AvgShrtBrk = 0;
unsigned long AvgShrtBrk1 = 0;
volatile unsigned long period = 0;
//volatile unsigned long Tstperiod = 0;// used for testing to study what the keydown time interval is at various points in the program
volatile unsigned int CodeValBuf[7];
volatile unsigned int DeCodeVal;
volatile unsigned int DCVStrd[2];//used by Bug3 to recover last recored DeCodeVal;
volatile unsigned int OldDeCodeVal;
volatile unsigned long STart = 0;
volatile unsigned long AltSTart = 0;
volatile unsigned long start1 = 0; //added in an attempt to recover from a failed/lost time stamp
volatile unsigned long MySTart = 0; //only used for testing
//volatile unsigned long EvntStart = 0;
volatile unsigned long lastDit1;  //added for testing
volatile unsigned long Oldstart = 0;
volatile unsigned long thisWordBrk = 0;
//volatile unsigned long noSigStrt;
volatile unsigned long WrdStrt;
volatile unsigned long lastWrdVal;
volatile unsigned long  space = 0;
volatile unsigned long ltrBrk = 0;// interval to wait after last NoSig Start event/timestamp to convert the current decode val to a character
unsigned long deadSpace = 0;
volatile unsigned long letterBrk = 0; //letterBrk =avgDit;
volatile unsigned long letterBrk1 = 0;
volatile unsigned long OldltrBrk = 0;
volatile unsigned long BadDedSpce = 0;
unsigned long LastdeadSpace = 0;
unsigned long BadSpceStk[5];
unsigned long SpaceStk[16];// use this to refine parsing of failed decodeval
unsigned long SpcIntrvl[16];// use this to refine parsing of failed decodeval
unsigned long SpcIntrvl1[16];// use this to refine parsing of failed decodeval
unsigned long SpcIntrvl2[16];// use this to refine parsing of failed decodeval
unsigned long PrdStack[10];
int PrdStackPtr = 0;
int Shrt = 1200;
int Long1 = 0;
float Ratio = 0;
unsigned long ShrtBrkA = 0;
unsigned long UsrLtrBrk = 100; // used to track the average letter break interval
unsigned long AvgLtrBrk = 0;
unsigned long bfrchksum;// used to compare/check 'speed' message strings
volatile unsigned long avgDit = 80; //average 'Dit' duration
volatile unsigned long DitVar = 0; //average 'Dit' duration
volatile unsigned long avgDah = 240; //average 'Dah' duration
volatile unsigned long lastDah = avgDah;
float lastDit = (float)avgDit;

volatile unsigned long avgDeadSpace = avgDit;
//volatile unsigned long wordBrk = avgDah ;
volatile unsigned long wordStrt;
volatile unsigned long deadTime;
volatile unsigned long MaxDeadTime;
volatile unsigned long LpCnt = 0;
volatile bool wordBrkFlg = false;
float CurNoise =10000;
float SqlchLvl = 0;
float curRatio = 3;
float SNR = 5.0;
float ShrtFctr = 0.48;//0.52;// 0.45; //used in Bug3 mode to control what percentage of the current "UsrLtrBrk" interval to use to detect the continuation of the previous character
uint8_t GudSig = 0;
TFTMsgBox *ptrmsgbx;
SemaphoreHandle_t DeCodeVal_mutex;
bool blocked = false;
bool KEISRwaiting = false;
//SerialClass Serial;
// End of CW Decoder Global Variables

///////////////////////////////////////////////////////////////////////////////////////////////
void StartDecoder(TFTMsgBox *pttftmsgbx)
{
	//Begin CW Decoder setup
	DeCodeVal_mutex = xSemaphoreCreateMutex();
	ptrmsgbx = pttftmsgbx;
	//ts.ReDefineTchPins(XP, YP, XM, YM);
	//tft.reset();
	//ID = tft.readID();
	// if (ID == 0x9090) ID = 0x9486; //do this to fix color issues
	// tft.begin(ID);//  The value here is screen specific & depends on the chipset used to drive the screen,
	// tft.setRotation(1); // valid values 1 or 3
	// tft.fillScreen(BLACK);
	// tft.fillScreen(BLACK);//Had to do this twice to get the Screen Fully "Blacked" out
	// scrnHeight = tft.height();
	// scrnWidth = tft.width();
	// displayW = scrnWidth;
	// // PORTRAIT is 240 x 320 2.8" inch Screen
	// if (scrnHeight == 240) { //If true change settings from 3.5 inch to 2.8" screen
	// 	px = 0; //mapped 'x' display touch value
	// 	py = 0; //mapped 'y' display touch value
	// 	LineLen = 27; //max number of characters to be displayed on one line
	// 	row = 7;//7; //max number of lines to be displayed
	// 	ylo = 200; //button touch reading 2.8" screen
	// 	yhi = 250; //button touch reading 2.8" screen
	// 	Stxl = 21; //Status touch reading 2.8" screen
	// 	Stxh = 130; //Status touch reading 2.8" screen
	// 	bxlo = 130; //blue button (CLEAR) touch reading 2.8" screen
	// 	bxhi = 200; //blue button (CLEAR)touch reading 2.8" screen
	// 	gxlo  = 200; //green button(MODE) touch reading 2.8" screen
	// 	gxhi  = 290; //green touch (MODE)reading 2.5" screen
	// 	CPL = 27; //number of characters each line that a 2.8" screen can contain

	// }

	// MsgChrCnt[0] = 0;
	// DrawButton2();
	// DrawButton();
	// ModeBtn();
	WPMdefault();
	// tft.setCursor(textSrtX, textSrtY);
	// tft.setTextColor(WHITE);//tft.setTextColor(WHITE, BLACK);
	// tft.setTextSize(2);//SemaphoreHandle_t  *ptrmutex;
	// tft.setTextWrap(false);

//	enableINT(); //This is a local function, and is defined below

	STart = 0;
	WrdStrt = pdTICKS_TO_MS(xTaskGetTickCount());//(GetTimr5Cnt()/10);
	wordBrk = avgDah ;
	/*initialize period stack with 15WPM dit intervals*/
	for(int i=0; i < 10; i++){
		PrdStack[i] = 1200/15;
	}
	// if (scrnHeight == 320) { //we are using a 3.5 inch screen
	// 	sprintf( Title, "             KW4KD (%s)           ", RevDate );

	// }
	// else {
	// 	sprintf( Title, "      KW4KD (%s)     ", RevDate );

	// }

	/*setup guard rail to prevent USB err when trying to display title*/
	// bool tstState = false;
	// if(Test){
	// 	tstState = true;
	// 	Test = false;
	// }
	// dispMsg(Title);
	// tft.setTextColor(TFT_YELLOW);
	// if(NoStrdVals){
	// 	sprintf( Title, "        No stored USER params Found       Using FACTORY values until params are   'Saved' via the Settings Screen");
	// 	dispMsg(Title);
	// 	sprintf( Title, "       ");
	// 	dispMsg(Title);
	// 	delay(2000);
	// 	tftmsgbx.setTxtSize(1);
	// 	TchScrnCal_MainLoop();
	// 	tft.fillScreen(BLACK);
	// 	tft.fillScreen(BLACK);//Had to do this twice to get the Screen Fully "Blacked" out
	// 	DrawButton2();
	// 	DrawButton();
	// 	ModeBtn();
	// 	tftmsgbx.setTxtSize(2);
	// 	tftmsgbx.setFont(12, 16);//setFont(w,h)

	// }
	// if(tstState) Test = true;
	wordBrk = ((5 * wordBrk) + (4 * avgDeadSpace)) / 6;
	// tft.setTextColor(WHITE);
	// sprintf(DeBugMsg, "");
	//End CW Decoder Setup
	/*ESP32 version doesn't need to jump to the 'Dcodloop'*/
	// Dcodeloop();
}/* END SetUp Code */
/////////////////////////////////////////

/* PB13 KeyIN interrupt routine. */
void KeyEvntSR(uint8_t state, unsigned long EvntTime)
{ // keydown state =0; Keyup state = 1;
	char tmpbuf[50];
	// EvntStart = pdTICKS_TO_MS(xTaskGetTickCount()); //This is now hapening in main.GoertzelHandler(void *param) when the adc signals that the sample group is ready for processing
	ChkDeadSpace();
	SetLtrBrk();
	chkChrCmplt();
	// uint8_t state = (uint8_t)HAL_GPIO_ReadPin (GPIOB, GPIO_PIN_13);
	XspctLo = true;
	XspctHi = true;
	/* See if we can obtain the semaphore.  If the semaphore is not available, wait 12 ticks to see if it becomes free. */
	if (xSemaphoreTake(DeCodeVal_mutex, (TickType_t)12) == pdFALSE)
	{
		/* failed obtain the semaphore. So abort */
		/* Which will normally happen, when we are actively sending CW via the BT Keyboard*/
		// if (!PlotFlg){ //if we are running in plot mode,suppress message 
		// 	sprintf(tmpbuf, "KeyEvntSR Blocked \r\n");
		// 	printf(tmpbuf);
		// }
		return;
	}
	else
	{
		blocked = true;
		// 	/* We could not obtain the semaphore and can therefore not access
		// 	the shared resource safely. */
		// 	char BTbuf[30] = "KeyEvntSR Blocked \r\n";
		// 	printf(BTbuf);
		// 	return;
		// }
		if (state == LOW && XspctLo)
		{					   // key-down event
			start1 = EvntTime; // HAL_GetTick();//used/saved in case recovery is needed
			if (((EvntTime - noSigStrt) < 15) && (wpm < 35))
			{
				// PIN_LOW(LED_GPIO_Port, LED_Pin);
				xSemaphoreGive(DeCodeVal_mutex);
				blocked = false;
				return; // appears to be a "glitch" so ignore it
			}
			bool PrntUSB = false;
			PrntBuf[0] = 0; // clear the print buffer
			XspctLo = false;
			XspctHi = true;
			Oldstart = STart;
			STart = EvntTime;				 // HAL_GetTick();
			MySTart = EvntTime;				 // for testing purposes only
			deadSpace = (STart - noSigStrt); //+4;//jmh 20230706 added this corection value for ESP32
			SpaceStk[Bitpos] = deadSpace;
			if (Bitpos <= 14)
				SpaceStk[Bitpos + 1] = 0; // make sure the next time slot has been "0SftReset(void)" out
			OldltrBrk = letterBrk;
			// 20200818 jmh following 3 lines to support sloppy code debuging
			if (GudSig)
				ShrtBrk[0] = int(STart - letterBrk1);
			// Serial.println(ShrtBrk[0]);
			//  test to detect sloppy sender timing, if "true" this keydown event appears to be a continuation of previous character
			if ((ShrtBrk[0] < ShrtBrkA) && Bug3 && GudSig)
			{									  // think I fixed needing to wait until we're on the 2nd line of the display "&& (cnt>CPL+1)"
				badLtrBrk = true;				  // this flag is just used to drive the "toneplot" graph
				AvgShrtBrk = (56 * ltrBrk) / 100; // AvgShrtBrk = (79*ltrBrk)/100;
				if ((ShrtBrk[0] < AvgShrtBrk) || (OldLtrPntr == 1))
				{
					if (Bug3 && SCD && Test)
					{
						sprintf(PrntBuf, "Concatenate  %d; ", (int)OldLtrPntr);

						for (int i = 0; i <= OldLtrPntr; i++)
						{
							// USBprintInt(ShrtBrk[i]);
							// printf(";");
							// sprintf(PrntBuf, "%s %d;", PrntBuf, (int)ShrtBrk[i]);
						}
						// printf("  ");
						// USBprintIntln(AvgShrtBrk);
						// sprintf(PrntBuf, "%s %d\n\r", PrntBuf, (int)AvgShrtBrk);
						PrntUSB = true; // printf(PrntBuf);
					}
					if (ShrtBrk[0] > UsrLtrBrk / 5)
						AvgShrtBrk = (2 * AvgShrtBrk + ShrtBrk[0]) / 3; // update AvgShrtBrk

					/*test to make sure that the last letter received has actually been processed */
					int BPtr = 0;
					while (CodeValBuf[BPtr] != 0)
					{
						DeCodeVal = CodeValBuf[BPtr];
						//        Serial.print(BPtr);
						//        Serial.print('\t');
						//        Serial.print(CodeValBuf[BPtr]);
						//        Serial.print('\t');
						++BPtr;
					}
					if (BPtr > 0)
					{
						CodeValBuf[BPtr] = 99999;
						/* restore old space interval values */
						// unsigned long LstSpcTm = SpaceStk[Bitpos];
						unsigned long LstSpcTm = SpaceStk[0];
						Bitpos = 0;
						// while((SpcIntrvl[Bitpos] != 0) && (Bitpos !=0) && (Bitpos <16 )){
						while ((SpcIntrvl[Bitpos] != 0) && (Bitpos < 16))
						{
							SpaceStk[Bitpos] = SpcIntrvl[Bitpos];
							++Bitpos;
						}
						// SpaceStk[Bitpos] = 7777;
						//++Bitpos;
						SpaceStk[Bitpos] = LstSpcTm;
						//++Bitpos;
						if (Bitpos > 15)
							Bitpos = 15;
					}
					if (DeCodeVal == 0)
					{ // Last letter received was posted to the screen
						// go get that last character displayed
						//         Serial.print("cnt: ");
						//         Serial.println(cnt);
						//         if(cnt>CPL+1){//Pgbuf now has enough data to recover sloppy sending results
						if (MsgChrCnt[1] > 0)
						{
							DeCodeVal = DCVStrd[1];
							/* restore old space interval values */
							// unsigned long LstSpcTm = SpaceStk[Bitpos];
							unsigned long LstSpcTm = SpaceStk[0];
							Bitpos = 0;
							// while((SpcIntrvl[Bitpos] != 0) && (Bitpos !=0) && (Bitpos <16 )){
							while ((SpcIntrvl2[Bitpos] != 0) && (Bitpos < 16))
							{
								SpaceStk[Bitpos] = SpcIntrvl2[Bitpos];
								++Bitpos;
							}
							// SpaceStk[Bitpos] = 9999;
							//++Bitpos;
							SpaceStk[Bitpos] = LstSpcTm;
							//++Bitpos;
							if (Bitpos > 15)
								Bitpos = 15;
							dletechar = true;
							DeleteID = 1;
							FrstSymbl = true;
							ConcatSymbl = true; // used to verify (at the next letter break) we did the right thing;
							if (Bug3 && SCD && Test)
							{
								for (int i = 0; i < sizeof(tmpbuf); i++)
								{
									tmpbuf[i] = PrntBuf[i];
									if (tmpbuf[i] == 0)
										break;
								}
								sprintf(PrntBuf, "%s; DeleteID = %d\n", tmpbuf, DeleteID);
							}
						}
						else
							badLtrBrk = false;
					}
					else
					{ // abort letter break process
						//++Bitpos;
						// SpaceStk[Bitpos] = 3333;
						//++Bitpos;
						if (Bitpos > 15)
							Bitpos = 15;
						if (Bug3 && SCD && Test)
						{
							// printf("Clawed Last Letter Back");
							// printf("\t");
							// USBprintIntln(DeCodeVal);
							// sprintf(PrntBuf, "%sClawed Last Letter Back\t%d\n\r",PrntBuf, (int)DeCodeVal);
							// sprintf(PrntBuf, "CLB\n\r");
							// delay(2);
							PrntUSB = true; // printf(PrntBuf);
						}
					}
				}
				else
				{
					if (Bug3 && SCD && Test)
					{
						// printf("Skip Concat  ");
						// USBprintInt(OldLtrPntr);
						// printf(" ");
						// sprintf(PrntBuf, "Skip Concat  %d", (int)OldLtrPntr);
						for (int i = 0; i <= OldLtrPntr; i++)
						{
							// USBprintInt(ShrtBrk[i]);
							// printf(";");
							// sprintf(PrntBuf, "%s  %d;", PrntBuf, (int)ShrtBrk[i]);
						}
						// printf("  ");
						// USBprintIntln(AvgShrtBrk);
						// sprintf(PrntBuf, "%s  %d\n\r", PrntBuf, (int)AvgShrtBrk);
						PrntUSB = true; // printf(PrntBuf);
					}
					AvgShrtBrk = (AvgShrtBrk + ShrtBrk[0]) / 2; // update AvgShrtBrk
																// AvgShrtBrk = (2*AvgShrtBrk+ShrtBrk[0])/3;// update AvgShrtBrk
																// AvgShrtBrk = (4*AvgShrtBrk+ShrtBrk[0])/5;// update AvgShrtBrk
				}
			}

			letterBrk = 0;
			ltrCmplt = -2200; //letter complete false; used in plot mode, to show where/when letter breaks are detected
			if (GudSig)
				CalcAvgdeadSpace = true;

			if (wordBrkFlg)
			{
				wordBrkFlg = false;
				thisWordBrk = STart - wordStrt;
				// Serial.println(thisWordBrk);
				if (thisWordBrk < 11 * avgDeadSpace)
				{
					if (GudSig)
						wordBrk = (5 * wordBrk + thisWordBrk) / 6;
					MaxDeadTime = 0;
					charCnt = 0;
					// Serial.println(wordBrk);
				}
			}
			else if (charCnt > 12)
			{
				if (MaxDeadTime < wordBrk)
				{
					wordBrk = MaxDeadTime;
					// Serial.println(wordBrk);
				}
				MaxDeadTime = 0;
				charCnt = 0;
			}
			noSigStrt = EvntTime; // HAL_GetTick();//jmh20190717added to prevent an absurd value

			if (DeCodeVal == 0)
			{ // we're starting a new symbol set
				DeCodeVal = 1;
				valid = LOW;
			}
			// Test = false;//false;//true;
			if (PrntUSB)
				printf(PrntBuf);
			xSemaphoreGive(DeCodeVal_mutex);
			blocked = false;
			return;
			/*****   end of "keydown" processing ******/
		}
		else if (state == HIGH && XspctHi)
		{ // "Key Up" event/evaluations;
			int prob = 0;
			if (DeCodeVal == 0)
			{
				DeCodeVal = 1;
				prob = 1;
				STart = start1;
			}
			// Tstperiod = EvntStart - STart;
			DitDahCD = 8;

			if (DeCodeVal != 0)
			{ // Normally true; We're working with a valid symbol set. Reset timers
				unsigned long OldNoStrt = noSigStrt;
				badLtrBrk = false;
				noSigStrt = EvntTime;
				// Prtflg = true; //enable or uncomment for diagnostic testing
				if (STart != 0)
					period = (noSigStrt - STart); //+4;//jmh 20230706 added this corection value for ESP32;
				else
					period = (noSigStrt - AltSTart); //+4;//jmh 20230706 added this corection value for ESP32; // Something weird happened so use backup start value

				if (prob)
				{
					prob = 0;
					STart = start1;
					period = (noSigStrt - STart); //+4;//jmh 20230706 added this corection value for ESP32
				}
				lastDit1 = period;
				/*20230708 For ESP32 removed this code becuase the issue is now being managed on the Goertzel side of the fence */
				// if ((1.72 * period < avgDit) || ((period < 15) && (wpm > 35)))
				// { // seems to be a glitch//1.5
				// 	if (DeCodeVal == 1)
				// 	{
				// 		glitchCnt++;
				// 		if (glitchCnt < 2)
				// 		{
				// 			noSigStrt = OldNoStrt;
				// 			XspctLo = false;
				// 			XspctHi = true;
				// 			exitCD = 1;
				// 			return; // its a glitch before any real key closures have been detected, so ignore completely
				// 		}
				// 	}

				// 	if (GudSig && period > 10) // if (SqlchLvl > (1.1 * CurNoise))
				// 	{
				// 		// if (GudSig)
				// 		// {
				// 		// 	avgDit = (4 * avgDit + period) / 5; // magC>10000 go ahead & factor this period in, just incase we are now listening to a faster WPM stream
				// 		// }
				// 		/*Ok lets try decide if this interval represents a dit or dah by comparing its duration with the last ten recorded periods*/
				// 		Shrt = 1200;
				// 		Long1 = 0;
				// 		int ValidCnt = 0;
				// 		DitDahCD = 4;
				// 		for (int i = 0; i < 10; i++)
				// 		{
				// 			if (PrdStack[i] > Long1)
				// 				Long1 = PrdStack[i];
				// 			if (PrdStack[i] < Shrt)
				// 			{
				// 				Shrt = PrdStack[i];
				// 			}
				// 			Ratio = (float)((float)period / (float)PrdStack[i]);
				// 			if ((Ratio < 1.5 && Ratio > 0.6) || (Ratio > 0.25 && Ratio < 0.44 && period < 0.5 * Long1) || (Ratio > 2.7 && Ratio < 3.9 && period > 2 * Shrt))
				// 				ValidCnt++;
				// 		}
				// 		PrdStackPtr++;
				// 		if (PrdStackPtr >= 10)
				// 			PrdStackPtr = 0;
				// 		PrdStack[PrdStackPtr] = period;
				// 		if (ValidCnt > 6)
				// 		{
				// 			if (period < Long1 / 2)
				// 			{
				// 				avgDit = (4 * avgDit + period) / 5; // then its a dit
				// 				DitDahCD = 1;
				// 			}
				// 			else if (period > 2 * Shrt)
				// 			{
				// 				avgDah = (4 * avgDah + period) / 5; // then its a dah
				// 				DitDahCD = 2;
				// 			}
				// 			else
				// 			{
				// 				avgDit = (4 * avgDit + period) / 5; // treat it as a dit
				// 				DitDahCD = 3;
				// 			}
				// 		}
				// 		else
				// 		{
				// 			exitCD = ValidCnt;
				// 			// return;
				// 		}
				// 	}
				// 	else
				// 	{
				// 		glitchCnt++;
				// 		if (glitchCnt < 2)
				// 		{
				// 			noSigStrt = OldNoStrt;
				// 			if (Oldstart != 0)
				// 				STart = Oldstart;
				// 			letterBrk = OldltrBrk;
				// 			if (GudSig && period > 10)
				// 				exitCD = 2;
				// 			else
				// 				exitCD = 22;
				// 			return; // abort processing this event
				// 		}
				// 	}
				// }
				WrdStrt = noSigStrt;
				TimeDat[Bitpos] = period;
				Bitpos += 1;
				if (Bitpos > 15)
				{
					Bitpos = 15;		   // watch out: this character has too many symbols for a valid CW character
					letterBrk = noSigStrt; // force a letter brk (to clear this garbage
					if (DeCodeVal == 1)
					{					   // this should never be the case, But if true, force DeCodeVal to be something that can be evaluated
						DeCodeVal = 99990; //"error"
					}
					exitCD = 3;
					xSemaphoreGive(DeCodeVal_mutex);
					blocked = false;
					return;
				}
				AltSTart = pdTICKS_TO_MS(xTaskGetTickCount()); // GetTimr5Cnt()/10;//use this as a backup start value when STrat = 0 on a keyup event
				STart = 0;

				if (glitchCnt == 2)
				{
					// if (GudSig)
					// {
					// 	avgDit = (4 * avgDit + period) / 5; //factor this period in, just incase we are now listening to a faster WPM stream
					// 	avgDit = (4 * avgDit + period) / 5; //factor this period in, just incase we are now listening to a faster WPM stream
					// }
					//////////////////////////////////////////////
					if (GudSig && period > 10) // if (SqlchLvl > (1.1 * CurNoise))
					{
						// if (GudSig)
						// {
						// 	avgDit = (4 * avgDit + period) / 5; // magC>10000 go ahead & factor this period in, just incase we are now listening to a faster WPM stream
						// }
						/*Ok lets try decide if this interval represents a dit or dah by comparing its duration with the last ten recorded periods*/
						Shrt = 1200;
						Long1 = 0;
						int ValidCnt = 0;
						DitDahCD = 5;
						for (int i = 0; i < 10; i++)
						{
							if (PrdStack[i] > Long1)
								Long1 = PrdStack[i];
							if (PrdStack[i] < Shrt)
							{
								Shrt = PrdStack[i];
							}
							Ratio = (float)((float)period / (float)PrdStack[i]);
							if ((Ratio < 1.5 && Ratio > 0.6) || (Ratio > 0.25 && Ratio < 0.44 && period < 0.5 * Long1) || (Ratio > 2.7 && Ratio < 3.9 && period > 2 * Shrt))
								ValidCnt++;
						}
						PrdStackPtr++;
						if (PrdStackPtr >= 10)
							PrdStackPtr = 0;
						PrdStack[PrdStackPtr] = period;
						if (ValidCnt > 6)
						{
							if (period < Long1 / 2)
							{
								avgDit = (4 * avgDit + period) / 5; // then its a dit
								DitDahCD = 1;
							}
							else if (period > 2 * Shrt)
							{
								avgDah = (4 * avgDah + period) / 5; // then its a dah
								DitDahCD = 2;
							}
							else
							{
								avgDit = (4 * avgDit + period) / 5; // treat it as a dit
								DitDahCD = 3;
							}
						}
						else
						{
							exitCD = ValidCnt;
							// return;
						}
					}
					else
					{
						exitCD = 9;
						xSemaphoreGive(DeCodeVal_mutex);
						blocked = false;
						return;
					}

					/////////////////////////////////////////////
					glitchCnt = 0;
					// exitCD = 7;
				}
				else
				{
					//////////////////////////////////////////////
					if (GudSig && period > 10) // if (SqlchLvl > (1.1 * CurNoise))
					{
						// if (GudSig)
						// {
						// 	avgDit = (4 * avgDit + period) / 5; // magC>10000 go ahead & factor this period in, just incase we are now listening to a faster WPM stream
						// }
						/*Ok lets try decide if this interval represents a dit or dah by comparing its duration with the last ten recorded periods*/
						Shrt = 1200;
						Long1 = 0;
						int ValidCnt = 0;
						DitDahCD = 6;
						for (int i = 0; i < 10; i++)
						{
							if (PrdStack[i] > Long1)
								Long1 = PrdStack[i];
							if (PrdStack[i] < Shrt)
							{
								Shrt = PrdStack[i];
							}
							Ratio = (float)((float)period / (float)PrdStack[i]);
							if ((Ratio > 0.6 && Ratio < 1.5) || (Ratio > 0.25 && Ratio < 0.44 && period < 0.5 * Long1) || (Ratio > 2.7 && Ratio < 3.9 && period > 2 * Shrt))
								ValidCnt++;
						}
						PrdStackPtr++;
						if (PrdStackPtr >= 10)
							PrdStackPtr = 0;
						PrdStack[PrdStackPtr] = period;
						if (ValidCnt > 6)
						{
							if (period < Long1 / 2)
							{
								avgDit = (4 * avgDit + period) / 5; // then its a dit
								DitDahCD = 1;
							}
							else if (period > 2 * Shrt)
							{
								avgDah = (4 * avgDah + period) / 5; // then its a dah
								DitDahCD = 2;
							}
							else
							{
								avgDit = (4 * avgDit + period) / 5; // treat it as a dit
								DitDahCD = 3;
							}
						}
						else
						{
							exitCD = ValidCnt;
							// return;
						}
					}
					else
					{
						if (period <= 10)
						{
							exitCD = 8;
							xSemaphoreGive(DeCodeVal_mutex);
							blocked = false;
							return;
						}
						else
						{
							Shrt = 1200;
							Long1 = 0;
							int ValidCnt = 0;
							DitDahCD = 9;
							for (int i = 0; i < 10; i++)
							{
								if (PrdStack[i] > Long1)
									Long1 = PrdStack[i];
								if (PrdStack[i] < Shrt)
								{
									Shrt = PrdStack[i];
								}
								Ratio = (float)((float)period / (float)PrdStack[i]);
								if ((Ratio > 0.6 && Ratio < 1.5) || (Ratio > 0.25 && Ratio < 0.44 && period < 0.5 * Long1) || (Ratio > 2.7 && Ratio < 3.9 && period > 2 * Shrt))
									ValidCnt++;
							}
							PrdStackPtr++;
							if (PrdStackPtr >= 10)
								PrdStackPtr = 0;
							PrdStack[PrdStackPtr] = period;
							if (ValidCnt > 6)
							{
								exitCD = ValidCnt;
								if (period < Long1 / 2)
								{
									avgDit = (4 * avgDit + period) / 5; // then its a dit
									DitDahCD = 1;
								}
								else if (period > 2 * Shrt)
								{
									avgDah = (4 * avgDah + period) / 5; // then its a dah
									DitDahCD = 2;
								}
								else
								{
									avgDit = (4 * avgDit + period) / 5; // treat it as a dit
									DitDahCD = 3;
								}
							}
							else
							{
								exitCD = 4;
								xSemaphoreGive(DeCodeVal_mutex);
								blocked = false;
								return;
							}
						}
					}

					/////////////////////////////////////////////
					glitchCnt = 0;
					// exitCD = 4;
				}

			} // End if(DeCodeVal!= 0)
		}
		// end of key interrupt processing;
		// Now, if we are here; the interrupt was a "Key-Up" event. Now its time to decide whether this last "Key-Down" period represents a "dit", a "dah", or just garbage.
		// 1st check. & throw out key-up events that have durations that represent speeds of less than 5WPM.
		// if (GudSig) GudSig = 0; //clear old signal status
		if (period > 720)
		{						  // Reset, and wait for the next key closure
			noSigStrt = EvntTime; // HAL_GetTick();//jmh20190717added to prevent an absurd value
			DeCodeVal = 0;
			dletechar = false;
			FrstSymbl = false;
			ConcatSymbl = false;
			XspctLo = true;
			XspctHi = false;
			exitCD = 5;
			xSemaphoreGive(DeCodeVal_mutex);
			blocked = false;
			return; // overly long key closure; Forget what you got; Go back, & start looking for a new set of events
		}
		LtrBrkFlg = true; // Arm (enable) SetLtrBrk() routine
		// test to determine that this is a significant signal (key closure duration) event, & warrants evaluation as a dit or dah
		if ((float)period < 0.3 * (float)avgDit)
		{ // if "true", key down event seems to related to noise
			// if here, this was an atypical event, & may indicate a need to recalculate the average period.

			if (period > 0)
			{ // test for when the wpm speed as been slow & now needs to speed by a significant amount
				++badKeyEvnt;
				if (badKeyEvnt >= 20)
				{
					badKeyEvnt = 0;
					noSigStrt = EvntTime; // HAL_GetTick();//jmh20190717added to prevent an absurd value
					letterBrk = 0;
				}
			}
			//    if(Test) Serial.println(DeCodeVal); //we didn't experience a full keyclosure event on this pass through the loop [this is normal]
			exitCD = 6;
			xSemaphoreGive(DeCodeVal_mutex);
			blocked = false;
			return;
		}

		/**** if here, its a usable event; Now, decide if its a "Dit" or "Dah"  ****/
		if (Test && 0)
		{
			// sprintf(PrntBuf,"%s", "  KU\n\r");
			// printf(PrntBuf);
		}
		badKeyEvnt = 15;			// badKeyEvnt = 20;
		DeCodeVal = DeCodeVal << 1; // shift the current decode value left one place to make room for the next bit.
		// if (((period >= 1.8 * avgDit)|| (period >= 0.8 * avgDah))||(DeCodeVal ==2 & period >= 1.4 * avgDit)) { // it smells like a "Dah".
		bool NrmlDah = false;
		if (((period >= 1.8 * avgDit) || (period >= 0.8 * avgDah)) && !Bug2)
			NrmlDah = true;
		else if (((period > 9 * avgDeadSpace) || (period > 2 * avgDit)) && Bug2)
			NrmlDah = true;
		else if ((period >= (1.46 * avgDit) + (DitVar)) && Bug3)
		{
			NrmlDah = true;
		}

		if ((NrmlDah) || ((DeCodeVal == 2) && ((period - 10) > 1.4 * avgDit) && (Bug3 || 0))) // normal dah path or special case handling
		{																					  // it smells like a "Dah".  think there's 10 millisecond uncertainty due to the sampling speed, so we're going to use the smallest possible interval for this decision

			if ((MsgChrCnt[1] > 0) && (deadSpace < 2.76 * avgDeadSpace) && (deadSpace > 1.4 * avgDeadSpace) && !NuWrd && !dletechar && Bug3)
			{
				DeCodeVal = DCVStrd[1]; // grab back previous deCodeVal so that wecan continue to work with it
				/* restore space interval values linked to DCVStrd[1] */
				unsigned long LstSpcTm = SpaceStk[0];
				Bitpos = 0;
				while ((SpcIntrvl2[Bitpos] != 0) && (Bitpos < 16))
				{
					SpaceStk[Bitpos] = SpcIntrvl2[Bitpos];
					++Bitpos;
				}
				SpaceStk[Bitpos] = LstSpcTm;
				++Bitpos;
				if (Bitpos > 15)
					Bitpos = 15;
				DeCodeVal = DeCodeVal << 1;
				DeCodeVal = DeCodeVal + 1;
				dletechar = true;
				DeleteID = 4;
				FrstSymbl = false;	// FrstSymbl = true;
				ConcatSymbl = true; // used to verify (at the next letter break) we did the right thing;
				//        printf("\tXX\t");
				//        USBprintInt(deadSpace);
				//        printf("\t");
				//        USBprintInt(avgDeadSpace);
				//        printf("\t");
				//        USBprintInt(DCVStrd[1]);
				//        printf("\t");
				//        USBprintIntln(DeCodeVal);
				// if(!Bug3 && !SCD && Test){
				if (Test && SCD)
				{
					// sprintf(PrntBuf, "  XX\t%d\t%d\t%d\t%d\tDeleteChar =%u ", (int)deadSpace, (int)avgDeadSpace, (int)DCVStrd[1], (int)DCVStrd[0], DeCodeVal);
					if (dletechar)
					{
						// printf("true;  ");
						for (int i = 0; i < sizeof(tmpbuf); i++)
						{
							tmpbuf[i] = PrntBuf[i];
							if (tmpbuf[i] == 0)
								break;
						}
						sprintf(PrntBuf, "%s; Delete;  ", tmpbuf);
					}
					else
					{
						// printf("false;  ");
						// sprintf(PrntBuf, "%s; NO Delete;  ",PrntBuf);
					}
					//			printf(PrntBuf);
					// sprintf(PrntBuf, "%s ConcatSymbl = ",PrntBuf);//printf("\ConcatSymbl = ");
					//			printf(PrntBuf);
					if (ConcatSymbl)
					{
						// printf("true");
						for (int i = 0; i < sizeof(tmpbuf); i++)
						{
							tmpbuf[i] = PrntBuf[i];
							if (tmpbuf[i] == 0)
								break;
						}
						sprintf(PrntBuf, "%s Concat-true\n\r", tmpbuf);
					}
					else
					{
						// printf("false");
						for (int i = 0; i < sizeof(tmpbuf); i++)
						{
							tmpbuf[i] = PrntBuf[i];
							if (tmpbuf[i] == 0)
								break;
						}
						sprintf(PrntBuf, "%s Concat-false\n\r", tmpbuf);
					}
					// if(ConcatSymbl)sprintf(PrntBuf, "true");//printf("true");
					// else sprintf(PrntBuf, "false");//printf("false");
					printf(PrntBuf);
					// USBprintln("");
				}
			}
			else
			{
				// JMH added 20020206
				DeCodeVal += 1; // it appears to be a "dah' so set the least significant bit to "one"
				//			printf("\tYY\t");
				//			USBprintInt(deadSpace);
				//			printf("\t");
				//			USBprintInt(avgDeadSpace);
				//			printf("\t");
				//			USBprintInt(DCVStrd[1]);
				//			printf("\t");
				//			USBprintInt(DCVStrd[0]);
				//			printf("\t");
				//			USBprintInt(DeCodeVal);
				//			printf("\tdletechar = ");
				// if(!Bug3 && !SCD && Test){
				if (Test && 0)
				{
					// sprintf(PrntBuf, "  YY\n\r");
					//				sprintf(PrntBuf, "\tYY\t%d\t%d\t%d\t%d\tDeleteChar = ", (int)deadSpace, (int)avgDeadSpace, (int)DCVStrd[1], (int)DCVStrd[0], (int)DeCodeVal);
					//				//			printf(PrntBuf);
					//				if(dletechar){
					//					//printf("true;  ");
					//					sprintf(PrntBuf, "%strue;  ",PrntBuf);
					//				}
					//				else{
					//					//printf("false;  ");
					//					sprintf(PrntBuf, "%sfalse;  ",PrntBuf);
					//				}
					//				//			printf(PrntBuf);
					//				sprintf(PrntBuf, "%s ConcatSymbl = ",PrntBuf);//printf("\ConcatSymbl = ");
					//				//			printf(PrntBuf);
					//				if(ConcatSymbl){
					//					//printf("true");
					//					sprintf(PrntBuf, "%strue\n\r",PrntBuf);
					//				}
					//				else{
					//					//printf("false");
					//					sprintf(PrntBuf, "%sfalse\n\r",PrntBuf);
					//				}
					// printf(PrntBuf);
				}
			}
			// if(Bug3 & SCD& badLtrBrk) sprintf(DeBugMsg, "1%s", DeBugMsg);
			if (Bug3 && SCD && Test)
			{
				// sprintf(DeBugMsg, "1%s", DeBugMsg);
				int i = 0;
				while (DeBugMsg[i] != 0)
					i++;
				DeBugMsg[i] = '1';
			}
			if (NrmlDah)
			{
				lastDah = period;
				if (DitDahCD != 8)
					CalcAvgDah(lastDah);
			}
			dahcnt += 1;
			if (dahcnt > 10)
			{
				dahcnt = 3;
				avgDit = int(1.5 * float(avgDit));
			}
		}
		else // treat this period as a dit
		{	 // if(period >= 0.5*avgDit) //This is a typical 'Dit'
			if (Test && 0)
			{
				// sprintf(PrntBuf, "  OO\n\r");
				//			sprintf(PrntBuf, "\tOO\t%d\t%d\t%d\t%d\tDeleteChar = ", (int)deadSpace, (int)avgDeadSpace, (int)DCVStrd[1], (int)DCVStrd[0], (int)DeCodeVal);
				//			if(dletechar){
				//				//printf("true;  ");
				//				sprintf(PrntBuf, "%strue;  ",PrntBuf);
				//			}
				//			else{
				//				//printf("false;  ");
				//				sprintf(PrntBuf, "%sfalse;  ",PrntBuf);
				//			}
				//			//			printf(PrntBuf);
				//			sprintf(PrntBuf, "%s ConcatSymbl = ",PrntBuf);//printf("\ConcatSymbl = ");
				//			//			printf(PrntBuf);
				//			if(ConcatSymbl){
				//				//printf("true");
				//				sprintf(PrntBuf, "%strue\n\r",PrntBuf);
				//			}
				//			else{
				//				//printf("false");
				//				sprintf(PrntBuf, "%sfalse\n\r",PrntBuf);
				//			}
				// printf(PrntBuf);
			}

			// if(Bug3 & SCD& badLtrBrk) sprintf(DeBugMsg, "0%s", DeBugMsg);
			if (Bug3 && SCD && Test)
			{
				// sprintf(DeBugMsg, "0%s", DeBugMsg);
				int i = 0;
				while (DeBugMsg[i] != 0)
					i++;
				DeBugMsg[i] = '0';
			}
			// if(FrstSymbl && ((DeCodeVal & 2) == 0)){// FrstSymbl can only be true if we are in Bug3 mode; if (DeCodeVal & 2) == 0, then the last symbol of the preceeding letter was a 'dit'
			if (FrstSymbl)
			{ // JMH 2020103 New way, doesn't make any difference about the last symbol of the preceding letter. The first symbol in the current letter is a 'dit', so forget the past
				// if we're here then we have recieved 2 'dits' back to back with a large gap between. So assume this is the begining of a new letter/character
				// put everything back to decoding a 'normal' character
				DeCodeVal = DeCodeVal >> 1;
				//        Serial.print(DeCodeVal);
				//        Serial.print('\t');
				//        Serial.println( DicTbl1[linearSearchBreak(DeCodeVal, CodeVal1, ARSIZE)] );
				DeCodeVal = 2;
				dletechar = false;
				FrstSymbl = false;
				ConcatSymbl = false;
				//        USBprintln("RESET");
			}
			if ((period != 0) && !FrstSymbl)
			{
				lastDit = period;
				// lastDit1= period;
			}

			dahcnt = 0;
			if (DeCodeVal != 2)
			{ // don't recalculate speed based on a single dit (it could have been noise)or a single dah ||(curRatio > 5.0 & period> avgDit )
				CalcDitFlg = true;
				int curvar = avgDit - period;
				if (curvar < 0)
					curvar = period - avgDit;
				DitVar = ((7 * DitVar) + curvar) / 8;
				//        Serial.print("DitVar: ");
				//        Serial.println(DitVar);
			}
		}
		xSemaphoreGive(DeCodeVal_mutex);
		blocked = false;
		FrstSymbl = false;
		exitCD += 50;
		// if (xSemaphoreGiveFromISR(DeCodeVal_mutex, &xHigherPriorityTaskWoken) == pdTRUE)
		// {
		// 	if (xHigherPriorityTaskWoken != pdFALSE )
		// 	{
		// 		char BTbuf[50] = "xHigherPriorityTaskWoken != pdFALSE\r\n";
		// 		printf(BTbuf);
		// 		portYIELD_FROM_ISR();
		// 	}
		// 	return; // ok, we are done with evaluating this usable key event
		// }

		xSemaphoreGive(DeCodeVal_mutex);
		blocked = false;
		return; // ok, we are done with evaluating this usable key event
	}
	
} // End CW Decoder Interrupt routine
////////////////////////////////////////////////////////////////////////////////////////////
void Dcodeloop(void)
{
	int BtnWaitCnt = 1; // had been 10
	int oldpy = 0;
	int oldpx = 0;
	while(blocked){
		vTaskDelay(2);
	}
	/* See if we can obtain the semaphore.  If the semaphore is not available wait 10 ticks to see if it becomes free. */
	if (xSemaphoreTake(DeCodeVal_mutex,portMAX_DELAY) == pdTRUE)
	{
		/* We were able to obtain the semaphore and can now access the shared resource. */

		ChkDeadSpace();
		SetLtrBrk();
		if (CalcDitFlg)
		{ // moved this from interrupt routine to reduce time needed to process interupts
			CalcDitFlg = false;
			avgDit = (5 * avgDit + lastDit) / 6;
			curRatio = 1.5 * ((float)(avgDah + avgDeadSpace) / (float)(avgDit + avgDeadSpace));
		}
		/* The following if statement is for diagnostic testing/evaluation of the ESP32 KEYISR task */
		if (Prtflg)
		{
			Prtflg = false;
			if (LstPstdchar[0] == NULL)
				LstPstdchar[0] = 0x20; // ASSCII "space" chacacter/value
			// sprintf(PrntBuf, "lastDit %d; Period %d; Tstperiod %d; exitCD %d; DitDahCD %d; Shrt %d; Long1 %d; Ratio %f\n\r", (int)lastDit, (int)period, (int)Tstperiod, exitCD, DitDahCD, Shrt, Long1, Ratio);
			// sprintf(PrntBuf, "lastDit %d; Period %d; exitCD %d; DitDahCD %d; Shrt %d; Long1 %d; Ratio %f %s\n\r", (int)lastDit, (int)period, exitCD, DitDahCD, Shrt, Long1, Ratio, LstPstdchar);
			sprintf(PrntBuf, "Period %d; exitCD %d; DitDahCD %d; Shrt %d; Long1 %d; Ratio %f %s\n\r", (int)period, exitCD, DitDahCD, Shrt, Long1, Ratio, LstPstdchar);
			printf(PrntBuf);
			if (LstPstdchar[0] != 0x20)
				LstPstdchar[0] = 0x20; // only want to print the decoded output once
			exitCD = 0;
			DitDahCD = 0;
		}

		/*BlackPill Touch Screen Code; Not needed for ESP32 version*/
		/* 		LpCnt++;
				if(LpCnt>14000){
					LpCnt = 0;
					readResistiveTouch();
					if (tp.z < 150){
						btnPrsdCnt = 0;
						ClrBtnCnt = 0;
						buttonEnabled = true;
					}
				} else{
					tp = {0, 0, 0};
					PlotIfNeed2();
				} */

		// 		if (tp.z > 150) { // if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE) { //
		// 			//use the following for Screen orientation set to 1
		// 			py = map(tp.y, TS_TOP, TS_BOT, 0, scrnHeight);
		// 			px = map(tp.x, TS_LEFT, TS_RT, 0, scrnWidth);
		// 			/* Un-comment diagnostic test/verification of screen touch coordinates */
		// 			tft.drawFastVLine(oldpx, oldpy-5, 10, TFT_BLACK);
		// 			tft.drawFastHLine(oldpx-5, oldpy, 10, TFT_BLACK);
		// 			tft.drawFastVLine(px, py-5, 10, TFT_RED);
		// 			tft.drawFastHLine(px-5, py, 10, TFT_RED);
		// 			oldpy = py;
		// 			oldpx = px;
		// //			if (px > Stxl && px < Stxh && py > ylo && py < yhi && buttonEnabled) { //&& !SwMode User has touched a point inside the status area, & wants to change WPM/avgtiming
		// 			if (px > 0 && px < 160 && py > 270 && py < 320 && buttonEnabled) {
		// 			//Note: SwMode gets reset to "false" in 'ShowSpeed' routine, after user quits pressing the status area of the screen.

		// 				if (btnPrsdCnt < BtnWaitCnt) { // press screen 'debounce' interval
		// 					btnPrsdCnt += 1;
		// 					// USBprintIntln(btnPrsdCnt);
		// 				}
		// 				else { // user pressed screen long enough to indicate real desire to change/flip the status info
		// 					if(btnPrsdCnt == BtnWaitCnt){
		// 						buttonEnabled = false;
		// 						statsMode++;
		// 						if(statsMode >3) statsMode = 0;//if(statsMode >2) statsMode = 0;
		// 						showSpeed();
		// 					}
		// 					else btnPrsdCnt = BtnWaitCnt;
		// 				}

		// 			}
		// 		}//end tft display "touch" check

		// if (px > 163 && px < 237 && py > 270 && py < 320 && buttonEnabled) {
		//  //reset (clear) the Screen
		// 	SetRtnflg = false;
		// 	SftReset();

		// } else if (px > 163 && px < 237 && py > 270 && py < 320 && !buttonEnabled && !SetRtnflg)
		// 	// so need to test if user presses long enough to signal change to go to menu setup mode
		// {
		// 	ClrBtnCnt += 1;
		// 	//delay(1);
		// 	if(ClrBtnCnt == 5){ //the user has pressed the clear button long enough to go into "setup mode"
		// 		setuploop();// BtnSprt Function
		// 		SftReset();
		// 	}
		// } else if (px > 239 && px < 325 && py > 270 && py < 320 && buttonEnabled) {
		// 	px = 0; //kill button press
		// 	py = 0; //kill button press
		// 	// USBprintIntln("Mode Button");
		// 	if (btnPrsdCnt < BtnWaitCnt) { // press screen 'debounce' interval
		// 		btnPrsdCnt += 1;
		// 		// USBprintIntln(btnPrsdCnt);
		// 	}
		// 	else { // user pressed screen long enough to indicate real desire to change Mode (normal/bug1/bug2/bug3)
		// 		if(btnPrsdCnt == BtnWaitCnt){
		// 			buttonEnabled = false;
		// 			ModeCnt += 1;
		// 			if (ModeCnt == 4) ModeCnt = 0;//if (ModeCnt == 4) ModeCnt = 0;
		// 			//ModeCntRef = ModeCnt;
		// 			SetModFlgs(ModeCnt);
		// 			enableDisplay();
		// 			ModeBtn();//Button2();
		// 		}
		// 		else btnPrsdCnt = BtnWaitCnt;
		// 	}
		// }/////////////////////////////////////////////
		// else if (px > 327 && px < 405 && py > 270 && py < 320 && buttonEnabled) {
		// 			px = 0; //kill button press
		// 			py = 0; //kill button press
		// 			if (btnPrsdCnt < BtnWaitCnt) { // press screen 'debounce' interval
		// 				btnPrsdCnt += 1;
		// 				// USBprintIntln(btnPrsdCnt);
		// 			}
		// 			else { // user pressed screen long enough to indicate real desire to change Tone Speed(Slow/Fast)
		// 				if(btnPrsdCnt == BtnWaitCnt){
		// 					buttonEnabled = false;
		// 					LongSmplFlg += 1;
		// 					if (LongSmplFlg == 2) LongSmplFlg = 0;
		// 					//ModeCntRef = ModeCnt;
		// 					//SetModFlgs(ModeCnt);
		// 					enableDisplay();
		// 					DrawButton2();//Button2();
		// 				}
		// 				else btnPrsdCnt = BtnWaitCnt;
		// 			}
		// 		}
		//////////////////////////////////////////////
	
		chkChrCmplt();
		/* We have finished accessing the shared resource.  Release the semaphore. */
		// blocked = false;
		// if(KEISRwaiting){
		// 	vTaskDelay(4);
		// }
		xSemaphoreGive(DeCodeVal_mutex);
	}
	while (CodeValBuf[0] > 0)
	{
		if (Test)
		{
			sprintf(PrntBuf, "codebuf: %d\n\r", (int)CodeValBuf[0]);
			printf(PrntBuf);
		}
		DisplayChar(CodeValBuf[0]);
		// RfshFlg = 0;
	}
} // end of Main Loop
//////////////////////////////////////////////////////////////////////////
void WPMdefault(void)
{
	avgDit = 80.0; // average 'Dit' duration
	avgDeadSpace = avgDit;
	avgDah = 240.0;
	AvgLtrBrk = avgDah;
	wpm = CalcWPM(avgDit, avgDah, avgDeadSpace);
}

void ChkDeadSpace(void)
{
	if (CalcAvgdeadSpace)
	{ // Just detected the start of a new keydown event
		CalcAvgdeadSpace = false;
		//    printf(DeCodeVal);
		if (NuWrd)
			lastWrdVal = pdTICKS_TO_MS(xTaskGetTickCount()) - WrdStrt; //(GetTimr5Cnt()/10) - WrdStrt;
		if ((deadSpace > 15) && (deadSpace < 240) && (!NuWrd))
		{ // looks like we have a legit dead space interval(its between 5 & 50 WPM)
			//      if (Bug2) {
			//        if (deadSpace < avgDit && deadSpace > avgDit / 4) {
			//        	avgDeadSpace = (15 * avgDeadSpace + deadSpace) / 16;
			//        }
			//      }else if(Bug2){
			if (Bug2)
			{
				if (deadSpace < avgDit)
					avgDeadSpace = (15 * avgDeadSpace + deadSpace) / 16;
			}
			else
			{
				if ((deadSpace < lastDah) && (DeCodeVal != 1))
				{ // 20200817 added "DeCodeVal != 1" for more consistent letter break calcs
					// if (DeCodeVal != 1) { //ignore letterbrk dead space intervals
					if (ltrCmplt < -350)
					{ // ignore letterbrk dead space intervals
						// avgDeadSpace = (3 * avgDeadSpace + deadSpace) / 4;
						avgDeadSpace = (7 * avgDeadSpace + deadSpace) / 8;
					}
					else
					{
						if (Bug2)
							AvgLtrBrk = ((9 * AvgLtrBrk) + avgDit) / 10;
						else
							AvgLtrBrk = ((9 * AvgLtrBrk) + deadSpace) / 10;
					}
				}
				//        printf("\t");
				//        printf(deadSpace);
				//        printf("\t");
				//        printf(avgDeadSpace);
				if (NrmMode && (avgDeadSpace < avgDit))
				{ // running Normall mode; use Dit timing to establish minmum "space" interval
					if (ltrCmplt < -350)
						avgDeadSpace = ((8 * avgDeadSpace) + deadSpace) / 9; // 20221105 changed from avgDit to ; //ignore letterbrk dead space intervals
																			 //          printf("  Path 3: ");
																			 //          printf(avgDeadSpace);
				}
			}
		}
		//    printf("\n\r");
		//    printf("; ");
		//    printf(avgDeadSpace);
		//    printf("; ");
		//    printf(avgDit);
		//    printf("; ");
		//    printf(avgDah);
		//    printf("; ");
		//    printf(ltrBrk);
		//    printf("; ");
		//    printf(AvgLtrBrk);
		//    printf("; ");
		//    printf(wordBrk);
		//    printf("; ");
		//    printf(lastWrdVal);
		//    printf("; ");
		//    printf(NuWrd);
		//    printf("; ");
		//     USBprintIntln(DeCodeVal);
		if (NuWrd)
			NuWrdflg = true;
		;
	}
}

///////////////////////////////////////////////////////////////////////
void SetLtrBrk(void)
{
	unsigned long ltrBrka;
	unsigned long ltrBrkb;
	char tmpbuf[150];
	if (!LtrBrkFlg)
		return;
	// Just detected the start of a new keyUp event
	LtrBrkFlg = false;
	if (Bug3)
	{
		if (LtrCntr < 9)
			LtrCntr++;
		for (int i = 9; i > 0; i--)
		{
			ShrtBrk[i] = ShrtBrk[i - 1];
		}
	}
	// Figure out how much time to allow for a letter break
	// sprintf(PrntBuf,"?:");
	if (Bug2)
	{
		space = ((9 * space) + (3 * avgDeadSpace)) / 10;
	}
	else
	{
		if (avgDeadSpace > avgDit)
		{
			space = avgDeadSpace; //((3*space)+avgDeadSpace)/4; //20190717 jmh - Changed to averaging space value to reduce chance of glitches causing mid character letter breaks
								  // sprintf(PrntBuf,"+");
		}
		else
		{
			space = ((3 * space) + avgDit) / 4; // 20190717 jmh - Changed to averaging space value to reduce chance of glitches causing mid character letter breaks
			// sprintf(PrntBuf,"-");
		}
	}

	if (wpm < 35)
	{
		ltrBrk = int(1.5 * (float(space))); // 20221106 went from 1.6 back to 1.5//20221105 went from 1.5 back to 1.6//20221022 went from 1.4 back to 1.5  // 20210410 went from 1.5 back to 1.4 // 20200306 went from 1.6 back to 1.5 to reduce the chance of having to deal with multi letter symbol groups
		if (BugMode)
		{										// use special case spacing
			ltrBrk = int(1.0 * (float(space))); // 20221022 assume this is part of a dit series
			// sprintf(tmpbuf,"%s",PrntBuf);
			// sprintf(PrntBuf,"%s!:", tmpbuf);
			if (((DeCodeVal & 1) == 1) && (DeCodeVal > 3))
			{										// this dead space interval appears to be an mid-character event AND the last symbol detected was a "DAH".
				ltrBrk = int(2.3 * (float(space))); // int(1.8*(float(space)));
				// sprintf(tmpbuf,"%s",PrntBuf);
				// sprintf(PrntBuf,"%sA:", tmpbuf);
			}
			else if ((deadSpace < wordBrk) && (DeCodeVal == 3))
			{ // the first symbol sent is a dash
				// At this point(time) the letter looks like a "T", but it could be the beginning of something else;i.e., "N"
				ltrBrka = long(float(avgDah) * 0.90);
				ltrBrkb = long(float(avgDeadSpace) * 1.5); // 20221026 changed x factor from 2.1 to 2.5
				if (ltrBrka >= ltrBrkb)
				{ // hold on, new ltrBrk interval seems short
					ltrBrk = ltrBrka;
					// sprintf(tmpbuf,"%s",PrntBuf);
					// sprintf(PrntBuf,"%sB:", tmpbuf);
				}
				else
				{
					ltrBrk = ltrBrkb;
					// sprintf(tmpbuf,"%s",PrntBuf);
					// sprintf(PrntBuf,"%sC:", tmpbuf);
				}
			}
		}
		else if (Bug2)
		{
			ltrBrk = int(2.0 * (float(avgDit))); // int(1.6*(float(avgDit)));
		}
	}
	// if here, setup "letter break" timing for speeds greater than 35 wpm
	else
	{
		ltrBrk = ((8 * ltrBrk) + (0.95 * avgDah)) / 9;
	}

	if (ltrBrk > wordBrk)
	{
		wordBrk = int(1.1 * float(ltrBrk));
	}
	/*Uncomment the following when diagnosing letter break timing */

	// sprintf(tmpbuf,"%s",PrntBuf);
	// sprintf(PrntBuf,"%s\t%lu; %lu; %lu; %lu; %lu\n\r ", tmpbuf, ltrBrk, SpaceStk[Bitpos-1], space, avgDeadSpace, avgDah);
	//	printf(PrntBuf);
	letterBrk = ltrBrk + noSigStrt; // set the next letter break "time stamp"

	// if (BugMode) letterBrk = letterBrk + 0.8 * avgDit ;//20200306 removed to reduce having to deal with multi letter sysmbols
	if (NuWrd && NuWrdflg)
	{
		NuWrd = false;
		NuWrdflg = false;
	}
}
////////////////////////////////////////////////////////////////////////
void chkChrCmplt(void) {
	state = 0;
	unsigned long Now = pdTICKS_TO_MS(xTaskGetTickCount());//(GetTimr5Cnt()/10);
	if((Now - letterBrk1) > 35000) letterBrk1 = Now - 10000;// keep "letterBrk1" from becoming an absurdly large value
	//check to see if enough time has passed since the last key closure to signify that the character is complete
	if ((Now >= letterBrk) && letterBrk != 0 && DeCodeVal > 1) {
		state = 1; //have a complete letter
		ltrCmplt = -3000;
		letterBrk1 = letterBrk;
		/*testing only; comment out when running normally*/
//		if((Now-letterBrk)>1){
//		sprintf(PrntBuf,"Ltr:");
//		sprintf(PrntBuf,"%s %d %d;\n\r ", PrntBuf, (Now-letterBrk), letterBrk);
//		printf(PrntBuf);
//		}
	}
	float noKeySig = (float)(Now - noSigStrt);
	if ((noKeySig >= 0.75 * ((float)wordBrk) ) && noSigStrt != 0 && !wordBrkFlg && (DeCodeVal == 0)) {
		//printf(wordBrk);
		//printf("\t");
		state = 2;//have word

		wordStrt = noSigStrt;
		if ( DeCodeVal == 0) {
			noSigStrt =  pdTICKS_TO_MS(xTaskGetTickCount());//(GetTimr5Cnt()/10);//jmh20190717added to prevent an absurd value
			//noSigStrt =  micros();
			MaxDeadTime = 0;
			charCnt = 0;
		}
		wordBrkFlg = true;

	}

	// for testing only
	//  if(OldDeCodeVal!=DeCodeVal){
	//    OldDeCodeVal=DeCodeVal;
	//    printf(DeCodeVal);
	//    printf("; ");
	//  }

	if (state == 0) {
		if ((unsigned long)noKeySig > MaxDeadTime && noKeySig < 7 * avgDeadSpace ) MaxDeadTime = (unsigned long)noKeySig;
		return;
	}
	else {

		if (state >= 1) { //if(state==1){

			if (DeCodeVal >= 2) {
				int i = 0;
				while (CodeValBuf[i] > 0) {
					++i;// move buffer pointer to 1st available empty array position
					if (i == 7) { // whoa, we've gone too far! Stop, & force an end to this madness
						i = 6;
						CodeValBuf[i] = 0;
					}
				}
				CodeValBuf[i] = DeCodeVal;
				/*
        printf(CodeValBuf[i]);// for testing only
        printf(";  i:");
         USBprintIntln(i);
				 */
				int p = 0;
				//for (int p = 0;  p < Bitpos; p++ ) { // map timing info into time buffer (used only for debugging
				while(p < 16) {
					if(p < Bitpos){
						TimeDatBuf[p] = TimeDat[p];
						SpcIntrvl[p] = SpaceStk[p];
					}else{
						TimeDatBuf[p] = 0;
						SpcIntrvl[p] = 0;
					}

					TimeDat[p] = 0; // clear out old time data
					SpaceStk[p] = 0;
					p++;
				}
			}
			//      printf("DeCodeVal = ");  USBprintIntln(DeCodeVal);
			if (state == 2) {
				int i = 0;
				while (CodeValBuf[i] > 0) ++i; // move buffer pointer to 1st available empty array position
				CodeValBuf[i] = 255; //insert space in text to create a word break
				NuWrd = true;
			}
			TDBptr = Bitpos;
			Bitpos = 0;

			letterBrk = 0;
			++charCnt;
			DeCodeVal = 0; // make program ready to process next series of key events
			period = 0;//     before attemping to display the current code value
		}
	}
}
//////////////////////////////////////////////////////////////////////


int CalcAvgPrd(unsigned long thisdur) {

  //if(magC<10000) return thisdur; //JMH 02020811 Don't do anything with this period. Signal is too noisy to evaluate
	if(!GudSig) return thisdur;
	if( SNR < 4.0)  return thisdur; //JMH 02021004 Don't do anything with this period. Signal is too noisy to evaluate
  //if (thisdur > 3.4 * avgDit) thisdur = 3.4 * avgDit; //limit the effect that a single sustained "keydown" event can have
//  Serial.print(thisdur);
  int fix = 0;
  bool UpDtDeadSpace = true;
  BadDedSpce = 0;
  //Serial.print('\t');
  if(DeCodeVal < 4){ // we're testing the first symbol in a letter and the current dead space is likely a word break or letter break
    if(deadSpace > 700){// huge gap & occurred between this symbol and last. Could be a new sender. Reset and start over
      BadDedSpceCnt = 0;
      //Serial.print(deadSpace);
      //USBprintln("  Start Over");
      return thisdur;
    }
    UpDtDeadSpace = false;
    BadDedSpce = deadSpace;
    deadSpace = avgDeadSpace;
    if((curRatio < 2.0) && (float(3600/avgDah)< 14)&& !Bug2){// curRatio < 2.5
   	/*We're running at a slow WPM but the dit to dah ratio is looking "whaky". Let's speed things up*/
       BadDedSpceCnt = 0;
       avgDah = thisdur;
//       sprintf(TmpMthd,"%s",DahMthd);
//       sprintf(DahMthd,"%s A:%d ;",TmpMthd, avgDah);
       avgDit = avgDah/3;
//       if(avgDit <63){
//    	   avgDit +=0;
//       }
       //Serial.print("SpdUp");
    }
    else{

      BadSpceStk[BadDedSpceCnt] = BadDedSpce;
      BadDedSpceCnt +=1;
      //AvgBadDedSpce = (2*AvgBadDedSpce+BadDedSpce)/3;
      if(BadDedSpceCnt == 5){// we've been down this path 3 times in a row, so something is wrong; time to recalibrate
         BadDedSpceCnt = 0;
         UpDtDeadSpace = true;
          //deadSpace = AvgBadDedSpce;
         // out of the last three space intervals find the shortest two and average their intervals and use that as the current "deadSpace" value
         if((BadSpceStk[2]>BadSpceStk[3])&& (BadSpceStk[2]>BadSpceStk[4])) deadSpace = (BadSpceStk[3]+BadSpceStk[4])/2;
         else if((BadSpceStk[3]>BadSpceStk[2])&& (BadSpceStk[3]>BadSpceStk[4])) deadSpace = (BadSpceStk[2]+BadSpceStk[4])/2;
         else if((BadSpceStk[4]>BadSpceStk[2])&& (BadSpceStk[4]>BadSpceStk[3])) deadSpace = (BadSpceStk[3]+BadSpceStk[3])/2;
         LastdeadSpace = deadSpace;
         if (thisdur < 2*deadSpace){// This "key down" interval looks like a dit
          if(3*thisdur > 1.5*avgDah){//Well, we thought was a dit, if we treat it as such, it will cause a seismic shift downward. So let's proceed with caution
            avgDah = 1.5*avgDah;
//            sprintf(TmpMthd,"%s",DahMthd);
//            sprintf(DahMthd,"%s B;", TmpMthd);
            avgDit = avgDah/3;
//            if(avgDit <63){
//            	avgDit +=0;//
//            }
            //Serial.print("???");
          }
          else{
          avgDah = 3*thisdur;
//          sprintf(TmpMthd,"%s",DahMthd);
//          sprintf(DahMthd,"%s C;", TmpMthd);
          avgDit = thisdur;
//          if(avgDit <63){
//          	avgDit +=0;//
//          }
          //Serial.print("NDT");// "Not a Dit"
          }
         }
         else{// This "key down" interval looks like a Dah
          avgDah = thisdur;
//          sprintf(TmpMthd,"%s",DahMthd);
//          sprintf(DahMthd,"%s D;", TmpMthd);
          avgDit = thisdur/3;
//          if(avgDit <63){
//          	avgDit +=0;//
//          }
         }
         //Serial.print('#');
         //Serial.print(BadSpceStk[4]);
      } else{

        //Serial.print(BadDedSpce);
        //Serial.print('\t');
        //Serial.print(avgDeadSpace);
//        Serial.print("\tNuWrd = ");
//        if(NuWrd)USBprintln("true");
//        else USBprintln("false");
//        Serial.print("\t1st");
      }
    }
  }
  else{ //DecodeVal >= 4; we're analyzing Symbol timing of something other than 'T' or 'E'
    if(thisdur > 0.7 * deadSpace){
      //Serial.print("Mid");// this is pretty commom path
      BadDedSpceCnt = 0;
    }
    else{
      if (((deadSpace > 2.5*LastdeadSpace)||(deadSpace < avgDeadSpace/4)) && (LastdeadSpace != 0) ){
        UpDtDeadSpace = false;
        BadDedSpce = deadSpace;
        deadSpace = avgDeadSpace;
        BadSpceStk[BadDedSpceCnt] = BadDedSpce;
        BadDedSpceCnt +=1;
        //AvgBadDedSpce = (2*AvgBadDedSpce+BadDedSpce)/3;
        if(BadDedSpceCnt == 3){// we've been down this path 3 times in a row, so something is wrong; time to recalibrate (never see this side go true)
           BadDedSpceCnt = 0;
           UpDtDeadSpace = true;
           //deadSpace = AvgBadDedSpce;
           if((BadSpceStk[0]>BadSpceStk[1]) && (BadSpceStk[0]>BadSpceStk[2])) deadSpace = (BadSpceStk[1]+BadSpceStk[2])/2;
           else if((BadSpceStk[1]>BadSpceStk[0]) && (BadSpceStk[1]>BadSpceStk[2])) deadSpace = (BadSpceStk[0]+BadSpceStk[2])/2;
           else if((BadSpceStk[2]>BadSpceStk[0]) && (BadSpceStk[2]>BadSpceStk[1])) deadSpace = (BadSpceStk[0]+BadSpceStk[1])/2;
           //Serial.print('%');
        }
      }else{
        //Serial.print('!');// this occasionally happens
        BadDedSpceCnt = 0;
      }
    }


  }
  //use current deadSpace value to see if thisdur is a dit
  if(thisdur < 1.5 * deadSpace && thisdur > 0.5 * deadSpace ){//Houston, we have a "DIT"
	int olddit = avgDit;//just for debugging
	if(!Bug2) avgDit = (5 * avgDit + thisdur) / 6; //avgDit = (3 * avgDit + thisdur) / 4;
//    if(avgDit <63){
//    	avgDit +=0;
//    }
    fix += 1;
  }
  /* lets try to use the current deadSpace value to see if thisdur is a dah*/
  else if(thisdur < 1.5 *3* deadSpace && thisdur > 1.0 *3* deadSpace ){ //lets try to use the current deadSpace value to see if thisdur is a dah
    //it sure smells like a DAH
    avgDah = (5 * avgDah + thisdur) / 6; //avgDah = (3 * avgDah + thisdur) / 4;
//    sprintf(TmpMthd,"%s",DahMthd);
//    sprintf(DahMthd,"%s E:%d/%d;", TmpMthd, thisdur, deadSpace);
    fix += 2;
  }
  else // doesn't fit either of the above cases, so lets try something else; the following tests rarely get used
  {
    if (thisdur > 2 * avgDah) thisdur = 2 * avgDah; //first, set a max limit to aviod the absurd
    if (thisdur > avgDah){
      avgDah = ((2*avgDah)+thisdur)/3;
//      sprintf(TmpMthd,"%s",DahMthd);
//      sprintf(DahMthd,"%s F:%d;", TmpMthd, thisdur);
      fix += 3;
    }
    else if (thisdur < avgDit){
      avgDit = ((2*avgDit)+thisdur)/3;
//      sprintf(TmpMthd,"%s",DahMthd);
//      sprintf(DahMthd,"%s H:%d;", TmpMthd, 3*avgDit);
//      if(avgDit <63){
//      	avgDit +=0;
//      }
      fix += 4;
    }
    else {// this duration is somewhere between a dit & a dah
      if (thisdur > avgDah/2){
        avgDah = ((12*avgDah)+thisdur)/13;
//        sprintf(TmpMthd,"%s",DahMthd);
//        sprintf(DahMthd,"%s G:%d;", TmpMthd, thisdur);
        fix += 5;
      }
      else{
        avgDit = ((9*avgDit)+thisdur)/10;
//        sprintf(TmpMthd,"%s",DahMthd);
//        sprintf(DahMthd,"%s I:%d;", TmpMthd, 3*avgDit);
//        if(avgDit <63){
//        	avgDit +=0;
//        }
        fix += 6;
      }
    }
  }
  if(UpDtDeadSpace) LastdeadSpace = deadSpace;


  //Serial.print('\t');
  //Serial.print(thisdur);
  //Serial.print('\t');
  //Serial.print(deadSpace);
  if(!UpDtDeadSpace){
    //Serial.print('*');
    //Serial.print(BadDedSpce);
  }
  //Serial.print('\t');
  //Serial.print(avgDeadSpace);
  //Serial.print('\t');
  //Serial.print(avgDit);
  //Serial.print('\t');
  //Serial.print(avgDah);
  //Serial.print('\t');
  //Serial.print(float(3600/avgDah));
  curRatio = (float)avgDah / (float)avgDit;
  if(curRatio <2.5 && Bug3){
    curRatio = 2.5;
    avgDit = avgDah/curRatio;
//    if(avgDit <63){
//    	avgDit +=0;
//    }
     //Serial.print('*');
  }
  //Serial.print('\t');
  //Serial.print(curRatio);
  //Serial.print('\t');
  //Serial.print(DeCodeVal);
  //Serial.print('\t');
  //Serial.println(fix);
    //set limits on the 'avgDit' values; 80 to 3.1 WPM
  if (avgDit > 384) avgDit = 384;
  if ((avgDit < 15)  && (wpm>35)) avgDit = 15;
  if (DeCodeVal == 1) DeCodeVal = 0;
  //      if(Test){
  //        Serial.print(DeCodeVal);
  //        Serial.print(";  ");
  //        Serial.println("Valid");
  //      }
  //thisdur = avgDah / curRatio;
  return thisdur;
}

/////////////////////////////////////////////////////////////////////

int CalcWPM(int dotT, int dahT, int spaceT) {
	int avgt = (dotT+ dahT+ (2*spaceT))/6;//20221105
	int codeSpeed = 1200 /avgt;
//	sprintf(PrntBuf,"WPM: %d; %d; %d; %d; %s\n\r", codeSpeed, dotT, dahT, spaceT, DahMthd);//test/check timing only
//	Serial.print(PrntBuf);
//	sprintf(DahMthd," ");
  return codeSpeed;
}

///////////////////////////////////////////////////////////////////////

void CalcAvgDah(unsigned long thisPeriod) {
  if (NuWrd || !GudSig) return; //{ //don't calculate; period value is not reliable following word break
  avgDah = int((float(9 * avgDah) + thisPeriod) / 10);
//  sprintf(TmpMthd,"%s",DahMthd);
//  sprintf(DahMthd,"%s H:%d;", TmpMthd, thisPeriod);

}
/////////////////////////////////////////////////////////////////////////
void SetModFlgs(int ModeVal) {
  switch (ModeVal) {
    case 0:
      BugMode = false;
      Bug2 = false;
      NrmMode = true;
      Bug3 = false;
      break;
    case 1:
      BugMode = false;
      Bug2 = false;
      NrmMode = true;
      if(!Bug3){
        ShrtBrkA = ltrBrk;
        letterBrk1 = pdTICKS_TO_MS(xTaskGetTickCount())-100;//(GetTimr5Cnt()/10)-100;//initialize "letterBrk1" so the first "KeyEvntSR()" interrupt doesn't generate an absurd value
        LtrCntr = 0;
        ShrtFctr = 0.48;
        AvgShrtBrk = ShrtBrkA;//initialize  AvgShrtBrk with a reasonable value
      }
      Bug3 = true;

      break;
    case 2:
      BugMode = true;
      NrmMode = false;
      Bug3 = false;
      break;
    case 3:
      BugMode = false;
      Bug2 = true;
      NrmMode = false;
      Bug3 = false;
      break;
  }
}
//////////////////////////////////////////////////////////////////////

void DisplayChar(unsigned int decodeval) {
	char curChr = 0 ;
	int pos1 = -1;
	int ChrCntFix = 0;
	

	if (decodeval == 2 || decodeval == 3) ++TEcnt;
	else TEcnt = 0;
	if (Test && !Bug3){
		// sprintf(PrntBuf, "%d\t", (int)decodeval);
		// printf(PrntBuf);
	}
	//clear buffer
	for ( int i = 0; i < sizeof(Msgbuf);  ++i )
		Msgbuf[i] = 0;
	if(decodeval != 99999){//99999 is the 'end of File' code; So we see this, there is nothing left to process
		/* prepare to transfer inter-symbol space intervals to next buffer */
		int SymblLen = 0;
		int GlitchCnt = 0;
		int GlitchCnt1 = 0;
		int p1 = 0;
		/* But 1st, test space timing for glitches*/
		for(p1=1; p1 < 16; p1++) { //ignore the space interval found in the 1st position
			if(SpcIntrvl[p1] == 0) break;
			if((SpcIntrvl[p1] <= 6)) GlitchCnt++;
		}
		if((GlitchCnt <= 4) && (GlitchCnt != 0)){ //glitch(s) found; try to recover
			/*use TimeDatBuf to rebuild the decodeval*/
			/*1st combine/merge TimeDatBuf values where/when "glitches" occurred" */
			unsigned long tempbuf[16];
			unsigned long tempSpcBuf[16];
			for(int p = 0; p <16; p++ ) {// make sure the temp buffers are "0" out
				tempbuf[p] = 0;
				tempSpcBuf[p] = 0;
			}
			int p = 0;
			int offset = 0;
			tempbuf[p] = TimeDatBuf[p];
			tempSpcBuf[p] = SpcIntrvl[p];
			/*Merge "key-down" timing and space timing*/
			for(p = 1; p <p1; p++ ) {
				if(SpcIntrvl[p]>6) tempSpcBuf[p+offset] = SpcIntrvl[p];
				else offset--;
				tempbuf[p+offset] += TimeDatBuf[p];
			}

			/*now rebuild "decodeval" using new timing data*/
			decodeval = 1;
			p=0;
			while(tempbuf[p] !=0){
				decodeval = decodeval << 1;
				if(tempbuf[p]> 2*avgDit) decodeval +=1;
				p++;
			}
			/* copy back new space timing */
			for(int p=0; p < 16; p++) {
				SpcIntrvl[p] = tempSpcBuf[p];
			}
			GlitchCnt1 = GlitchCnt;
			GlitchCnt = 0;
		}
		DCVStrd[0] = decodeval;
		for(int p=0; p < 16; p++) {
			if((SpcIntrvl[p] <= 6)&& (SpcIntrvl[p] != 0) && (p !=0)) GlitchCnt++; //ignore the first "space" value. It could be anything
			if((SpcIntrvl[p] == 0) && (SymblLen == 0)) SymblLen = p;
			SpcIntrvl1[p] = SpcIntrvl[p];
		}
		if(GlitchCnt == 0){ //Ok, we appear to have a usable/valid value
			bool DpScan = false; //"DpScan" = Deep Scan
			if(!Bug2 && !Bug3) DpScan = true;
			/* if "true", we're not operating in one of the "Bug" timing modes
			 * so use both dictionaries to find a match to the current decodeval
			 */
			if(Srch4Match(decodeval, DpScan )<0){// note: if Srch4Match() is > 0, then the msgbuf is automatically loaded with the corresponding ASCII value
				/*Didn't find a match, so use "Keyup" timing to
				 * break decodeval down into 2 parts,
				 * & then see if match can be found
				 */
				//Msgbuf[0] = 0; //clear buffer
				char TmpBufA[10];
				char TmpBufB[10];
				if(DeleteID == 4) DeleteID = 2; //probably going to get a multi character string, so to ignore at least one delete 
				for(int p =0; p <10; p++){
					Msgbuf[p] = 0; //erase buffer
					TmpBufA[p] = 0;
					TmpBufB[p] = 0;
				}

				int EndSPntr = 0; //End Symbol Pointer
				unsigned long maxSpcT = 0; //max symbol Space time
				for(int p=1; p <= SymblLen; p++) {
					if(SpcIntrvl1[p]> maxSpcT){
						maxSpcT = SpcIntrvl1[p];
						EndSPntr = p;
					}
				}
				decodeval = DCVStrd[0]>>(SymblLen-EndSPntr);
				if(Srch4Match(decodeval, true)<0){
					Msgbuf[0] = 0; //clear buffer
					////sprintf( TmpBuf, "%s?%d?", Msgbuf, GlitchCnt1);
					//if(TonPltFlg ||Test)  //sprintf( TmpBufA, "?%d?", GlitchCnt1);
					//else   sprintf( TmpBufA, "");
				}/* else{
					if(TonPltFlg ||Test)  //sprintf( TmpBufA, "{%s}", Msgbuf);
					else //sprintf( TmpBufA, "%s", Msgbuf);
				} */
				/*now construct the 2nd decodeval */
				int DcdVal2 = (1<<((SymblLen-EndSPntr)));
				int Mask =0;
				int MaskWidth = SymblLen-EndSPntr;
				/* create a 1's bit mask the width of the 2nd part of where the apparent pause in sending occured */
				while (MaskWidth>0){
					Mask = (Mask<<1)+1;
					MaskWidth--;
				}
				DcdVal2 += (DCVStrd[0]& Mask);
				for(int p =0; p <8; p++) Msgbuf[p] = 0; //erase buffer
				if(Srch4Match(DcdVal2, true)<0){
					//Msgbuf[0] = 0; //clear buffer
					//if(TonPltFlg ||Test) //sprintf( Msgbuf, "%s#%d#", TmpBufA, GlitchCnt1);
					//else  //sprintf( Msgbuf, "%s",  TmpBufA);
					DCVStrd[0] = 0;
					for(int p=0; p < 16; p++) {
						SpcIntrvl1[p] = 0;
						SpcIntrvl[p] = 0;
					}
				}else{
					/* it appears that a legitimate decodeval was found, so use this data for potential concatenation with the next symbol set */
					//if(TonPltFlg ||Test) //sprintf(TmpBufB, "[%s]", Msgbuf);
					//else //sprintf(TmpBufB, "%s", Msgbuf);
					//sprintf( Msgbuf, "%s%s", TmpBufA, TmpBufB);
					//ChrCntFix = 1;
					while( TmpBufA[ChrCntFix] != 0) ++ChrCntFix; // record how many characters will be printed in this group
					DCVStrd[0] = DcdVal2;
					for(int p=0; p < 16; p++) {
						if(p+EndSPntr<16) SpcIntrvl1[p] = SpcIntrvl[p+EndSPntr];
						else SpcIntrvl1[p] = 0;
					}
				}

			}else{
				/*a single character or characters was found */
			}
		}else{
			//sprintf( Msgbuf, "!");
			DCVStrd[0] = 0;
			for(int p=0; p < 16; p++) {
				SpcIntrvl1[p] = 0;
				SpcIntrvl[p] = 0;
			}
		}
		/* count & store the number characters now in the "MsgBuf" */
		MsgChrCnt[0] = 0;
		while(Msgbuf[MsgChrCnt[0]] != 0) ++MsgChrCnt[0]; // record how many characters will be printed in this group
		MsgChrCnt[0] -= ChrCntFix;
		/*(normally 1, but the extended dictionary can have many)
		 * Will use this later if deletes are needed
		 */
		ConcatSymbl = false;
		if (Msgbuf[0] == 'E' || Msgbuf[0] == 'T') ++badCodeCnt;
		else if(decodeval !=255) badCodeCnt = 0;
		if (badCodeCnt > 5 && wpm > 25){ // do an auto reset back to 15wpm
			WPMdefault();
		}
	}else{
		//sprintf( Msgbuf, "");
		Msgbuf[0] = 0;
		dletechar = true;
		DeleteID = 3;
	}
	/*Not needed for ESP32*/
	// if (((cnt) - offset)*fontW >= displayW) {
	// 	//if ((cnt -(curRow*LineLen) == LineLen)){
	// 	curRow++;
	// 	offset = cnt;
	// 	cursorX = 0;
	// 	cursorY = curRow * (fontH + 10);
	// 	//tft.setCursor(cursorX, cursorY); // in the ESP32 version this will be handled else where
	// 	if (curRow + 1 > row) scrollpg(); // its time to Scroll the Text up one line
	// }
	//sprintf ( Msgbuf, "%s%c", Msgbuf, curChr );
	dispMsg(Msgbuf); // print current character(s) to LCD display
	int avgntrvl = int(float(avgDit + avgDah) / 4);
	
	wpm = CalcWPM(avgDit, avgDah, avgDeadSpace);//use all current time intervalls to extract a composite WPM
	if (wpm != 1) { //if(wpm != lastWPM & decodeval == 0 & wpm <36){//wpm != lastWPM
		if (curChr != ' ') showSpeed();
		if (TEcnt > 7 &&  curRatio > 4.5) { // if true, we probably not waiting long enough to start the decode process, so reset the dot dash ratio based o current dash times
			avgDit = avgDah / 3;
		}
	}
	// slide all values in the CodeValBuf to the left by one position & make sure that the array is terminated with a zero in the last position
	for (int i = 1; i < 7; i++) {
		CodeValBuf[i - 1] = CodeValBuf[i];
	}
	CodeValBuf[6] = 0;
}
//////////////////////////////////////////////////////////////////////
int Srch4Match(unsigned int decodeval, bool DpScan)
{
	int pos1 = linearSearchBreak(decodeval, CodeVal1, ARSIZE); // note: decodeval '255' returns SPACE character
	if(pos1<0 && DpScan){// did not find a match in the standard Morse table. So go check the extended dictionary
		pos1 = linearSearchBreak(decodeval, CodeVal2, ARSIZE2);
		if(pos1<0) sprintf( Msgbuf, "*");
		else{
			char TmpBufA[10];
			for(int i = 0; i < sizeof(TmpBufA);i++){
				TmpBufA[i] =  Msgbuf[i];
				if(Msgbuf[i] == 0) break;
			}
			sprintf( Msgbuf, "%s%s", TmpBufA, DicTbl2[pos1] );
		}
		//if(pos1 >=0) sprintf( Msgbuf, "%s%s", Msgbuf, DicTbl2[pos1] );
		//else sprintf( Msgbuf, "%s*", Msgbuf);
	}else sprintf( Msgbuf, "%s", DicTbl1[pos1] ); //sprintf( Msgbuf, "%s%s", Msgbuf, DicTbl1[pos1] );
	return pos1;
}
//////////////////////////////////////////////////////////////////////
int linearSearchBreak(long val, unsigned int arr[], int sz)
{
  int pos = -1;
  for (int i = 0; i < sz; i++)
  {
    if (arr[i] == val)
    {
      pos = i;
      break;
    }
  }
  return pos;
}

//////////////////////////////////////////////////////////////////////
int linearSrchChr(char val, char arr[ARSIZE][2], int sz)
{
  int pos = -1;
  //Serial.print(val);
  //Serial.print(";\t");
  for (int i = 0; i < sz; i++)
  {
    char tstchar = arr[i][0];
    //Serial.print(tstchar);

    if(tstchar==val){
      pos = i;
      break;
    }
  }
  return pos;
}

//////////////////////////////////////////////////////////////////////
/* the following function "posts" decoded characters to the LCD Display */
void dispMsg(char Msgbuf[50]) {
	
	if (Test ){
		sprintf(PrntBuf, "\t\tdispMsg: \t%s\n\r", Msgbuf);
		printf(PrntBuf);
	}
	int msgpntr = 0;
	int xoffset = 0;
	char info[25];
	char tmpbuf[2];
	// sprintf(info, "");
	info[0] = 0;
	
	while ( Msgbuf[msgpntr] != 0) {
		if((Msgbuf[msgpntr]==' ') & dletechar){
			dletechar = false;
			if(Bug3  && SCD && Test){
				sprintf(PrntBuf, "Space Based Skip; DeleteID: %d\n\r", DeleteID);
				printf(PrntBuf);
			}
		}

		/*TODO for the ESP32 version, need to rework this 'if' clause */
		if((DeleteID == 2) && (dletechar)){
			dletechar = false;
			if(Bug3  && SCD && Test){
				sprintf(PrntBuf, "DeleteID: %d Delete Cancelled \n\r", DeleteID);
				printf(PrntBuf);
			}
			DeleteID = 0;
		} 
		if(dletechar){ // we need to erase the last displayed character
			dletechar = false;
			/* For the ESP32 version, Commented the following out */
			if(Bug3 && SCD && Test){
				//sprintf( info, "*Replace Last Char*"); //Serial.print("Replace Last Char :");
			}
			while(MsgChrCnt[1] !=0){// delete display of ever how many characters were printed in the last decodeval (may be more than one letter generated)
				/*for diagnosit testing*/
				// switch (DeleteID)
				// {
				// case 0:
				// 	tmpbuf[0] = 0x30;
				// 	break;
				// case 1:
				// 	tmpbuf[0] = 0x31;
				// 	break;
				// case 2:
				// 	tmpbuf[0] = 0x32;
				// 	break;
				// case 3:
				// 	tmpbuf[0] = 0x33;
				// 	break;			
				// }
				tmpbuf[0] = 0x8; // ASCII symbol for "Backspace"
				tmpbuf[1] = 0;
				ptrmsgbx->dispMsg(tmpbuf, TFT_GREENYELLOW);
				// //first,buzz thru the pgbuf array until we find the the last charater (delete the last character in the pgbuf)
				// int TmpPntr = 0;
				// while(Pgbuf[TmpPntr]!=0) TmpPntr++;
				// if(TmpPntr>0) Pgbuf[TmpPntr-1] =0;// delete last character in the array by replacing it with a "0"
				// cnt--;
				// xoffset = cnt;
				// curRow = 0;
				// while (xoffset >= CPL){
				// 	xoffset -=CPL;
				// 	curRow++;
				// }
				// cursorX  =  xoffset*(fontW);
				// cursorY = curRow * (fontH + 10);
				// if(xoffset==(CPL-1)) offset= offset-CPL; //we just jump back to last letter in the previous line, So we need setup to properly calculate what display row we will be on, come the next character
				// //tft.fillRect(cursorX, cursorY, fontW+4, (fontH + 10), BLACK); //black out/erase last displayed character
				// //tft.setCursor(cursorX, cursorY);
				--MsgChrCnt[1];
			}
			DeleteID = 0;

		}//end delete character
		else{
			//tft.setCursor(cursorX, cursorY);// In the ESP32 version this will be handle elsewhere
		}


		MsgChrCnt[1] =MsgChrCnt[0];
		DCVStrd[1] = DCVStrd[0];// used by the KeyEvntSR()routine to facilitate grabbing back the last key sequence data received
		for(int p=0; p < 16; p++) {
			SpcIntrvl2[p] = SpcIntrvl1[p];
		}
		char curChar = Msgbuf[msgpntr];
		if(LstPstdchar[0] == 0x20 ) LstPstdchar[0] = curChar;
		tmpbuf[0] = curChar;
		tmpbuf[1] = 0;
		/*TODO for ESP32 version, need to rework the tft.print(curChar)*/
		//tft.print(curChar);
		/* ESP32 version print to screen method, */
		// if(xSemaphoreTake(mutex, portMAX_DELAY) ==pdTRUE){
               ptrmsgbx->dispMsg(tmpbuf, TFT_GREENYELLOW);
        //       xSemaphoreGive(mutex);
        //     } 
		
		//now test/correct letter groups that represent common mis-prints
		if (curRow > 0){
			char tmpbuf[440];
			for(int i = 0; i<440; i++){
				tmpbuf[i] = Pgbuf[i];
				if(Pgbuf[i] == 0) break;
			}
			tmpbuf[439] =0;
			sprintf ( Pgbuf, "%s%c", tmpbuf, curChar);  // add the character just "printed" to the "PgBuf"
		} 

		if (cnt>CPL){//Pgbuf now has enough data, to test for special character combos often found with sloppy sending
			/**/
			if(Pgbuf[cnt-(CPL+1)]== '@'  && Pgbuf[cnt-(CPL)]=='D'){ //test for "@"
				sprintf ( Msgbuf, " (%c%s", Pgbuf[cnt-(CPL+2)], "AC)"); //"true"; Insert preceeding character plus correction "AC"
			}


			if(Pgbuf[cnt-(CPL+1)]== 'P'  && Pgbuf[cnt-(CPL)]=='D'){ //test for "PD"
				sprintf ( Msgbuf, " (%c%s", Pgbuf[cnt-(CPL+2)], "AND)"); //"true"; Insert preceeding character plus correction "AND"
			}
			if(Pgbuf[cnt-(CPL+1)]== '6'  && Pgbuf[cnt-(CPL)]=='E'){ //test for "6E"
				sprintf ( Msgbuf, " (%c%s", Pgbuf[cnt-(CPL+2)], "THE)"); //"true"; Insert preceeding character plus correction "THE"
			}
			if(Pgbuf[cnt-(CPL+1)]== '6' && Pgbuf[cnt-(CPL)]=='A'){ //test for "6A"
				sprintf ( Msgbuf, " (%c%s", Pgbuf[cnt-(CPL+2)], "THA)"); //"true"; Insert preceeding character plus correction "THA"
			}
			if(Pgbuf[cnt-(CPL+1)]== '9' && Pgbuf[cnt-(CPL)]=='E'){ //test for "9E"
				sprintf ( Msgbuf,  " (ONE)" ); //"true"; Insert correction "ONE"
			}
			if(Pgbuf[cnt-(CPL+2)]=='P'&& Pgbuf[cnt-(CPL+1)]=='L' && Pgbuf[cnt-(CPL)]=='L'){ //test for "PLL"
				sprintf ( Msgbuf, " (WELL)" ); //"true"; Insert correction "WELL"
			}
			if((Pgbuf[cnt-(CPL+2)]=='N' || Pgbuf[cnt-(CPL+2)]=='L') && Pgbuf[cnt-(CPL+1)]=='M'  && Pgbuf[cnt-(CPL)]=='Y'){ //test for "NMY/LMY"
				sprintf ( Msgbuf, " (%c%s", Pgbuf[cnt-(CPL+2)], "OW)"); //"true"; Insert correction "NOW"/"LOW"
			}
			if(Pgbuf[cnt-(CPL+2)]=='T'&& Pgbuf[cnt-(CPL+1)]=='T' && Pgbuf[cnt-(CPL)]=='O'){ //test for "PD"
				sprintf ( Msgbuf, "  (0)"); //"true"; Insert correction "TTO" = "0"
			}

		}
		//recalculate maximum wait interval to splice decodeval
		if(Bug3 && curChar != ' '){//if(Bug3 & (cnt>CPL) & (Pgbuf[cnt-(CPL)]!=' ') ){ //JMH 20200925 with current algorithm, no longer need to wait for "Pgbuf" to become active
			//if ((ShrtBrk[LtrCntr] > 1.5*ShrtBrkA) & (ShrtBrk[LtrCntr] <3* ltrBrk)){
			//if ((ShrtBrk[LtrCntr] > 1.5*ltrBrk) & (ShrtBrk[LtrCntr] <3* ltrBrk)& (info[0] == 0)){
			//if ((ShrtBrk[LtrCntr] < 1.2*ltrBrk) & (ShrtBrk[LtrCntr]> ShrtBrkA)& (info[0] == 0)){ // this filter is based on W1AW sent code
			ltrBrk = (60*UsrLtrBrk)/100; //Jmh 20200925 added this to keep "ltrBrk" at a dynamic/reasonable value with respect to what the sender is doing
			if( ShrtBrk[LtrCntr]< UsrLtrBrk){// we're working with the last letter received was started before a normal letter break period
				//        if(Pgbuf[cnt-(CPL+1)]=='T'&& Pgbuf[cnt-(CPL)]=='T'){ // we have 2 'T's in a row so increase the "ShrtFctr" a bit
				//          if(ShrtBrk[LtrCntr]> ltrBrk) ShrtFctr = float(float(ShrtBrk[LtrCntr])/float(UsrLtrBrk));
				//          else ShrtFctr = float(float(ltrBrk)/float(UsrLtrBrk));

				ShrtFctr = float(float(80*ltrBrk)/float(100*UsrLtrBrk));
				ShrtBrkA = (80*UsrLtrBrk)/100; //ShrtBrkA = (76*UsrLtrBrk)/100; //ShrtBrkA = (88*UsrLtrBrk)/100; //ShrtBrkA = (90*UsrLtrBrk)/100; //ShrtBrkA =  ShrtFctr*UsrLtrBrk;
				//          AvgShrtBrk = ShrtBrkA;

				//        }

				//          ShrtFctr += 0.05;
				//          if(Pgbuf[cnt-(CPL+2)]=='T') {// Whoa, Now we have 3 T's in a row, recalibrate the timing settings
				//           float ShrtFctrold = ShrtFctr;
				//           ShrtFctr = float(float(ShrtBrk[LtrCntr])/float(UsrLtrBrk));
				//           if(ShrtFctr < ShrtFctrold) ShrtFctr = ShrtFctrold;// the new value should have been greater than what we had been using, but not true, so "bumb up" the old value a bit es use it
				//           //ShrtFctr = 2.0*float(float(AvgShrtBrk)/float(UsrLtrBrk));
				//           AvgShrtBrk = ShrtBrk[LtrCntr];
				//           ShrtBrkA =  ShrtFctr*UsrLtrBrk;
				//          }
				//        }
			}

			if ((ShrtBrk[LtrCntr] < 0.6*wordBrk) && curChar != ' ' && (info[0] == 0)){ //if ((ShrtBrk[LtrCntr] < 0.6*wordBrk) & (ShrtBrk[LtrCntr]> ShrtBrkA)& (info[0] == 0)){ // this filter is based on Bug sent code
				UsrLtrBrk = (5*UsrLtrBrk+ShrtBrk[LtrCntr])/6; //(6*UsrLtrBrk+ShrtBrk[LtrCntr])/7; //(9*UsrLtrBrk+ShrtBrk[LtrCntr])/10;
				//ShrtBrkA =  ShrtFctr*UsrLtrBrk;//0.45*UsrLtrBrk;
				//} else if((info[0] != 0)&(ShrtBrk[LtrCntr]<ShrtBrkA/2) ){// we just processed a spliced character
			} //else if((info[0] == '*')){// we just processed a spliced character

			//}

		}
		if(Bug3 && SCD && Test){
			char info1[150];
			//sprintf(info1, "{%s}\t%s",DeBugMsg, info);
			if(info[0] == '*') info[0] = '^';//change the info to make it recognizable when the replacement characters are part the same group
			char str_ShrtFctr[6];
			//sprintf(str_ShrtFctr,"%d.%d", int(ShrtFctr), int(1000*ShrtFctr));
			char Ltr;
			if(cnt < CPL){
				Ltr = curChar;
			}else Ltr = Pgbuf[cnt-(CPL)];
			//sprintf(DeBugMsg, "%d %d \t%c%c%c %d/%d/%d/%d/%s   \t%d/%lu\t  \t%lu\t%d \t%s \n\r",LtrCntr, ShrtBrk[LtrCntr],'"', Ltr,'"', ShrtBrkA, ltrBrk, wordBrk, UsrLtrBrk, str_ShrtFctr, cursorX, cursorY,  cnt, xoffset, info1);
			//printf(DeBugMsg);
			//LtrCntr = 0;
		}
		OldLtrPntr = LtrCntr;
		int i = 0;
		while((DeBugMsg[i] != 0) && (i<150) ){
			DeBugMsg[i] = 0;
			i++;
		}
		msgpntr++;
		cnt++;
		/*Test to see if we just reached the end of the current display line*/
		if ((cnt - offset)*fontW >= displayW) {
			curRow++;
			cursorX = 0;
			cursorY = curRow * (fontH + 10);
			offset = cnt;
			//tft.setCursor(cursorX, cursorY);//for esp32 version this will be handled elsewhere.
			if (curRow + 1 > row) {
				scrollpg();
			}
		}
		else{
			cursorX = (cnt - offset) * fontW;
			//newRow = false;
		}

	}
	LtrCntr = 0;
	ChkDeadSpace();
	SetLtrBrk();
	chkChrCmplt();
}
//////////////////////////////////////////////////////////////////////
void scrollpg() {
/* for the ESP32 BT Keybrd & CW decoder version this function will be addressed elesewhere
	SO commented out references to 'tft.' to keep the esp32 compiler happy*/	
  //buttonEnabled =false;
  curRow = 0;
  cursorX = 0;
  cursorY = 0;
  cnt = 0;
  offset = 0;
  //enableDisplay(); // Not needed forESP32. This is a Blackpill/TouchScreen thing
  //tft.fillRect(cursorX, cursorY, displayW, row * (fontH + 10), BLACK); //erase current page of text
  //tft.setCursor(cursorX, cursorY);
  while (Pgbuf[cnt] != 0 && curRow + 1 < row) { //print current page buffer and move current text up one line
    ChkDeadSpace();
    SetLtrBrk();
    chkChrCmplt();
    //tft.print(Pgbuf[cnt]);
//    if (displayW == 480) Pgbuf[cnt] = Pgbuf[cnt + 40]; //shift existing text character forward by one line
//    else Pgbuf[cnt] = Pgbuf[cnt + 27]; //shift existing text character forward by one line
    Pgbuf[cnt] = Pgbuf[cnt + CPL]; //shift existing text character forward by one line
    cnt++;
    //delay(300);
    if (((cnt) - offset)*fontW >= displayW) {
      curRow++;
      offset = cnt;
      cursorX = 0;
      cursorY = curRow * (fontH + 10);
      //tft.setCursor(cursorX, cursorY);
    }
    else cursorX = (cnt - offset) * fontW;

  }//end While Loop
  while (Pgbuf[cnt] != 0) { //finish cleaning up last line
    chkChrCmplt();
    //tft.print(Pgbuf[cnt]);
    Pgbuf[cnt] = Pgbuf[cnt + 26];
    cnt++;
  }
}
/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////

void showSpeed(void)
{
  char buf[50];
  char tmpbuf[15];
  char tmpbufA[4];
  int ratioInt = (int)curRatio;
  int ratioDecml = (int)((curRatio - ratioInt) * 10);
  //int SI = (int) SmpIntrl; //un-comment for diagnositic testing only;used to find/display the ADC total sample time
  chkStatsMode = true;
  // if (SwMode && buttonEnabled) SwMode = false;
  switch (statsMode)
  {
  case 0:
	if (AutoTune) sprintf(tmpbuf, "AF");
	else sprintf(tmpbuf, "FF");
	//if(ModeCnt == -1) ModeCnt = 0;
	switch (ModeCnt)
	{
	case 0:
	  sprintf(tmpbufA, "Nrm");
	  break;
	case 1:
	  sprintf(tmpbufA, "Bg1");
	  break;
	case 2:
	  sprintf(tmpbufA, "Bg2");
	  break;
	case 3:
	  sprintf(tmpbufA, "Bg3");
	  break;
	case 4:
	  sprintf(tmpbufA, "OFF");
	  break;
	default:
	  sprintf(tmpbufA, "???");
	  break;     
	}
	DFault.TRGT_FREQ = (int)TARGET_FREQUENCYC;// update the default setting with the current Geortzel center frequency; Can & will change while in the AUTO-Tune mode
	sprintf(buf, "%d/%d.%d WPM FREQ %dHz %s %s", wpm, ratioInt, ratioDecml, int(TARGET_FREQUENCYC), tmpbuf, tmpbufA);// normal ESP32 CW deoder status display
	//sprintf(buf, "SI %dms  FREQ %dHz %s %s", SI, int(TARGET_FREQUENCYC), tmpbuf, tmpbufA); //un-comment for diagnositic testing only;; Shws ADC sample interval
	//sprintf(buf, "SR %d  FREQ %dHz %s %s", (int)SAMPLING_RATE, int(TARGET_FREQUENCYC), tmpbuf, tmpbufA); //un-comment for diagnositic testing only (current sample rate)
	break;
  case 1:
	sprintf(buf, "%lu", avgDit); // sprintf ( buf, "%d", (int)lastDit1);
	for (int i = 0; i < sizeof(tmpbuf); i++)
	{
	  tmpbuf[i] = Pgbuf[i];
	  if (Pgbuf[i] == 0)
				break;
	}
	tmpbuf[sizeof(tmpbuf) - 1] = 0;
	sprintf(buf, "%s/%lu", tmpbuf, avgDah);
	for (int i = 0; i < sizeof(tmpbuf); i++)
	{
	  tmpbuf[i] = Pgbuf[i];
	  if (Pgbuf[i] == 0)
				break;
	}
	tmpbuf[sizeof(tmpbuf) - 1] = 0;
	sprintf(buf, "%s/%lu", tmpbuf, avgDeadSpace);
	break;
  case 2:
	sprintf(buf, "FREQ: %dHz", int(TARGET_FREQUENCYC));
	break;
  case 3:
	ratioInt = (int)SNR;
	ratioDecml = (int)((SNR - ratioInt) * 10);
	sprintf(buf, "SNR: %d.%d/1", ratioInt, ratioDecml);
	break;
  }

  // now, only update/refresh status area of display, if info has changed
  int ptr = 0;
  unsigned long chksum = 0;
  while (buf[ptr] != 0)
  {
	chksum += buf[ptr];
	++ptr;
  }
  if (chksum != bfrchksum)
  {
	ptrmsgbx->showSpeed(buf, TFT_CYAN);
  }
  bfrchksum = chksum;
}
///////////////////////////////////////////////////////////////////////////

void SftReset(void)  //Not called in ESP32 version
{ // Modified for Black Pill
  /*TODO need to rework the commented out lines for ESP32 version*/
  // buttonEnabled = false; //Disable button
  ClrBtnCnt = 0;
  // btnPrsdCnt = 0;
  LpCnt = 13500;
  // enableDisplay(); // Not needed forESP32. This is a Blackpill/TouchScreen thing
  // tft.fillScreen(BLACK);
  // px = 0;
  // py = 0;
  DrawButton2();// commented  out on ESP32 version
  DrawButton();// commented  out on ESP32 version
  ModeBtn();// commented  out on ESP32 version
  WPMdefault();

  showSpeed();
  
  for (int i = 0; i < sizeof(Pgbuf); ++i)
	Pgbuf[i] = 0;
  cnt = 0;
  curRow = 0;
  offset = 0;
  cursorX = 0;
  cursorY = 0;
  return;
}
//////////////////////////////////////////////////////////////////////////

void CLrDCdValBuf(void){
for (int i = 0; i < 7; i++) {
	CodeValBuf[i] = 0;
}
for (int i = 0; i < sizeof(Msgbuf); i++) {
	Msgbuf[i] = 0;
}
for (int i = 0; i < sizeof(DCVStrd); i++) {
	DCVStrd[i] = 0;
}
DeCodeVal = 0;
OldDeCodeVal = 0;
};
/////////////////////////////////////////////////////////////////////////