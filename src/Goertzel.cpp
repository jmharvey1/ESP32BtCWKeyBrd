/* 
 * Goertzel.c
 *
 *  Created on: May 15, 2023
 *      Author: jim (KW4KD)
 */
/*20230729 added calls to DcodeCW SetLtrBrk() & chkChrCmplt() to ensure that the letter break gets refreshed with each ADC update (i.e., every 4ms)*/
/*20240101 Revamped Goertzel code for 8ms sample groups but updating every 4ms 
 * also included changes to passing Keystate & letterbreak timing to DeCodeCW.cpp
 * plus changes to keystate detection (curnois lvl)
 */
/*20240103 added void CurMdStng(int MdStng) primarily to switch in extended symbol set timing while in Bg1 mode*/
/*20240114 added small differential term to noisLvl to improve noise tracking with widely spaced characters
			Added timing link to AdvPaser to imporve FltrPrd timing (glitch protection/rejection)*/
#include <stdio.h>
#include "Arduino.h"
#include "Goertzel.h"
#include "DcodeCW.h"
#include "TFTMsgBox.h"
#define MagBSz  6//3
AdvParser advparser;
uint16_t adc_buf[Goertzel_SAMPLE_CNT];
uint8_t LongSmplFlg =1; // controls high speed sampling VS High Sensitivity ;can not set the value here
uint8_t NuMagData = 0x0;
uint8_t delayLine;

uint8_t LEDRED = 0;
uint8_t LEDGREEN = 0;
uint8_t LEDBLUE = 0;
uint8_t LightLvl = 0;
uint8_t LstLightLvl = 0;
bool Ready = true;
bool toneDetect = false;
bool SlwFlg = false;
bool TmpSlwFlg = false;
bool NoisFlg = false;//In ESP32 world NoisFlg is not used & is always false
bool AutoTune = true; //false; //true;
bool Scaning = false;

int ModeVal = 0;
int N = 0; //Block size sample 6 cycles of a 750Hz tone; ~8ms interval
int NL = Goertzel_SAMPLE_CNT/2;
int NC = 0;
int NH = 0;
int RfshFlg = 0; // used when continuous tone testing is 
//int BIAS = 2040;// not used in ESP32 version; bias value set in main.cpp->GoertzelHandler(void *param)
int TonSig = 0;
int MBpntr=0;
//int SkipCnt =0;
int SkipCnt1 =0;
int KeyState = 0; // used for plotting to show Key "UP" or "DOWN" state
int OldKeyState = 0;//used as a comparitor to test/detect change in keystate
// float feqlratio = 0.97;//BlackPill Ratio////0.958639092;//0.979319546;
// float feqhratio = 1.02;//BlackPill Ratio//1.044029352;//1.022014676;
float feqlratio = 0.95;//ESP32 Ratio////0.958639092;//0.979319546;
float feqhratio = 1.06;//ESP32 Ratio//1.044029352;//1.022014676;
//unsigned long now = 0;
unsigned long NoisePrd = 0;
unsigned long EvntTime = 0;
unsigned long StrtKeyDwn = 0;
unsigned long TmpEvntTime = 0;
float NFlrBase = 0;
float avgKeyDwn = 1200/15;
int NFlrRatio = 0;
uint8_t GlthCnt = 0;
bool StrngSigFLg = false; // used as part of the glitch detection process

uint8_t Sentstate = 1;
bool GltchFlg =false;
volatile unsigned long noSigStrt; //this is primarily used in the DcodeCW.cpp task; but declared here as part of a external/global declaration
volatile unsigned long wordBrk; //this is primarily used in the DcodeCW.cpp task; but declared here as part of a external/global declaration
float TARGET_FREQUENCYC = 750; //Hz// For ESP32 version moved declaration to DcodeCW.h
float TARGET_FREQUENCYL;// = feqlratio*TARGET_FREQUENCYC;//734.0; //Hz
float TARGET_FREQUENCYH;// = feqhratio*TARGET_FREQUENCYC;//766.0; //Hz
//float SAMPLING_RATE = 98750;
float SAMPLING_RATE = 98000;//102000;// based on tests continuous tone tests made 20230714
float Q1;
float Q2;
float Q1H;
float Q2H;
float Q1C;
float Q2C;
float PhazAng;
float coeff;
float coeffL;
float coeffC;
float coeffH;
float magA = 0;
float magB = 0;
float magC = 0;
/*used for continuous tone autotune processing */
bool CTT = false ;// set to true when doing continuous tone diagnostic testing
float magAavg = 0;
float magBavg = 0;
float magCavg = 0;
int MagavgCnt = 50;
int avgCntr = 0;

float magC2 = 0;
float magL = 0;
float magH = 0;
float CurLvl = 0;
float NSR = 0;
int NowLvl = 0;
float OldLvl = 0;
float AdjSqlch= 0;
float OldAvg = 0;
float AvgLo = 0;
float AvgHi = 0;
float AvgVal = 0;
float DimFctr = 1.2;
float TSF = 1.2; //TSF = Tone Scale Factor; used as part of calc to set/determine Tone Signal to Noise ratio; Tone Component
//float NSF = 1.4;//1.6;//1.24; //1.84; //1.55; //2.30;//2.85;//0.64; //NSF = Noise Scale Factor; used as part of calc to set determine Tone Signal to Noise ratio; Noise Component
float NSF = 1.7;//1.8;//2.2;//1.6;
float ClipLvlF = 18000;//based on 20230811 configuration//100000;//150000;//1500000;
float ClipLvlS = 25000;//based on 20230811 configuration

