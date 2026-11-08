// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025-2026 SASANO Takayoshi <uaa@uaa.org.uk>

#define LED_PIN 5 // D13 -> PB5
#define LED_PORT PORTB
#define LED_DDR DDRB

void led_output(bool state)
{
	if (state) LED_PORT |= 1 << LED_PIN;
	else LED_PORT &= ~(1 << LED_PIN);
}

void led_init(void)
{
	LED_PORT &= ~(1 << LED_PIN);
	LED_DDR |= 1 << LED_PIN;
}
