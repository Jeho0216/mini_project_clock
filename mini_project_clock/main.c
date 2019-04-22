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

typedef struct alarm{
	uint8_t hh;
	uint8_t mm;
	uint8_t ss;
}alarm;

volatile int count = 0;
volatile int clock_val = 0;
volatile int stop_ms = 0, stop_ss = 0, stop_mm = 0;
volatile uint8_t hh = 12, mm = 0, ss = 0;
volatile uint8_t year = 19, month = 4, day = 21;
volatile uint8_t mode = 0;		//0 : 시간출력, 1 : 세계시간 2 : 스탑워치, 3 : 알람
volatile uint8_t mode_change = 0;
volatile uint8_t stop_watch_flag = 0, time_print_flag = 0, alarm_set_flag = 0, alarm_flag = 0;		//모드설정 플래그
volatile uint8_t position_cur = 1;
alarm alarm_1;

void alarm_set();
//ISR 정의
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
	if(alarm_set_flag != 1){
		mode++;
		mode = mode % 4;
		printf("mode : %d\n", mode);
		mode_change = 1;
	}
}

ISR(INT5_vect){
	if(mode == 2){
		stop_watch_flag ^= 0x01;
	}
	else if((mode == 3) && (alarm_set_flag == 1)){
		printf("alaram time increase.\n");
		alarm_set();	
		printf("hh : %d  mm : %d  ss : %d\n", alarm_1.hh, alarm_1.mm, alarm_1.ss);
	}
}

ISR(INT6_vect){
	if((mode == 2) && (stop_watch_flag == 0)){
		stop_mm = 0;
		stop_ss = 0;
		stop_ms = 0;
	}
	else if((mode == 3) && (alarm_set_flag == 1)){
		position_cur += 3;
		if(position_cur > 7)
			position_cur = 1;
		printf("position move : %d\n", position_cur);
		LCD_goto_XY(1, position_cur);
	}
}

ISR(INT7_vect){	
	if(mode == 3){
		if(alarm_set_flag == 1){
			eeprom_update_block((alarm *)&alarm_1, (int *)0, sizeof(alarm));		//알람화면에서 3번 버튼을 누르면, 현재 알람시간이 저장됨.
		}
		alarm_set_flag ^= 0x01;		//플래그 반복.
	}
	else if(alarm_flag == 1){
		alarm_flag = 0;
	}
	printf("alarm set flag : %d\n", alarm_set_flag);
}
//초기화 함수
void INT_init(){
	EIMSK |= (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);
	EICRB |= (1 << ISC41) | (1 << ISC51) | (1 << ISC61) | (1 << ISC71);
}

void TIMER0_init(void){			//1ms마다 인터럽트 발생
	TCCR0 = (1 << WGM01);		//CTC모드 사용. -> 비교일치 발생시 자동으로 TCNT = 0;
	TCCR0 = (1 << CS02);		//분주비 64

	OCR0 = 250;

	TIMSK = (1 << OCIE0);		//비교일치 인터럽트 0 허용
	sei();
}
//LCD출력 함수
void print_LCD(int select){
	char buff[20] = {0};
	if(select == 0){
		LCD_goto_XY(0, 2);
		LCD_write_string("KOR");
		LCD_goto_XY(0, 6);
		sprintf(buff, "%02d.%02d.%02d", year, month, day);
		LCD_write_string(buff);
		LCD_goto_XY(1, 4);
		sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
		LCD_write_string(buff);
	}
	else if(select == 1){
		LCD_goto_XY(0, 2);
		LCD_write_string("KOR");
		LCD_goto_XY(0, 6);
		sprintf(buff, "%02d.%02d.%02d", year, month, day);
		LCD_write_string(buff);
		LCD_goto_XY(1, 4);
		sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
		LCD_write_string(buff);
	}
	else if(select == 2){
		LCD_goto_XY(0, 0);
		LCD_write_string("ALARM");
		LCD_goto_XY(1, 0);
		sprintf(buff, "%02d:%02d:%02d", alarm_1.hh, alarm_1.mm, alarm_1.ss);
		LCD_write_string(buff);
	}
	else if(select == 3){
		LCD_goto_XY(0, 0);
		LCD_write_string("ALARM SETTING");
		LCD_goto_XY(1, 0);
		sprintf(buff, "%02d:%02d:%02d", alarm_1.hh, alarm_1.mm, alarm_1.ss);
		LCD_write_string(buff);
	}
}
//알람 읽기(EEPROM)
void read_alarm(){
	eeprom_read_block((alarm *)&alarm_1, (int *)0, sizeof(alarm));		//알람화면에서 3번 버튼을 누르면, 현재 알람시간이 저장됨.
	if((alarm_1.hh == 255) | (alarm_1.mm == 255) | (alarm_1.ss == 255)){		//초기값은 255이므로 이를 처리해줌.
		alarm_1.hh = 0;
		alarm_1.mm = 0;
		alarm_1.ss = 0;
	}
}
//알람 처리함수
void alarm_process(){
	while(alarm_set_flag == 1){
		LCD_write_command(0x0F);
		print_LCD(3);
		LCD_goto_XY(1, 1);
		while(alarm_set_flag == 1);
		LCD_goto_XY(0, 0);
		LCD_write_string("ALARM          ");
		LCD_write_command(0x0C);
	}
}
//알람 설정함수
void alarm_set(){
	switch(position_cur){		//커서위치에 따라 변경할 값을 결정.
		case 1:
			alarm_1.hh++;
			if(alarm_1.hh > 12)
			alarm_1.hh = 0;
			break;
		case 4:
			alarm_1.mm++;
			if(alarm_1.mm > 60)
			alarm_1.mm = 0;
			break;
		case 7:
			alarm_1.ss++;
			if(alarm_1.ss > 60)
			alarm_1.ss = 0;
			break;
	}
	print_LCD(2);
	LCD_goto_XY(1, position_cur);
}
//알람 실행 조건 검사함수.
void check_alarm(){
	if((alarm_1.hh == hh) && (alarm_1.mm == mm) && (alarm_1.ss == ss)){		//알람시간과 현재시간이 일치할 경우,
		printf("alarm! alarm! alarm!\n");
		alarm_flag = 1;
	}
}

int main(void){
	char buff[16] = { 0, };

	DDRE = 0x00;
	PORTE = 0xF0;		//0110 ->7654
	DDRF = 0xF0;
	PORTF = 0xFF;
	
	UART0_init();
	stdout = &OUTPUT;
	stdin = &INPUT;
	INT_init();
	TIMER0_init();
	I2C_LCD_init();
	LCD_init();
	read_alarm();

	while(1){
		key_process();
		check_alarm();
		if(mode_change == 1){
			LCD_clear();
			mode_change = 0;
		}
		if(mode == 0){		//시간출력
			print_LCD(0);
			time_print_flag = 0;
		}
		else if(mode == 1){
			print_LCD(1);
			time_print_flag = 0;
		}
		else if(mode == 2){		//스탑워치 출력
			LCD_goto_XY(1, 0);
			sprintf(buff,"%4d %4d %4d", stop_mm, stop_ss, stop_ms);
			LCD_write_string(buff);
		}
		else if(mode== 3){		//알람 출력
			print_LCD(2);
			alarm_process();
		}
	}
}