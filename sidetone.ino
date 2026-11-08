// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025-2026 SASANO Takayoshi <uaa@uaa.org.uk>

#define TONE_PIN 3 // D11 -> PB3/OC2A
#define TONE_PORT PORTB
#define TONE_DDR DDRB
#define TONE_HZ 780

void sidetone_output(bool state)
{
	if (state) {
		TCCR2A = 0x42;	// CTC, toggle OC2A on compare match
		TCCR2B = 0x06;	// CTC, F_CLK / 256
	} else {
		TCCR2A = 0;
		TCCR2B = 0;
	}
}

void sidetone_init(void)
{
	// set gpio/low when tone off
	TONE_PORT &= ~(1 << TONE_PIN);
	TONE_DDR |= 1 << TONE_PIN;

	TIMSK2 = 0;
	TCCR2A = 0;
	TCCR2B = 0;
	OCR2A = ((F_CPU / 256UL) / TONE_HZ + 1) / 2 - 1;
}
