/*
 * File:   main.c
 * Author: Tony Lab
 *
 * Created on August 7, 2017, 9:47 AM
 */

#include <i2c.h>
#include <stdlib.h>

#include "utils.h"
#include "hal.h"
#include "lcdi2c.h"
//#include "lcdi2c.h"

void callFunc(int n);
void testOneValve(int n, int iti, int repeat);
void testValveFast(int board, int valve, int keep);
void readADCData();
void testPorts();
void testNSetThres();
void DPASessionsHuman(int trialsPerSession, int totalSession);
void DNMSessionsHuman(int totalSession);
void addCLOdors();

unsigned int taskType_G2 = HUMAN_DNMS_TASK;
const char odorTypes_G2[] = "BYRQHNKLTXZdMAES0123456";
int correctionRepeatCount = 0;

int main(void) {
    initPorts();
    initTMR1();
    initUART2();
    //initI2C();
    //LCD_Init();
    initADC();
    splash_G2(__DATE__, __TIME__);
    while (1) {
        callFunc(getFuncNumber(2, "Main Func?"));
    }

    //StopI2C();
    //CloseI2C();
    return 0;
}

void sendChart(int val, int idx) {
    int high = ((val & 0x0fc0) >> 6)+(idx == 1 ? 0x40 : 0);
    serialSend(SpChartHigh, high);
    int low = (val & 0x3f)+(idx == 1 ? 0x40 : 0);
    serialSend(SpChartLow, low);
}

void addHOdors(int isDPA) {
    int odorPairs = taskParamH.odorPairs;
    taskParamH.samples = malloc(odorPairs * sizeof (int));
    taskParamH.tests = malloc(odorPairs * sizeof (int));
    int i;
    int odor;
    for (i = 0; i < odorPairs; i++) {
        odor = getFuncNumber(2, "Add a sample");
        taskParamH.samples[i] = odor;
        if (isDPA) {
            odor = getFuncNumber(2, "Add a test");
            taskParamH.tests[i] = odor;
        } else {
            taskParamH.tests[i] = odor;
        }

    }
}

void callFunc(int n) {
    lickThresh = (read_eeprom_G2(EEP_LICK_THRESHOLD)) << 2;
    //lickThresh = 400;
    waterLen = read_eeprom_G2(EEP_WATER_LEN_MS);
    srand((unsigned int) millisCounter);
    switch (n) {
        case 21:
        {
            int b = getFuncNumber(1, "Board?");
            int v = getFuncNumber(2, "Valve?");
            int k = getFuncNumber(1, "Keep?");
            testValveFast(b, v, k);
        }
        case 22:
            readADCData();
            break;
        case 23:
            testPorts();
            break;
        case 24:
            testNSetThres();
            break;

        case 27:
        {
            int dpadrOdors[] = {0, 1, 2, 3, 4, 5, 6, 7, 12};
            int i;
            for (i = 0; i < (sizeof (dpadrOdors) / sizeof (int)); i++) {
                testOneValve(dpadrOdors[i], 10, 1);
            }
            break;
        }

        case 41:
        {
            // Human ODPA
            taskType_G2 = HUMAN_ODPA_TASK;
            taskParamH.odorlength = malloc(10 * sizeof (int));
            int i;
            for (i = 0; i < 10; i++) {
                taskParamH.odorlength[i] = 500 * ((i % 5) + 1);
            }
            taskParamH.odorPairs = 2;
            addHOdors(1);
            taskParamH.delay = getFuncNumber(2, "Delay duration(6-30s)");
            taskParamH.ITI = getFuncNumber(2, "ITI duration");
            int sessNum = getFuncNumber(2, "Session number?");
            DPASessionsHuman(16, sessNum);
            break;
        }

        case 42:
        {
            //Human DNMS
            taskType_G2 = HUMAN_DNMS_TASK;
            taskParamH.odorlength = malloc(10 * sizeof (int));
            int i;
            for (i = 0; i < 10; i++) {
                taskParamH.odorlength[i] = 500 * ((i % 5) + 1);
            }
            taskParamH.odorPairs = 4;
            addHOdors(0);
            taskParamH.delay = getFuncNumber(2, "Delay duration");
            taskParamH.ITI = getFuncNumber(2, "ITI duration");
            int sessNum = getFuncNumber(2, "Session number?");
            DNMSessionsHuman(sessNum);
            break;
        }
        case 25:
        case 101 ... 115:
            stateLED=512;
            splash_G2("Test","Valves");
            ioRecycle();
            break;

        default:
        {
            int i;
            i = n;
            testOneValve(i, 8, 500);
        }
            break;

    }
    free(taskParam.sample1s);
    free(taskParam.test1s);
}

