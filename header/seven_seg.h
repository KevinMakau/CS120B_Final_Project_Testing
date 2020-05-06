#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H

void WriteNumber(int Value){
	unsigned char Hundreth = 0;
	unsigned char Tenth = 0;
	unsigned char Ones = 0;
	//Find the value of each digit on the display
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
	//Get the boolean number pattern for each digit on the display
	Hundreth = NumberPattern(Hundreth);
	Tenth = NumberPattern(Tenth);
	Ones = NumberPattern(Ones);
	//Rapidly switch the display on and off in order to write to each digit seperetly 
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

//Stores the boolean value for each number on the display
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
#endif // SEVEN_SEG_H
