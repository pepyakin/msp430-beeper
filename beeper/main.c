/*
 * main.c
 */

#include  "msp430g2553.h"
#include <uart.h>

#define SPEAKER_COUNT 2
#define COMMANDS_COUNT 2

#define CMD_HANDSHAKE 0
#define CMD_PLAY 1

#define F_TCLK 500000

#define SPK1_OUT P1OUT
#define SPK1_PORT BIT7

#define SPK2_OUT P1OUT
#define SPK2_PORT BIT4

unsigned int spk_freq[SPEAKER_COUNT];


unsigned short get_timer_compare(unsigned short freq) {
	// return (freq / 2) - 1;
	return (unsigned short)((F_TCLK / (unsigned int)freq) - 1);
}

void set_speaker(unsigned char speaker, unsigned short freq) {
	spk_freq[speaker] = freq;

	volatile unsigned int *t_ccr;
	volatile unsigned int *t_ctl;

	switch (speaker) {
	case 0:
		t_ccr = &TA0CCR0;
		t_ctl = &TA0CTL;
		break;

	case 1:
		t_ccr = &TA1CCR0;
		t_ctl = &TA1CTL;
		break;
	}

	if (freq == 0) {
		// Служит для выключения звука.
		*t_ccr = 0;
		*t_ctl &= ~(MC_1 + MC_2);
	} else {
		*t_ccr = get_timer_compare(freq);
		*t_ctl |= MC_1;
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

void timers_init() {
	TA0CCTL0 = CM_0 + CCIS_0 + OUTMOD_4;
	TA0CTL = TASSEL_2 + ID_1;
	TA0CCR0 = 999;

	TA1CCTL0 = CM_0 + CCIS_0 + OUTMOD_4;
	TA1CTL = TASSEL_2 + ID_1;
	TA1CCR0 = 499;
}

// #pragma FUNC_NEVER_RETURNS(main);
void main(void) {
	WDTCTL = WDTPW | WDTHOLD;

	timers_init();

	P1SEL |= BIT5;
	P1DIR |= BIT5;

	P2SEL |= BIT3;
	P2DIR |= BIT3;


	unsigned char i;
	for (i = 0; i < SPEAKER_COUNT; i++) {
		set_speaker(i, 0);
	}

	__enable_interrupt();

	// Инициализировать UART.
	uart_init();

	while (1) {
		unsigned char ch = uart_getc();

		if (ch < COMMANDS_COUNT) {
			command_handlers[ch]();
		}
	}
}