void testValveFast(int board, int valve, int keep) {
    LCDclear();
    LCDhome();
    LCD_Write_Str("BOARD   VALVE");

    lcdWriteNumber_G2(board, 6, 0);
    lcdWriteNumber_G2(valve, 6, 1);

    set4076_4bit(valve - 1);

    while (1) {
        muxDis(~board);
        if (!keep) {
            wait_ms(500);
            muxDis(0x0f);
            wait_ms(500);
        }

    }

}

void testOneValve(int valve, int iti, int repeat) {
    const int preCharge = 500;

    //    int repeat = 10;
    //    int iti = 8;
    const int onTime = 1000;
    int closingAdvance = 195;
    //    int valve;
    int rpt;
    for (rpt = 0; rpt < repeat; rpt++) {
        int P10Val = valve < 16 ? valve : valve - 16;
        int boardA = valve < 16 ? 1 : 4;
        int boardB = valve < 16 ? 2 : 8;
        serialSend(SpIO, valve);
        set4076_4bit(P10Val);
        muxDis(~boardA);
        wait_ms(preCharge);
        BNC_1 = 1;
        muxDis(~(boardA | boardB));
        BNC_2 = (valve & 0x10) >> 4;
        wait_ms(onTime / 5);
        BNC_2 = (valve & 0x08) >> 3;
        wait_ms(onTime / 5);
        BNC_2 = (valve & 0x04) >> 2;
        wait_ms(onTime / 5);
        BNC_2 = (valve & 0x02) >> 1;
        wait_ms(onTime / 5);
        BNC_2 = (valve & 0x01);
        wait_ms(onTime / 5 - closingAdvance);
        muxDis(~boardB);
        wait_ms(closingAdvance);
        BNC_2 = 0;
        muxDis(0x0F);
        serialSend(SpIO, 0);
        BNC_1 = 0;
        wait_ms(iti * 1000 - preCharge);

    }
}

void readADCData(void) {
    while (1) {
        volatile int temp = adcdata;
        int highByte = temp / 100;
        int lowByte = temp % 100;

        serialSend(23, highByte);
        serialSend(24, lowByte);
        wait_ms(50);

    }
}

void testPorts() {
    TRISF = 0xFF3F;
    int i = 0;
    while (1) {
        i = i^1;
        PORTFbits.RF6 = i;
        Nop();
        Nop();
        Nop();
        PORTBbits.RB10 = i;
        Nop();
        Nop();
        Nop();
        PORTBbits.RB9 = i;
        Nop();
        Nop();
        Nop();

        PORTFbits.RF7 = i;
        Nop();
        Nop();
        Nop();
        PORTFbits.RF6 = i;
        Nop();
        Nop();
        Nop();
        PORTAbits.RA14 = i;
        Nop();
        Nop();
        Nop();
        PORTDbits.RD12 = i;
        Nop();
        Nop();
        Nop();
        PORTBbits.RB11 = i;
        //        Nop();
        //        Nop();
        //        Nop();
        //        PORTAbits.RA15=i;
        wait_ms(250);
    }
}

void testNSetThres() {
    unsigned long startTime = millisCounter;
    while (millisCounter < startTime + 10000ul) {
        int sum = 0;
        int i = 0;
        for (; i < 15; i++) {
            sum += adcdata;
        }
        int val = sum / 15;
        sendLargeValue(val);
        wait_ms(500);
    }
    sendLargeValue(lickThresh);
    int newThres = getFuncNumber(3, "New Lick Thres?");
    sendLargeValue(newThres);
    write_eeprom_G2(EEP_LICK_THRESHOLD, newThres >> 2);
    serialSend(61, 0);
    asm("Reset");
}

