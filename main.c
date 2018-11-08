/*
 * File:   main.c
 * Author: Tony Lab
 *
 * Created on August 7, 2017, 9:47 AM
 */


#include <stdlib.h>

#include "utils.h"
#include "hal.h"


void callFunc(int n);
void testOneValve(int n, int iti, int repeat);
void testValveFast(int board, int valve, int keep);
void DPASessionsHuman(int trialsPerSession, int totalSession);
void DNMSessionsHuman(int totalSession);
void gradTeach(int sessNum, int delay);
void gradTest(int sessNum, int delay, int ITI);

unsigned int taskType_G2 = HUMAN_DNMS_TASK;
const char odorTypes_G2[] = "BYRQHNKLTXZdMAES0123456";
int correctionRepeatCount = 0;

int main(void) {
    initPorts();
    initTMR1();
    initUART2();
    splash_G2(__DATE__, __TIME__);
    wait_Sec(2);
    while (1) {
        callFunc(getFuncNumber(2, "Main Func?"));
    }
    return 0;
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
    srand((unsigned int) millisCounter);
    switch (n) {
        case 21:
        {
            int b = getFuncNumber(1, "Board?");
            int v = getFuncNumber(2, "Valve?");
            int k = getFuncNumber(1, "Keep?");
            testValveFast(b, v, k);
            break;
        }

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
            //        {
            //            // Human ODPA
            //            taskType_G2 = HUMAN_ODPA_TASK;
            //            taskParamH.odorPairs = 2;
            //            addHOdors(1);
            //            taskParamH.delay = getFuncNumber(2, "Delay duration(6-30s)");
            //            taskParamH.ITI = getFuncNumber(2, "ITI duration");
            //            int sessNum = getFuncNumber(2, "Session number?");
            //            DPASessionsHuman(16, sessNum);
            //            break;
            //        }

        case 42:
        case 102:
        {
            //Human Grad Teach
            taskParamH.delay = getFuncNumber(2, "Delay duration");
            int sessNum = getFuncNumber(2, "Session number?");
            gradTeach(sessNum, taskParamH.delay);
            break;
        }
        case 43:
        case 104:
        {
            //Human Grad Test
            taskParamH.delay = getFuncNumber(2, "Delay duration");
            int sessNum = getFuncNumber(2, "Session number?");
            taskParamH.ITI = getFuncNumber(2, "ITI");
            gradTest(sessNum, taskParamH.delay, taskParamH.ITI);
            break;
        }
        case 44:
        case 108:
        {
            //Human DNMS
            taskType_G2 = HUMAN_DNMS_TASK;
            taskParamH.odorPairs = 4;
            addHOdors(0);
            taskParamH.delay = getFuncNumber(2, "Delay duration");
            taskParamH.ITI = getFuncNumber(2, "ITI duration");
            int sessNum = getFuncNumber(2, "Session number?");
            DNMSessionsHuman(sessNum);
            break;
        }
        case 25:
        case 101:
            stateLED = 512;
            splash_G2("Test", "Valves");
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
    splash_G2("BOARD", "VALVE");

    lcdWriteNumber_G2(board, 7, 0);
    lcdWriteNumber_G2(valve, 7, 1);

    if (board == 2) {
        set2WayValve(valve, 1);
    } else if (board == 3) {
        set3WayValve(valve, 1);
    } else if (board == 5) {
        set2WayValve(valve, 1);
        Nop();
        Nop();
        set3WayValve(valve, 1);
    }

    while (1) {
        if (!keep) {
            wait_ms(500);
            set2WayValve(valve, 0);
            set3WayValve(valve, 0);
            wait_ms(500);
            if (board == 2) {
                set2WayValve(valve, 1);
            } else if (board == 3) {
                set3WayValve(valve, 1);
            }
        } else if (valve >= 20) {
            int v;
            for (v = 1; v < 10; v++) {
                if (v != (valve % 10)) {
                    set3WayValve(v, 1);
                    wait_ms(50);
                }
            }

        }
    }
}

void testOneValve(int valve, int iti, int repeat) {
    const int preCharge = 500;
    const int onTime = 1000;
    int closingAdvance = 195;
    int rpt;
    for (rpt = 0; rpt < repeat; rpt++) {
        serialSend(SpIO, valve);

        set3WayValve(valve, 1);
        wait_ms(preCharge);
        BNC_1 = 1;
        set2WayValve(valve, 1);
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
        set3WayValve(valve, 0);
        wait_ms(closingAdvance);
        BNC_2 = 0;
        set2WayValve(valve, 0);
        serialSend(SpIO, 0);
        BNC_1 = 0;
        wait_ms(iti * 1000 - preCharge);

    }
}

void stim_H(int place, int odorPort) {

    set3WayValve(odorPort, 1);
    waitTaskTimer(500);
    set2WayValve(odorPort, 1);
    static int stimSend[] = {0, 9, 10};
    serialSend(stimSend[place], odorPort);
    waitTaskTimer(taskParamH.odorlength);
    set2WayValve(odorPort, 0);
    set3WayValve(odorPort, 0);
    serialSend(stimSend[place], 0);

}

static void processHit_G2(int id, int ratio) {
    serialSend(22, 1);
    if (ratio > 0 || (ratio == 0 && rand() % 2)) {
        serialSend(SpDebugInfo, 121);
    } else {
        serialSend(SpDebugInfo, 120);
    }
    if (ratio == 2) {
        serialSend(SpDebugInfo, 122);
        waitTaskTimer(500);
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

void waterNResult_H(int sampleIndex, int testIndex, int rewardWindow) {
    int choice;
    splash_G2("1 for match", "2 for non-match");
    for (timerCounterI = 0; timerCounterI < rewardWindow && (choice = uartCheck(rewardWindow)) < 0;);
    taskTimeCounter = millisCounter;
    /////Reward
    if (choice != 1 && choice != 2) {
        splash_G2("Missed", "");
        processMiss_G2(1);
    } else if ((sampleIndex != testIndex && choice == 1)
            || (sampleIndex == testIndex && choice == 2)) {
        splash_G2("Incorrect", "");
        processFalse_G2(1);
    } else {
        splash_G2("Correct", "");
        processHit_G2(1, 1);
    }
}

static void human_Trial(int sampleIndex, int testIndex) {
    taskTimeCounter = millisCounter;
    waitTaskTimer(3500u);
    splash_G2("Sample", "");
    stim_H(1, taskParamH.samples[sampleIndex]);
    splash_G2("Delay", "");
    waitTaskTimer(1000u);
    waitTaskTimer(taskParamH.delay * 1000u - 2000u);
    splash_G2("Test", "");
    waitTaskTimer(500u);
    stim_H(2, taskParamH.tests[testIndex]);
    splash_G2("", "");
    waterNResult_H(sampleIndex, testIndex, 5000);
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
//
//void DPASessionsHuman(int trialsPerSession, int totalSession) {
//    int *odorAlengthOrder = malloc(taskParamH.odorPairs * sizeof (int));
//    int *odorBlengthOrder = malloc(taskParamH.odorPairs * sizeof (int));
//    int pair, lengthOrder, rank;
//    rank = 3;
//    lengthOrder = 3;
//    pair = 0;
//    while (rank != 3 || pair < taskParamH.odorPairs) {
//        int times;
//        for (times = 0; times < 5; times++) {
//            stim_H(1, taskParamH.samples[pair], lengthOrder);
//            waitTaskTimer(6000u);
//        }
//        rank = getFuncNumber(1, "Ranking 1-5");
//        switch (rank) {
//            case 1:
//                lengthOrder = lengthOrder + 2;
//                if (lengthOrder > 9) {
//                    lengthOrder = 9;
//                    rank = 3;
//                    serialSend(SpValHigh, 255);
//                }
//                continue;
//            case 2:
//                ++lengthOrder;
//                if (lengthOrder > 9) {
//                    lengthOrder = 9;
//                    rank = 3;
//                    serialSend(SpValHigh, 255);
//                }
//                continue;
//            case 3:
//                odorAlengthOrder[pair] = lengthOrder;
//                pair++;
//                continue;
//            case 4:
//                --lengthOrder;
//                if (lengthOrder < 0) {
//                    lengthOrder = 0;
//                    rank = 3;
//                    serialSend(SpValLow, 0);
//                }
//                continue;
//            case 5:
//                lengthOrder = lengthOrder - 2;
//                if (lengthOrder < 0) {
//                    lengthOrder = 0;
//                    rank = 3;
//                    serialSend(SpValLow, 0);
//                }
//                continue;
//        }
//    }
//    rank = 0;
//    lengthOrder = 3;
//    pair = 0;
//    while (rank != 3 || pair < taskParamH.odorPairs) {
//        int times2;
//        for (times2 = 0; times2 < 5; times2++) {
//            stim_H(2, taskParamH.tests[pair], lengthOrder);
//            waitTaskTimer(6000u);
//        }
//        rank = getFuncNumber(1, "Ranking 1-5");
//        switch (rank) {
//            case 1:
//                lengthOrder = lengthOrder + 2;
//                if (lengthOrder > 9) {
//                    lengthOrder = 9;
//                    rank = 3;
//                    serialSend(SpValHigh, 255);
//                }
//                continue;
//            case 2:
//                ++lengthOrder;
//                if (lengthOrder > 9) {
//                    lengthOrder = 9;
//                    rank = 3;
//                    serialSend(SpValHigh, 255);
//                }
//                continue;
//            case 3:
//                odorBlengthOrder[pair] = lengthOrder;
//                pair++;
//                continue;
//            case 4:
//                --lengthOrder;
//                if (lengthOrder < 0) {
//                    lengthOrder = 0;
//                    rank = 3;
//                    serialSend(SpValLow, 0);
//                }
//                continue;
//            case 5:
//                lengthOrder = lengthOrder - 2;
//                if (lengthOrder < 0) {
//                    lengthOrder = 0;
//                    rank = 3;
//                    serialSend(SpValLow, 0);
//                }
//                continue;
//        }
//    }
//
//
//    int currentShapingTrial;
//    unsigned int shuffleGot;
//    unsigned int *shuffledList = malloc(8 * sizeof (unsigned int));
//    int ready = 0;
//    while (ready == 0) {
//        serialSend(SpTaskType, HUMAN_ODPA_SHAPING);
//        hit = miss = falseAlarm = correctRejection = 0;
//        int sampleIndex, testIndex;
//        srand((unsigned int) millisCounter);
//        serialSend(SpSess, 1);
//        for (currentShapingTrial = 0; currentShapingTrial < 16;) {
//            int idxInMinBlock = currentShapingTrial % 8;
//            if (idxInMinBlock == 0) {
//                shuffleArray_G2(shuffledList, 8);
//            }
//            shuffleGot = shuffledList[idxInMinBlock];
//            sampleIndex = shuffleGot % 2;
//            testIndex = shuffleGot % 2;
//            xdTrial_H(sampleIndex, testIndex, odorAlengthOrder, odorBlengthOrder);
//            currentShapingTrial++;
//        }
//        serialSend(SpSess, 0);
//        totalOutcomes = hit + correctRejection + miss + falseAlarm;
//        ready = getFuncNumber(1, "Are you ready");
//    }
//
//    // DPA TEST
//    int currentTrial;
//    int currentSession = 0;
//    serialSend(SpTaskType, HUMAN_ODPA_TASK);
//    while (currentSession++ < totalSession) {
//        serialSend(SpSess, 1);
//        hit = miss = falseAlarm = correctRejection = 0;
//        int sampleIndex, testIndex;
//        srand((unsigned int) millisCounter);
//        int idxInMinBlock;
//        for (currentTrial = 0; currentTrial < 16;) {
//            idxInMinBlock = currentTrial % 8;
//            if (idxInMinBlock == 0) {
//                shuffleArray_G2(shuffledList, 8);
//            }
//            shuffleGot = shuffledList[idxInMinBlock];
//            sampleIndex = shuffleGot % 2;
//            testIndex = (shuffleGot < 4) ? 0 : 1;
//            xdTrial_H(sampleIndex, testIndex, odorAlengthOrder, odorBlengthOrder);
//            currentTrial++;
//        }
//        serialSend(SpSess, 0);
//        totalOutcomes = hit + correctRejection + miss + falseAlarm;
//    }
//    serialSend(SpTrain, 0); // send it's the end
//    u2Received = -1;
//    free(shuffledList);
//    free(odorAlengthOrder);
//    free(odorBlengthOrder);
//    free(taskParamH.samples);
//    free(taskParamH.tests);
//
//
//}

void DNMSessionsHuman(int totalSession) {

    unsigned int shuffleGot;
    unsigned int *shuffledList = malloc(8 * sizeof (unsigned int));
    int currentTrial;
    int currentSession = 0;
    serialSend(SpTaskType, HUMAN_DNMS_TASK);
    wait_ms(5000);
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
            human_Trial(sampleIndex, testIndex);
            currentTrial++;
        }
        serialSend(SpSess, 0);
        totalOutcomes = hit + correctRejection + miss + falseAlarm;
    }
    serialSend(SpTrain, 0); // send it's the end
    u2Received = -1;
    free(shuffledList);
    free(taskParamH.samples);
    free(taskParamH.tests);
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

void gradTeach(int sessNum, int delay) {

    int oIdx;
    int sIdx;
    wait_ms(5000);
    for (sIdx = 0; sIdx < sessNum; sIdx++) {
        for (oIdx = 1; oIdx <= 7; oIdx++) {
            taskTimeCounter = millisCounter;
            set3WayValve(oIdx, 1);
            waitTaskTimer(500u);
            set2WayValve(oIdx, 1);
            serialSend(SpOdor_C, oIdx);
            splash_G2("Gradient", "");
            lcdWriteNumber_G2(oIdx, 1, 1);
            waitTaskTimer(2000u);
            set3WayValve(oIdx, 0);
            set2WayValve(oIdx, 0);
            serialSend(SpOdor_C, 0);
            splash_G2("Pls_Wait", "");
            waitTaskTimer(delay * 1000u);

        }
    }
}

void gradTest(int sessNum, int delay, int ITI) {
    int rewardWindow = 5000;
    int rawIdx, oIdx;
    int sIdx;
    wait_ms(5000);
    for (sIdx = 0; sIdx < sessNum; sIdx++) {
        serialSend(SpSess, sIdx + 1);
        unsigned int *shuffledList = malloc(7 * sizeof (unsigned int));
        shuffleArray_G2(shuffledList, 7u);
        for (rawIdx = 1; rawIdx <= 7; rawIdx++) {
            oIdx = shuffledList[rawIdx] + 1;
            taskTimeCounter = millisCounter;
            set3WayValve(oIdx, 1);
            waitTaskTimer(500u);
            set2WayValve(oIdx, 1);
            serialSend(SpOdor_C, oIdx);
            splash_G2("Gradient", "");
            lcdWriteNumber_G2(0, 1, 1);
            waitTaskTimer(2000u);
            set3WayValve(oIdx, 0);
            set2WayValve(oIdx, 0);
            serialSend(SpOdor_C, 0);
            splash_G2("Pls_Wait", "");
            waitTaskTimer(delay * 1000u);


            int choice;
            splash_G2("Pls_Choose", "");
            for (timerCounterI = 0; timerCounterI < rewardWindow && (choice = uartCheck(rewardWindow)) < 0;);
            taskTimeCounter = millisCounter;
            /////Reward
            if (choice < 1 || choice > 7) {
                splash_G2("Missed", "");
                processMiss_G2(1);
            } else if (choice != oIdx) {
                splash_G2("Incorrect", "");
                processFalse_G2(1);
            } else {
                splash_G2("Correct", "");
                processHit_G2(1, 1);
            }
            taskTimeCounter = millisCounter;
            waitTaskTimer((unsigned int) ITI * 1000u);
            ////ITI
        }
        serialSend(SpSess, 0);
    }
}