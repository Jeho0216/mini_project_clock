/*
 * mini_project_clock.c
 *
 * Created: 2019-04-21 일 오후 8:55:58
 * Author : LEE jeho
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include "I2C.h"
#include "I2C_LCD.h"
#include "CLCD.h"
#include "UART0.h"
#include "key_matrix.h"

FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);
FILE INPUT = FDEV_SETUP_STREAM(NULL, UART0_receive, _FDEV_SETUP_READ);

volatile int count = 0;
volatile int clock_val = 0;
volatile int stop_ms = 0, stop_ss = 0, stop_mm = 0;
volatile uint8_t hh = 12, mm = 0, ss = 0;
volatile uint8_t year = 19, month = 4, day = 21;
volatile uint8_t mode = 0;		//0 : 시간출력, 1 : 스탑워치, 2 : 알람
volatile uint8_t mode_change = 0;
volatile uint8_t stop_watch_flag = 0;
volatile uint8_t time_print_flag = 0;		//모드설정 플래그

ISR(TIMER0_COMP_vect){
	count++;
	if(count >= 1000){		//1초 경과시
		count = 0;
		if(stop_watch_flag != 1)
		time_print_flag = 1;
		ss++;
		if(ss >= 60){
			mm++;
			ss = 0;
			if(mm >= 60){
				hh++;
				mm = 0;
				if(hh >= 24){
					day++;
					hh = 0;
				}
			}
		}
	}
	if(stop_watch_flag == 1){
		stop_ms++;
		if(stop_ms >= 1000){
			stop_ms = 0;
			stop_ss++;
			if(stop_ss >= 60){
				stop_ss = 0;
				stop_mm = 0;
			}
		}
	}
}

ISR(INT4_vect){		//모드버튼
	mode++;
	mode = mode % 3;
	printf("mode : %d\n", mode);
	mode_change = 1;
}

ISR(INT5_vect){
	printf("stopwatch_btn\n");
	if(mode == 1){
		stop_watch_flag ^= 0x01;
	}
}

void INT_init(){
	EIMSK |= (1 << INT4) | (1 << INT5);
	EICRB |= (1 << ISC41) | (1 << ISC51);
}

void TIMER0_init(void){			//1ms마다 인터럽트 발생
	TCCR0 = (1 << WGM01);		//CTC모드 사용. -> 비교일치 발생시 자동으로 TCNT = 0;
	TCCR0 = (1 << CS02);		//분주비 64

	OCR0 = 250;

	TIMSK = (1 << OCIE0);		//비교일치 인터럽트 0 허용
	sei();
}

void print_LCD(){
	char buff[20] = {0};
	LCD_goto_XY(0, 3);
	sprintf(buff, "%2d.%2d.%2d", year, month, day);
	LCD_write_string(buff);
	LCD_goto_XY(1, 0);
	sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
	LCD_write_string(buff);
}

int main(void){
	char buff[16] = { 0, };
	
	DDRE = 0x00;
	PORTE = 0x60;
	DDRF = 0xF0;
	PORTF = 0xFF;
	
	UART0_init();
	stdout = &OUTPUT;
	stdin = &INPUT;
	INT_init();
	TIMER0_init();
	I2C_LCD_init();
	LCD_init();

	
	while(1){
		key_process();
		if(mode_change == 1){
			LCD_clear();
			mode_change = 0;
		}
		if(mode == 0){		//시간출력
			printf("print time\n");
			print_LCD();
			time_print_flag = 0;
		}
		else if(mode == 1){		//스탑워치 출력
			LCD_goto_XY(1, 0);
			sprintf(buff,"%4d %4d %4d", stop_mm, stop_ss, stop_ms);
			LCD_write_string(buff);
		}
		else if(mode== 2){		//알람 출력
			
		}
	}
}