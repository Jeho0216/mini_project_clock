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

typedef enum world_time{SEL = 0, TYO, LON, NYC} world_time;

volatile int count = 0, count_alarm = 0, alarm_time = 0;
volatile int clock_val = 0;
volatile int stop_ms = 0, stop_ss = 0, stop_mm = 0;
volatile uint8_t hh = 0, mm = 0, ss = 1;
volatile uint8_t country_1 = LON, country_2 = SEL;
volatile uint8_t year = 19, month = 12, day = 31;
volatile uint8_t mode = 0;		//0 : 시간출력, 1 : 세계시간 2 : 스탑워치, 3 : 알람
volatile uint8_t mode_change = 0;
volatile uint8_t stop_watch_flag = 0, time_print_flag = 0, time_set_flag = 0, alarm_set_flag = 0, alarm_flag = 0;		//모드설정 플래그
volatile uint8_t position_cur = 1, position_cur_2 = 0;

const int month_end_day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

alarm alarm_1;

void alarm_set();
void time_set(int time_num);
void calc_time(int time_num, char buff[]);
//ISR 정의
ISR(TIMER0_COMP_vect){
	if(time_set_flag == 0){
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
						if(day > month_end_day[month - 1]){
							day = 1;
							month++;
							if(month > 12){
								month = 1;
								year++;
							}
						}
						hh = 0;
					}
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
	if(alarm_flag == 1){
		count_alarm++;
		if(count_alarm >= 100){
			alarm_time++;
			if(alarm_time < 10)
				OCR3A ^= 0x020B;
			else{
				OCR3A = 0x00;
				if(alarm_time == 16)
					alarm_time = 0;
			}
			printf("OCR3A : %X\nalarm_time : %d\n", OCR3A, alarm_time);
			count_alarm = 0;
		}
	}
}