void stim_H(int place, int odorPort, int lengthOrder) {
    if (place == 1 || place == 2) {
        set4076_4bit(lengthOrder > 4 ? odorPort + 8 : odorPort);
        muxDis(odorPort < 16 ? (~1) : (~4));
        waitTaskTimer(500);
    }

    muxDis(odorPort < 16 ? (~3) : (~0x0c));
    int stimSend;
    switch (place) {
        case 1:
            stimSend = 9;
            break;
        case 2:
            stimSend = 10;
            break;
    }
    serialSend(stimSend, odorPort);
    LCDsetCursor(3, 0);
    switch (place) {
        case 1:
            waitTaskTimer(taskParamH.odorlength[lengthOrder] - 200);
            break;
        case 2:
            waitTaskTimer(taskParamH.odorlength[lengthOrder] - 200);
            break;
    }

    muxDis(odorPort < 16 ? (~2) : (~8));
    waitTaskTimer(200);
    muxDis(0x0f);
    BNC_2 = 0;
    Nop();
    Nop();
    BNC_3 = 0;
    Nop();
    Nop();
    serialSend(stimSend, 0);

}

static void processHit_G2(int id, int ratio) {
    serialSend(22, 1);
    if (ratio > 0 || (ratio == 0 && rand() % 2)) {
        serialSend(SpDebugInfo, 121);
        setWaterPortOpen(1);
        waitTaskTimer(waterLen);
        setWaterPortOpen(0);
    } else {
        serialSend(SpDebugInfo, 120);
        waitTaskTimer(waterLen);
    }
    if (ratio == 2) {
        serialSend(SpDebugInfo, 122);
        waitTaskTimer(500);
        setWaterPortOpen(1);
        waitTaskTimer(waterLen);
        setWaterPortOpen(0);
    }

    currentMiss = 0;
    serialSend(SpHit, id);
}

static void processFalse_G2(int id) {
    currentMiss = 0;
    serialSend(SpFalseAlarm, id);
    lcdWriteNumber_G2(++falseAlarm, 5, 1);
    waitTaskTimer(4000);
}

static void processMiss_G2(int id) {
    currentMiss++;
    serialSend(SpMiss, id);
}

//static void processAbortTrial(int id) {
//    currentMiss = 0;
//    serialSend(SpAbortTrial, id);
//    ++abortTrial;
//}

void waterNResult_H(int sampleIndex, int testIndex, int id, int rewardWindow) {
    int choice;
    splash_G2("1 for match", "2 for non-match");
    for (timerCounterI = 0; timerCounterI < rewardWindow && (choice = matchornot(rewardWindow)) < 0;);
    taskTimeCounter = millisCounter;
    /////Reward
    if (choice != 1 && choice != 2) {
        splash_G2("", "Missed");
        processMiss_G2(0);
    } else if ((sampleIndex != testIndex && choice == 1)
            || (sampleIndex == testIndex && choice == 2)) {
        splash_G2("", "Incorrect");
        processFalse_G2(id);
    } else {
        splash_G2("Correct", "");
        processHit_G2(id, 1);
    }
}

static void xdTrial_H(int sampleIndex, int testIndex, int *odorAlength, int *odorBlength) {
    taskTimeCounter = millisCounter;
        waitTaskTimer(3500u);
    splash_G2("Sample", "");
    stim_H(1, taskParamH.samples[sampleIndex],  odorAlength[sampleIndex]);
    splash_G2("Delay", "");
    waitTaskTimer(1000u);
    waitTaskTimer(taskParamH.delay * 1000u - 2000u);
    waitTaskTimer(500u);
    splash_G2("Test", "");
    stim_H(2, taskParamH.tests[testIndex],  odorBlength[testIndex]);
    splash_G2("", "");
    waterNResult_H(sampleIndex, testIndex, 1, 1000);
    ///--ITI1---///
    int trialITI;
    if (taskParamH.ITI >= 4u) {
        trialITI = taskParamH.ITI - 4u;
        while (trialITI > 60u) {
            waitTaskTimer(60u * 1000u);
            trialITI -= 60u;
        }
        waitTaskTimer(trialITI * 1000u); //another 4000 is at the beginning of the trials.
    }
    serialSend(SpITI, 0);
}

