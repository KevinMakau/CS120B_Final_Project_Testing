/*	Author: kmaka003
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */


#include <avr/io.h>
#include "pwm.h"
#include "io.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include <avr/interrupt.h>
#endif

#define ButtonA (~PINA & 0x01)
#define ButtonB (~PINA & 0x02)
#define ButtonC (~PINA & 0x04)

// Easy Medium or Hard. 
typedef struct _GameMode{
	// Time allowed to complete Game
	unsigned int TimeAllowed;
	// Time subtracted for each error
	unsigned int TimeOff;
} GameMode;


GameMode Easy;

GameMode Medium; 

GameMode NoMode;


unsigned char Period = 100;

typedef struct _Task{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct) (int);

} Task;


unsigned char tasksSize = 2;
Task tasks[2];


volatile unsigned char TimerFlag = 0; //TimerISR() sets this to a 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; //Start count from here, down to 0. Default to 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1 ms ticks

void TimerOn()
{
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit3 = 0: CTC mode (clear timer on compare)
	//bit2bit1bit0 = 011: pre-scaler /64
	// 00001011: 0x0B
	// so, 8MHz clock or 8,000,000 /64 =125,000 ticks/s
	// Thus, TCNT1 register will count as 125,000 ticks/s
	//AVR output compare register OCR1A.
	OCR1A = 125;   // Timer interrupt will be generated when TCNT1 == OCR1A
	// We want a 1 ms tick. 0.001s *125,000 ticks/s = 125
	// so when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	// Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |=0x80; // 0x80: 1000000

}

void TimerOff()
{
	TCCR1B = 0x00; // bit3bitbit0 -000: Timer off
}

void TimerISR()
{
	unsigned char i;
	for (i = 0;i < tasksSize;++i) {
		if (tasks[i].elapsedTime >= tasks[i].period) {
			tasks[i].state = tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime = 0;
		}
		tasks[i].elapsedTime += Period;
	}
}

ISR(TIMER1_COMPA_vect)
{
	//CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) //results in a more efficient compare
	{
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
//Set TimerISR() to tick every M ms
void TimerSet(unsigned long M)
{
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//------------------------------------------------------------------------------------
//----------------------------------TckFct & Enums//---------------------------------
int TickFct_GameSart(int);
typedef enum GameStart_States {GameStart_init, GameStart_Wait, GameStart_Start, GameStart_DeadState} GameStart_States;

int TickFct_PWMTick(int);
typedef enum PWMTick_States {PWMTick_init, PWMTick_Press} PWMTick_States;

int TickFct_BombTick(int);
typedef enum BombTick_States {BombTick_init, BombTick_Tick} BombTick_States;
//-----------------------------------------------------------------------------------------
//-----------------------------Global Vaiables---------------------------------------------
unsigned char tempB;
unsigned char tempC;
unsigned char tempD;
unsigned char BombTick = 0;
GameMode ChosenMode;

//------------------------------------------------------------------------------------------

int main(void) {
    /* Insert DDR and PORT initializations */
	DDRA = 0x00; PINA = 0xFF;
	DDRB = 0xFF; PINB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRC = 0xFF; PORTD = 0x00; 
   /* Insert your solution below */
	LCD_init();	

	//Easy.TimeAllowed = 0; 
	//Easy.TimeOff = 0;
 
	//Medium.TimeAllowed = 0; 
	//Medium.TimeOff = 0; 

	//NoMode.TimeAllowed = 0; 
	//NoMode.TimeOff = 0;


	unsigned char i = 0;
	//Game Start	
	tasks[i].state = GameStart_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_GameSart;
	i++;

	//PWMTick	
	tasks[i].state = PWMTick_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_PWMTick;
	i++;

	//BombTick	
	tasks[i].state = BombTick_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_BombTick;
	i++;


	TimerSet(100);
	TimerOn();
    while (1) {

    }
    return 1;
}
//----------------------------------------------------------------------------
//------------------------------GameStart SM---------------------------------
unsigned char GameStart_timer = 0;
GameMode ChosenMode_temp;
Game_Begin = 0;
int TickFct_GameSart(int state){
	switch (state){
		case GameStart_init:
			state = ButtonA || ButtonB? GameStart_Wait: GameStart_init;
			if(ButtonA){
				ChosenMode_temp = Easy;
			}
			else if(ButtonB){
				ChosenMode_temp = Medium;
			}
				
			break;
		case GameStart_Wait:
			state = GameStart_timer > 30? GameStart_Start: GameStart_Wait;
			break;
		case GameStart_Start:
			state = GameStart_DeadState;
			break;
		case GameStart_DeadState:
			state = GameStart_DeadState;			
			break;
	}
	switch (state){
		case GameStart_init:
			LCD_DisplayString(1, "Choose Moode      Easy=A/Hard=B");
			break;
		case GameStart_Wait:
			GameStart_timer++;
			LCD_DisplayString(1, "BombCode:       BM7L1");
			break;
		case GameStart_Start:
			LCD_ClearScreen();
			ChosenMode = ChosenMode_temp;
			Game_Begin = 1;
			break;
		case GameStart_DeadState:		
			break;
	}
	return state;
}
//-------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------
//---------------------------PWM Ticks-------------------------------------------------------
TickFct_PWMTick(int state){
	unsigned char A0 = ~PINA & 0x01;
	unsigned char A1 = (~PINA >> 1) & 0x01;
	unsigned char A2 = (~PINA >> 2) & 0x01;
	
	unsigned char Pressed = A0 + A1 + A2;
	if(!Game_Begin){
		return state;
	}

	switch (state){
		case PWMTick_init:
			state = Pressed? PWMTick_Press: PWMTick_init;
			break;
		case PWMTick_Press:
			state = Pressed? PWMTick_Press: PWMTick_init;
			break;
	}
	switch (state){
		case PWMTick_init:
			set_PWM(0);
			if(BombTick){
				set_PWM(450);
			}
				
			break;
		case PWMTick_Press:
			if (Pressed > 1){ 
				set_PWM(0);
			}
			else if(A0){
				set_PWM(261.63);
			}			
			else if(A1){
				set_PWM(293.66);
			}
			else{
				set_PWM(329.63);			
			}
			break;
	}
	return state;	
}


//-------------------------------------------------------------------------------------------
//---------------------------Bomb Ticks-------------------------------------------------------
unsigned char BombTick_Timer = 0;
unsigned char SetTick = 0;
TickFct_BombTick(int state){
	if(!Game_Begin){
		return state;
	}

	switch (state){
		case BombTick_init:
			state = BombTick_Timer > 10? BombTick_Tick: BombTick_init;
			break;
		case BombTick_Tick:
			state = BombTick_init;
			break;
	}
	switch (state){
		case BombTick_init:
			BombTick_Timer++;
			BombTick = 0;
			break;
		case BombTick_Tick:
			BombTick = 1;
			break;
	}
}
//--------------------------------------------------------------------------------------------










	
