#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include "nec.h"


#define BAUD 9600
#define BAUDRATE (F_CPU / (BAUD * 16UL) - 1)

void uart_init(void);
void uart_send(uint8_t data);


void uart_init(void) {
    UBRR0H = BAUDRATE >> 8;
    UBRR0L = BAUDRATE >> 0;
    UCSR0B |= _BV(TXEN0) | _BV(RXEN0);
    UCSR0C |= _BV(USBS0) | (3 << UCSZ00);
}


void uart_send(uint8_t data) {
    while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = data;
}

#define blink(delay) do {PORTC |= _BV(0);_delay_ms(delay); PORTC &= (uint8_t)~_BV(0);} while (0)

int main(void) {
    uint32_t command;

    nec_init();
    nec_start();
    DDRC |= _BV(PC0);
    sei();
    for (;;) {
        if (nec_read(&command)) {
            nec_start();
            if (command) {
                blink(200);
            }
            else {
                blink(100);
                blink(100);
            }
        }
        _delay_ms(500);
    }

    return 0;
}

