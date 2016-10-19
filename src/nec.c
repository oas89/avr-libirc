#include <avr/io.h>
#include <avr/interrupt.h>
#include "nec.h"

// NEC IR protocol using TSOP173x receivers (with active LOW)
//
//  +               +-------+ +-+ +---+ + . -+ +---+ +
//  |      AGC      |  GAP  |P|S|P| L |P| .  |P| L |P|
//  +---------------+       +-+ +-+   +-+ .  +-+   +-+
//
//  |_______________________|___|_____|  ....  |_____|
//           HEADER          LSB                 MSB
//                          |____ 32 bit payload ____|
//
//  +        +--------+        +---------------------+
//  | 0.56ms | 0.56ms | 0.56ms |       1.68ms        |
//  +--------+        +--------+                     +
//
//  |    LOGIC '1'    |           LOGIC '0'          |
//
//
//  |   8 bit  |    8 bit   |    8 bit  |    8 bit   |
//  +----------+------------+-----------+------------+
//  |  ADDRESS |  ~ADDRESS  |  COMMAND  |  ~COMMAND  |
//  +----------+------------+-----------+------------+
//  |LSB    MSB|LSB      MSB|LSB     MSB|LSB      MSB|
//
// AGC     | 9.00ms preabula
// GAP     | 4.50ms or 2.25ms for repeat code
// P(ULSE) | 0.56ms pulse
// S(HORT) | 0.56ms logic one
// L(ONG)  | 1.68ms logic zero
//
// Using 8-bit timer with /1024 prescaler at 16MHz gives one tick per 64us
// with overflow at 16ms, so state machine should be reset if overflow flag
// is high.


#define NEC_STATE_AGC 0
#define NEC_STATE_GAP 1
#define NEC_STATE_BIT 2
#define NEC_STATE_FIN 3

#define NEC_TIMING_AGC_MIN 120
#define NEC_TIMING_AGC_MAX 160
#define NEC_TIMING_NEW_MIN 60
#define NEC_TIMING_NEW_MAX 80
#define NEC_TIMING_REPEAT_MIN 30
#define NEC_TIMING_REPEAT_MAX 40
#define NEC_TIMING_SHORT_MIN 5
#define NEC_TIMING_SHORT_MAX 10
#define NEC_TIMING_LONG_MIN 20
#define NEC_TIMING_LONG_MAX 30


static volatile uint32_t buffer = 0;
static volatile uint8_t ready = 0;
static volatile uint8_t counter = 32;
static volatile uint8_t state = NEC_STATE_AGC;


void nec_init() {
     // Configure PD1 as input
     DDRD &= (uint8_t)~_BV(PD2);
     // Configure INT0 triggering on any edge
     EICRA |= _BV(ISC00);
     // Configure TIMER0 normal operation
     TCCR0A = 0;
     // Configure TIMER0 prescaler to 1024
     TCCR0B |= _BV(CS00) | _BV(CS02);
}


void nec_start(void) {
    buffer = 0;
    ready = 0;
    counter = 32;
    state = NEC_STATE_AGC;
    // Enable INT0 interrupt
    EIMSK |= (uint8_t)_BV(INT0);
}


void nec_stop(void) {
    // Disable INT0 interrupt
    EIMSK &= (uint8_t)~_BV(INT0);
}


uint8_t nec_read(uint32_t *data) {
    if (ready) {
        *data = buffer;
        return 1;
    }
    return 0;
}


ISR(INT0_vect) {
    uint8_t pin = PIND & _BV(PD2);
    uint8_t tau = TCNT0;
    TCNT0 = 0;

    if (0 && TIFR0 & _BV(TOV0)) {
        buffer = 0;
        ready = 0;
        counter = 32;
        state = NEC_STATE_AGC;
    }

    TIFR0 |= _BV(TOV0);

    if (state == NEC_STATE_AGC && pin) {
        if (tau > NEC_TIMING_AGC_MIN && tau < NEC_TIMING_AGC_MAX) {
            state = NEC_STATE_GAP;
            return;
        }
    }

    if (state == NEC_STATE_GAP && !pin) {
        if (tau > NEC_TIMING_NEW_MIN && tau < NEC_TIMING_NEW_MAX) {
            state = NEC_STATE_BIT;
            return;
        }
        if (tau > NEC_TIMING_REPEAT_MIN && tau < NEC_TIMING_REPEAT_MAX) {
            state = NEC_STATE_FIN;
            return;
        }
    }

    if (state == NEC_STATE_BIT && !pin) {
        if (tau > NEC_TIMING_SHORT_MIN && tau < NEC_TIMING_SHORT_MAX) {
            counter--;
            buffer |= (uint32_t)(0 << counter);
            if (counter == 0) {
                state = NEC_STATE_FIN;
            }
            return;
        }
        if (tau > NEC_TIMING_LONG_MIN && tau < NEC_TIMING_LONG_MAX) {
            counter--;
            buffer |= (uint32_t)(1 << counter);
            if (counter == 0) {
                state = NEC_STATE_FIN;
            }
            return;
        }
    }

    if (state == NEC_STATE_FIN && pin) {
        //if (((buffer >> 24 & 0xff) ^ ((buffer >> 16) & 0xff)) != 0xff) {
        //    state = NECX_STATE_AGC;
        //    return;
        //}
        //if (((buffer >> 8 & 0xff) & ((buffer >> 0) & 0xff)) != 0xff) {
        //    state = NECX_STATE_AGC;
        //    return;
        //}
        ready = 1;
        EIMSK &= (uint8_t)~_BV(INT0);
        return;
    }

    state = NEC_STATE_AGC;
}