float NoiseFlr = 0;
float SigDect = 0;//introduced primarily for Detecting Hi-speed CW keydown events
float OLDNoiseFlr = 0;
float SigPk =0;
float MagBuf[MagBSz];
float NoisBuf[2*MagBSz];
int NoisPtr = 0;
float noisLvl = 0;
int ClimCnt = 0;
int KeyDwnCnt = 0;
int GData[Goertzel_SAMPLE_CNT];
bool prntFlg; //used for debugging
////////////////////////////////////////
void CurMdStng(int MdStng){
	ModeVal = MdStng;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
void PlotIfNeed2(void){
	if (NuMagData) {
	//if (TonPltFlg && NuMagData) {
		// int PltGudSig = -3000;
		// if(GudSig) PltGudSig = -2200;
		int NFkeystate = -100;
		if(Sentstate == 1) NFkeystate = -1700;//KeyUp
		// if(GltchFlg) PltGudSig = -10000;
		NuMagData = 0x0;
		//int scaledNSR = (int)(NSR*1000000);
		// int scaledNSR = 0;
		// if(NSR>0) scaledNSR = (int)(1000.00/NSR);
		char PlotTxt[200];
		/*Generic plot values*/
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (int)magH, (int)magL, (int)magC, (int)AdjSqlch, (int)NoiseFlr, KeyState, (int)CurNoise);//good for continuous tone testing
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\n", (int)magC, (int)SqlchLvl, (int)NoiseFlr, KeyState, (int)CurNoise, PltGudSig);
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (int)CurLvl, (int)SqlchLvl, (int)NoiseFlr, KeyState, (int)CurNoise, (int)AdjSqlch-500, NFkeystate, PltGudSig);//standard plot display
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (int)noisLvl, (int)CurLvl, (int)SigDect, KeyState, NFkeystate, (int)CurNoise, (int)AdjSqlch-90, ClimCnt);
	
		sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (int)noisLvl, (int)CurLvl, (int)SigDect, KeyState, NFkeystate, (int)CurNoise, ltrCmplt);//standard plot display
		
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", (int)CurLvl, (int)NFlrBase, NFlrRatio, (int)NoiseFlr, KeyState, (int)CurNoise, (int)AdjSqlch-500, NFkeystate);
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\n", (int)now, (int)NoisePrd, (int)avgDit, keystate);
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\n", (int)CurLvl, (int)SqlchLvl, (int)NoiseFlr, KeyState, (int)CurNoise, (int)TARGET_FREQUENCYC);
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\n", (int)magH, (int)magL, (int)magC), (int)0;
		/*Uncomment the following to Study RGB LED tone Light*/
		//sprintf(PlotTxt, "%d\t%d\t%d\t%d\t%d\t%d\n", (int)LightLvl, (int)LEDGREEN, (int)LEDBLUE, (int)LEDRED, (int)NowLvl, (int)ClipLvl);
		//sprintf(PlotTxt, "%d\t%d\n", (int)avgDit, (int)avgKeyDwn);
		printf(PlotTxt);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
void ResetGoertzel(void)
{
  Q2 = 0;
  Q1 = 0;
  Q2C = 0;
  Q1C = 0;
  Q2H = 0;
  Q1H = 0;
  AvgVal = 0;
  TARGET_FREQUENCYL = feqlratio*TARGET_FREQUENCYC;//734.0; //Hz
  TARGET_FREQUENCYH = feqhratio*TARGET_FREQUENCYC;//766.0; //Hz
}

