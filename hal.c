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
//int hundMS=0;

const _prog_addressT EE_Addr_G2 = 0x7ff000;

void initPorts() {
    ADPCFG = 0xFEFF;
    ADCSSL = 0x0100;
    //1 = Select ANx for input scan
    //0 = Skip ANx for input scan

    ADCON2bits.SMPI = 0x0f; //Interrupt on 2nd sample
    ADCON2bits.CHPS = 0; //Sample Channel CH0
    ADCON2bits.BUFM = 0; //    bit 1 BUFM: Buffer Mode Select bit
    //1 = Buffer configured as two 8-word buffers ADCBUF(15...8), ADCBUF(7...0)
    //0 = Buffer configured as one 16-word buffer ADCBUF(15...0.)
    ADCON2bits.CSCNA = 1; //Scan Input Selections for CH0+ S/H Input for MUX A Input Multiplexer Setting bit
    //    The CSCNA bit
    //(ADCON2<10>) enables the CH0 channel inputs to be scanned across a selected number of
    //analog inputs. When CSCNA is set, the CH0SA<3:0> bits are ignored.

    ADCHSbits.CH0NA = 0; //Select VREF- for CH0- input

    ADCON1bits.ADSIDL = 1; //If ADSIDL = 1, the module will stop in Idle.
    ADCON1bits.FORM = 0; //00 = Integer (DOUT = 0000 00dd dddd dddd)
    ADCON1bits.SSRC = 7; //Conversion Trigger Source Select bits,Internal counter ends sampling and starts conversion (auto convert)

    ADCON1bits.SAMP = 1; //SAMP: A/D Sample Enable bit //1 = At least one A/D sample/hold amplifier is sampling
    ADCON1bits.ASAM = 1; //ASAM: A/D Sample Auto-Start bit
    //1 = Sampling begins immediately after last conversion completes. SAMP bit is auto set
    //0 = Sampling begins when SAMP bit set
    ADCON2bits.VCFG = 0; //VCFG<2:0>: Voltage Reference Configuration bits 0=AVDD/AVSS

    ADCON2bits.ALTS = 0; //ALTS: Alternate Input Sample Mode Select bit
    //1 = Uses MUX A input multiplexer settings for first sample, then alternate between MUX B and MUX A input
    //multiplexer settings for all subsequent samples
    //0 = Always use MUX A input multiplexer settings

    ADCON3bits.SAMC = 31; //Auto-Sample Time bits
    //11111 = 31 TAD
    //·····
    //00001 = 1 TAD
    //00000 = 0 TAD (only allowed if performing sequential conversions using more than one S/H amplifier)
    ADCON3bits.ADRC = 0; //bit 7 ADRC: A/D Conversion Clock Source bit
    //1 = A/D internal RC clock
    //0 = Clock derived from system clock

    ADCON3bits.ADCS = 31; //ADCS<5:0>: A/D Conversion Clock Select bits
    //                    111111 = TCY/2 ? (ADCS<5:0> + 1) = 32 ? TCY
    //                    ······
    //                    000001 = TCY/2 ? (ADCS<5:0> + 1) = TCY
    //                    000000 = TCY/2 ? (ADCS<5:0> + 1) = TCY/2


    IFS0bits.ADIF = 1;
    IEC0bits.ADIE = 1;

    ADCON1bits.ADON = 1; //ADON: A/D Operating Mode bit
    //1 = A/D converter module is operating
    //0 = A/D converter is off


    TRISA = 0x39FF;
    LATA = 0;
    TRISB = 0x010F;
    LATB = 0x000F;
    TRISC = 0xFFFF;
    LATC = 0;
    TRISD = 0x0F00;
    LATD = 0x0;
    TRISE = 0;
    LATE = 0;
    TRISF = 0xFEF3;
    LATF = 0;
    TRISG = 0xFCFF;
    LATG = 0;

}

void __attribute__((__interrupt__, no_auto_psv)) _ADCInterrupt(void) {
    adcdata = ADCBUF0; //RB14
    IFS0bits.ADIF = 0; //After conversion ADIF is set to 1 and must be cleared
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
    PORTDbits.RD15 = (millisCounter % stateLED)>(stateLED / 2);
    Nop();
    Nop();
    PORTDbits.RD14 = (millisCounter % stateLED)>(stateLED / 2);
    Nop();
    Nop();
}

void sendChart(int val, int idx) {
    int high = ((val & 0x0fc0) >> 6)+(idx == 1 ? 0x40 : 0);
    serialSend(SpChartHigh, high);
    int low = (val & 0x3f)+(idx == 1 ? 0x40 : 0);
    serialSend(SpChartLow, low);
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void) {
    tick(5u);
    IFS0bits.T1IF = 0;
    if (millisCounter % 100u == 0) {
//        sendLargeValue(adcdata);
        sendChart(adcdata, 0);
    }
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