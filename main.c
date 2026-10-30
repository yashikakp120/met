/*
 * Exercise 1 - Digital I/O and Interrupts
 * Mikrocomputertechnik / Microcontroller Techniques, WS 26/27
 *
 * Author: YOUR FIRST NAME YOUR LAST NAME      <-- put your name here!
 *
 * Wiring (done with cables on the board):
 *   PB5 (X11)        -> CON3:P1.3   (button, connects to GND when pressed)
 *   PB6 (X12)        -> CON3:P1.4   (button, connects to GND when pressed)
 *   D6  red LED      -> CON3:P1.5
 *   D5  green LED    -> CON3:P1.6
 *   D7  blue LED     -> CON3:P1.0
 *   D9  yellow LED   -> CON3:P1.7  (connected to the RIGHT pin of JP3)
 *
 * Behaviour (Table 1 of the exercise sheet):
 *   State | PB5 PB6 | D5  | D6                 | D7  | D9 (= !D6)
 *     1   |  0   0  | OFF | OFF                | OFF | ON
 *     2   |  1   0  | OFF | ON 250 ms, then OFF| OFF | OFF 250 ms, then ON
 *     3   |  0   1  | ON  | OFF                | OFF | ON
 *     4   |  1   1  | OFF | OFF                | ON  | ON
 *
 * Task 1a: both buttons are read by polling (with software debouncing).
 * Task 1b: the red LED D6 and the yellow LED D9 are controlled only by the
 *          function setRedLed().
 * Task 1c: comment out "#define polling" below -> PB5 is read with a
 *          Port 1 interrupt instead, PB6 is still polled.
 */

#include "templateEMP.h"

// ---------------------------------------------------------------------------
// Task 1c switch:
//   #define polling      -> PB5 is polled           (Task 1a / 1b)
//   // #define polling   -> PB5 uses the interrupt  (Task 1c)
// ---------------------------------------------------------------------------
#define polling

// Pin names, so the code is easier to read
#define PB5        BIT3   // button PB5 on P1.3
#define PB6        BIT4   // button PB6 on P1.4
#define LED_BLUE   BIT0   // D7 on P1.0
#define LED_RED    BIT5   // D6 on P1.5
#define LED_GREEN  BIT6   // D5 on P1.6
#define LED_YELLOW BIT7   // D9 on P1.7

// Timing (clock is 1 MHz -> 1 cycle = 1 us)
#define DEBOUNCE_CYCLES 20000    // 20 ms
#define FLASH_CYCLES    250000   // 250 ms

#ifndef polling
// Set by the interrupt routine: 1 = PB5 pressed, 0 = released.
// "volatile" because it is changed inside an interrupt.
volatile unsigned char pb5Pressed = 0;
#endif

/*
 * Task 1b: controls the red LED D6 and the yellow LED D9.
 * D9 is always the opposite of D6 (D9 = !D6).
 *   on = 1 -> D6 ON,  D9 OFF
 *   on = 0 -> D6 OFF, D9 ON
 */
void setRedLed(unsigned char on) {
  if (on) {
    P1OUT |= LED_RED;
    P1OUT &= ~LED_YELLOW;
  } else {
    P1OUT &= ~LED_RED;
    P1OUT |= LED_YELLOW;
  }
}

/*
 * Reads the buttons once (raw, not debounced).
 * Returns a 2-bit number: bit 0 = PB5 pressed, bit 1 = PB6 pressed.
 * The buttons pull the pin to GND when pressed, so pressed = pin reads 0.
 */
unsigned char readButtons(void) {
  unsigned char buttons = 0;

#ifdef polling
  if (!(P1IN & PB5)) {        // PB5 read by polling
    buttons |= 0x01;
  }
#endif
#ifndef polling
  if (pb5Pressed) {           // PB5 state comes from the interrupt
    buttons |= 0x01;
  }
#endif

  if (!(P1IN & PB6)) {        // PB6 is always polled
    buttons |= 0x02;
  }
  return buttons;
}

