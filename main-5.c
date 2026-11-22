/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    22/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 5 - Watchdog and timer.
 *        D5 blinks with 4 Hz, PB5 causes a deadlock and the watchdog
 *        resets the controller. The NTC temperature is shown on D1-D4 / D6
 *        every 2 s (timer interrupt) and a controller keeps it in range 3-4.
 *
 * All Pin Connections:
 * - P1.0 - LED gn (D5)
 * - P1.3 - PB5 (X11)
 * - P1.5 - U_NTC
 * - P3.2 - LED rt (D6)
 * - P3.4 - REL_STAT (X5)
 * - P3.5 - REL_DYN (X4)
 * - X3 (REL_ID) - X10 (HEATER), JP4 set
 * - CON2 jumpers: P2.0-S0(2), P2.1-S1(2), P2.4-CK, P2.5-/CLR, P2.6-SR(2)
 *
 * Tasks Completed:
 * - Task 1:  blinking + deadlock, reset after ~5.5 s          -> working
 * - Task 2a: reset after ~21.9 s (#define LONG_WDT)          -> working
 * - Task 2b: min / max measured (#define MEASURE), 5 ranges   -> working
 * - Task 2c: display every 2 s with timer A0 interrupt        -> working
 * - Task 2d: controller with REL_STAT                         -> working
 * - Task 2e: controller with REL_DYN (#define DYNAMIC)        -> working
 * - Task 3:  feedback.txt                                     -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

#define LONG_WDT        // 21.9 s, without it 5.5 s
#define DYNAMIC         // heater with REL_DYN, without it REL_STAT
// #define MEASURE      // heater always on, prints min / max of the ADC

// ACLK = VLO (~12 kHz) / divider, watchdog overflows after 32768 ACLK ticks
#ifdef LONG_WDT
  #define DIVIDER  DIVA_3   // 12000 / 8 = 1500 Hz -> 21.9 s
  #define ACLK_HZ     1500
#else
  #define DIVIDER  DIVA_1   // 12000 / 2 = 6000 Hz -> 5.5 s
  #define ACLK_HZ     6000
#endif

// my measured ADC values (room temperature / heater on all the time)
#define ADC_MIN  341
#define ADC_MAX  682
#define STEP ((ADC_MAX - ADC_MIN) / 5)

volatile int range = 1;     // temperature range 1-5

#ifdef MEASURE
int minValue = 1023;
int maxValue = 0;
#endif

// D1-D4 through shift register 2 (bit 0 = D1)
void showLeds(char leds) {
  int i;
  P2OUT |= BIT0;            // shift right
  P2OUT &= ~BIT1;
  for (i = 3; i >= 0; i--) {
    if (leds & (1 << i)) P2OUT |= BIT6;
    else P2OUT &= ~BIT6;
    P2OUT |= BIT4;          // clock
    P2OUT &= ~BIT4;
  }
  P2OUT &= ~(BIT0 + BIT1);  // hold
}

// read the NTC and show the range
void thermometer(void) {
  int value;

  ADC10CTL0 |= ENC + ADC10SC;
  while (ADC10CTL1 & ADC10BUSY);
  value = ADC10MEM;

#ifdef MEASURE
  if (value < minValue) minValue = value;
  if (value > maxValue) maxValue = value;
  serialPrint("value ");
  serialPrintInt(value);
  serialPrint("  min ");
  serialPrintInt(minValue);
  serialPrint("  max ");
  serialPrintInt(maxValue);
  serialPrintln("");
#endif

  range = (value - ADC_MIN) / STEP + 1;
  if (value < ADC_MIN) range = 1;
  if (range > 5) range = 5;

  if (range == 5) {         // too hot -> red LED
    showLeds(0);
    P3OUT |= BIT2;
  } else {
    showLeds(1 << (range - 1));
    P3OUT &= ~BIT2;
  }
}

void main(void) {
  int count = 0;
  char heat = 0;

  initMSP();

  // ACLK from VLO, then start the watchdog with ACLK
  BCSCTL3 |= LFXT1S_2;
  BCSCTL1 |= DIVIDER;
  WDTCTL = WDTPW + WDTSSEL + WDTCNTCL;

  // D5 output, PB5 input with pull-up
  P1DIR |= BIT0;
  P1OUT &= ~BIT0;
  P1DIR &= ~BIT3;
  P1REN |= BIT3;
  P1OUT |= BIT3;

  // D6, REL_STAT, REL_DYN outputs, all off
  P3DIR |= BIT2 + BIT4 + BIT5;
  P3OUT &= ~(BIT2 + BIT4 + BIT5);

  // shift register pins
  P2SEL &= ~(BIT6 + BIT7);
  P2SEL2 &= ~(BIT6 + BIT7);
  P2DIR |= BIT0 + BIT1 + BIT4 + BIT5 + BIT6;
  P2OUT &= ~(BIT0 + BIT1 + BIT4 + BIT5 + BIT6);   // /CLR low = clear
  P2OUT |= BIT5;

  // ADC on P1.5 (A5)
  ADC10CTL0 = ADC10ON + ADC10SHT_2;
  ADC10AE0 |= BIT5;
  ADC10CTL1 = INCH_5;

  // timer A0: interrupt every 2 s
  TA0CCR0 = 2 * ACLK_HZ - 1;
  TA0CCTL0 = CCIE;
  TA0CTL = TASSEL_1 + MC_1;   // ACLK_HZ, up mode

  thermometer();
  __enable_interrupt();

  while (1) {
    // PB5 -> artificial deadlock
    if (!(P1IN & BIT3)) {
      while (1);
    }

    // feed the watchdog
    WDTCTL = WDTPW + WDTSSEL + WDTCNTCL;

    // blink D5 with 4 Hz (toggle every 125 ms)
    count++;
    if (count == 250) {
      P1OUT ^= BIT0;
      count = 0;
    }

    // controller: heat in range 1-3, stop in range 4-5
#ifdef MEASURE
    heat = 1;
#else
    if (range <= 3) heat = 1;
    else heat = 0;
#endif

#ifdef DYNAMIC
    // relay only stays on with a ~1 kHz signal on REL_DYN,
    // so if the program hangs the heater turns off
    if (heat) P3OUT ^= BIT5;
    else P3OUT &= ~BIT5;
#else
    if (heat) P3OUT |= BIT4;
    else P3OUT &= ~BIT4;
#endif

    __delay_cycles(500);    // 0.5 ms
  }
}

// every 2 s
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
  thermometer();
}
