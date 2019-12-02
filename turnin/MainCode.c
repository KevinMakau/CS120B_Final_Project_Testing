/*	Author: 
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */


#include <avr/io.h>
#include "io.h"
#include "io.c"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include <avr/interrupt.h>
#endif

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


unsigned char tasksSize = 9;
Task tasks[9];

void set_PWM(double frequency){
	static double current_frequency; // keeps track of the currently set frequency
	//Will only update the registers when the frequency changes, otherwise allows
	//music to play uninterrupted
	
	if(frequency != current_frequency)
	{
		if(!frequency) {TCCR1B &= 0x08;}//stops timer/counter
		else {TCCR1B |= 0x03;} //resumes/continues timer/counter
		
		//prevents OCR3A from overflowing using prescaler 64
		//0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) {OCR3A = 0xFFFF;}
		
		//prevents OCR3A from underflowing using prescaler 64
		//31250 is largest frequency that will not result in underflow
		if (frequency > 31250) {OCR3A = 0x0000;}
		
		else { OCR3A = (short) (8000000/(128*frequency))-1; }
		
		TCNT3 = 0; //resets counter
		current_frequency = frequency; // updates the current frequency
	}
}

void PWM_on(){
	TCCR3A = (1 << COM3A0);
	//COM3A0: Toggle PB3 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// wGM32: When counter (TCNT3) matches OCR3A, reset counter
	//CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_oFF (){
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

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

void ADC_init (){
	ADCSRA |= (1 << ADEN) | (1 <<  ADSC) | (1 << ADATE);
}

char ADC_X(){
	ADC_init();
	ADMUX |= 0x03;
	if(ADC > 700){
		return 1;
	}
	else if(ADC < 400){
		return 2;
	}
	else{
		return 0;
	}
}

//------------------------------------------------------------------------------------
//----------------------------------TckFct & Enums//---------------------------------
int TickFct_GameSart(int);
typedef enum GameStart_States {GameStart_init, GameStart_Wait, GameStart_Start, GameStart_DeadState, GameStart_Wait2} GameStart_States;

int TickFct_PWMTick(int);
typedef enum PWMTick_States {PWMTick_init, PWMTick_Press} PWMTick_States;

//int TickFct_BombTick(int);
//typedef enum BombTick_States {BombTick_init, BombTick_Tick} BombTick_States;

int TickFct_Questions(int);
typedef enum Questions_States {Questions_init, Questions_wait, Questions_Answer, Questions_Check, Questions_Over, Questions_waitS} Questions_States;
	
int TickFct_Questions_two(int);
typedef enum Questions_States_two {Questions_init_two, Questions_wait_two, Questions_Answer_two, Questions_Check_two, Questions_Over_two, Questions_waitS_two} Questions_States_two;

int TickFct_Questions_three(int);
typedef enum Questions_States_three {Questions_init_three, Questions_wait_three, Questions_Answer_three, Questions_Check_three, Questions_Over_three, Questions_waitS_three} Questions_States_three;

int TickFct_Mary(int);
typedef enum Mary_States {Mary_init, Mary_wait, Mary_Answer, Mary_Check, Mary_Over, Mary_waitS} Mary_States;


int TickFct_Joystick(int);
typedef enum Joystick_States {Joystick_init, Joystick_wait, Joystick_Answer, Joystick_Check, Joystick_Over, Joystick_waitS} Joystick_States;
	
int TickFct_GameOver(int);
typedef enum GameOver_States {GameOver_init, GameOver_wait, GameOver_HighScore, GameOver_Reset} GameOver_States;

int TickFct_Lost(int);

int TickFct_Reset(int);
typedef enum Reset_States {Reset_init, Reset_wait, Reset_Reset} Reset_States;


int TickFct_ButtonPress(int);

int TickFct_GameClockTick(int);
void WriteNumber(int);
char NumberPattern(char);
//-----------------------------------------------------------------------------------------
//-----------------------------Global Vaiables---------------------------------------------
unsigned short GameClock = 500;
unsigned char Game_Won = 0;
unsigned char Reset = 0;
unsigned char BombTick = 0;
GameMode ChosenMode;
unsigned char ButtonA = 0;
unsigned char ButtonB = 0;
unsigned char ButtonC = 0;
unsigned char ButtonD = 0;
unsigned char Game_Lost = 0;
unsigned char HighScore = 0;



//------------------------------------------------------------------------------------------

int main(void) {
    /* Insert DDR and PORT initializations */
	DDRA = 0x00; PINA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	DDRC = 0xFF; PORTD = 0x00; 
   /* Insert your solution below */
	LCD_init();	
	PWM_on();
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

	//Button Press	
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_ButtonPress;
	i++;

	//GameClock	
	tasks[i].state  = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_GameClockTick;
	i++;

	//PWMTick	
	tasks[i].state = PWMTick_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_PWMTick;
	i++;

	//BombTick	
	//tasks[i].state = BombTick_init;
	//tasks[i].period = 100;
	//tasks[i].elapsedTime = tasks[i].period;
	//tasks[i].TickFct = &TickFct_BombTick;
	//i++;
	
	//Question1
	tasks[i].state = Questions_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Questions;
	i++;
	
	//Question2
	tasks[i].state = Questions_init_two;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Questions_two;
	i++;
	
	//Question3
	tasks[i].state = Questions_init_three;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Questions_three;
	i++;
	
	//Mary Had a little lamb
	tasks[i].state = Mary_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Mary;
	i++;
	
	//Joystick init
	tasks[i].state = Joystick_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Joystick;
	i++;
	
	//Game Over
	tasks[i].state = GameOver_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_GameOver;
	i++;
	
	//Reset
	tasks[i].state = Reset_init;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Reset;
	i++;
	
	//Game Lost
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TickFct_Lost;
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
unsigned char Game_Begin = 0;
int TickFct_GameSart(int state){
	if(Game_Begin){
		return state;
	}
	switch (state){
		case GameStart_init:
			state = ButtonA? GameStart_Wait: GameStart_init;
			/*if(ButtonA){
				ChosenMode_temp = Easy;
			}
			else if(ButtonB){
				ChosenMode_temp = Medium;
			}*/
				
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
			LCD_DisplayString(1, "Defuse the bomb A to start");
			break;
		case GameStart_Wait:
			GameStart_timer++;
			LCD_DisplayString(1, "BombCode:       Black:6M47L1");
			break;
		case GameStart_Start:
			LCD_ClearScreen();
			ChosenMode = ChosenMode_temp;
			Game_Begin = 1;
			state = GameStart_init;
			GameStart_timer = 0;
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
	
	
	unsigned char Pressed = ButtonA + ButtonB + ButtonC;
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
			set_PWM(1);	
			break;
		case PWMTick_Press:
			if (ButtonA && ButtonB || ButtonA && ButtonC || ButtonB && ButtonC){ 
				set_PWM(1);
			}
			else if(ButtonA){
				set_PWM(261.63);
			}			
			else if(ButtonB){
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
/*
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
*/
//--------------------------------------------------------------------------------------------
TickFct_ButtonPress(int state){
	ButtonA = ~PINA & 0x01;
	ButtonB = ~PINA & 0x02;
	ButtonC = ~PINA & 0x04;
	ButtonD = ~PINA & 0x10;

}

//---------------------------------------------------------------------------------------


//-----------------------------------7Seg Display----------------------------------------
//---------------------------------------------------------------------------------------
unsigned char GameClocki = 0;
int TickFct_GameClockTick(int state){
	if(!Game_Begin){
		return 0;
	}
	GameClocki++;
	if (GameClocki > 10){
		GameClocki = 0;
		GameClock--;
	}
	WriteNumber(GameClock);

}		
			

void WriteNumber(int Value){
	unsigned char Hundreth = 0;
	unsigned char Tenth = 0;
	unsigned char Ones = 0;
	while(Value > 99){
		Hundreth++;
		Value = Value - 100;
	}
	for(Value = Value; Value > 9; Value = Value - 10){
		Tenth++;
	}
	for(Value = Value; Value > 0; Value-- ){
		Ones++;
	}
	//LCD_Cursor(1);
	//LCD_WriteData(Hundreth + '0');
	//LCD_Cursor(2);
	//LCD_WriteData(Tenth + '0');
	//LCD_Cursor(3);
	//LCD_WriteData(Ones + '0');
	
	Hundreth = NumberPattern(Hundreth);
	Tenth = NumberPattern(Tenth);
	Ones = NumberPattern(Ones);
	PORTD = PORTD & 0xC0;
	PORTB = PORTB & 0xC0;
	for(Value = 0; Value < 10000; Value++){		
		PORTD = PORTD | (Hundreth & 0x3F);
		PORTB = PORTB | 0x20;
		PORTB = PORTB | (Hundreth >> 6);
	}
	PORTD = PORTD & 0xC0;
	PORTB = PORTB & 0xC0;
	for(Value = 0; Value < 10000; Value++){

		PORTD = PORTD | (Tenth & 0x3F);
		PORTB = PORTB | 0x10;
		PORTB = PORTB | (Tenth >> 6);
	}
	PORTD = PORTD & 0xC0;
	PORTB = PORTB & 0xC0;
	for(Value = 0; Value < 10000; Value++){
		PORTD = PORTD | (Ones & 0x3F);
		PORTB = PORTB | 0x08;
		PORTB = PORTB | (Ones >> 6);
	}
	PORTD = PORTD & 0xC0;
	PORTB = PORTB & 0xC0;
	
}

char NumberPattern(char LCD_Number){
	if (LCD_Number == 0){
		return 0b10000001;
	} 	
	else if(LCD_Number == 1){
		return 0b10110111;
	}	
	else if (LCD_Number == 2){
		return 0b11000010;
	}
	else if (LCD_Number == 3){
		return 0b10100010;
	}
	else if (LCD_Number == 4){
		return 0b10110100;
	}
	else if (LCD_Number == 5){
		return 0b10101000;
	}
	else if (LCD_Number == 6){
		return 0b10001000;
	}
	else if (LCD_Number == 7){
		return 0b10110011;
	}
	else if (LCD_Number == 8){
		return 0b10000000;
	}
	else if (LCD_Number == 9){
		return 0b10100000;
	}

}		
	
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

//========================================Game Logic====================================
//--------------------------------------------------------------------------------------


//----------------------------------------3Questions-------------------------------------
//-------------------------------------------------------------------------------------
unsigned char Questions_clock = 0;
unsigned char Q_Over = 0;
int TickFct_Questions(int state){
	if(!Game_Begin | Q_Over){
		return state;
	}
	
	switch (state){
		case Questions_init:
			LCD_DisplayString(1, "Character 3 On  the Bomb Code");
			state = Questions_wait;
			break;
		case Questions_wait:
			state = Questions_clock > 15? Questions_Answer: Questions_wait;
			break;
		case Questions_Answer:
			state = Questions_Check;
			break;
		case Questions_Check:
			state = ButtonA? Questions_Over: Questions_Check;
			break;
		case Questions_Over:
			state = Questions_Over;
			break;
		case Questions_waitS:
			state = Questions_clock > 7? Questions_init: Questions_waitS;
	}
	switch (state){
		case Questions_init:
			break;
		case Questions_wait:
			Questions_clock++;
			break;
		case Questions_waitS:
			Questions_clock++;
			break;	
		case Questions_Answer:
			LCD_DisplayString(1, "A: M    B: J    C: 4");
			break;
		case Questions_Check:
			if(ButtonB | ButtonC){
				LCD_DisplayString(1, "ERROR!!!");
				GameClock = GameClock - 10;
				state = Questions_waitS;
				Questions_clock = 0;
			}
			break;
		
		case Questions_Over:
			Q_Over = 1;
			Questions_clock = 0;
			state = Questions_init;
			break;
	}		 
	return state;
}

unsigned char Questions_clock_two = 0;
unsigned char Q_Over_two = 0;
int TickFct_Questions_two(int state){
	if(!Q_Over | Q_Over_two | !Game_Begin){
		return state;
	}
	
	switch (state){
		case Questions_init_two:
			LCD_DisplayString(1, "Cut a wire");
			state = Questions_wait_two;
			break;
		case Questions_wait_two:
			state = Questions_clock_two > 8? Questions_Answer_two: Questions_wait_two;
			break;
		case Questions_Answer_two:
			state = Questions_Check_two;
			break;
		case Questions_Check_two:
			state = ButtonB? Questions_Over_two: Questions_Check_two;
			break;
		case Questions_Over_two:
			state = Questions_Over_two;
			break;
		case Questions_waitS_two:
			state = Questions_clock_two > 7? Questions_init_two: Questions_waitS_two;
	}
	switch (state){
		case Questions_init_two:
			break;
		case Questions_wait_two:
			Questions_clock_two++;
			break;
		case Questions_waitS_two:
			Questions_clock_two++;
			break;
		case Questions_Answer_two:
			LCD_DisplayString(1, "A: Red  B: BlackC: Green");
			break;
		case Questions_Check_two:
			if(ButtonA | ButtonC){
				LCD_DisplayString(1, "ERROR!!!");
				GameClock = GameClock - 10;
				state = Questions_waitS_two;
				Questions_clock_two = 0;
		}
		break;
		case Questions_Over_two:
			Q_Over_two = 1;
			Questions_clock_two = 0;
			state = Questions_init_two;
			break;
	}
	return state;
}


unsigned char Questions_clock_three = 0;
unsigned char Q_Over_three = 0;
int TickFct_Questions_three(int state){
	if(!Game_Begin | Q_Over_three | !Q_Over_two){
		return state;
	}
	
	switch (state){
		case Questions_init_three:
			LCD_DisplayString(1, "The Other button");
			state = Questions_wait_three;
			break;
		case Questions_wait_three:
			state = Questions_clock_three > 10? Questions_Answer_three: Questions_wait_three;
			break;
		case Questions_Answer_three:
			state = Questions_Check_three;
			break;
		case Questions_Check_three:
			state = ButtonC? Questions_Over_three: Questions_Check_three;
			break;
		case Questions_Over_three:
			state = Questions_Over_three;
			break;
		case Questions_waitS:
			state = Questions_clock_three > 7? Questions_init_three: Questions_waitS_three;
	}
	switch (state){
		case Questions_init_three:
			break;
		case Questions_wait_three:
			Questions_clock_three++;
		break;
			case Questions_waitS_three:
			Questions_clock_three++;
			break;
		case Questions_Answer_three:
			LCD_DisplayString(1, "A: A    B: B    C: C");
			break;
		case Questions_Check_three:
			if(ButtonB | ButtonA){
				LCD_DisplayString(1, "ERROR!!!");
				GameClock = GameClock - 10;
				state = Questions_waitS_three;
				Questions_clock_three = 0;
			}
			break;
		
		case Questions_Over_three:
			Q_Over_three = 1;
			Questions_clock_three = 0;
			state = Questions_init_three;
			break;
	}
	return state;
}

//------------------------------------------------------Mary Had a Little Lamb-----------------------------------------
//---------------------------------------------------------------------------------------------------------------------

unsigned char Mary_Array[13] = {4, 2, 1, 2, 4, 4, 4, 2, 2, 2, 4, 4, 4};
unsigned char Mary_clock = 0;
unsigned char M_Over = 0;
unsigned char Mary_i = 0;
int TickFct_Mary(int state){
	unsigned char Pressed = ButtonA + ButtonB + ButtonC;
	if(!Game_Begin | !Q_Over_three | M_Over){
		return state;
	}
	switch (state){
		case Mary_init:
			LCD_DisplayString(1, "What did Mary   Have");
			state = Pressed? Mary_init: Mary_Answer;
			break;
		case Mary_Answer:
			state = Pressed? Mary_Check: Mary_Answer;
			break;
		case Mary_Check:
			state = Mary_wait;
			break;
		case Mary_wait:
			state = Pressed? Mary_wait: Mary_Answer;
			state = Mary_i > 12? Mary_Over: state;
			break;
		case Mary_Over:
			state = Mary_Over;
			break;
		case Mary_waitS:
			state = Mary_clock > 7? Mary_init: Mary_waitS;
	}
	switch (state){
		case Mary_init:
			break;
		case Mary_wait:
			break;
		case Mary_waitS:
			Mary_clock++;
			break;
		case Mary_Answer:
			break;
		case Mary_Check:
			if(ButtonA && ButtonB || ButtonA && ButtonC || ButtonB && ButtonC){
			 break;
			}
			if(Pressed == Mary_Array[Mary_i]){
				Mary_i++;
			}
			else{
				LCD_DisplayString(1, "Error!!!");
				GameClock = GameClock - 10;
				state = Mary_waitS;
				Mary_clock = 0;
			}
			break;
		case Mary_Over:
			Mary_clock = 0;
			Mary_i = 0;
			M_Over = 1;
			Game_Won = 1;
			break;
	}
	return state;
}

//--------------------------------------------------JOYSTICK--------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
unsigned char Joystick_Array[6] = {2, 1, 2, 2, 1, 2};
unsigned char Joystick_clock = 0;
unsigned char J_Over = 0;
unsigned char Joystick_i = 0;
int TickFct_Joystick(int state){
	if(!Game_Begin | !M_Over | J_Over){
		return state;
	}
	unsigned char Moved = ADC_X();
	
	switch (state){
		case Joystick_init:
			LCD_DisplayString(1, "Left Right Left Left Right Left");
			state = Joystick_clock < 5? Joystick_init: Joystick_Answer;
			break;
		case Joystick_Answer:
			Joystick_clock = 0; 
			LCD_ClearScreen();
			state = Moved? Joystick_Check: Joystick_Answer;
			break;
		case Joystick_Check:
			state = Joystick_wait;
			break;
		case Joystick_wait:
			state = Moved? Joystick_wait: Joystick_Answer;
			state = Joystick_i > 5? Joystick_Over: state;
			break;
		case Joystick_Over:
			state = Joystick_Over;
			break;
		case Joystick_waitS:
			state = Joystick_clock > 7? Joystick_init: Joystick_waitS;
	}	
	switch (state){
		case Joystick_init:
			Joystick_clock++;
			break;
		case Joystick_wait:
			break;
		case Joystick_waitS:
			Joystick_clock++;
			break;
		case Joystick_Answer:
			break;
		case Joystick_Check:
			if(Moved == Joystick_Array[Joystick_i]){
				Joystick_i++;
			}
			else{
				LCD_DisplayString(1, "Error!!!");
				Joystick_i = 0;
				GameClock = GameClock - 10;
				state = Joystick_waitS;
				Joystick_clock = 0;
			}
			break;
		case Joystick_Over:
			Joystick_clock = 0;
			Joystick_i = 0;
			J_Over = 1;
			break;
	}
	return state;
}

unsigned char GameOver_clock = 0;
unsigned char Game_Over = 0;
int TickFct_GameOver(int state){
	if(!Game_Begin | Game_Over){
		return state;
	}
	switch(state){
		case GameOver_init:
			if(Game_Won){
				LCD_DisplayString(1, "Game Won");		
			}
			else if(Reset){
				LCD(1, "Game Reset");
			}
			else{
				LCD_DisplayString(1, "Game Lost");
			}
			state = GameOver_clock > 15? GameOver_HighScore: GameOver_init;
			break;
		case GameOver_HighScore:
			if(GameClock > HighScore && !Reset){
				state = GameOver_wait;
			}
			else{
				state = GameOver_Reset;
			}
		case GameOver_wait:
			state = GameOver_clock > 15? GameOver_Reset: GameOver_wait;
			break;
		case GameOver_Reset:
			state = GameOver_init;
			break;
		}
		switch(state){
			case GameOver_init:
			break;
			case GameOver_HighScore:
				GameOver_clock = 0;
			break;
			case GameOver_wait:
				GameOver_clock++;
			break;
			case GameOver_Reset:
				Q_Over_three = 0;
				Q_Over_two = 0;
				Q_Over = 0;
				M_Over = 0;
				J_Over = 0;
				GameClock = 500;
				Game_Begin = 0;
				Game_Over = 0;
				GameOver_clock = 0;
				Game_Won = 0; 
				Reset = 0;
				break;
		}
}

unsigned char Reset_Clock = 0;
int TickFct_Reset(int state){
	switch(state){
		case Reset_init:
			state = ButtonD? Reset_wait: Reset_init;
			break;
		case Reset_wait:
			state = ButtonD? Reset_wait: Reset_init;
			state = Reset_Clock > 30? Reset_Reset: state;
			break;
		case Reset_Reset:
			state = Reset_init;
			break; 
	}
	switch(state){
		case Reset_init:
			break;
		case Reset_wait:
			Reset_Clock++;
			break;		
		case Reset_Reset:
			Reset = 1;
			Reset_Clock = 0;
			break;
	}
	
	
}



int TickFct_Lost(int state){
	if(GameClock == 0){
		Game_Lost = 1;
	}
	return state;
}
