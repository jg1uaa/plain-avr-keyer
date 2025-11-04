// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2025 SASANO Takayoshi <uaa@uaa.org.uk>

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

#define DOT 0x01
#define DASH 0x02

#define DOT_LEN 750 // in 100us unit
#define BASE_TICK ((F_CPU / 1024) * DOT_LEN / 10000)
#define DOT_ON_TICK BASE_TICK
#define DOT_OFF_TICK BASE_TICK
#define DASH_ON_TICK (BASE_TICK * 3)
#define DASH_OFF_TICK BASE_TICK

volatile unsigned char PinMemory = 0;
volatile unsigned char PinStatus = 0;
volatile unsigned char TimerExpired = 0;

static void timer_start(unsigned short tick);
static void timer_stop(void);

static void update_pin_status(void)
{
	unsigned char d = IN_PIN;

	PinStatus = 0;

	if (!(d & (1 << IN_0))) {
		PinMemory |= DOT;
		PinStatus |= DOT;
	}

	if (!(d & (1 << IN_1))) {
		PinMemory |= DASH;
		PinStatus |= DASH;
	}
}

ISR(IN_vect)
{
	update_pin_status();
}

ISR(TIMER1_COMPA_vect)
{
	TimerExpired = 1;
}

static void clear_pin_memory(unsigned char d)
{
	cli();
	update_pin_status();
	if (!(PinStatus & d)) PinMemory &= ~d;
	sei();
}

static void wait_for_tick(unsigned short tick)
{
	TimerExpired = 0; // timer is stopped here
	timer_start(tick);

	while (!TimerExpired)
		asm __volatile__("sleep");	

	timer_stop();
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

	IN_MSK = (1 << IN_0) | (1 << IN_1);
	PCICR = 1 << IN_ICR;
}

static void output_init(void)
{
	OUT_PORT = 0;
	OUT_DDR = 1 << OUT_0;
}

static void timer_init(void)
{
	TIMSK0 = 0;		// disble Arduino system timer (required)
	TCCR0B = 0;

	TIMSK1 = 0;
	TCCR1B = 0;
	TCCR1A = 0;
	TCCR1C = 0;
}

static void timer_start(unsigned short tick)
{
	OCR1A = tick - 1;
	TCNT1 = 0;
	TIFR1 = ~0;
	TCCR1B = 0x0d;		// CTC, F_CLK / 1024 (1tick = 64us @ 16MHz)
	TIMSK1 = 0x02;		// Output compare A interrupt enable
}

static void timer_stop(void)
{
	TIMSK1 = 0;
	TCCR1B = 0;
}

static void start_output(void)
{
	OUT_PORT = 1 << OUT_0;
}

static void stop_output(void)
{
	OUT_PORT = 0;
}

static void do_output(unsigned short on_tick, unsigned short off_tick, unsigned char status)
{
	start_output();
	wait_for_tick(on_tick);
	stop_output();
	wait_for_tick(off_tick);
	clear_pin_memory(status);
}

void setup(void)
{
	input_init();
	output_init();
	timer_init();
}

void loop(void)
{
	while (!PinStatus)
	      asm __volatile__("sleep");

	while (PinMemory) {
		if (PinMemory & DOT)
			do_output(DOT_ON_TICK, DOT_OFF_TICK, DOT);
		if (PinMemory & DASH)
			do_output(DASH_ON_TICK, DASH_OFF_TICK, DASH);
	}
}
