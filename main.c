/*
 * Exercise 1 - Digital I/O and Interrupts
 * Name: Yashika Kundranda Ponnappa
 *
 * PB5 -> P1.3, PB6 -> P1.4
 * D7 (blue) -> P1.0, D6 (red) -> P1.5, D5 (green) -> P1.6, D9 (yellow) -> P1.7
 */

#include "templateEMP.h"

// comment this line out to use the interrupt for PB5 (task 1c)
#define polling

#ifndef polling
volatile char pb5 = 0;   // set in the ISR
#endif

// task 1b: red LED D6, yellow LED D9 is always the opposite
void redLed(char on) {
  if (on) {
    P1OUT |= BIT5;
    P1OUT &= ~BIT7;
  } else {
    P1OUT &= ~BIT5;
    P1OUT |= BIT7;
  }
}

// returns 1 = PB5, 2 = PB6, 3 = both, 0 = none
char readButtons(void) {
  char b = 0;
#ifdef polling
  if (!(P1IN & BIT3)) b |= 1;
#endif
#ifndef polling
  if (pb5) b |= 1;
#endif
  if (!(P1IN & BIT4)) b |= 2;
  return b;
}

void main(void) {
  char state = 0;
  char now;

  initMSP();

  // LEDs as outputs
  P1DIR |= BIT0 + BIT5 + BIT6 + BIT7;

  // buttons as inputs with pull-up (button connects to GND)
  P1DIR &= ~(BIT3 + BIT4);
  P1REN |= BIT3 + BIT4;
  P1OUT |= BIT3 + BIT4;

#ifndef polling
  P1IE  |= BIT3;    // interrupt for PB5
  P1IES |= BIT3;    // falling edge = pressed
  P1IFG &= ~BIT3;
#endif

  // start in state 1
  P1OUT &= ~(BIT0 + BIT6);
  redLed(0);

  while (1) {
    // simple debounce: read, wait 20 ms, read again
    now = readButtons();
    __delay_cycles(20000);
    if (now != readButtons()) continue;

    // only do something when the state changes
    if (now == state) continue;
    state = now;

    P1OUT &= ~(BIT0 + BIT6);   // green and blue off
    redLed(0);                 // red off, yellow on

    if (state == 1) {          // state 2: only PB5
      redLed(1);
      __delay_cycles(250000);  // 250 ms
      redLed(0);
    } else if (state == 2) {   // state 3: only PB6
      P1OUT |= BIT6;
    } else if (state == 3) {   // state 4: both
      P1OUT |= BIT0;
    }
  }
}

#ifndef polling
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
  __delay_cycles(20000);       // debounce
  if (!(P1IN & BIT3)) {
    pb5 = 1;
    P1IES &= ~BIT3;            // wait for release next
  } else {
    pb5 = 0;
    P1IES |= BIT3;             // wait for press next
  }
  P1IFG &= ~BIT3;
}
#endif
