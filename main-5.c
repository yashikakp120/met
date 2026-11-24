/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    25/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 5 - Watchdog & Timer
 *        Use the defines below to switch between the tasks.
 *
 * All Pin Connections:
 * - Task 1:  PB5 (X11) -> P1.3, D5 (LED gn) -> P1.0
 * - Task 2:  X3 -> X10, jumper on JP4
 *            X4 (REL_DYN)  -> P3.5
 *            X5 (REL_STAT) -> P3.4
 *            D6 (LED rt)   -> P3.2
 *            X9 (U_NTC)    -> P1.5
 *            D1-D4 via shift register 2 (CON2 jumpers P2.0, P2.1, P2.4,
 *            P2.5, P2.6)
 *
 * Tasks Completed:
 * - Task 1:  4 Hz blinking, deadlock with PB5, reset after ~5.5 s -> working
 * - Task 2a: reset after ~21.9 s (WDT_21S)                       -> working
 * - Task 2b: ADC min / max over serial (MEASURE), 5 ranges        -> working
 * - Task 2c: display every 2 s with Timer A0 interrupt (THERMO)   -> working
 * - Task 2d: controller with REL_STAT (STATIC_RELAY)              -> working
 * - Task 2e: controller with REL_DYN (DYNAMIC_RELAY)              -> working
 * - Task 3:  feedback.txt                                         -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

// ---- watchdog time (only one!) ----
//#define WDT_5S      // task 1   -> ~5.5 s
#define WDT_21S       // task 2a  -> ~21.9 s

// ---- what the program does (only one!) ----
//#define TASK1       // blinking + deadlock
//#define MEASURE     // task 2b: heater on, print ADC min/max
#define THERMO        // task 2c-e: thermometer + controller

// ---- how the heater is switched (only one!) ----
//#define STATIC_RELAY   // task 2d: X5, just high/low
#define DYNAMIC_RELAY    // task 2e: X4, has to be toggled with ~1 kHz

// values I measured in task 2b (MEASURE mode)
// schematic says about 1.1 V at room temp and 2.2 V when hot
// -> 1.1/3.3*1023 = 341 and 2.2/3.3*1023 = 682
// TODO: put in the real numbers from the serial output
#define ADC_MIN 341
#define ADC_MAX 682


// VLO is ~12 kHz, watchdog overflows after 32768 clocks
//   12000/2 = 6000 Hz  -> 32768/6000 = 5.46 s
//   12000/8 = 1500 Hz  -> 32768/1500 = 21.8 s
#ifdef WDT_5S
#define ACLK_DIV  DIVA_1
#define ACLK_FREQ 6000
#endif
#ifdef WDT_21S
#define ACLK_DIV  DIVA_3
#define ACLK_FREQ 1500
#endif

// watchdog mode (WDTTMSEL = 0), clock from ACLK, clear the counter
// WDTIS = 00 -> 32768 cycles
#define KICK_DOG()  WDTCTL = WDTPW | WDTSSEL | WDTCNTCL

volatile int adcValue = 0;
volatile int range = 1;      // 1 (cold) ... 5 (too hot)


// put 4 bits on D1-D4 using shift register 2
// S0(2)=P2.0  S1(2)=P2.1  CK=P2.4  /CLR=P2.5  SR(2)=P2.6
void setLeds(unsigned char bits)
{
    int i;
    P2OUT |= BIT0;           // S0 = 1, S1 = 0 -> shift right
    P2OUT &= ~BIT1;
    for (i = 3; i >= 0; i--) {   // last bit ends up in QA
        if (bits & (1 << i))
            P2OUT |= BIT6;
        else
            P2OUT &= ~BIT6;
        P2OUT |= BIT4;       // clock pulse
        P2OUT &= ~BIT4;
    }
    P2OUT &= ~BIT0;          // back to "hold"
}

int readAdc(void)
{
    ADC10CTL0 |= ENC + ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);
    return ADC10MEM;
}

// split ADC_MIN..ADC_MAX into 5 equal parts
int getRange(int value)
{
    int r = (long)(value - ADC_MIN) * 5 / (ADC_MAX - ADC_MIN) + 1;
    if (r < 1) r = 1;
    if (r > 5) r = 5;
    return r;
}

void showRange(int r)
{
    // range 1 = D1, range 2 = D1+D2, ... range 4 = all four
    unsigned char leds[] = {0x0, 0x1, 0x3, 0x7, 0xF, 0xF};
    setLeds(leds[r]);

    if (r == 5)
        P3OUT |= BIT2;       // red LED -> too hot!
    else
        P3OUT &= ~BIT2;
}

