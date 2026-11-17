/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    17/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 4 - Dynamic circuit configuration
 *        The piezo buzzer is switched with the relay between the analog
 *        front-end (knock sensor) and the PWM output (speaker).
 *        Knock once = melody 1, knock twice within 1 s = melody 2.
 *        The relay is only switched on while a melody is playing, and the
 *        CPU sleeps in LPM4 while waiting for a knock (low power).
 *
 * All Pin Connections:
 * - BUZZER (X1)         - CON4 middle pin (common contact of the relay)
 * - CON4 right pin      - DAC_IN (X2)    (relay OFF: buzzer -> front-end)
 * - CON4 left pin       - CON3:P3.6      (relay ON:  buzzer -> PWM_PIN, TA0.2)
 * - COMP_OUT (X7)       - CON3:P1.3      (INTERRUPT_PIN, rising edge)
 * - REL_STAT (X5)       - CON3:P1.0      (relay control, high = relay on)
 * - Jumper JP5 set to VFO
 *
 * Tasks Completed:
 * - Task 1a: relay switching, knock detection with COMP_OUT interrupt,
 *            two melodies with PWM, relay only on while playing,
 *            LPM4 while waiting                                  -> working
 * - Task 1b: advantages and signal flow in Task1b.txt            -> done
 * - Task 2:  feedback.txt                                        -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

#define PWM_PIN        BIT6    // P3.6 (TA0.2)
#define INTERRUPT_PIN  BIT3    // P1.3 <- COMP_OUT
#define REL_STAT       BIT0    // P1.0 -> REL_STAT

// note frequencies in Hz
#define C4 262
#define D4 294
#define E4 330
#define F4 349
#define G4 392
#define A4 440

#define T 300           // quarter note in ms

// melodies: {frequency, length in ms}
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

volatile char knocked = 0;      // set by the interrupt

void delayMs(int ms) {
  while (ms > 0) {
    __delay_cycles(1000);       // 1 ms at 1 MHz
    ms--;
  }
}

// tone with PWM on P3.6, 50 % duty cycle
void toneOn(int freq) {
  P3SEL |= PWM_PIN;
  TA0CCR0 = 1000000 / freq;     // period in us (SMCLK = 1 MHz)
  TA0CCR2 = TA0CCR0 / 2;        // 50 %
  TA0CCTL2 = OUTMOD_3;
  TA0CTL = TASSEL_2 + MC_1 + TACLR;
}

// stop the timer, P3.6 = normal output, low
void toneOff(void) {
  TA0CTL = MC_0;
  TA0CCTL2 = OUTMOD_0;
  P3SEL &= ~PWM_PIN;
  P3OUT &= ~PWM_PIN;
}

void knockInterruptOn(void) {
  P1IFG &= ~INTERRUPT_PIN;      // forget old edges
  P1IE  |= INTERRUPT_PIN;
}

void playSong(int number) {
  const int (*song)[2];
  int len, i;

  if (number == 1) {
    song = song1;
    len = len1;
  } else {
    song = song2;
    len = len2;
  }

  P1IE &= ~INTERRUPT_PIN;       // no knock interrupt while playing

  // relay ON: buzzer is now connected to the PWM pin
  P1OUT |= REL_STAT;
  delayMs(20);                  // wait until the relay contact has switched

  for (i = 0; i < len; i++) {
    toneOn(song[i][0]);
    delayMs(song[i][1] - 30);
    toneOff();
    delayMs(30);                // short break between the notes
  }

  // relay OFF: buzzer back to the analog front-end, no coil current
  P1OUT &= ~REL_STAT;
  delayMs(500);                 // relay click and piezo must settle first

  knockInterruptOn();
}

void main(void) {
  int knocks = 0;       // number of knocks
  int waitTime = 0;     // ms since the first knock
  int ignore = 0;       // ms in which new knocks are ignored

  initMSP();

  // relay control: output, relay off (buzzer on front-end)
  P1DIR |= REL_STAT;
  P1OUT &= ~REL_STAT;

  // PWM pin: output, low while not playing
  P3DIR |= PWM_PIN;
  toneOff();

  // COMP_OUT: input, interrupt on rising edge (comparator goes high)
  P1DIR &= ~INTERRUPT_PIN;
  P1REN &= ~INTERRUPT_PIN;      // driven by the comparator, no resistor
  P1IES &= ~INTERRUPT_PIN;      // low -> high

  delayMs(500);                 // let the front-end settle after power-up
  knockInterruptOn();

  while (1) {
    // nothing to do -> sleep in LPM4 until the comparator interrupt.
    // Interrupts are disabled for the check, so no knock can get lost
    // between the check and going to sleep.
    __disable_interrupt();
    if (knocks == 0 && !knocked) {
      __bis_SR_register(LPM4_bits + GIE);   // sleep, GIE on again
    } else {
      __enable_interrupt();
    }

    delayMs(1);

    if (knocked) {
      knocked = 0;
      if (knocks == 0) {
        waitTime = 0;           // first knock starts the 1 s window
      }
      knocks++;
      ignore = 150;             // the piezo rings, ignore 150 ms
    }

    if (ignore > 0) {
      ignore--;
      if (ignore == 0) {
        knockInterruptOn();
      }
    }

    if (knocks > 0) {
      waitTime++;
      if (waitTime >= 1000) {   // 1 s over
        playSong(knocks == 1 ? 1 : 2);
        knocks = 0;
        ignore = 0;
      }
    }
  }
}

// COMP_OUT went high -> knock
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
  if (P1IFG & INTERRUPT_PIN) {
    knocked = 1;
    P1IE &= ~INTERRUPT_PIN;     // switched on again after 150 ms
    P1IFG &= ~INTERRUPT_PIN;
    __bic_SR_register_on_exit(LPM4_bits);   // wake up the main loop
  }
}
