//0.954 hz is lowest frequency possible with this function,
//based on settings in PWN_on()
//Passing in 0 as the frequency will stop the speaker from generating sound
#ifndef pwm_H
#define pwm_H

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

#endif 


