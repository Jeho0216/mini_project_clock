/*
* key_matrix.c
*
* Created: 2019-04-21 일 오후 9:59:47
*  Author: leeje
*/
#include "key_matrix.h"
#include "I2C_LCD.h"

void print_i2c_LCD(char num){
	I2C_LCD_write_data(num);
}

void key_process(){
	static char opcode = 0;
	static uint8_t key_val = 0;
	static uint8_t old_key = 0;
	static long int result = 0, x = 0, y = 0;
	static uint8_t opcode_flag = 0;
	//연산자 입력 순서 플래그. 0:피연산자(x) 입력, 1:피연산자(y) 입력. 2:연산자 입력
	
	key_val = keypad_scan();

	if(old_key != key_val){
		old_key = key_val;
		switch(key_trans(key_val)){
			case '0' :
			if(opcode_flag == 1)			y = y * 10 + 0;
			else if(opcode_flag == 0)		x = x * 10 + 0;
			if(opcode_flag != 2)			I2C_LCD_write_data('0');
			break;
			case '1' :
			if(opcode_flag == 1)			y = y * 10 + 1;
			else if(opcode_flag == 0)		x = x * 10 + 1;
			if(opcode_flag != 2)			I2C_LCD_write_data('1');
			break;
			case '2' :
			if(opcode_flag == 1)			y = y * 10 + 2;
			else if(opcode_flag == 0)		x = x * 10 + 2;
			if(opcode_flag != 2)			I2C_LCD_write_data('2');
			break;
			case '3' :
			if(opcode_flag == 1)			y = y * 10 + 3;
			else if(opcode_flag == 0)		x = x * 10 + 3;
			if(opcode_flag != 2)			I2C_LCD_write_data('3');
			break;
			case '4' :
			if(opcode_flag == 1)			y = y * 10 + 4;
			else if(opcode_flag == 0)		x = x * 10 + 4;
			if(opcode_flag != 2)			I2C_LCD_write_data('4');
			break;
			case '5' :
			if(opcode_flag == 1)			y = y * 10 + 5;
			else if(opcode_flag == 0)		x = x * 10 + 5;
			if(opcode_flag != 2)			I2C_LCD_write_data('5');
			break;
			case '6' :
			if(opcode_flag == 1)			y = y * 10 + 6;
			else if(opcode_flag == 0)		x = x * 10 + 6;
			if(opcode_flag != 2)			I2C_LCD_write_data('6');
			break;
			case '7' :
			if(opcode_flag == 1)			y = y * 10 + 7;
			else if(opcode_flag == 0)		x = x * 10 + 7;
			if(opcode_flag != 2)			I2C_LCD_write_data('7');
			break;
			case '8' :
			if(opcode_flag == 1)			y = y * 10 + 8;
			else if(opcode_flag == 0)		x = x * 10 + 8;
			if(opcode_flag != 2)			I2C_LCD_write_data('8');
			break;
			case '9' :
			if(opcode_flag == 1)			y = y * 10 + 9;
			else if(opcode_flag == 0)		x = x * 10 + 9;
			if(opcode_flag != 2)			I2C_LCD_write_data('9');
			break;
			case 'C':
			I2C_LCD_clear();
			result = 0;
			x = 0;
			y = 0;
			opcode = 0;
			opcode_flag = 0;
			break;
			//연산자는 첫번째 피연산자가 입력 되었을 경우에만 필요함.
			case '/' :
			if(opcode_flag == 2 || opcode_flag == 0){
				opcode = '/';
				I2C_LCD_write_data('/');
				opcode_flag = 1;
			}
			break;
			case '*' :
			if(opcode_flag == 2 || opcode_flag == 0){
				opcode = '*';
				I2C_LCD_write_data('*');
				opcode_flag = 1;
			}
			break;
			case '+' :
			if(opcode_flag == 2 || opcode_flag == 0){
				opcode = '+';
				I2C_LCD_write_data('+');
				opcode_flag = 1;
			}
			break;
			case '-' :
			if(opcode_flag == 2 || opcode_flag == 0){
				opcode = '-';
				I2C_LCD_write_data('-');
				opcode_flag = 1;
			}
			break;
			case '=' :
			result = calculate(x, y, opcode);
			print_result_value(result, x, y, opcode);
			opcode_flag = 2;		//연산자 입력.
			opcode = 0;
			x = result;
			y = 0;
			break;
		}
	}
}

uint8_t keypad_scan(){
	for(int i = 0; i < 4; i++){		//i는 row를 의미.
		PORTF |= 0xF0;
		PORTF &= ~(0x10 << i);
		//1110 -> 1101 -> 1011 -> 0111 -> 1110 -> .....
		_delay_ms(1);
		
		if(key_scan_col())
		return (i * 4) + key_scan_col();
	}
	return 0;
}

uint8_t key_scan_col(void){
	uint8_t key_val;
	
	key_val = PINF & 0x0F;		//C1,2,3,4 중에서
	
	if(key_val == 14)
	return 1;
	else if(key_val == 13)
	return 2;
	else if(key_val == 11)
	return 3;
	else if(key_val == 7)
	return 4;
	else
	return 0;
}

unsigned char key_trans(unsigned char key_val){
	switch(key_val){
		case 1:		return '7';
		case 2:		return '8';
		case 3:		return '9';
		case 4:		return '/';
		case 5:		return '4';
		case 6:		return '5';
		case 7:		return '6';
		case 8:		return '*';
		case 9:		return '1';
		case 10:	return '2';
		case 11:	return '3';
		case 12:	return '-';
		case 13:	return '0';
		case 14:	return 'C';
		case 15:	return '=';
		case 16:	return '+';
	}
}

long int calculate(long int x, long int y, char opcode){
	switch(opcode){
		case '+':
		return x + y;
		case '-':
		return x - y;
		case '*':
		return x * y;
		case '/':
		return x / y;
	}
}

void print_result_value(long int result, long int x, long int y, char opcode){
	char buff[17];
	I2C_LCD_clear();
	sprintf(buff, "%ld%c%ld", x, opcode, y);
	I2C_LCD_goto_XY(0, 0);
	I2C_LCD_write_string(buff);
	I2C_LCD_goto_XY(1, 0);
	sprintf(buff, "%ld", result);
	I2C_LCD_write_string(buff);
}