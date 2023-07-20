/*
 * Goertzel.h
 *
 *  Created on: May 15, 2023
 *      Author: jim (KW4KD)
 */

#ifndef INC_GOERTZEL_H_
#define INC_GOERTZEL_H_
//#define Goertzel_SAMPLE_CNT   158 // @750Hz tone input & ~39Khz sample rate = 52 samples per cycle & 3 cycle sample duration. i.e. ~4ms
#define Goertzel_SAMPLE_CNT 398//402 // 406 // @750Hz tone input & ~101.250Khz sample rate = 135 samples per cycle & 3 cycle sample duration. i.e. ~4ms
extern bool PlotFlg;
extern float SAMPLING_RATE;
extern float Grtzl_Gain;
float GetMagnitudeSquared(float q1, float q2, float Coeff, int SmplCnt);
void ResetGoertzel(void);
void InitGoertzel(void);
void ProcessSample(int sample, int Scnt);
void ComputeMags(unsigned long now);
void Chk4KeyDwn(float NowLvl);
uint16_t ToneClr(void);
void ScanFreq(void);
void CalcFrqParams(float NewToneFreq);

#endif /* INC_GOERTZEL_H_ */