void DPASessionsHuman(int trialsPerSession, int totalSession) {
        int *odorAlengthOrder = malloc(taskParamH.odorPairs * sizeof (int));
    int *odorBlengthOrder = malloc(taskParamH.odorPairs * sizeof (int));
    int pair, lengthOrder, rank;
    rank = 3;
    lengthOrder = 3;
    pair = 0;
    while (rank != 3 || pair < taskParamH.odorPairs) {
        int times;
        for (times = 0; times < 5; times++) {
            stim_H(1, taskParamH.samples[pair],  lengthOrder);
            waitTaskTimer(6000u);
        }
        rank = getFuncNumber(1, "Ranking 1-5");
        switch (rank) {
            case 1:
                lengthOrder = lengthOrder + 2;
                if (lengthOrder > 9) {
                    lengthOrder = 9;
                    rank = 3;
                    serialSend(SpValHigh, 255);
                }
                continue;
            case 2:
                ++lengthOrder;
                if (lengthOrder > 9) {
                    lengthOrder = 9;
                    rank = 3;
                    serialSend(SpValHigh, 255);
                }
                continue;
            case 3:
                odorAlengthOrder[pair] = lengthOrder;
                pair++;
                continue;
            case 4:
                --lengthOrder;
                if (lengthOrder < 0) {
                    lengthOrder = 0;
                    rank = 3;
                    serialSend(SpValLow, 0);
                }
                continue;
            case 5:
                lengthOrder = lengthOrder - 2;
                if (lengthOrder < 0) {
                    lengthOrder = 0;
                    rank = 3;
                    serialSend(SpValLow, 0);
                }
                continue;
        }
    }
    rank = 0;
    lengthOrder = 3;
    pair = 0;
    while (rank != 3 || pair < taskParamH.odorPairs) {
        int times2;
        for (times2 = 0; times2 < 5; times2++) {
            stim_H(2, taskParamH.tests[pair], lengthOrder);
            waitTaskTimer(6000u);
        }
        rank = getFuncNumber(1, "Ranking 1-5");
        switch (rank) {
            case 1:
                lengthOrder = lengthOrder + 2;
                if (lengthOrder > 9) {
                    lengthOrder = 9;
                    rank = 3;
                    serialSend(SpValHigh, 255);
                }
                continue;
            case 2:
                ++lengthOrder;
                if (lengthOrder > 9) {
                    lengthOrder = 9;
                    rank = 3;
                    serialSend(SpValHigh, 255);
                }
                continue;
            case 3:
                odorBlengthOrder[pair] = lengthOrder;
                pair++;
                continue;
            case 4:
                --lengthOrder;
                if (lengthOrder < 0) {
                    lengthOrder = 0;
                    rank = 3;
                    serialSend(SpValLow, 0);
                }
                continue;
            case 5:
                lengthOrder = lengthOrder - 2;
                if (lengthOrder < 0) {
                    lengthOrder = 0;
                    rank = 3;
                    serialSend(SpValLow, 0);
                }
                continue;
        }
    }


    int currentShapingTrial;
    unsigned int shuffleGot;
    unsigned int *shuffledList = malloc(8 * sizeof (unsigned int));
    int ready = 0;
    while (ready == 0) {
        serialSend(SpTaskType, HUMAN_ODPA_SHAPING);
        hit = miss = falseAlarm = correctRejection = 0;
        int sampleIndex, testIndex;
        srand((unsigned int) millisCounter);
        serialSend(SpSess, 1);
        for (currentShapingTrial = 0; currentShapingTrial < 16;) {
            int idxInMinBlock = currentShapingTrial % 8;
            if (idxInMinBlock == 0) {
                shuffleArray_G2(shuffledList, 8);
            }
            shuffleGot = shuffledList[idxInMinBlock];
            sampleIndex = shuffleGot % 2;
            testIndex = shuffleGot % 2;
            xdTrial_H(sampleIndex, testIndex,  odorAlengthOrder, odorBlengthOrder);
            currentShapingTrial++;
        }
        serialSend(SpSess, 0);
        totalOutcomes = hit + correctRejection + miss + falseAlarm;
        ready = getFuncNumber(1, "Are you ready");
    }

    // DPA TEST
    int currentTrial;
    int currentSession = 0;
    serialSend(SpTaskType, HUMAN_ODPA_TASK);
    while (currentSession++ < totalSession) {
        serialSend(SpSess, 1);
        hit = miss = falseAlarm = correctRejection = 0;
        int sampleIndex, testIndex;
        srand((unsigned int) millisCounter);
        int idxInMinBlock;
        for (currentTrial = 0; currentTrial < 16;) {
            idxInMinBlock = currentTrial % 8;
            if (idxInMinBlock == 0) {
                shuffleArray_G2(shuffledList, 8);
            }
            shuffleGot = shuffledList[idxInMinBlock];
            sampleIndex = shuffleGot % 2;
            testIndex = (shuffleGot < 4) ? 0 : 1;
            xdTrial_H(sampleIndex, testIndex,  odorAlengthOrder, odorBlengthOrder);
            currentTrial++;
        }
        serialSend(SpSess, 0);
        totalOutcomes = hit + correctRejection + miss + falseAlarm;
    }
    serialSend(SpTrain, 0); // send it's the end
    u2Received = -1;
    free(shuffledList);
    free(odorAlengthOrder);
    free(odorBlengthOrder);
    free(taskParamH.samples);
    free(taskParamH.tests);
    free(taskParamH.odorlength);

}