ISR(INT4_vect){		//모드버튼
	if(alarm_set_flag != 1 && time_set_flag != 1){
		mode++;
		mode = mode % 4;
		printf("mode : %d\n", mode);
		mode_change = 1;
	}
}
//시간증가 버튼
ISR(INT5_vect){
	if(mode == 2){
		stop_watch_flag ^= 0x01;
	}

	else if((mode == 3) && (alarm_set_flag == 1)){
		printf("alaram time increase.\n");
		alarm_set();
		printf("hh : %d  mm : %d  ss : %d\n", alarm_1.hh, alarm_1.mm, alarm_1.ss);
	}
	else if(((mode == 0) || (mode == 1)) && (time_set_flag == 1)){
		printf("time time increase\n");
		printf("mode = %d, position_cur = %d position_cur_2 = %d\n", mode, position_cur, position_cur_2);
		time_set(mode);
		printf("hh : %d mm : %d ss : %d\n", hh, mm, ss);
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
		printf("alarm position move : %d\n", position_cur);
		LCD_goto_XY(1, position_cur);
	}
	else if(((mode == 0) || (mode == 1)) && (time_set_flag == 1)){
		position_cur +=3;
		if(position_cur == 16){
			position_cur = 5;
			position_cur_2 = 1;
		}
		if(position_cur == 14){
			position_cur = 4;
			position_cur_2 = 0;
		}
		printf("time position move : %d\n", position_cur);
		LCD_goto_XY(position_cur_2, position_cur + 2);
	}
}
//각종 setting모드 진입 버튼, 알람 끄기 버튼.
ISR(INT7_vect){
	if(mode == 3){
		if(alarm_set_flag == 1){
			eeprom_update_block((alarm *)&alarm_1, (int *)0, sizeof(alarm));		//알람화면에서 3번 버튼을 누르면, 현재 알람시간이 저장됨.
		}
		alarm_set_flag ^= 0x01;		//플래그 반복.
		printf("alarm set flag : %d\n", alarm_set_flag);
	}
	else if(alarm_flag == 1){		//알람 끄기.
		TCCR3A &= ~(1 << COM3A0);
		alarm_flag = 0;
		printf("alarm flag : %d\n", alarm_flag);
	}
	else if(mode == 0){
		time_set_flag ^= 0x01;
		printf("time set flag : %d\n", time_set_flag);
	}
	else if(mode == 1){
		time_set_flag ^= 0x01;
		printf("time set flag : %d\n", time_set_flag);
	}
	
}
//초기화 함수
void INT_init(){
	EIMSK |= (1 << INT4) | (1 << INT5) | (1 << INT6) | (1 << INT7);
	EICRB |= (1 << ISC41) | (1 << ISC51) | (1 << ISC61) | (1 << ISC71);
	
	DDRE |= (1 << PORTE3);
	PORTE = 0x00;
	
	TCCR3B |= (1 << WGM32) | (1 << CS31);		//분주비 256, CTC모드(OCR값이 TOP) -> CTC모드 50% PWM출력
	OCR3A = 0x00;
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
		LCD_goto_XY(0, 0);
		LCD_write_string("T1");
		calc_time(0, buff);
		LCD_goto_XY(1, 6);
		LCD_write_string(buff);
	}
	else if(select == 1){
		LCD_goto_XY(0, 0);
		LCD_write_string("T2");
		calc_time(1, buff);
		LCD_goto_XY(1, 6);
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

void time_set_process(){
	while(time_set_flag == 1){
		LCD_write_command(0x0F);
		position_cur = 4;
		position_cur_2 = 0;
		LCD_goto_XY(0, position_cur + 2);
		while(time_set_flag == 1);
		LCD_goto_XY(0, 0);
		LCD_write_command(0x0C);
	}
}

//도시별 시간 계산
void calc_time(int time_num, char buff[]){
	int temp_hh = hh;
	int temp_day = day;
	int temp_month = month;
	int temp_year = year;
	
	if(time_num == 0){
		switch(country_1){
			case SEL :		//서울
				LCD_goto_XY(0, 4);
				LCD_write_string("SEL");
				temp_hh += 8;
				if(temp_hh >= 24){
					temp_day++;
					if(temp_day > month_end_day[temp_month - 1]){
						temp_day = 1;
						temp_month++;
						if(temp_month > 12){
							temp_year++;
							temp_month = 1;
						}
					}
					temp_hh -= 24;
					printf("day : %d %d %d\n", temp_year, temp_month, temp_day);
				}
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", temp_year, temp_month, temp_day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", temp_hh, mm, ss);
			break;
			case TYO :		//도쿄
				LCD_goto_XY(0, 4);
				LCD_write_string("TYO");
				temp_hh += 8;
				if(temp_hh >= 24){
					temp_day++;
					if(temp_day > month_end_day[temp_month - 1]){
						temp_day = 1;
						temp_month++;
						if(temp_month > 12){
							temp_year++;
							temp_month = 1;
						}
					}
					temp_hh -= 24;
					printf("day : %d %d %d\n", temp_year, temp_month, temp_day);
				}
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", temp_year, temp_month, temp_day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", temp_hh, mm, ss);
			break;
			case LON :		//런던
				LCD_goto_XY(0, 4);
				LCD_write_string("LON");
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", year, month, day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
			break;
			case NYC :		//뉴욕
				LCD_goto_XY(0, 4);
				LCD_write_string("NYC");
				temp_hh += 11;
				if(temp_hh >= 24){
					temp_day++;
					if(temp_day > month_end_day[temp_month - 1]){
						temp_day = 1;
						temp_month++;
						if(temp_month > 12){
							temp_year++;
							temp_month = 1;
						}
					}
					temp_hh -= 24;
				}
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", temp_year, temp_month, temp_day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", temp_hh, mm, ss);
			break;
		}
	}
	else if(time_num == 1){
		switch(country_2){
			case SEL :		//서울
				LCD_goto_XY(0, 4);
				LCD_write_string("SEL");
				temp_hh += 8;
				if(temp_hh >= 24){
					temp_day++;
					if(temp_day > month_end_day[temp_month - 1]){
						temp_day = 1;
						temp_month++;
						if(temp_month > 12){
							temp_year++;
							temp_month = 1;
						}
					}
					temp_hh -= 24;
					printf("day : %d %d %d\n", temp_year, temp_month, temp_day);
				}
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", temp_year, temp_month, temp_day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", temp_hh, mm, ss);
			break;
			case TYO :		//도쿄
				LCD_goto_XY(0, 4);
				LCD_write_string("TYO");
				temp_hh += 8;
				if(temp_hh >= 24){
					temp_day++;
					if(temp_day > month_end_day[temp_month - 1]){
						temp_day = 1;
						temp_month++;
						if(temp_month > 12){
							temp_year++;
							temp_month = 1;
						}
					}
					temp_hh -= 24;
					printf("day : %d %d %d\n", temp_year, temp_month, temp_day);
				}
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", temp_year, temp_month, temp_day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", temp_hh, mm, ss);
			break;
			case LON :		//런던
				LCD_goto_XY(0, 4);
				LCD_write_string("LON");
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", year, month, day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", hh, mm, ss);
			break;
			case NYC :		//뉴욕
				LCD_goto_XY(0, 4);
				LCD_write_string("NYC");
				temp_hh += 11;
				if(temp_hh >= 24){
					temp_day++;
					if(temp_day > month_end_day[temp_month - 1]){
						temp_day = 1;
						temp_month++;
						if(temp_month > 12){
							temp_year++;
							temp_month = 1;
						}
					}
					temp_hh -= 24;
				}
				LCD_goto_XY(0, 8);
				sprintf(buff, "%02d.%02d.%02d", temp_year, temp_month, temp_day);
				LCD_write_string(buff);
				sprintf(buff, "%02d:%02d:%02d", temp_hh, mm, ss);
			break;
		}
	}
}