/* Call this once, to precompute the constants. */
void InitGoertzel(void)
{
	int CYCLE_CNT;//, k;// 6.0
	float  omega;
	float CyRadians;
	//float floatnumSamples = (float) (Goertzel_SAMPLE_CNT);
	//if(SlwFlg) floatnumSamples = 2*floatnumSamples;
	ResetGoertzel();// make sure you're working with the current set of frequeincies needed for this set of parameters
	/* For the lowest frequency of interest, Find the Maximum Number of whole Cycles we can look at  */
	if(SlwFlg) CYCLE_CNT = (int)((((float)(2*Goertzel_SAMPLE_CNT))*TARGET_FREQUENCYL)/SAMPLING_RATE);
	else CYCLE_CNT = (int)((((float)(Goertzel_SAMPLE_CNT))*TARGET_FREQUENCYL)/SAMPLING_RATE);
	//floatnumSamples = (float) (Goertzel_SAMPLE_CNT);
    //k = (int) (0.5 + ((floatnumSamples * TARGET_FREQUENCYL) / (float)SAMPLING_RATE)); 
	CyRadians = (2.0 * PI * CYCLE_CNT);
	//omega = (2.0 * PI * k) / floatnumSamples;
	NL = (int)(0.5 +((SAMPLING_RATE / TARGET_FREQUENCYL) * (float)(CYCLE_CNT)));
	omega = CyRadians / (float)NL;
	//coeffL = 2*cos(2*PI*TARGET_FREQUENCYL/SAMPLING_RATE);
	//coeffL = 2*cos(2*PI*(TARGET_FREQUENCYL*(1/SAMPLING_RATE))/NL);
	coeffL = 2*cos(omega);

	NC = (int)(0.5 +((SAMPLING_RATE / TARGET_FREQUENCYC) * (float)(CYCLE_CNT)));
	//k = (int) (0.5 + ((floatnumSamples * TARGET_FREQUENCYC) / (float)SAMPLING_RATE)); 
	//CyRadians = (2.0 * PI * CYCLE_CNT);
	//omega = (2.0 * PI * k) / floatnumSamples;
	omega = CyRadians / (float)NC;
	//coeffC = 2*cos(2*PI*TARGET_FREQUENCYC/SAMPLING_RATE);
	//coeffC = 2*cos(2*PI*(TARGET_FREQUENCYC*(1/SAMPLING_RATE))/NC);
	coeffC = 2*cos(omega);

	NH = (int)(0.5 +((SAMPLING_RATE / TARGET_FREQUENCYH) * (float)(CYCLE_CNT)));
	//k = (int) (0.5 + ((floatnumSamples * TARGET_FREQUENCYH) / (float)SAMPLING_RATE)); 
	//CyRadians = (2.0 * PI * CYCLE_CNT);
	//omega = (2.0 * PI * k) / floatnumSamples;
	omega = CyRadians / (float)NH;
	//coeffH = 2*cos(2*PI*TARGET_FREQUENCYH/SAMPLING_RATE);
	//coeffH = 2*cos(2*PI*(TARGET_FREQUENCYH*(1/SAMPLING_RATE))/NH);
	coeffH = 2*cos(omega);
	
	/* uncomment for Debug/Diagnostic testing*/
	// char buf[50];
    // sprintf(buf, "CYCLE_CNT: %d; SmplCnt: %d; Frq: %4.1f; Coef: %3.3f\n", CYCLE_CNT, NC, TARGET_FREQUENCYC, coeffC );
	// printf( buf);
}

/* Call this routine for every sample. */
void ProcessSample(int sample, int Scnt)
{
	char Smpl[10];
	if(Scnt >= Goertzel_SAMPLE_CNT) GData[Scnt - Goertzel_SAMPLE_CNT] = sample;
	//if(Scnt ==0 ) prntFlg = true; //just used for debugging
	if (Scnt > NL){
		//just used for debugging
		// if(prntFlg){
		// 	prntFlg = false;
		// 	sprintf( Smpl,"%d\n", Scnt);
		// 	printf( Smpl);
		// }
		return; // don't look or care about anything beyond the lowest number of samples needed for this frequency set
	}
	float Q0;
	float FltSampl = (float)sample;
	
	Q0 = (coeffL * Q1) - Q2 + FltSampl;
	Q2 = Q1;
	Q1 = Q0;
	if (Scnt > NC){
		return;
	}
	Q0 = (coeffC * Q1C) - Q2C + FltSampl;
	Q2C = Q1C;
	Q1C = Q0;
	
	if (Scnt > NH)
		return;
	Q0 = (coeffH * Q1H) - Q2H + FltSampl;
	Q2H = Q1H;
	Q1H = Q0;
		
}

/* Optimized Goertzel */
/* Call this after every block to get the RELATIVE magnitude squared. */
float GetMagnitudeSquared(float q1, float q2, float Coeff, int SmplCnt)
{
	float result;
	float SclFctr = (float)SmplCnt/2.0;
	if(SlwFlg) SclFctr = 2*SclFctr;
	// float CyRadians = (2.0 * M_PI * CYCLE_CNT);
	// float omega = CyRadians / (float)floatN;
	// float cosine = cos(omega);
	result = ((q1 * q1) + (q2 * q2) - (q1 * q2 * Coeff))/SclFctr;
	// float real = (q1 - q2 * cosine);
	// float imag = (q2 * sine);
	// result = (real * real) + (imag * imag);
	//PhazAng = 360 * ((atan(real / imag)) / (2 * M_PI));
	return result;
}

