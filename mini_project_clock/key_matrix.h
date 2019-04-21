/*
 * key_matrix.h
 *
 * Created: 2019-04-21 일 오후 9:59:39
 *  Author: leeje
 */ 


#ifndef KEY_MATRIX_H_
#define KEY_MATRIX_H_
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

uint8_t keypad_scan(void);		//누른 버튼 번호계산 후, 리턴
uint8_t key_scan_col(void);		//각 row마다 눌린 col버튼을 확인.
unsigned char key_trans(unsigned char key_val);		//입력된 버튼(1~16)을 계산기 값으로 매핑.
void key_process();
long int calculate(long int x, long int y, char opcode);
void print_i2c_lcd(char num);
void print_result_value(long int result, long int x, long int y, char opcode);


#endif /* KEY_MATRIX_H_ */