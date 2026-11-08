// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025-2026 SASANO Takayoshi <uaa@uaa.org.uk>

#define OUT_0 4 // D4 -> PD4
#define OUT_PORT PORTD
#define OUT_DDR DDRD

#define IN_0 1 // D15 -> PC1
#define IN_1 0 // D14 -> PC0
#define IN_PORT PORTC
#define IN_PIN PINC
#define IN_DDR DDRC
#define IN_MSK PCMSK1
#define IN_ICR 1 // PCIE1 -> PCICR bit 1
#define IN_vect PCINT1_vect

#define PIN0_ON 0x01 // dit
#define PIN1_ON 0x02 // dah

#define DIT_LEN 750 // in 100us unit
#define BASE_TICK ((F_CPU / 8) / 10000UL)
#define DIT_ON_TICK DIT_LEN
#define DIT_OFF_TICK DIT_LEN
#define DAH_ON_TICK (DIT_LEN * 3)
#define DAH_OFF_TICK DIT_LEN

volatile unsigned char TimerExpired = 0;
volatile unsigned char CurrPinStatus = 0;
volatile unsigned char PrevPinStatus = 0;
#define PinMaskCount 40 // 4ms debounce
volatile unsigned char PinMaskCounter0 = 0;
volatile unsigned char PinMaskCounter1 = 0;
volatile unsigned short PlayerCount = 0;
volatile unsigned char PinMemory = 0;

enum player_status {
	PlayNothing,
	PlayDit,
	PlayDah,
	StopDit,
	StopDah,
};
volatile enum player_status PlayerStatus = PlayNothing;

static void timer_start(void);
static void timer_stop(void);
static void gpio_int_disable(void);

extern void led_output(bool state) __attribute__((weak));
extern void led_init(void) __attribute__((weak));
extern void sidetone_output(bool state) __attribute__((weak));
extern void sidetone_init(void) __attribute__((weak));

static void update_pin_status(void)
{
	unsigned char d = IN_PIN;

	CurrPinStatus = 0;

	if (!(d & (1 << IN_0))) CurrPinStatus |= PIN0_ON;
	if (!(d & (1 << IN_1))) CurrPinStatus |= PIN1_ON;
}

ISR(IN_vect)
{
	gpio_int_disable();
	timer_start();

	update_pin_status();

	PlayerCount = 0;
	PinMaskCounter0 = 0;
	PinMaskCounter1 = 0;

	TimerExpired = 1;
}

ISR(TIMER1_COMPA_vect)
{
	update_pin_status();

	if (PlayerCount) PlayerCount--;
	if (PinMaskCounter0) PinMaskCounter0--;
	if (PinMaskCounter1) PinMaskCounter1--;

	TimerExpired = 1;
}

static void gpio_int_enable(void)
{
	// no need to clear PCIFR
	PCICR = 1 << IN_ICR;
	IN_MSK = (1 << IN_0) | (1 << IN_1);
}

static void gpio_int_disable(void)
{
	IN_MSK = 0;
	PCICR = 0;
}

static void input_init(void)
{
	/* set input pins */
	IN_DDR &= ~((1 << IN_0) | (1 << IN_1));
	MCUCR &= 0xef; // clear PUD (-> pull-up enable)
	IN_PORT |= ((1 << IN_0) | (1 << IN_1));

	/* enable port change interrupt (rise/fall edge) */
	PCICR = 0;
	PCMSK0 = 0;
	PCMSK1 = 0;
	PCMSK2 = 0;
	PCIFR = ~0;
}

static void gpio_init(void)
{
	OUT_PORT &= ~(1 << OUT_0);
	OUT_DDR |= 1 << OUT_0;
}

static void timer_init(void)
{
	TIMSK0 = 0;		// disble Arduino system timer (required)
	TCCR0B = 0;

	timer_stop();
	TCCR1A = 0;
	TCCR1C = 0;
}

static void timer_start(void)
{
	OCR1A = BASE_TICK - 1;
	TCNT1 = 0;
	TIFR1 = ~0;
	TCCR1B = 0x0a;		// CTC, F_CLK / 8 (1tick = 0.5us @ 16MHz)
	TIMSK1 = 0x02;		// Output compare A interrupt enable
}

static void timer_stop(void)
{
	TIMSK1 = 0;
	TCCR1B = 0;
}

static void gpio_output(bool state)
{
	if (state) OUT_PORT |= 1 << OUT_0;
	else OUT_PORT &= ~(1 << OUT_0);
}

static void output(unsigned char status, unsigned short count)
{
	PlayerStatus = status;
	PlayerCount = count;

	bool state = (status == PlayDit) || (status == PlayDah);

	gpio_output(state);
	if (led_output) led_output(state);
	if (sidetone_output) sidetone_output(state);
}

static void do_play(void)
{
again:
	switch (PlayerStatus) {
	case PlayNothing:
		if (PinMemory & PIN0_ON)
			output(PlayDit, DIT_ON_TICK);
		else if (PinMemory & PIN1_ON)
			output(PlayDah, DAH_ON_TICK);
		break;
	case PlayDit:
		if (!PlayerCount)
			output(StopDit, DIT_OFF_TICK);
		break;
	case PlayDah:
		if (!PlayerCount)
			output(StopDah, DAH_OFF_TICK);
		break;
	case StopDit:
		if (!PlayerCount) {
			PinMemory &= ~PIN0_ON;
			PlayerStatus = PlayNothing;
			goto again;
		}
		break;
	case StopDah:
		if (!PlayerCount) {
			PinMemory &= ~PIN1_ON;
			PlayerStatus = PlayNothing;
			goto again;
		}
		break;
	}
}

void setup(void)
{
	input_init();
	gpio_init();
	if (led_init) led_init();
	if (sidetone_init) sidetone_init();
	timer_init();
	gpio_int_enable();
}

void loop(void)
{
	unsigned char changed, useprev, status;

	while (1) {
		cli();
		if (TimerExpired)
			break;

		asm __volatile__(
			"sei	\n"
			"sleep	\n"
			);
	}

	if (PlayerStatus != PlayNothing) do_play();

	changed = (CurrPinStatus ^ PrevPinStatus);
	useprev = ((PinMaskCounter0 ? PIN0_ON : 0) |
		   (PinMaskCounter1 ? PIN1_ON : 0));

	PrevPinStatus = status =
		(PrevPinStatus & useprev) | (CurrPinStatus & ~useprev);

	/* refresh pin memory with debounced paddle status */
	if (PrevPinStatus & PIN0_ON) PinMemory |= PIN0_ON;
	if (PrevPinStatus & PIN1_ON) PinMemory |= PIN1_ON;

	/* set debounce counter for newly changed pin */
	if (changed & ~useprev & PIN0_ON) PinMaskCounter0 = PinMaskCount;
	if (changed & ~useprev & PIN1_ON) PinMaskCounter1 = PinMaskCount;

	if (PlayerStatus == PlayNothing) {
		if (PinMemory) {
			do_play();
		} else {
			timer_stop();
			gpio_int_enable();
		}
	}

	TimerExpired = 0;
	sei();
}
