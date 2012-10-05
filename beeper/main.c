/*
 * main.c
 */

#include  "msp430g2553.h"
#include <uart.h>

#define SPEAKER_COUNT 2
#define COMMANDS_COUNT 2

#define CMD_HANDSHAKE 0
#define CMD_PLAY 1

#define F_TCLK 1000000

#define SPK1_OUT P1OUT
#define SPK1_PORT BIT7

#define SPK2_OUT P1OUT
#define SPK2_PORT BIT4

unsigned int spk_freq[SPEAKER_COUNT];

inline unsigned short get_timer_compare(unsigned short freq) {
	return (freq / 2) - 1;
}

void set_speaker(unsigned char speaker, unsigned short freq) {
	spk_freq[speaker] = freq;

	if (speaker == 0) {
		return;
	}

	switch (speaker) {
	case 0:
		CCR0 = get_timer_compare(freq);
		CCTL0 |= CCIE;
		break;

	case 1:
		CCR1 = get_timer_compare(freq);
		CCTL1 |= CCIE;
		break;
	}
}

/*
 * Обработка комманды HANDSHAKE (поздороваться).
 */
void cmd_handshake() {
	uart_putc(SPEAKER_COUNT);
}

/**
 * Обработка комманды PLAY.
 */
void cmd_play() {
	unsigned char speaker = uart_getc();
	unsigned int freq = uart_getw();

	if (speaker >= SPEAKER_COUNT) {
		return;
	}

	set_speaker(speaker, freq);

}

void (*command_handlers[COMMANDS_COUNT])() = {
	/* CMD_HANDSHAKE */
	&cmd_handshake,

	/* CMD_PLAY */
	&cmd_play
}	;

void main(void) {
	WDTCTL = WDTPW | WDTHOLD;

	TA0CTL = TASSEL_2 | MC_2 | TAIE;
	CCTL1 |= CCIE;

	// Включить на вывод P1.4 и P1.7
	P1DIR |= BIT4 + BIT7;
	P1SEL |= BIT4 + BIT7;

	set_speaker(0, 220);
	set_speaker(1, 440);

	__enable_interrupt();

	// Инициализировать UART.
	uart_init();

	while (1) {

//		unsigned short c = uart_getw();
//
//
//		uart_putc(c & 0xFF);
//
//		int i = 5000;
//		while (i--);
//
//		uart_putc((c >> 8) & 0xFF);

		unsigned char ch = uart_getc();

		if (ch < COMMANDS_COUNT) {
			command_handlers[ch]();
		}
	}
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER1_ISR(void) {
	switch (TA0IV) {
	// CCR1
	case 2:
		SPK1_OUT ^= SPK1_PORT;
		CCR1 += get_timer_compare(spk_freq[0]);
		break;

	case 4:
		SPK2_OUT ^= SPK2_PORT;
		CCR2 += get_timer_compare(spk_freq[1]);
		break;
	}
}