void DNMSessionsHuman(int totalSession) {
        int *odorAlengthOrder = malloc(taskParamH.odorPairs * sizeof (int));
    int *odorBlengthOrder = malloc(taskParamH.odorPairs * sizeof (int));
    int pair, lengthOrder, rank;
    rank = 0;
    lengthOrder = 3;
    pair = 0;
    while (rank != 3 || pair < taskParamH.odorPairs) {

        rank = 3;
        switch (rank) {
            case 1:
                lengthOrder = lengthOrder + 2;
                if (lengthOrder > 9) {
                    lengthOrder = 9;
                    rank = 3;
                    serialSend(SpValHigh, 255);
                }
                continue;
            case 2:
                ++lengthOrder;
                if (lengthOrder > 9) {
                    lengthOrder = 9;
                    rank = 3;
                    serialSend(SpValHigh, 255);
                }
                continue;
            case 3:
                odorAlengthOrder[pair] = lengthOrder;
                odorBlengthOrder[pair] = lengthOrder;
                pair++;
                continue;
            case 4:
                --lengthOrder;
                if (lengthOrder < 0) {
                    lengthOrder = 0;
                    rank = 3;
                    serialSend(SpValLow, 0);
                }
                continue;
            case 5:
                lengthOrder = lengthOrder - 2;
                if (lengthOrder < 0) {
                    lengthOrder = 0;
                    rank = 3;
                    serialSend(SpValLow, 0);
                }
                continue;
        }
    }

    unsigned int shuffleGot;
    unsigned int *shuffledList = malloc(8 * sizeof (unsigned int));
    int currentTrial;
    int currentSession = 0;
    serialSend(SpTaskType, HUMAN_DNMS_TASK);
    while (currentSession++ < totalSession) {
        serialSend(SpSess, 1);
        hit = miss = falseAlarm = correctRejection = 0;
        int sampleIndex, testIndex;
        srand((unsigned int) millisCounter);
        int idxInMinBlock;
        for (currentTrial = 0; currentTrial < 16;) {
            idxInMinBlock = currentTrial % 8;
            if (idxInMinBlock == 0) {
                shuffleArray_G2(shuffledList, 8);
            }
            shuffleGot = shuffledList[idxInMinBlock];
            sampleIndex = shuffleGot / 2;
            if (shuffleGot % 2 == 1) {
                testIndex = sampleIndex;
            } else {
                int falseIdx;
                falseIdx = rand() % 4;
                while (falseIdx == shuffleGot / 2) {
                    falseIdx = rand() % 4;
                }
                testIndex = falseIdx;
            }
            xdTrial_H(sampleIndex, testIndex,  odorAlengthOrder, odorBlengthOrder);
            currentTrial++;
        }
        serialSend(SpSess, 0);
        totalOutcomes = hit + correctRejection + miss + falseAlarm;
    }
    serialSend(SpTrain, 0); // send it's the end
    u2Received = -1;
    free(shuffledList);
    free(odorAlengthOrder);
    free(odorBlengthOrder);
    free(taskParamH.samples);
    free(taskParamH.tests);
    free(taskParamH.odorlength);




}

void ioRecycle() {
    int valve;
    while (1) {
        
        for (valve = 1; valve <= 10; valve++) {
            set2WayValve(valve, 1);
            wait_ms(500);
            set2WayValve(valve, 0);
            wait_ms(500);
        }

        for (valve = 1; valve <= 10; valve++) {
            set3WayValve(valve, 1);
            wait_ms(500);
            set3WayValve(valve, 0);
            wait_ms(500);
        }

    }
}