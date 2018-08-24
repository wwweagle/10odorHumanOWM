/*
 * File:   hal.c
 * Author: Tony Lab
 *
 * Created on August 7, 2017, 9:48 AM
 */


#include "hal.h"
#include "adc10.h"
#include "utils.h"


uint32_t timerCounterI, millisCounter, taskTimeCounter;
int u2Received = -1;
volatile int adcdata;
volatile int isSending;
volatile int sendLick;
int lickThresh = 400; // larger is more sensitive
int stateLED = 1024;

const _prog_addressT EE_Addr_G2 = 0x7ff000;

void initPorts() {
    TRISA = 0x39FF;
    LATA = 0;
    ADPCFG=0xFFFF;
    TRISB = 0x000F;
    LATB = 0x000F;
    TRISC = 0xFFFF;
    LATC = 0;
    TRISD = 0xCF00;
    LATD = 0x0;
    TRISE = 0;
    LATE = 0;
    TRISF = 0xFEF3;
    LATF = 0;
    TRISG = 0xFCFF;
    LATG = 0;

}

void initTMR1(void) {

    TMR1 = 0;
    PR1 = 5000; // 5ms @ 1K counter++ per ms 
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CON = 0x8010; //FCY @ 1:8 prescale, 1K counter++ per ms
    ConfigIntTimer1(T1_INT_PRIOR_7 & T1_INT_ON);
    timerCounterI = 0u;
    millisCounter = 0u;
}

inline void tick(unsigned int i) {
    timerCounterI += (uint32_t) i;
    millisCounter += (uint32_t) i;
    PORTFbits.RF8 = (millisCounter % stateLED)>(stateLED / 2);
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {

    tick(5u);


    IFS0bits.T1IF = 0;
}

void wait_ms(int time) {
    uint32_t t32 = (uint32_t) time;
    timerCounterI = 0u;
    while (timerCounterI < t32);
}

void wait_Sec(int time) {
    while (time--) {
        wait_ms(1000);
    }
}

void initUART2(void) {
    unsigned int baudvalue;
    unsigned int U2MODEvalue;
    unsigned int U2STAvalue;
    CloseUART2();
    ConfigIntUART2(UART_RX_INT_EN & UART_RX_INT_PR1 &
            UART_TX_INT_DIS & UART_TX_INT_PR1);
    U2MODEvalue = UART_EN & UART_IDLE_CON &
            UART_DIS_WAKE & UART_DIS_LOOPBACK &
            UART_DIS_ABAUD & UART_NO_PAR_8BIT&
            UART_1STOPBIT;

    U2STAvalue = UART_INT_TX &
            UART_TX_PIN_NORMAL &
            UART_TX_ENABLE & UART_INT_RX_CHAR &
            UART_ADR_DETECT_DIS &
            UART_RX_OVERRUN_CLEAR;

    baudvalue = ((FCY / 16) / BAUDRATE) - 1;
    OpenUART2(U2MODEvalue, U2STAvalue, baudvalue);
    serialSend(61, 0);
}

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void) {
    if (U2STAbits.OERR == 1) {
        U2STAbits.OERR = 0;
    }
    if (0x2a == (u2Received = U2RXREG)) {
        serialSend(61, 0);
        wait_ms(50);
        asm("RESET");
    }
    IFS1bits.U2RXIF = 0;
}

void serialSend(int u2Type, int u2Value) {

    isSending = 1;
    while (BusyUART2());
    U2TXREG = (unsigned char) (u2Type & 0x7f);
    while (BusyUART2());
    U2TXREG = (unsigned char) (u2Value | 0x80);
    while (BusyUART2());

    if (sendLick) {
        U2TXREG = (unsigned char) (0);
        while (BusyUART2());
        U2TXREG = (unsigned char) (2 | 0x80);
        while (BusyUART2());
    }
    sendLick = 0;
    isSending = 0;

}

void write_eeprom_G2(int offset, int value) {

    _erase_eedata(EE_Addr_G2 + offset, _EE_WORD);
    _wait_eedata();
    _write_eedata_word(EE_Addr_G2 + offset, value);
    _wait_eedata();
}

int read_eeprom_G2(int offset) {
    int temp;
    _memcpy_p2d16(&temp, EE_Addr_G2 + offset, 2);
    return temp;
}

int checkKeyboard() {

    int out = 0;
    if (PORTBbits.RB0 == 0) {
        out += 1;
    }
    if (PORTBbits.RB1 == 0) {
        out += 2;
    }
    if (PORTBbits.RB2 == 0) {
        out += 4;
    }
    if (PORTBbits.RB3 == 0) {
        out += 8;
    }
    return out;
}