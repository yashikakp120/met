/***************************************************************************
 * @file                    main.c
 * @author                  Yashika Kundranda Ponnappa
 * @date                    10/11/2026
 * @matriculation number    XXXXXXX
 * @e-mail contact          XXXXXXX@students.uni-freiburg.de
 *
 * @brief Exercise 3 - PWM jukebox with two melodies on the piezo buzzer.
 *        PB5 or knocking: once = melody 1, twice = melody 2.
 *        PB6 = pause / resume.
 *
 * All Pin Connections:
 * - P3.6 - BUZZER (X1)
 * - P1.3 - PB5 (X11)
 * - P1.4 - PB6 (X12)
 * - JP5 set to VFO
 *
 * Tasks Completed:
 * - Task 1a: PWM, melodies in arrays, both played with 1 s pause
 *            (#define TASK_1A)                                 -> working
 * - Task 1b: PB5 with interrupt, 1x / 2x within one second    -> working
 * - Task 1c: knock detection with P3IN                        -> working
 * - Task 1d: PB6 pause / resume                               -> working
 * - Task 2:  feedback.txt                                     -> done
 *
 ******************************************************************************/

#include <templateEMP.h>

// #define TASK_1A      // uncomment for task 1a (just play both melodies)

#define C4 262
#define D4 294
#define E4 330
#define F4 349
#define G4 392
#define A4 440

#define T 300           // length of a quarter note in ms

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

const int (*song)[2];   // song that is playing right now
int songLen;
int note;               // which note
int noteTime;           // how long this note is already playing (ms)
char playing = 0;
char paused = 0;

volatile char pb5Pressed = 0;

// play a tone with PWM on P3.6 (50 % duty cycle)
void toneOn(int freq) {
  P3SEL |= BIT6;
  P3DIR |= BIT6;
  P3REN &= ~BIT6;
  TA0CCR0 = 1000000 / freq;       // 1 MHz clock -> period in us
  TA0CCR2 = TA0CCR0 / 2;          // 50 %
  TA0CCTL2 = OUTMOD_3;
  TA0CTL = TASSEL_2 + MC_1 + TACLR;
}

// stop the tone and use the piezo as knock sensor (input with pull-down)
void toneOff(void) {
  TA0CTL = MC_0;
  TA0CCTL2 = OUTMOD_0;
  P3SEL &= ~BIT6;
  P3DIR &= ~BIT6;
  P3OUT &= ~BIT6;
  P3REN |= BIT6;
}

void startSong(int number) {
  if (number == 1) {
    song = song1;
    songLen = len1;
  } else {
    song = song2;
    songLen = len2;
  }
  note = 0;
  noteTime = 0;
  playing = 1;
  paused = 0;
  P1IE &= ~BIT3;                  // no PB5 interrupt while a song is playing
  toneOn(song[0][0]);
}

void main(void) {
  int count = 0;        // presses / knocks
  int waitTime = 0;     // time since first press / knock
  int ignore = 300;     // ignore inputs for some ms (debouncing)
  char pb6Old = 0;
  char knock;
  int i;
#ifdef TASK_1A
  int step = 0;
  int pause = 0;
#endif

  initMSP();

  // PB5 and PB6 as inputs with pull-up
  P1DIR &= ~(BIT3 + BIT4);
  P1REN |= BIT3 + BIT4;
  P1OUT |= BIT3 + BIT4;

  // PB5 interrupt on falling edge, switched on later in the loop
  P1IES |= BIT3;
  P1IFG &= ~BIT3;

  toneOff();

  while (1) {
    // wait ~1 ms and look for a knock in between
    knock = 0;
    for (i = 0; i < 10; i++) {
      __delay_cycles(90);
      if (!playing && (P3IN & BIT6)) knock = 1;
    }

#ifdef TASK_1A
    // task 1a: song 1, 1 s pause, song 2
    knock = 0;
    if (!playing && !paused) {
      if (step == 0) {
        startSong(1);
        step = 1;
      } else if (step == 1) {
        pause++;
        if (pause == 1000) {
          startSong(2);
          step = 2;
        }
      }
    }
#endif

    // PB5 / knock counting
    if (ignore > 0) {
      ignore--;
#ifndef TASK_1A
      if (ignore == 0 && !playing) {
        P1IFG &= ~BIT3;
        P1IE |= BIT3;             // PB5 interrupt on again
      }
#endif
    } else if (pb5Pressed || knock) {
      if (count == 0) waitTime = 0;
      count++;
      ignore = 150;
    }
    pb5Pressed = 0;

    if (count > 0) {
      waitTime++;
      if (waitTime >= 1000) {     // one second is over
        if (count == 1) startSong(1);
        else startSong(2);
        count = 0;
      }
    }

    // PB6: pause / resume
    if (!(P1IN & BIT4) && !pb6Old) {
      if (playing) {
        playing = 0;
        paused = 1;
        toneOff();
        ignore = 300;             // PB5 and knocking work again after that
      } else if (paused) {
        playing = 1;
        paused = 0;
        P1IE &= ~BIT3;
        if (noteTime < song[note][1] - 30) toneOn(song[note][0]);
      }
      __delay_cycles(30000);      // debounce
    }
    pb6Old = !(P1IN & BIT4);

    // play the song note by note
    if (playing) {
      noteTime++;
      if (noteTime == song[note][1] - 30) {
        TA0CTL = MC_0;            // short break between notes
        TA0CCTL2 = OUTMOD_0;
      }
      if (noteTime >= song[note][1]) {
        note++;
        noteTime = 0;
        if (note < songLen) {
          toneOn(song[note][0]);
        } else {
          playing = 0;            // song finished
          toneOff();
          ignore = 300;
        }
      }
    }
  }
}

// PB5 interrupt
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
  pb5Pressed = 1;
  P1IE &= ~BIT3;      // switched on again after debouncing
  P1IFG &= ~BIT3;
}