/*
 * Debounced read: read the buttons, wait 20 ms, read again.
 * Only if both readings are the same, the result is accepted.
 * Returns 0xFF if the buttons were still bouncing.
 */
unsigned char readButtonsDebounced(void) {
  unsigned char first = readButtons();
  __delay_cycles(DEBOUNCE_CYCLES);
  if (readButtons() == first) {
    return first;
  }
  return 0xFF;
}

/*
 * Sets the LEDs for a newly entered state (Table 1).
 * buttons: bit 0 = PB5, bit 1 = PB6
 */
void enterState(unsigned char buttons) {
  switch (buttons) {
    case 0x00:                          // State 1: nothing pressed
      P1OUT &= ~(LED_GREEN + LED_BLUE);
      setRedLed(0);                     // D6 OFF, D9 ON
      break;

    case 0x01:                          // State 2: only PB5 pressed
      P1OUT &= ~(LED_GREEN + LED_BLUE);
      setRedLed(1);                     // D6 ON, D9 OFF ...
      __delay_cycles(FLASH_CYCLES);     // ... for 250 ms ...
      setRedLed(0);                     // ... then D6 OFF, D9 ON
      break;

    case 0x02:                          // State 3: only PB6 pressed
      P1OUT &= ~LED_BLUE;
      P1OUT |= LED_GREEN;               // D5 ON
      setRedLed(0);
      break;

    case 0x03:                          // State 4: both pressed
      P1OUT &= ~LED_GREEN;
      P1OUT |= LED_BLUE;                // D7 ON
      setRedLed(0);
      break;
  }
}

void main(void) {
  unsigned char currentState;   // which button combination is active
  unsigned char buttons;

  // ALWAYS call this first (stops watchdog, sets 1 MHz clock, starts UART,
  // enables interrupts globally).
  initMSP();

  // LEDs: outputs. Only touch our own bits (P1.1/P1.2 are used by the UART).
  P1DIR |= LED_BLUE + LED_RED + LED_GREEN + LED_YELLOW;

  // Buttons: inputs with internal PULL-UP resistors.
  // (Button pressed -> pin connected to GND -> reads 0.
  //  Button released -> without pull-up the pin would "float", so we pull
  //  it up to VCC -> reads 1.)
  P1DIR &= ~(PB5 + PB6);   // input
  P1REN |= PB5 + PB6;      // enable pull resistor
  P1OUT |= PB5 + PB6;      // resistor pulls UP

#ifndef polling
  // Task 1c: interrupt for PB5 (P1.3), similar to Listing 3
  P1IE  |= PB5;            // enable interrupt for P1.3
  P1IES |= PB5;            // interrupt on high->low edge (= press)
  P1IFG &= ~PB5;           // clear interrupt flag
  pb5Pressed = (P1IN & PB5) ? 0 : 1;   // start value
#endif

  // Start in state 1 (nothing pressed)
  currentState = 0x00;
  enterState(currentState);

  while (1) {
    buttons = readButtonsDebounced();

    // Only react when a stable reading shows a DIFFERENT state
    if (buttons != 0xFF && buttons != currentState) {
      currentState = buttons;
      enterState(currentState);
    }
  }
}

#ifndef polling
/*
 * Task 1c: Port 1 interrupt routine. Runs automatically when PB5 is
 * pressed or released.
 * We switch the edge every time, so we get one interrupt on press and one
 * on release.
 */
#pragma vector = PORT1_VECTOR
__interrupt void Port_1(void) {
  if (P1IFG & PB5) {
    __delay_cycles(DEBOUNCE_CYCLES);   // wait until the bouncing is over

    if (!(P1IN & PB5)) {
      pb5Pressed = 1;          // button is pressed
      P1IES &= ~PB5;           // next interrupt: low->high (= release)
    } else {
      pb5Pressed = 0;          // button is released
      P1IES |= PB5;            // next interrupt: high->low (= press)
    }

    // Clear the flag LAST (changing P1IES can set the flag again)
    P1IFG &= ~PB5;
  }
}
#endif
