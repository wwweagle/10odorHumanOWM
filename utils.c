/*
 * File:   utils.c
 * Author: Tony Lab
 *
 * Created on August 9, 2017, 2:59 PM
 */


#include "xc.h"
#include "utils.h"
#include "hal.h"
#include <stdlib.h>

void LCD_Write_Char(char message) {
    serialSend(SpLCD_Char, (unsigned char) message);
}

void LCDsetCursor(unsigned char col, unsigned char row) {
    int val = (((col & 0x0f) << 1) | (row & 0x01)) & 0x1F;
    serialSend(SpLCD_SET_XY, val);
}

void LCDclear(void) {
    serialSend(SpLCD_SET_XY, 0x20);
}

void LCDhome(void) {
    serialSend(SpLCD_SET_XY, 0x20);
}

void LCD_Write_Str(const char *message) {
    int iter;
    for (iter = 0; message[iter] && iter < 16; iter++) {
        serialSend(SpLCD_Char, (unsigned char) message[iter]);
    }
}

void lcdWriteNumber_G2(int val, int x, int y) {

    int tenK = val / 10000;
    int K = (val % 10000) / 1000;
    int H = (val % 1000) / 100;
    int T = (val % 100) / 10;
    int N = val % 10;
    int leadFlag = 0;

    LCDsetCursor(x, y);
    if (tenK) {
        LCD_Write_Char('0' + tenK);
        LCDsetCursor(++x, y);
        leadFlag = 1;
    }
    if (K || leadFlag) {
        LCD_Write_Char('0' + K);
        LCDsetCursor(++x, y);
        leadFlag = 1;
    }
    if (H || leadFlag) {
        LCD_Write_Char('0' + H);
        LCDsetCursor(++x, y);
        leadFlag = 1;
    }
    if (T || leadFlag) {
        LCD_Write_Char('0' + T);
        LCDsetCursor(++x, y);
    }
    LCD_Write_Char('0' + N);

}

void splash_G2(const char* line1, const char* line2) {
    LCDhome();
    LCDclear();
    LCD_Write_Str(line1);
    LCDsetCursor(0, 1);
    LCD_Write_Str(line2);
}



LICK_T_G2 lick_G2 = {
    .current = 0u, .stable = 0u, .filter = 0u, .portSide = 0u, .LCount = 0u, .RCount = 0u
};

TASK_T taskParam = {
    .sample1s = NULL,
    .test1s = NULL,
    .pairs1Count = 0,
    .sample2 = 0,
    .test2 = 0,
    .respCue = NULL,
    .respCount = 0,
    .sample1Length = 1000,
    .sample2Length = 1000,
    .test1Length = 1000,
    .test2Length = 1000,
    .respCueLength = 1000,
    .falsePunish = 0,
    .correctionCue = 0,
    .correctionCueLength = 1000,
    .delay1 = 5,
    .delay2 = 0,
    .delay3 = 0,
    .ITI = 8,
    .waitForTrial = 1,
    .minBlock = 4
};

TASK_H taskParamH = {
    .odorPairs = 2,
    .samples = NULL,
    .tests = NULL,
    .odorlength = 2000,
    .delay = 6,
    .ITI = 10,
    .phase = 0,
};

int hit, miss, falseAlarm, correctRejection, abortTrial;
int totalOutcomes;
int currentMiss, correctRatio;

int getFuncNumber(int targetDigits, const char* message) {
    int bitValue[targetDigits];
    int n = 0;
    int iter;
    int iter1;

    LCDclear();
    LCDhome();
    LCD_Write_Str(message);


    for (iter = 0; iter < targetDigits; iter++) {
        bitValue[iter] = -6;
    }
    //outer:
    for (iter = 0; iter < targetDigits; iter++) {
        int lcdPos;
        for (lcdPos = 0; lcdPos < targetDigits; lcdPos++) {
            LCDsetCursor(lcdPos, 1);
            LCD_Write_Char(bitValue[lcdPos] + 0x30);
        }

        while (bitValue[iter] < 0) {
            int kb = checkKeyboard();
            if (kb > 0) {
                serialSend(1, 100 + kb);
                return 100 + kb;
            }

            if (u2Received > 0) {
                bitValue[iter] = u2Received - 0x30;
                u2Received = -1;
            }
        }
        serialSend(1, bitValue[iter]);

    }

    int lcdPos;
    for (lcdPos = 0; lcdPos < targetDigits; lcdPos++) {
        LCDsetCursor(lcdPos, 1);
        LCD_Write_Char(bitValue[lcdPos] + 0x30);
    }


    for (iter1 = 0; iter1 < targetDigits; iter1++) {
        n = n * 10 + bitValue[iter1];
    }
    return n;
}

int matchornot(int respWindow) {
    int n = -1;
    millisCounter = 0;
    while (millisCounter < respWindow) {
        if (u2Received > 0) {
            n = u2Received - 0x30;
            u2Received = -1;
            break;
        }
    }
    serialSend(1, n);
    return n;
}

void sendLargeValue(int val) {
    char valHigh = (char) (val / 100);
    char valLow = (char) (val % 100);
    serialSend(23, valHigh);
    serialSend(24, valLow);
}

void shuffleArray_G2(unsigned int * orgArray, unsigned int arraySize) {
    if (arraySize == 0 || arraySize == 1)
        return;

    int iter;
    for (iter = 0; iter < arraySize; iter++) {
        orgArray[iter] = iter;
    }
    int index, temp;
    for (iter = arraySize - 1; iter > 0; iter--) {

        index = rand() % (iter + 1);
        temp = orgArray[index];
        orgArray[index] = orgArray[iter];
        orgArray[iter] = temp;
    }
}

int waitTaskTimer(unsigned int dTime) {
    int currLick = lick_G2.LCount;
    taskTimeCounter += dTime;
    while (millisCounter < taskTimeCounter);

    return lick_G2.LCount>currLick;
}

void set2WayValve(int valve, int state) {
    switch (valve) {
        case 1:
            PORTFbits.RF2 = state;
            break;
        case 2:
            PORTFbits.RF3 = state;
            break;
        case 3:
            PORTDbits.RD0 = state;
            break;
        case 4:
            PORTDbits.RD1 = state;
            break;
        case 5:
            PORTDbits.RD2 = state;
            break;
        case 6:
            PORTDbits.RD3 = state;
            break;
        case 7:
            PORTDbits.RD4 = state;
            break;
        case 8:
            PORTDbits.RD5 = state;
            break;
        case 9:
            PORTDbits.RD6 = state;
            break;
        case 10:
            PORTDbits.RD7 = state;
            break;
    }

    Nop();
    Nop();
    Nop();
}

void set3WayValve(int valve, int state) {
    switch (valve) {
        case 1:
            PORTBbits.RB8 = state;
            break;
        case 2:
            PORTBbits.RB9 = state;
            break;
        case 3:
            PORTBbits.RB10 = state;
            break;
        case 4:
            PORTBbits.RB11 = state;
            break;
        case 5:
            PORTBbits.RB12 = state;
            break;
        case 6:
            PORTBbits.RB13 = state;
            break;
        case 7:
            PORTBbits.RB14 = state;
            Nop();
            Nop();
            Nop();
            PORTDbits.RD12 = state;
            break;
        case 8:
            PORTBbits.RB15 = state;
            break;
        case 9:
            PORTAbits.RA14 = state;
            break;
        case 10:
            PORTAbits.RA15 = state;
            break;
    }

    Nop();
    Nop();
    Nop();
}