void ComputeMags(unsigned long now){
	TmpEvntTime = now;
	// magC = Grtzl_Gain * 4.0*sqrt(GetMagnitudeSquared(Q1C, Q2C, coeffC, NC));
	// magH = Grtzl_Gain * 4.0*sqrt(GetMagnitudeSquared(Q1H, Q2H, coeffH, NH));
	// magL = Grtzl_Gain * 4.0*sqrt(GetMagnitudeSquared(Q1, Q2, coeffL, NL));
	magC = Grtzl_Gain * 10.0*sqrt(GetMagnitudeSquared(Q1C, Q2C, coeffC, NC));
	magH = Grtzl_Gain * 10.0*sqrt(GetMagnitudeSquared(Q1H, Q2H, coeffH, NH));
	magL = Grtzl_Gain * 10.0*sqrt(GetMagnitudeSquared(Q1, Q2, coeffL, NL));
	/*Added the following to preload the current sample set to be combined with next incoming data set*/
	if(SlwFlg){
		//printf( "KK");
		ResetGoertzel();
		for(int i = 0; i <  Goertzel_SAMPLE_CNT; i++){
			ProcessSample(GData[i], i);
		}
	}
	/*End of preload process */
	CurLvl = (magC + magL + magH)/3;
	if(CurLvl<100) CurLvl = NowLvl;// something went wrong use last datapoint
	NowLvl = CurLvl;//'NowLvl will be used later for showing current LED state
	/* ESP32 Plot code to do a simple look at the sampling & conversion process */
    // char buf[20];
    // sprintf(buf, "%d, %d, %d, %d, \n", (int)magH, (int)magL, (int)magC, 0);
	// printf( buf);
    
	/* Find Noisefloor for this recent period */
	// int OldestDataPt = MBpntr;
	// SigDect = MagBuf[OldestDataPt];//get the oldest sample in the data buffer
	MagBuf[MBpntr++] = CurLvl;
	if(MBpntr==MagBSz) MBpntr = 0;
	int OldestDataPt = MBpntr;
	SigDect = MagBuf[OldestDataPt];//get the oldest sample in the data buffer
	OLDNoiseFlr = NoiseFlr;
	NoiseFlr = MagBuf[0];
	SigPk = MagBuf[0];
	
	
	for( int i =1 ; i <MagBSz; i++){
		if(MagBuf[i]<NoiseFlr) NoiseFlr = MagBuf[i];
		if(MagBuf[i]>SigPk) SigPk = MagBuf[i];
		/*look for the smallest magnitude found in the 3 oldest samples; remember we alrady have one, the oldest of the three. So only need to check two more */
		if( i < 2){
			OldestDataPt++;
			if(MagBuf[OldestDataPt] < SigDect) SigDect = MagBuf[OldestDataPt];
		}

	}
	float curNois = (SigPk - NoiseFlr);
	if(curNois>6000) curNois = 6000;//if(curNois>8000) curNois = 8000;
	//printf("%d\n", (int)curNois);
	curNois *= 2.2;//curNois *= 1.8;
	noisLvl = ((35*noisLvl) + curNois)/36;
	/*Now look to see if the noise is increasing
	And if it is, Add a differential term/componet to the noisLvl*/
	int LstNLvlIndx = NoisPtr-1;
	if(LstNLvlIndx < 0) LstNLvlIndx = 2*MagBSz- 1;
	if(noisLvl>NoisBuf[LstNLvlIndx]) {
		noisLvl += 0.3*(noisLvl-NoisBuf[LstNLvlIndx]);
	}
	/*save this noise value to histrory ring buffer for later comparison */
	NoisPtr++;
	if(NoisPtr == 2*MagBSz) NoisPtr = 0;
	NoisBuf[NoisPtr] = noisLvl;
	
	if(NoiseFlr > NFlrBase){
		NFlrRatio = (int)(1000.0*NoiseFlr/NFlrBase);
		if(NFlrRatio > 30000) NFlrRatio = 30000;
		NFlrBase = ((399*NFlrBase)+ NoiseFlr)/400;
		 
	} 
	else NFlrBase = ((19*NFlrBase)+ NoiseFlr)/20;
	NSR = NoiseFlr/SigPk;
	/*Now use the magnitude found six samples back*/
	CurLvl = MagBuf[MBpntr];
	if(CurLvl < 0) CurLvl = 0; 
	magB = ((magB)+CurLvl)/2; //((2*magB)+CurLvl)/3; //(magC + magL + magH)/3; //
	/* try to establish what the long term noise floor is */
	/* This sets the squelch point with/when only white noise is applied*/
	if(!toneDetect)  CurNoise = ((399*CurNoise)+(1.4*SigPk))/400;
	/* now look or figure noise level based on what could be a tone driven result*/
	if ((2*NoiseFlr > CurLvl) && (CurLvl > NoiseFlr) && (NoiseFlr/SigPk >0.5)) // && (SigPk/NoiseFlr>0.5
	{ 
		if(CurNoise < 0.5 * CurLvl) CurNoise = ((7 * CurNoise) + (0.4 * CurLvl))/8;//raise squelch/noise level, based on current "Key down" level
		else if(CurNoise > CurLvl) CurNoise = (19* CurNoise + 1.1*CurLvl)/20; //drop  squelch/noise level, based on current "Key down" level
	}


	/* If we're in a key-down state, and the Noise floor just Jumped up, as a reflection,
	 * of this state, give up the gain in "curnoise" that happened due to the time lag in
	 * noisefloor */
	/* the following should never happen, but if "true" reset noise level to something realistic */
	if(CurNoise<0) CurNoise = (SqlchLvl);
	/* Now try to establish "key down" & "key up" average levels */
	if((NSF*NoiseFlr)>SqlchLvl){
		/* Key Down bucket */
		AvgHi= ((5*AvgHi)+(1.2*magB))/6;//((3*AvgHi)+magC)/4;
	}
	else{
		/* Key Up bucket, & bleed off some of the key down level; but never let it go below the current average low*/
		AvgLo= ((49*AvgLo)+magB)/50;
		if(AvgHi>AvgLo) AvgHi -= 0.0015*AvgHi;
		else if(AvgHi<AvgLo) AvgHi = AvgLo; 
	}
	/* use the two Hi & Lo buckets to establish the current mid point */
	//float Avg = ((AvgHi-AvgLo)/40)+(AvgLo);
	float Avg = ((AvgHi-AvgLo)/2)+(AvgLo);
	/* add this current mid point to the running average mid point */
	SqlchLvl = ((25*SqlchLvl)+Avg)/26;
	if(OldLvl>CurLvl) {
		 SqlchLvl -= 0.2*(OldLvl-CurLvl); //-= 0.5*(OldLvl-CurLvl);
		 //AvgHi = AvgLo;
	}
	if(SqlchLvl<0) SqlchLvl = 0; 
	OldLvl = CurLvl;
	/* Now based on the current key down level see if a differential correction needs to be added into the average */
	/* Finally, make sure the running average is never less than the current noise floor */
	/* removed the following line for esp32; To give a more consistant noise floor to gauge where squelch action needs to take place*/
	
	if(SqlchLvl <=CurNoise){
		/*Test for a decent tone by looking at the current noise floor value, & if true, correct CurNoise (squelch level) value to be below the noisefloor level*/
		//if((NoiseFlr> 0.75*SigPk)&&(CurNoise>4000) && (NoiseFlr>60000)){
		if((NoiseFlr> 0.75*SigPk)&&(CurNoise>4000) && (NoiseFlr>30000)){	
			GudSig = 1;
			SkipCnt1 =0;
		} 
		
		AdjSqlch = CurNoise;
		if(SlwFlg) AdjSqlch = 1.20*AdjSqlch; 
		 
		
	}else{
		AdjSqlch = SqlchLvl;
		GudSig = 1;
		SkipCnt1 =0;
	}
	if(noisLvl < CurNoise) noisLvl = CurNoise;//20231022 added this to lessen the chance that noise might induce a false keydown at the 1st of a letter/character
	AdjSqlch = noisLvl;
	if(AdjSqlch < 1500) AdjSqlch = 1500; //20230814 make sure squelch lvl is always above the "no input signal" level; this stops the decoder from generating random characters when no signal is applied
	if((NoiseFlr>CurNoise)&& ((NoiseFlr/SigPk)>0.65)){
			GudSig = 1;
			SkipCnt1 =0;
		}
	if((NoiseFlr<CurNoise)&& ((SigPk/NoiseFlr)>5.0)){
		SkipCnt1++;
		/* For ESP32,extended countout to 20 to improve WPM speed detection/correction*/
		if(SkipCnt1 >7){
			SkipCnt1 = 7;
			GudSig = 0;
		}
	}
	Ready = true;
	NuMagData = 0x1;
	/* The Following (Commented out) code will generate the CW code for "5" with a Calibrated 31ms "keydown" period */
	//	next1++;
	//	if(next1==2){
	//		next1 =0;
	//		next++;
	//		if(next<20){
	//			if(next % 2 == 0){
	//				toggle1 ^= 1; //exclusive OR
	//				if(toggle1) HAL_GPIO_WritePin(DMA_Evnt_GPIO_Port, DMA_Evnt_Pin, GPIO_PIN_RESET);//PIN_HIGH(LED_GPIO_Port, LED_Pin);
	//				else HAL_GPIO_WritePin(DMA_Evnt_GPIO_Port, DMA_Evnt_Pin, GPIO_PIN_SET);//PIN_LOW(LED_GPIO_Port, LED_Pin);
	//			}
	//		}else if(next>40) next = 0;
	//		else{
	//			HAL_GPIO_WritePin(DMA_Evnt_GPIO_Port, DMA_Evnt_Pin, GPIO_PIN_SET);
	//			toggle1 = 0;
	//		}
	//	}
	/* END Calibrated TEST Code generation; Note when using the above code the Ckk4KeyDwn routine should be commented out */
	Chk4KeyDwn(NowLvl);
}
////////////////////////////////////////////////////////////////////////////
/*Added 'NowLvl' to better sync the LED with the incoming tone */
void Chk4KeyDwn(float NowLvl)
{
	/* to get a Keydown condition "toneDetect" must be set to "true" */
	//float ToneLvl = magB; // tone level delayed by six samples
	bool GudTone = true;
	unsigned long FltrPrd = 0;
	//printf("Chk4KeyDwn\n");
	if (avgDit <= 1200 / 35) // WPM is the denominator here
	{						 /*High speed code keying process*/
		if (SigDect  > AdjSqlch)
		{
			if ((OLDNoiseFlr < NoiseFlr) && (OLDNoiseFlr < CurNoise))
			{
				float NuNoise = 0.5 * (NoiseFlr - OLDNoiseFlr) + OLDNoiseFlr;
				if (NuNoise < NoiseFlr)
					CurNoise = (5*CurNoise+NuNoise)/6;
			}
			toneDetect = true;
			if (!GudTone)
				GudTone = true;
			
		}
		else if(CurLvl < CurNoise)
		{
			toneDetect = false;
		}
	}
	else
	{ /*slow code keying process*/
		if ((CurLvl < 0.6*AdjSqlch) && toneDetect)// toneDetect has the value found in the previous sample (not the current sample)
		{
			GudTone = false;
			toneDetect = false;
		}
		/*Added (SigDect  > 2*AdjSqlch)||, for when in slow speed mode, to recognize a strong Hi-speed CW signal */
		if ( !toneDetect && !Scaning)//if (((NoiseFlr > AdjSqlch)  ) && !toneDetect && !Scaning)
		{
			if(((SigDect  > 2*AdjSqlch)||(NoiseFlr > AdjSqlch))) toneDetect = true;
			if((SigDect > AdjSqlch) && SlwFlg) toneDetect = true; //used to dectect key down state while using 8ms sample interval
		}
	}
	/*initialize in KeyUp state */
	uint8_t state = 1; //Keyup, or "no tone" state
	KeyState = -2000;  //Keyup, or "no tone" state
	
	if(toneDetect)
	{ /** Fast code method */
		KeyState = -400;// Key Down
		state = 0;
		/*go back & get noise lvl from the sample before the keydown signal*/
		int OldestSmpl = NoisPtr+1;
		if(OldestSmpl == 2*MagBSz) OldestSmpl = 0;
		KeyDwnCnt++;
		if(KeyDwnCnt >4){
			KeyDwnCnt--;
			noisLvl = CurNoise;
		} else noisLvl = NoisBuf[OldestSmpl];
		
		OldestSmpl++;
		if(OldestSmpl == 2*MagBSz) OldestSmpl = 0;
		NoisBuf[OldestSmpl] = noisLvl; 

	} else KeyDwnCnt = 0;
	/*now if this last sample represents a keydown state(state ==0), 
	Lets set the CurNoise to be mid way between the lowest curlevel and the NFlrBase value*/
	if(!state){
		float tmpcurnoise;
		if(!SlwFlg) tmpcurnoise = ((CurLvl-NFlrBase)/2) + NFlrBase;
		else tmpcurnoise = 1.0*CurNoise;
		if((tmpcurnoise < CurNoise)&& (tmpcurnoise > 3*NFlrBase) ) CurNoise = tmpcurnoise; 
	}
	/* new auto-adjusting glitch detection; based on average dit length */
	unsigned long Now2 = TmpEvntTime; //set Now2 to the current sampleset's timestamp
	/*Added to support ESP32 CW decoding process*/
	if (OldKeyState != KeyState){
		if(!state) StrtKeyDwn = TmpEvntTime; //if !state and we're here, then we're at the start of a keydown event
		else {//if we're here, we just ended a keydown event
			float thisKDprd = (float)(TmpEvntTime - StrtKeyDwn);
			if((thisKDprd > 1200/60) && (thisKDprd < 1200/5)){ //test to see if this interval looks like a real morse code driven event
				if(thisKDprd > 2.5 *avgKeyDwn) thisKDprd /= curRatio;  //looks like a dah compared to the last 50 entries, So cut this interval by the dit to dah ratio currently found in DcodeCW.cpp
				avgKeyDwn = ((49*avgKeyDwn)+ thisKDprd)/50;
			}
		}
		//just used for debugging
		// char Smpl[10];
		// sprintf( Smpl,"%d\n", 1200/(int)avgDit);
		// printf( Smpl);
		//if((avgDit > 1200 / 35)) avgDit = (unsigned long)advparser.DitIntrvlVal;//20240521 added this becauxe now believe this is a more reliable value
		if((avgDit < 1200 / 30)){//20231031 changed from 1200/30 to 1200/35
			TmpSlwFlg = false; //we'll determine the flagstate here but wont engage it until we have a letter complete state
        }else if(avgDit > 1200 / 28){//20231031 added else if()to auto swap both ways
			TmpSlwFlg = true;//we'll determine the flagstate here but wont engage it until we have a letter complete state
		}
		if(!GltchFlg && (avgDit >= 1200 / 30)){ // don't use "glitch" detect/correction for speeds greater than 30 wpm
			
			/*Some straight keys & Bug senders have unusually small dead space between their dits (and Dahs). 
			When thats the case, use the DcodeCW's avgDeadspace to set the duration of the glitch period */
			//if(FltrPrd > AvgSmblDedSpc/2) FltrPrd = (unsigned long)((AvgSmblDedSpc)/3.0);
			if(AvgSmblDedSpc< advparser.AvgSmblDedSpc){
				FltrPrd = (unsigned long)((AvgSmblDedSpc)/3.0);
			} else FltrPrd = (unsigned long)((advparser.AvgSmblDedSpc)/3.0);
			if(ModeCnt == 3) FltrPrd = 4;//we are running in cootie mode, so lock the glitch inerval to a fixed value of 8ms.
			NoisePrd = Now2 + FltrPrd;
			OldKeyState = KeyState;
			GltchFlg = true;
			EvntTime = TmpEvntTime;//capture/remember when this state change occured
			//printf("SET NoisePrd %d\n", (int)NoisePrd);
			
		} else {//
			NoisePrd = Now2;
			OldKeyState = KeyState;
			GltchFlg = true;
			EvntTime = TmpEvntTime;
			
		}
	}
	if(GltchFlg && !state){
		//20231230  just got a keydown condition but its noisy, so add glitch delay to letter complete interval
		if(FltrPrd > 0 && ModeVal == 1) StrechLtrcmplt(1.25*FltrPrd);
	}
	if ((Now2 <= NoisePrd) && (Sentstate == state))
	{ /*We just had a 'glitch event, load/reset advparser watchdog timer, to enable 'glitch check'*/
		//printf("\nRESET LstGltchEvnt: %d \n", (int)TmpEvntTime);
		advparser.LstGltchEvnt = 10000 + TmpEvntTime; // used by advParser.cpp to know if it needs to appply glitch detection
	}

	if (OldKeyState != KeyState)
	{	
		/*the following never happens*/
		//printf("OldKeyState != KeyState\n");
		if ( Now2 <= NoisePrd) {
			if( GltchFlg){
				/*We had keystate change but it returned back to its earlier state before the glitch interval expired*/
				OldKeyState = KeyState;
				GltchFlg = false;
				GlthCnt++;
				if(GlthCnt >= 3){
				/*3 consectutive glicth fixes in 1 keydown interval; thats too many; readj the avg keydown period */
					avgKeyDwn = avgKeyDwn/2;
					GlthCnt = 0;	
				} 
			}
		}
	} else{
		if (Now2 >= NoisePrd){ 
			if(Sentstate != state)
			{/*We had keystate change ("tone" on, or "tone" off)  & passed the glitch interval; Go evaluate the key change*/
				GltchFlg = false;
				Sentstate = state;//'Sentstate' is what gets plotted
				OldKeyState = KeyState;
				GlthCnt = 0;
				if(1) GudSig = 1;
				//if(GltchFlg) printf("~\n");
				//else  printf("^\n");
				KeyEvntSR(Sentstate, TmpEvntTime);
			}
		} 
	}
	if(Sentstate){ // 1 = Keyup, or "no tone" state; 0 = Keydown
		if(chkChrCmplt() && SlwFlg != TmpSlwFlg){//key is up
			SlwFlg = TmpSlwFlg;
		}
		if(CalGtxlParamFlg){
			CalGtxlParamFlg = false;
          	CalcFrqParams((float)DemodFreq); // recalculate Goertzel parameters, for the newly selected target grequency
		}
	}
	else SetLtrBrk();//key is down; so reset/recalculate new letter-brake clock value
	
	
	// The following code is just to support the RGB LED.
	/*Added 'NowLvl' to better sync the LED with the incoming tone */
	//	if (GudTone){
	//if ((NowLvl > AdjSqlch) && !Scaning)
	float ClipLvl;
	if(SlwFlg) ClipLvl = ClipLvlS;
	else ClipLvl = ClipLvlF;
	if(!state)
	{
		if (CurLvl > ClipLvl)
		{ // Excessive Tone Amplitude
			LEDGREEN = 0xFF;
			LEDRED = 0xFF;
			LEDBLUE = 0xFF;
		} // End Excessive Tone Amplitude
		else
		{ // modulated light tests
			// LightLvl = (uint8_t)(256 * (NowLvl / ClipLvl));
			float curfltval = (float)(256 * (CurLvl / ClipLvl));
			/*Scale the level to work with typical SDR line lvl output*/
			curfltval = 5 * curfltval;
			if (curfltval > 255)
				LightLvl = (uint8_t)255;
			else
				LightLvl = (uint8_t)curfltval;
			LstLightLvl = LightLvl;
			if (CurLvl < 4000)
			{ // Just @ or above noisefloor
				LEDGREEN = LightLvl;
				LEDRED = LightLvl;
				LEDBLUE = LightLvl;
			} // End Just detectable Tone
			else
			{				  // Freq Dependant Lighting
				magC2 = 1.05*magC;//magC; // 
				if ((magC2 >= magH) && (magC2 >= magL))
				{ // Freq Just Right
					LEDGREEN = LightLvl;
					LEDBLUE = 0;
					LEDRED = 0;
				} // End Freq Just Right
				else if ((magH > magC2) && (magH >= magL))
				{ // Freq High
					LEDGREEN = 0;
					LEDBLUE = LightLvl;
					LEDRED = 0;
				} // End Freq High
				else if ((magL > magC2) && (magL >= magH))
				{ // Freq Low
					LEDGREEN = 0;
					LEDBLUE = 0;
					LEDRED = LightLvl;
				} // End Freq Low
			}	  // End Freq Dependant Lighting
		}		  // End modulated light tests
	}			  // end Key Closed Tests
	else
	{ // key open
		LEDGREEN = 0;
		LEDRED = 0;
		LEDBLUE = 0;
		LstLightLvl = 0;
	}

	// this round of sound processing is complete, so setup to start to collect the next set of samples

	/*the following lines support plotting tone processing to analyze "tone in" vs "key state"*/
	if (PlotFlg)
		PlotIfNeed2();
	// if (AutoTune || CTT )
	// 	ScanFreq(); // go check to see if the center frequency needs to be adjusted
	// else
	// 	Scaning = false;
} /* END Chk4KeyDwn(float NowLvl) */