int buttonPressed(void)
{
    // PB5 pulls to GND -> pressed = 0
    return !(P1IN & BIT3);
}


void main(void)
{
    initMSP();

    // just to see if the watchdog really did the reset
    if (IFG1 & WDTIFG) {
        serialPrintln("watchdog reset");
        IFG1 &= ~WDTIFG;
    } else {
        serialPrintln("start");
    }

    // green LED
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    // button with pull-up (no resistor on the board)
    P1DIR &= ~BIT3;
    P1REN |= BIT3;
    P1OUT |= BIT3;

    // red LED, REL_STAT, REL_DYN -> outputs, all off
    P3DIR |= BIT2 | BIT4 | BIT5;
    P3OUT &= ~(BIT2 | BIT4 | BIT5);

    // clock: ACLK from VLO, divider depends on watchdog time.
    // has to come after initMSP because initMSP overwrites BCSCTL1
    BCSCTL3 |= LFXT1S_2;
    BCSCTL1 = (BCSCTL1 & ~DIVA_3) | ACLK_DIV;

    KICK_DOG();   // this starts the watchdog


#ifdef TASK1
    while (1) {
        KICK_DOG();
        if (buttonPressed()) {
            while (1);   // deadlock -> no more kicking -> reset
        }
        P1OUT ^= BIT0;
        __delay_cycles(125000);   // 125 ms on/off = 4 Hz
    }
#endif


#ifdef MEASURE
    int minV = 1023, maxV = 0, v, n = 0;

    ADC10CTL0 = ADC10ON + ADC10SHT_2;
    ADC10AE0 |= BIT5;
    ADC10CTL1 = INCH_5;

    P3OUT |= BIT4;   // heater on (static relay)

    while (1) {
        KICK_DOG();
        v = readAdc();
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;

        n++;
        if (n == 10) {   // print once per second
            n = 0;
            serialPrint("now ");  serialPrintInt(v);
            serialPrint(" min "); serialPrintInt(minV);
            serialPrint(" max "); serialPrintInt(maxV);
            serialPrintln("");
        }
        __delay_cycles(100000);
    }
#endif


#ifdef THERMO
    // shift registers. P2.6/P2.7 are XIN/XOUT by default -> switch to GPIO
    P2SEL &= ~(BIT6 | BIT7);
    P2SEL2 &= ~(BIT6 | BIT7);
    P2DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6;
    P2OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT6);  // both SR on hold
    P2OUT &= ~BIT5;   // clear
    P2OUT |= BIT5;

    // ADC on P1.5
    ADC10CTL0 = ADC10ON + ADC10SHT_2;
    ADC10AE0 |= BIT5;
    ADC10CTL1 = INCH_5;

    adcValue = readAdc();
    range = getRange(adcValue);
    showRange(range);

    // timer A0: ACLK, up mode, interrupt every 2 s
    CCR0 = 2 * ACLK_FREQ - 1;
    CCTL0 = CCIE;
    TACTL = TASSEL_1 + MC_1 + TACLR;
    __enable_interrupt();

    int heater = 0;
    int count = 0;

    while (1) {
        KICK_DOG();

        if (buttonPressed()) {
            while (1);   // deadlock, timer interrupt still shows the temp
        }

        // simple on/off controller, stay between range 3 and 4
        if (range <= 3)
            heater = 1;
        else
            heater = 0;

#ifdef STATIC_RELAY
        if (heater)
            P3OUT |= BIT4;
        else
            P3OUT &= ~BIT4;
#endif

#ifdef DYNAMIC_RELAY
        // relay only stays on while we toggle X4 (~1 kHz, see schematic).
        // if the program hangs, the toggling stops -> heater goes off
        if (heater)
            P3OUT ^= BIT5;
        else
            P3OUT &= ~BIT5;
#endif

        // green LED still blinks with 4 Hz so I can see the program runs
        count++;
        if (count >= 250) {
            count = 0;
            P1OUT ^= BIT0;
        }

        __delay_cycles(500);   // 0.5 ms -> toggle period 1 ms = 1 kHz
    }
#endif
}


// Timer A0 interrupt service routine (every 2 s)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
#ifdef THERMO
    adcValue = readAdc();
    range = getRange(adcValue);
    showRange(range);
#endif
}
