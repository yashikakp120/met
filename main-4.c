/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    17/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 4 - the relay switches the buzzer between the analog
 *        front-end (knock sensor) and the PWM output (melody).
 *        Knock once = melody 1, knock twice = melody 2.
 *
 * All Pin Connections:
 * - BUZZER (X1)     - CON4 middle (relay common)
 * - CON4 right      - DAC_IN (X2)   (relay off)
 * - CON4 left       - P3.6          (relay on, PWM_PIN TA0.2)
 * - COMP_OUT (X7)   - P1.3          (INTERRUPT_PIN)
 * - REL_STAT (X5)   - P1.0
 * - JP5 set to VFO
 *
 * Tasks Completed:
 * - Task 1a: relay switching, knock interrupt, two melodies     -> working
 * - Task 1b: Task1b.txt                                         -> done
 * - Task 2:  feedback.txt                                       -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

#define C4 262
#define D4 294
#define E4 330
#define F4 349
#define G4 392
#define A4 440

#define T 300           // quarter note in ms

// {frequency, length in ms}
// melody 1: Alle meine Entchen
const int song1[][2] = {
  {C4,T},{D4,T},{E4,T},{F4,T},{G4,2*T},{G4,2*T},
  {A4,T},{A4,T},{A4,T},{A4,T},{G4,4*T},
  {A4,T},{A4,T},{A4,T},{A4,T},{G4,4*T},
  {F4,T},{F4,T},{F4,T},{F4,T},{E4,2*T},{E4,2*T},
  {D4,T},{D4,T},{D4,T},{D4,T},{C4,4*T}
};
// melody 2: Ode to Joy
const int song2[][2] = {
  {E4,T},{E4,T},{F4,T},{G4,T},{G4,T},{F4,T},{E4,T},{D4,T},
  {C4,T},{C4,T},{D4,T},{E4,T},{E4,3*T/2},{D4,T/2},{D4,2*T},
  {E4,T},{E4,T},{F4,T},{G4,T},{G4,T},{F4,T},{E4,T},{D4,T},
  {C4,T},{C4,T},{D4,T},{E4,T},{D4,3*T/2},{C4,T/2},{C4,2*T}
};
const int len1 = sizeof(song1) / sizeof(song1[0]);
const int len2 = sizeof(song2) / sizeof(song2[0]);

volatile char knock = 0;

void wait(int ms) {
  while (ms--) __delay_cycles(1000);
}

void play(const int song[][2], int len) {
  int i;

  P1IE &= ~BIT3;            // ignore knocks while playing
  P1OUT |= BIT0;            // relay on -> buzzer on P3.6
  wait(20);                 // give the relay time to switch

  for (i = 0; i < len; i++) {
    // PWM with 50 % duty cycle
    P3SEL |= BIT6;
    TA0CCR0 = 1000000 / song[i][0];
    TA0CCR2 = TA0CCR0 / 2;
    TA0CCTL2 = OUTMOD_3;
    TA0CTL = TASSEL_2 + MC_1 + TACLR;
    wait(song[i][1] - 30);

    // short break between notes
    TA0CTL = MC_0;
    P3SEL &= ~BIT6;
    wait(30);
  }

  P1OUT &= ~BIT0;           // relay off -> buzzer back on the front-end
  wait(500);                // wait until the relay click is over

  P1IFG &= ~BIT3;
  P1IE |= BIT3;             // listen for knocks again
}

void main(void) {
  int count = 0;            // knocks
  int time = 0;             // ms since the first knock
  int ignore = 0;           // ms to ignore new knocks

  initMSP();

  // relay off at the start
  P1DIR |= BIT0;
  P1OUT &= ~BIT0;

  // P3.6 output, low when no tone is played
  P3DIR |= BIT6;
  P3OUT &= ~BIT6;

  // COMP_OUT on P1.3, interrupt on rising edge
  P1DIR &= ~BIT3;
  P1IES &= ~BIT3;
  wait(500);
  P1IFG &= ~BIT3;
  P1IE |= BIT3;

  while (1) {
    // nothing to do -> go to sleep (LPM4) until a knock wakes us up
    __disable_interrupt();
    if (count == 0 && !knock) {
      __bis_SR_register(LPM4_bits + GIE);
    }
    __enable_interrupt();

    wait(1);

    if (knock) {
      knock = 0;
      if (count == 0) time = 0;
      count++;
      ignore = 150;         // the piezo vibrates a bit, so wait
    }

    if (ignore > 0) {
      ignore--;
      if (ignore == 0) {
        P1IFG &= ~BIT3;
        P1IE |= BIT3;
      }
    }

    if (count > 0) {
      time++;
      if (time >= 1000) {   // one second is over
        if (count == 1) play(song1, len1);
        else play(song2, len2);
        count = 0;
        ignore = 0;
      }
    }
  }
}

// comparator output went high = knock
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
  knock = 1;
  P1IE &= ~BIT3;
  P1IFG &= ~BIT3;
  __bic_SR_register_on_exit(LPM4_bits);   // wake up
}
