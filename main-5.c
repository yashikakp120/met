/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    22/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 5 - Watchdog and timer
 *  - Main loop (every 0.5 ms): feeds the watchdog, blinks D5 with 4 Hz,
 *    runs the heater controller.
 *  - PB5: endless loop (deadlock) -> watchdog resets after ~5.5 / ~21.9 s.
 *  - Timer A0 interrupt every 2 s: reads the NTC, shows range 1-5.
 *  - Controller keeps the temperature in range 3-4 (REL_STAT or REL_DYN).
 *
 * All Pin Connections:
 * - P1.0 - LED gn (D5)
 * - P1.3 - PB5 (X11)
 * - P1.5 - U_NTC (X9)
 * - P3.2 - LED rt (D6)
 * - P3.4 - REL_STAT (X5)
 * - P3.5 - REL_DYN (X4)
 * - X3 (REL_ID) - X10 (HEATER), JP4 set
 * - CON2 jumpers: P2.0-S0(2), P2.1-S1(2), P2.4-CK, P2.5-/CLR, P2.6-SR(2)
 *
 * Tasks Completed:
 * - Task 1:  4 Hz blinking, deadlock, reset after ~5.5 s   -> working
 * - Task 2a: reset after ~21.9 s (LONG_WDT)               -> working
 * - Task 2b: min / max measured (MEASURE), 5 ranges        -> working
 * - Task 2c: display every 2 s with Timer A0 interrupt     -> working
 * - Task 2d: controller with REL_STAT                      -> working
 * - Task 2e: controller with REL_DYN (DYNAMIC)             -> working
 * - Task 3:  feedback.txt                                  -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

// Switches (put // in front to turn one off)
#define LONG_WDT    // on: reset after 21.9 s     off: 5.5 s
#define DYNAMIC     // on: heater via REL_DYN     off: via REL_STAT
#define MEASURE     // on: measure min/max (Task 2b), turn OFF for submission

// ACLK = VLO (~12 kHz) / divider. Watchdog resets after 32768 ACLK ticks.
#ifdef LONG_WDT
  #define DIVIDER  DIVA_3     // 1500 Hz -> 32768 / 1500 = 21.9 s
  #define ACLK_HZ  1500
#else
  #define DIVIDER  DIVA_1     // 6000 Hz -> 32768 / 6000 = 5.5 s
  #define ACLK_HZ  6000
#endif

// Measured ADC values: room temperature / heater on. Split into 5 ranges.
#define ADC_MIN  341
#define ADC_MAX  682
#define STEP     ((ADC_MAX - ADC_MIN) / 5)

volatile int range = 1;       // temperature range 1-5 (set in the ISR)

#ifdef MEASURE
int minValue = 1023;          // lowest ADC value
int maxValue = 0;             // highest ADC value
volatile int heatTime = 0;    // seconds of heating
#endif

// LEDs D1-D4 via shift register 2 (bit 0 = D1). D4 is shifted in first.
void showLeds(char leds) {
  int i;
  P2OUT |= BIT0;              // mode: shift right (S0=1, S1=0)
  P2OUT &= ~BIT1;
  for (i = 3; i >= 0; i--) {
    if (leds & (1 << i)) P2OUT |= BIT6;   // data
    else P2OUT &= ~BIT6;
    P2OUT |= BIT4;            // clock pulse
    P2OUT &= ~BIT4;
  }
  P2OUT &= ~(BIT0 + BIT1);    // mode: hold
}

// Read the NTC and show the range: 1-4 on D1-D4, 5 (too hot) on D6.
void thermometer(void) {
  int value;

  ADC10CTL0 |= ENC + ADC10SC;       // start conversion
  while (ADC10CTL1 & ADC10BUSY);    // wait
  value = ADC10MEM;                 // 0-1023, warmer = higher

#ifdef MEASURE
  if (value < minValue) minValue = value;
  if (value > maxValue) maxValue = value;
  if (heatTime < 300) heatTime += 2;      // called every 2 s
#endif

  range = (value - ADC_MIN) / STEP + 1;
  if (value < ADC_MIN) range = 1;
  if (range > 5) range = 5;

  if (range == 5) {
    showLeds(0);
    P3OUT |= BIT2;            // D6 on
  } else {
    showLeds(1 << (range - 1));
    P3OUT &= ~BIT2;
  }
}

void main(void) {
  int count = 0;
  char heat = 0;

  initMSP();                  // also stops the watchdog

  // Watchdog with ACLK from VLO (not used while measuring,
  // so the debugger can pause the program)
  BCSCTL3 |= LFXT1S_2;        // ACLK = VLO
  BCSCTL1 |= DIVIDER;
#ifndef MEASURE
  WDTCTL = WDTPW + WDTSSEL + WDTCNTCL;    // start watchdog with ACLK
#endif

  // D5 output, PB5 input with pull-up
  P1DIR |= BIT0;
  P1OUT &= ~BIT0;
  P1DIR &= ~BIT3;
  P1REN |= BIT3;
  P1OUT |= BIT3;

  // D6, REL_STAT, REL_DYN: outputs, off
  P3DIR |= BIT2 + BIT4 + BIT5;
  P3OUT &= ~(BIT2 + BIT4 + BIT5);

  // Shift register pins (P2.6/P2.7 are crystal pins by default)
  P2SEL &= ~(BIT6 + BIT7);
  P2SEL2 &= ~(BIT6 + BIT7);
  P2DIR |= BIT0 + BIT1 + BIT4 + BIT5 + BIT6;
  P2OUT &= ~(BIT0 + BIT1 + BIT4 + BIT5 + BIT6);   // /CLR low: LEDs off
  P2OUT |= BIT5;

  // ADC: NTC on P1.5 (channel A5)
  ADC10CTL0 = ADC10ON + ADC10SHT_2;
  ADC10AE0 |= BIT5;
  ADC10CTL1 = INCH_5;

  // Timer A0: counts ACLK up to CCR0 -> interrupt every 2 s
  TA0CCR0 = 2 * ACLK_HZ - 1;
  TA0CCTL0 = CCIE;
  TA0CTL = TASSEL_1 + MC_1;   // ACLK, up mode

  thermometer();
  __enable_interrupt();

  while (1) {
    // PB5: artificial deadlock (interrupts keep running)
    if (!(P1IN & BIT3)) {
      while (1);
    }

#ifndef MEASURE
    WDTCTL = WDTPW + WDTSSEL + WDTCNTCL;    // feed the watchdog
#endif

    // blink D5 with 4 Hz: toggle every 250 * 0.5 ms = 125 ms
    count++;
    if (count == 250) {
      P1OUT ^= BIT0;
      count = 0;
    }

    // Controller: heat in range 1-3, stop in 4-5
#ifdef MEASURE
    heat = (heatTime < 300);  // heat 5 min, then stop (D7 off = done)
#else
    heat = (range <= 3);
#endif

#ifdef DYNAMIC
    // REL_DYN: relay only stays on with a ~1 kHz signal. It is made here,
    // so if the program hangs, the relay and heater turn off.
    if (heat) P3OUT ^= BIT5;
    else P3OUT &= ~BIT5;
#else
    // REL_STAT: high level = relay on (stays on if the program hangs)
    if (heat) P3OUT |= BIT4;
    else P3OUT &= ~BIT4;
#endif

    __delay_cycles(500);      // 0.5 ms
  }
}

// Timer A0 interrupt: every 2 s
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
  thermometer();
}
