#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

unsigned int hours=0, minutes=0, seconds=0;

void Timer1_Init(){
	/*
	 * 1. OCR1A = 1sec/1.024ms = 976.5
	 * 2. Set OCIE1A in TIMSK register to enable compare mode CTC interrupt
	 * 2. Set FOC1A, FOC1B in TCCR1A for non PWM-mode
	 * 4. Choose prescaler value = F_CPU/1024 by setting CS10 , CS12 in TCCR1B
	 * 5. Set WGM12 in TCCR1B for CTC mode #4
	 */
	TCNT1 = 0;
	OCR1A = 977;
	TIMSK |= (1<<OCIE1A);
	TCCR1A = (1<<FOC1A) | (1<<FOC1B);
	TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS12);
}

ISR(TIMER1_COMPA_vect){
	seconds++;
	if(seconds==60){
		seconds=0;
		minutes++;
	}
	if(minutes==60){
		hours++;
		minutes=0;
	}
	if(hours==24){
		hours=0;
		minutes=0;
		seconds=0;
	}
}

void INT0_Init(){ //RESET
	DDRD &=~ (1<<2); 			//PD2 is input/INT0
	PORTD |= (1<<2); 			//INTERNAL PULL UP
	GICR |= (1<<INT0);
	MCUCR &=~ (1<<ISC00); 		//FALLING edge
	MCUCR |= (1<<ISC01);
	SREG |= (1<<7);				//enable I-bit
}

ISR(INT0_vect){ //RESET
	hours=0;
	minutes=0;
	seconds=0;
}

void INT1_Init(){ //PAUSE
	DDRD &=~ (1<<3); 					//PD3 is input/INT1
	GICR |= (1<<INT1);
	MCUCR |= (1<<ISC10) | (1<<ISC11); 	//RISING EDGE
	SREG |= (1<<7);
}

ISR(INT1_vect){ //PAUSE
	/*
	 * Stop or disable the timer by disabling the clock source
	 */
	TCCR1B &=~ (1<<CS10) & (1<<CS11) & (1<<CS12);
}

void INT2_Init(){ //RESUME
	GICR |= (1<<INT2);
	DDRB &=~ (1<<2); 		//PB2 is input/INT2
	PORTB |= (1<<2); 		//INTERNAL PULL UP
	MCUCSR &=~ (1<<ISC2); 	//FALLING EDGE
	SREG |= (1<<7);
}

ISR(INT2_vect){ //RESUME
	/*
	 * Enable the clock source again to resume counting
	 */
	TCCR1B |= (1<<CS10) | (1<<CS12);
	Timer1_Init();
}

int main(){
	INT0_Init();
	INT1_Init();
	INT2_Init();
	Timer1_Init();
	DDRC |= 0x0F; 					//first 4 bits are output to decoder then to the 6 7-seg7
	PORTC = (PORTC & 0xF0) | 0; 	//7-seg is initially set to zero
	DDRA |= 0x3F; 	                //first 6 bits of portA are output (selection)
	PORTA = 0;
	while(1){
		//seconds
		PORTA &=~ 0xFF; 			//disable ALL selection lines (all 7-segments)
		PORTA = (1<<0); 			//enable first 7seg only (units of seconds)
		PORTC = (PORTC & 0xF0) | ((seconds % 10) & 0x0F);	//use modulo-10 technique to separate the digits
		_delay_ms(2);				//we can't see the leds flashing with such a small delay (2ms~5ms)

		PORTA &=~ 0xFF;
		PORTA = (1<<1);				//enable second 7seg only (tens of seconds)
		PORTC = (PORTC & 0xF0) | ((seconds / 10) & 0x0F);
		_delay_ms(2);

		//minutes
		PORTA &=~ 0xFF;
		PORTA = (1<<2);				//enable third 7seg  only (units of minutes)
		PORTC = (PORTC & 0xF0) | ((minutes % 10) & 0x0F);
		_delay_ms(2);

		PORTA &=~ 0xFF;
		PORTA = (1<<3);				//enable fourth 7seg  only (tens of minutes)
		PORTC = (PORTC & 0xF0) | ((minutes / 10) & 0x0F);
		_delay_ms(2);

		//hours
		PORTA &=~ 0xFF;
		PORTA = (1<<4);				//enable fifth 7seg  only (units of hours)
		PORTC = (PORTC & 0xF0) | ((hours % 10) & 0x0F);
		_delay_ms(2);

		PORTA &=~ 0xFF;
		PORTA = (1<<5);				//enable sixth 7seg  only (tens of hours)
		PORTC = (PORTC & 0xF0) | ((hours / 10) & 0x0F);
		_delay_ms(2);
	}
}
