/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    03/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 2 - Interfacing external ICs (shift registers)
 *        Running light on D1-D4 (driven by shift register 2), controlled
 *        like a tape recorder with PB1-PB4 (read via shift register 1).
 *        Bonus: rotating light on D5-D7 running in sync.
 *
 * All Pin Connections:
 * - CON2: P2.0 - S0(2)    (jumper)
 * - CON2: P2.1 - S1(2)    (jumper)
 * - CON2: P2.2 - S0(1)    (jumper)
 * - CON2: P2.3 - S1(1)    (jumper)
 * - CON2: P2.4 - CK       (jumper)
 * - CON2: P2.5 - /CLR     (jumper)
 * - CON2: P2.6 - SR(2)    (jumper)
 * - CON2: P2.7 - QD(1)    (jumper)
 * - Bonus: CON3:P1.5 - LED rt (D6), CON3:P1.6 - LED gn (D5),
 *          CON3:P1.0 - X10 HEATER (D7, blue)
 *
 * Tasks Completed:
 * - Task 1: initial state pause, all LEDs off            -> working
 * - Task 1: PB3 play (4 steps/s, D1 -> D4)               -> working
 * - Task 1: PB2 pause                                    -> working
 * - Task 1: PB4 fast forward (8 steps/s) while held      -> working
 * - Task 1: PB1 rewind (8 steps/s, D4 -> D1) while held  -> working
 * - Task 1: buttons are read every 5 ms, so the program
 *           reacts at any time                           -> working
 * - Bonus:  rotating light D6 -> D7 -> D5 in sync        -> working
 * - Task 2: feedback.txt                                 -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

// comment this out if the bonus LEDs D5-D7 are not connected
#define BONUS

// Port 2 pins (CON2)
#define S0_2   BIT0
#define S1_2   BIT1
#define S0_1   BIT2
#define S1_1   BIT3
#define CK     BIT4
#define CLR    BIT5
#define SR_2   BIT6
#define QD_1   BIT7

// buttons as bits in the value returned by readButtons()
#define PB1    0x01
#define PB2    0x02
#define PB3    0x04
#define PB4    0x08

// timing: one loop pass takes about 5 ms
#define TICK_CYCLES  5000      // 5 ms at 1 MHz
#define NORMAL_TICKS 50        // 50 * 5 ms = 250 ms -> 4 steps per second
#define FAST_TICKS   25        // 25 * 5 ms = 125 ms -> 8 steps per second

// one rising edge on CK
void clockPulse(void) {
  P2OUT |= CK;
  P2OUT &= ~CK;
}

// Both shift registers share the same clock. So the register we are not
// using must always be in "hold" mode (S0 = 0, S1 = 0), otherwise it would
// also change on every clock pulse.

/*
 * Write a pattern to the LEDs D1-D4 (shift register 2).
 * bit 0 = D1, bit 1 = D2, bit 2 = D3, bit 3 = D4
 */
void setLeds(unsigned char pattern) {
  int i;

  P2OUT &= ~(S0_1 | S1_1);     // register 1: hold
  P2OUT |= S0_2;               // register 2: shift right (S0=1, S1=0)
  P2OUT &= ~S1_2;

  // shift right moves SR -> QA -> QB -> QC -> QD,
  // so the bit for D4 has to go in first
  for (i = 3; i >= 0; i--) {
    if (pattern & (1 << i)) {
      P2OUT |= SR_2;
    } else {
      P2OUT &= ~SR_2;
    }
    clockPulse();
  }

  P2OUT &= ~(S0_2 | S1_2);     // register 2: hold
}

/*
 * Read the buttons PB1-PB4 (shift register 1).
 * PB1-PB4 are connected to the inputs A-D. A pressed button gives a 1.
 * Returns bit 0 = PB1 ... bit 3 = PB4.
 */
unsigned char readButtons(void) {
  unsigned char buttons = 0;
  int i;

  P2OUT &= ~(S0_2 | S1_2);     // register 2: hold (keep the LEDs)

  P2OUT |= S0_1 | S1_1;        // register 1: parallel load (S0=1, S1=1)
  clockPulse();                // A-D (the buttons) -> QA-QD

  P2OUT &= ~S1_1;              // register 1: shift right (S0=1, S1=0)

  // QD holds PB4 now. After each shift right QD gets the next button.
  for (i = 3; i >= 0; i--) {
    if (P2IN & QD_1) {
      buttons |= (1 << i);
    }
    if (i > 0) {
      clockPulse();
    }
  }

  P2OUT &= ~(S0_1 | S1_1);     // register 1: hold
  return buttons;
}

#ifdef BONUS
/*
 * Bonus: rotating light D6 -> D7 -> D5 (clockwise around the LDR).
 * pos = -1 means all off.
 */
void setRing(int pos) {
  P1OUT &= ~(BIT0 | BIT5 | BIT6);
  if (pos == 0) P1OUT |= BIT5;   // D6 red
  if (pos == 1) P1OUT |= BIT0;   // D7 blue
  if (pos == 2) P1OUT |= BIT6;   // D5 green
}
#endif

void main(void) {
  unsigned char buttons;
  char playing = 0;    // 0 = pause, 1 = play
  int pos = -1;        // current LED 0-3 (D1-D4), -1 = all off at start
  int ring = -1;       // bonus LED 0-2, -1 = all off at start
  int dir;             // +1 = forward, -1 = backward, 0 = stand still
  int period;          // ticks between two steps
  int ticks = 0;

  initMSP();

  // Port 2: P2.6 and P2.7 are crystal pins by default -> make them normal I/O
  P2SEL  &= ~(BIT6 | BIT7);
  P2SEL2 &= ~(BIT6 | BIT7);

  // P2.0 - P2.6 outputs, P2.7 (QD of register 1) input
  P2DIR |= S0_2 | S1_2 | S0_1 | S1_1 | CK | CLR | SR_2;
  P2DIR &= ~QD_1;

  // both registers on hold, clock low
  P2OUT &= ~(S0_2 | S1_2 | S0_1 | S1_1 | CK | SR_2);

  // reset the shift registers (all LEDs off), then /CLR back to 1
  P2OUT &= ~CLR;
  P2OUT |= CLR;

#ifdef BONUS
  P1DIR |= BIT0 | BIT5 | BIT6;
  setRing(-1);
#endif

  while (1) {
    __delay_cycles(TICK_CYCLES);
    buttons = readButtons();

    // play and pause stay active after releasing the button
    if (buttons & PB3) playing = 1;
    if (buttons & PB2) playing = 0;

    // fast forward and rewind only while the button is held,
    // afterwards we automatically go back to play or pause
    if (buttons & PB4) {
      dir = 1;
      period = FAST_TICKS;
    } else if (buttons & PB1) {
      dir = -1;
      period = FAST_TICKS;
    } else if (playing) {
      dir = 1;
      period = NORMAL_TICKS;
    } else {
      dir = 0;
      period = NORMAL_TICKS;
    }

    if (dir == 0) {
      ticks = 0;       // paused: keep the current LED
      continue;
    }

    if (pos == -1) {
      // very first step: start at D1 (forward) or D4 (rewind)
      pos  = (dir > 0) ? 0 : 3;
      ring = (dir > 0) ? 0 : 2;
      ticks = 0;
    } else {
      ticks++;
      if (ticks < period) {
        continue;
      }
      ticks = 0;
      // only go to the neighbouring LED (wraps around D4 <-> D1)
      pos  = (pos + dir + 4) % 4;
      ring = (ring + dir + 3) % 3;
    }

    setLeds(1 << pos);
#ifdef BONUS
    setRing(ring);
#endif
  }
}