void time_set(int time_num){
	switch(position_cur){		//커서위치에 따라 변경할 값을 결정.
		case 4:
			if(time_num == 0){
				country_1++;
				country_1 = country_1 % 4;
			}
			else if(time_num == 1){
				country_2++;
				country_2 = country_2 % 4;	
			}
		break;
		case 7:
			year++;
		break;
		case 10:
			month++;
			if(month > 12)
				month = 1;
		break;
		case 13:
			day++;
			if(day > month_end_day[month - 1])
				day = 1;
		break;
		case 5:
			hh++;
			if(hh > 24)
				hh = 0;
		break;
		case 8:
			mm++;
			if(mm > 60)
				mm = 0;
		break;
		case 11:
			ss++;
			if(ss > 60)
				ss = 0;
		break;
	}
	if(time_num == 0)		print_LCD(0);
	else if(time_num == 1)	print_LCD(1);
	printf("country : %d\n", country_1);
	LCD_goto_XY(position_cur_2, position_cur + 2);
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
		position_cur = 1;
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
			if(alarm_1.hh > 24)
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
	uint8_t temp_hh = hh;
	switch(country_1){
		case TYO :
		case SEL : 
			temp_hh += 8;
			if(temp_hh >= 24)
				temp_hh -= 24;
		break;
		case NYC :
			temp_hh += 11;
			if(temp_hh >= 24)
				temp_hh -= 24;
		break;
	}
	if(((alarm_1.hh == temp_hh) && (alarm_1.mm == mm) && (alarm_1.ss == ss)) || (alarm_flag == 1)){		//알람시간과 현재시간이 일치할 경우,
		printf("alarm! alarm! alarm!\n");
		TCCR3A |= (1 << COM3A0);
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
	LCD_init();
	I2C_LCD_init();
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
			time_print_flag = 0;		//1초에 한번 출력하기 위한 변수.
			time_set_process();
		}
		else if(mode == 1){		//세계시간 출력
			print_LCD(1);
			time_print_flag = 0;
			time_set_process();
		}
		else if(mode == 2){		//스탑워치 출력
			LCD_goto_XY(0, 0);
			LCD_write_string("STOP WATCH");
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