/* Added to support ESP32/RTOS based processing */
uint16_t ToneClr(void)
{
	uint8_t r = LEDRED;
	uint8_t g = LEDGREEN;
	uint8_t b = LEDBLUE;
	uint16_t color = ((r & 0xF8) << 8) | ((g & 0xF8) << 3) | (b >> 3);
	return color;
}
////////////////////////////////////////////////////////////////////////////
void ScanFreq(void)
{
	/*  this routine will start a Sweep the audio tone range by decrementing the Goertzel center frequency when a valid tome has not been heard
	 *  within 4 wordbreak intervals since the last usable tone was detected
	 *  if nothing is found at the bottom frquency (500Hz), it jumps back to the top (900 Hz) and starts over
	 */
	float DltaFreq = 0.0;
	
	unsigned long waitinterval = (60*3*5);//5 * wordBrk// normal wordBrk = avgDah; use for diagnositic continuous tone testing
	float max = 900.0;
	float min = 550.0;
	if(CTT){
		max =1500.0;
		min = 150.0;
	}
	if(!CTT) waitinterval =  5 * wordBrk; // normal wordBrk = avgDah;

	//if(!CTT) magC *= 1.02;  
	if (magC > AdjSqlch)
	{					 // We have a valid tone.
		Scaning = false; // enable the tonedetect process to allow key closer events.
		magAavg += magL;
		magBavg += magC;
		magCavg += magH;
		avgCntr++;
		if(CTT) {
			noSigStrt = pdTICKS_TO_MS(xTaskGetTickCount());// included here just to assure we have recent "No Signal" Marker; needed when a steady tone is applied
			// magAavg += magL;
			// magBavg += magC;
			// magCavg += magH;
			// avgCntr++;
			if( avgCntr > MagavgCnt){
				/*We've collected 50 Goertzel data points. Find their average values*/
				avgCntr = 0;
				magL = magAavg/50;
				magC = magBavg/50;
				magH = magCavg/50;
				magAavg = magBavg = magCavg =0;
			}
			else return; // do nothing more, we dont have a enough data points yet 
		} else
		{ // normal cw mode; Not Continuous tone testing
			if( avgCntr > 10){
				/*We've collected 10 Goertzel data points. Find their average values*/
				avgCntr = 0;
				magL = magAavg/10;
				magC = magBavg/10;
				magH = magCavg/10;
				magAavg = magBavg = magCavg =0;
			}
			else return; // do nothing more, we dont have a enough data points yet 
		}
		//ok, now have an averaded reading
		
		if ((magC > magL) && (magC > magH)){
				if(!CTT) return; // the current center frequency had the best overall magnitude. So no frequency correction needed. Go and collect another set of samples
		}
		if (magH > magL)
				DltaFreq = +1.0;//+0.5;//2.5;
		else if (magL > magH)
				DltaFreq = -1.0;//-0.5;//2.5;
	}
	else
	{ // no signal detected
		if ((pdTICKS_TO_MS(xTaskGetTickCount()) - noSigStrt) > waitinterval) 
		{ // no signal, after what should have been several "word" gaps. So lets make a big change in the center frequency, & see if we get a useable signal
				DltaFreq = -20.0;
				Scaning = true; // lockout the tonedetect process while we move to a new frequency; to prevent false key closer events while in the frequency hunt mode
		}
	}
	if (DltaFreq == 0.0)
		if(!CTT) return; // Go back; Nothing needs fixing
	TARGET_FREQUENCYC += DltaFreq;
	if (TARGET_FREQUENCYC < min) TARGET_FREQUENCYC = max; // start over( back to the top frequency); reached bottom of the frequency range (normal loop when no signals are detected)
	if (TARGET_FREQUENCYC > max) TARGET_FREQUENCYC = min; // not likely to happen. But could when making small frequency adjustments
			 	
	CalcFrqParams(TARGET_FREQUENCYC);// recalculate Goertzel parameters, for the newly selected target grequency
	if(CTT) //
	{ //if we're runing in continuous tone mode, we need to periodically update the display's bottom status line. mainly to show the current frequency value the auto-tune process has selected 
		RfshFlg++;
		if(RfshFlg > 2){
			RfshFlg = 0;
			showSpeed();
			//printf("showSpeed\n");
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
void CalcFrqParams(float NewToneFreq){
  //float NewToneFreq = TARGET_FREQUENCYC + 10.0;
  TARGET_FREQUENCYC = NewToneFreq; //Hz
  TARGET_FREQUENCYL = feqlratio*NewToneFreq; //Hz
  TARGET_FREQUENCYH = feqhratio*NewToneFreq; //Hz
  InitGoertzel();
  /* TODO Display current freq, only if we are running BtnSuprt.cpp setup loop */
  //if(setupflg && !Scaning)ShwUsrParams();
}
//////////////////////////////////////////////////////////////////////////