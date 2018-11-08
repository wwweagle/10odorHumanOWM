/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef UTILS_H
#define	UTILS_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h>

int getFuncNumber(int targetDigits, const char* message);
int uartCheck(int respWindow);


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#define SpLick 	 0 	//        !PORTDbits.RD8  
#define SpKey 	 1  	//Keypad #[ZX]
#define SpDebugInfo 2
#define SpIO 3


#define SpFalseAlarm		 4	// false alarm
#define SpCorrectRejection		 5	// correct rejection
#define SpMiss		 6	// miss
#define SpHit		 7 	// hit

#define SpWater 8

#define SpOdor_A    9         
#define SpOdor_B	10

#define SpTrialWait 	 20
#define SpPermInfo   21 
#define SpLickFreq   22

#define SpValHigh   23 
#define SpValLow   24 
#define SpLCD_SET_XY 25
#define SpLCD_Char 26
#define SpChartHigh   27
#define SpChartLow   28
#define SpTaskType         51
#define Sptrialtype     58
#define SpITI           59  // 1 start 0 end

#define SpSess          61  // 1 start 0 end
#define SpTrain         62  // 1 start 0 end
//#define Splaser         65
#define SpOdor_C 66
//#define SpLaserSwitch    79

#define SpResponseCue 83
#define SpAbortTrial 84
#define SpCorrectionCue 85




#define HUMAN_ODPA_SHAPING 87
#define HUMAN_ODPA_TASK 88
#define HUMAN_DNMS_TASK 89







//#define atTrialStart 10
#define at4SecBeforeS1 4
#define at3SecBeforeS1 5
#define at1SecBeforeS1 10
#define at500msBeforeS1 18
#define atS1Beginning 20
#define atS1End 30
#define atDelayBegin 40
#define atDelay500MsIn 42
#define atDelay1SecIn 200
#define atDelay1_5SecIn 205
#define atDelay2SecIn 210
#define atDelay2_5SecIn 212
#define atDelay3SecIn 214
#define atDelay3_5SIn 216
#define atDelay4_5SIn 218
#define atPreDualTask 220
#define atPostDualTask 222
#define atDelay1sToMiddle 224
#define atDelay500msToMiddle 225
#define atDelayMiddle 230
#define atDelayMid2Sec 235
#define atDelayMid2_5Sec 240
#define atDelayMid3Sec 245
#define atDelayLast2_5SecBegin 250
#define atDelayLast2SecBegin 255
#define atDelayLast1_5SecBegin 61
#define atDelayLastSecBegin 63
#define atDelayLast500mSBegin 65
#define atSecondOdorBeginning 70
#define atSecondOdorEnd 80
#define atResponseCueBeginning 90
#define atResponseCueEnd 95

#define atRewardBeginning 100
//#define atRewardBeginning 110
#define atRewardEnd 120


typedef struct {
    volatile unsigned int current;
    volatile unsigned int stable;
    volatile uint32_t filter;
    unsigned int portSide;
    volatile unsigned int LCount;
    volatile unsigned int RCount;
} LICK_T_G2;

typedef struct {
    int *sample1s;
    int *test1s;
    int pairs1Count;
    int sample2;
    int test2;
    int *respCue;
    int respCount;
    int sample1Length;
    int sample2Length;
    int test1Length;
    int test2Length;
    int respCueLength;
    int falsePunish;
    int correctionCue;
    int correctionCueLength;
    int delay1;
    int delay2;
    int delay3;
    int ITI;
    int waitForTrial;
    int minBlock;

} TASK_T;

typedef struct{
    int odorPairs;
    int *samples;
    int *tests;
    int odorlength;
    int delay;
    int ITI;
    int phase;
} TASK_H;

typedef struct {
    unsigned int laserSessionType;
    unsigned int laserTrialType;
    volatile unsigned int timer;
    unsigned int onTime;
    unsigned int offTime;
    unsigned int on;
    unsigned int side;
} LASER_T_G2;


extern LICK_T_G2 lick_G2;

extern LASER_T_G2 laser_G2;

extern TASK_T taskParam;

extern TASK_H taskParamH;

extern int ITI;

extern int currentMiss, correctRatio, totalOutcomes;

extern int hit, miss, falseAlarm, correctRejection, abortTrial;

#define LICKING_DETECTED 2
#define LICKING_RIGHT 3
#define LICKING_SENT 8
#define LICKING_BOTH 127

void feedWaterFast_G2();
void setWaterPortOpen(int i);
void sendLargeValue(int val);
void shuffleArray_G2(unsigned int * orgArray, unsigned int arraySize);
int waitTaskTimer(unsigned int dTime);
void assertLaser_G2(int type, int step);
void waitTrial_G2();
void turnOnLaser_G2();
void turnOffLaser_G2();
void set2WayValve(int va,int state);
void set3WayValve(int va,int state);

#endif	/* XC_HEADER_TEMPLATE_H */

