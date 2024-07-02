////////////////////////////////////////////////////////////////////////////////////
//	과제명: TP1. 커피 자동판매기
//	과제개요: 커피를 자동으로 제조 및 판매하는 커피자판기 제어 프로그램을 작성함
//	사용한 하드웨어(기능): GPIO, joy_stick, EXTI, GLCD, LED, Buzzer, Switch
//	제출일: 2024. 6. 15
//	제출자 클래스: 목요일반
//	학번: 2020132025
//	이름: 이정우
/////////////////////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"
void _GPIO_Init(void);
void BEEP(void);
void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void _EXTI_Init(void);
void COIN(void);				//현재까지 입력된 돈의 양을 나타내는 함수
void TOTAL(void);				//현재까지의 매출을 나타내는 함수
void COFFEERESET(void);			//커피를 조이스틱으로 고르는 상태를 커피를 다 만들고 상태를 초기화 하는 함수
void NoC(void);					//현재까지 판 커피의 수를 나타내는 함수
void STOCK(void);				//현재 남은 커피, 설탕, 우유, 컵의 수량을 나타내는 함수
void RF(void);					//재고가 0이 됐을때 채우는 함수
void WORKING(void);				//커피를 만드는 함수
void MAKING(void);				//커피를 만드는 동작을 하는 함수
void CHANGE(void);				//잔돈을 반환하는 함수
void CLEAR(void);				//매출과 판매수량을 초기화 하는 함수
uint8_t  IN;		//투입된 코인의 양 FRAM을 읽어옴
uint8_t  in10 = 0, in50 = 0, in1050 = 0, spent = 0;		//투입된 코인을 계산하기 위한 변수 /in1050은 총 돈 in10은 10원 in50은 50원 spent는 커피를 선택하고 만들때 사용 한 돈
uint8_t  IN100 = 0, IN10 = 0, IN1 = 0;		//코인을 GLCD에 표시하기 위한 변수
uint8_t   tot = 0, MONEY100 = 0, MONEY10 = 0, MONEY1 = 0, MONEY = 0;		//매출을 표시하기 위한 변수
uint8_t  noc = 0, NUMC100 = 0, NUMC10 = 0, NUMC1 = 0, NUMC = 0;		//판매된 커피를 표시하기 위한 변수

uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
uint8_t	SW0_Flag, SW1_Flag; 	// Switch 입력이 홀수번째인지? 짝수번째인지? 를 알기위한 변수 
uint8_t  Cup, Sugar, Milk, Coffee;		//재고를 표시하기 위한 변수
char COFFEESTATE;		//커피 상태를 나타내기 위한 변수

int main(void)
{
	BEEP();
	DelayMS(100);
	_GPIO_Init(); 		// GPIO (LED, SW, Buzzer) 초기화
	LCD_Init();		// LCD 모듈 초기화
	_EXTI_Init();
	GPIOG->ODR &= ~0x00FF;	// LED 초기값: LED0~7 Off
	DisplayInitScreen();	// LCD 초기화면


	while (1)
	{
		if (Cup != 0 & Sugar != 0 & Milk != 0 & Coffee != 0) {		//재고가 하나라도 0이면 조이스캔 동작을 하지 못하게 하기 위한 조건문
			switch (JOY_Scan())
			{
			case 0x01E0:	// NAVI_LEFT
				COFFEERESET();		//새로운 커피를 선택하면 그 전 선택지는 지우는 함수
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(3, 2, 'B');
				COFFEESTATE = 'B';		//커피 상태를 선택지에 맞게 바꿈
				break;
			case 0x03A0:	// NAVI_UP
				COFFEERESET();
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(2, 3, 'S');
				COFFEESTATE = 'S';
				break;
			case 0x02E0:	// NAVI_RIGHT
				COFFEERESET();
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(3, 4, 'M');
				COFFEESTATE = 'M';
				break;
			}
		}
		switch (KEY_Scan())	// 입력된 Switch 정보 분류 
		{
		case 0xFB00:		//SW2를 누르면
			if (Cup != 0 & Sugar != 0 & Milk != 0 & Coffee != 0)		//재고가 0아 아니라면
				WORKING();		//커피 만들기 시작
			break;
		}
	}
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정	*/
void _GPIO_Init(void)		//설정 끝
{
	// LED (GPIO G) 설정 : Output mode
	RCC->AHB1ENR |= 0x0040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER &= ~0xFFFF;	// GPIOG 0~7 : Clear (0b00)						
	GPIOG->MODER |= 0x5555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER &= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR &= ~0xFFFF;	// GPIOG 0~7 : Clear (0b00) 
	GPIOG->OSPEEDR |= 0x5555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 

	// SW (GPIO H) 설정 : Input mode 
	RCC->AHB1ENR |= 0x0080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER &= ~0x03FF0000;	// GPIOH 8~12 : Input mode (reset state)
	GPIOH->PUPDR &= ~0x03FF0000;	// GPIOH 8, 9, 10~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 : Output mode
	RCC->AHB1ENR |= 0x0020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER &= ~0x000C0000;	// GPIOF 9 : Clear (0b00)						
	GPIOF->MODER |= 0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER &= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR &= ~0x000C0000;	// GPIOF 9 : Clear (0b00) 
	GPIOF->OSPEEDR |= 0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 

	// Joy_Stick (GPIO I) 설정 : Input mode 
	RCC->AHB1ENR |= 0x0100;	// RCC_AHB1ENR GPIOI Enable	조이스틱 다운을 사용하기 위함
	GPIOI->MODER &= ~0x000F3000;	// GPIOI 6, 8, 9 : Input mode (reset state)
	GPIOI->PUPDR &= ~0x000F3000;	// GPIOI 6, 8, 9: Floating input (No Pull-up, pull-down) (reset state)

}

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	Fram_Init();                    // FRAM 초기화 H/W 초기화
	Fram_Status_Config();   // FRAM 초기화 S/W 초기화

	LCD_Clear(RGB_WHITE);		// 화면 클리어
	LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_YELLOW);	//배경  노란색
	LCD_SetTextColor(RGB_BLACK);	//글자색 검정
	LCD_DisplayText(0, 0, "LJW coffee");
	noc = Fram_Read(604);		//초기 상태에서 이전 상태를 읽어 오기 위해 Fram 읽어옴
	NoC();		//NoC 표시
	LCD_SetBackColor(RGB_WHITE);		//글씨중 배경이 흰색이며 검정글씨들 모두 작성
	LCD_DisplayText(0, 15, "IN");		//이후로는 GLCD 기본 세팅들
	LCD_DisplayText(1, 12, "10");
	LCD_DisplayChar(1, 11, 0x5c);		//\를 표시하기 위해 아스키 코드 사용
	LCD_DisplayText(2, 15, "TOT");
	LCD_DisplayText(3, 11, "\\50");		//아스키 코드가 아닌 \를 두번 쓰는 방법도 사용
	//	LCD_DisplayChar(3, 11, 0x5c);
	Cup = 9;
	Sugar = 5;
	Milk = 5;
	Coffee = 9;
	STOCK();		//재고표시
	LCD_DisplayText(6, 15, "RF");
	LCD_DisplayText(7, 1, "cp sg mk cf");
	LCD_DisplayText(7, 15, "NoC");
	LCD_SetTextColor(RGB_WHITE);		//커피 상태를 나타내는 칸들은 색이 다름
	LCD_SetBackColor(RGB_BLUE);
	LCD_DisplayChar(2, 3, 'S');
	LCD_DisplayChar(3, 2, 'B');
	LCD_DisplayChar(3, 4, 'M');
	LCD_SetBackColor(RGB_RED);		//W는 빨간색
	LCD_DisplayChar(4, 3, 'W');
	LCD_SetTextColor(RGB_YELLOW);		//IN 과 TOT는 검정 배경에 노란색 글씨
	LCD_SetBackColor(RGB_BLACK);
	in1050 = Fram_Read(602);		//리셋 이전 상태의 코인 읽어오기
	COIN();		//코인 표시
	tot = Fram_Read(603);		//리셋 이전 상태의 매출 읽어오기
	TOTAL();		//매출 표시

	LCD_SetPenColor(RGB_BLACK);
	LCD_DrawRectangle(0, 0, 80, 12);		//이름부분 검정 네모

	LCD_SetPenColor(RGB_GREEN);		//초록색 네모칸들
	LCD_DrawRectangle(8 * 15 - 1, 13 * 1 - 1, 8 * 3 + 1, 13 * 1);	//in 부분 검정 네모

	LCD_DrawRectangle(8 * 3 - 1, 13 * 2 - 1, 9 * 1, 13 * 1);		//S
	LCD_DrawRectangle(8 * 2 - 1, 13 * 3 - 1, 9 * 1, 13 * 1);		//B
	LCD_DrawRectangle(8 * 4 - 1, 13 * 3 - 1, 9 * 1, 13 * 1);		//N
	LCD_DrawRectangle(8 * 3 - 1, 13 * 4 - 1, 9 * 1, 13 * 1);		//W
	LCD_DrawRectangle(8 * 15 - 1, 13 * 3 - 1, 8 * 3 + 1, 13 * 1);	//TOT 부분 검정 네모


	LCD_DrawRectangle(8 * 2 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//9
	LCD_DrawRectangle(8 * 5 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//5
	LCD_DrawRectangle(8 * 8 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//5
	LCD_DrawRectangle(8 * 11 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//9

	LCD_DrawRectangle(8 * 15 - 1, 13 * 8 - 1, 8 * 3 + 1, 13 * 1);		//NoC
	LCD_DrawRectangle(8 * 17 - 1, 13 * 6 - 1, 8 * 1, 13 * 1);		//RF
	LCD_SetBrushColor(RGB_GREEN);						//RF 네모 색
	LCD_DrawFillRect(8 * 17, 13 * 6, 7, 12);					//RF 네모 칠

	LCD_DrawRectangle(8 * 12 - 1, 13 * 2, 8 * 1, 10 * 1);		//10원 네모
	LCD_DrawRectangle(8 * 12 - 1, 13 * 4, 8 * 1, 10 * 1);		//50원 네모
	LCD_SetBrushColor(RGB_GRAY);		//회색
	LCD_DrawFillRect(8 * 12, 13 * 2 + 1, 7, 9);		//10원 네모 회색 칠
	LCD_DrawFillRect(8 * 12, 13 * 4 + 1, 7, 9);		//50원 네모 회색 칠

}

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) 초기 설정  */
void _EXTI_Init(void)
{
	RCC->APB2ENR |= 0x00004000;	// Enable System Configuration Controller Clock
	// EXTI 8,9,11설정
	SYSCFG->EXTICR[2] |= 0x7077; 	// EXTI8,9,11에 대한 소스 입력은 GPIOH로 설정
					// EXTI8 <- PH8, EXTI9 <- PH9 
					// EXTICR3(EXTICR[2])를 이용 
					// reset value: 0x0000
	// EXTI12~13 설정
	SYSCFG->EXTICR[3] |= 0x0077; 	// EXTI12, 13에 대한 소스 입력은 GPIOH로 설정
					// EXTI12 <- PH12
					// EXTICR4(EXTICR[3])를 이용 
					// reset value: 0x0000	

	EXTI->FTSR |= 0x3B00;		// EXTI전부: Falling Trigger Enable 8~9, 11~15
	EXTI->IMR |= 0x3B00;		// EXTI IMR 설정

	NVIC->ISER[0] |= (1 << 23);	// 0x00800000
					// Enable 'Global Interrupt EXTI8, 9'
					// Vector table Position 참조
	NVIC->ISER[1] |= (1 << (40 - 32));// 0x00000100
					// Enable 'Global Interrupt EXTI11~13'
					// Vector table Position 참조
}

/* EXTI5~9 ISR */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & 0x0100)			//EXTI8 Interrupt Pending(발생) 여부?
	{
		EXTI->PR |= 0x0100;		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		BEEP();
		LCD_SetBrushColor(RGB_YELLOW);
		LCD_DrawFillRect(8 * 12, 13 * 2 + 1, 7, 9);		//10원 네모 노란색 칠
		DelayMS(1000);
		LCD_SetBrushColor(RGB_GRAY);
		LCD_DrawFillRect(8 * 12, 13 * 2 + 1, 7, 9);		//10원 네모 회색 칠
		in10 += 10;		//IN +10
		COIN();		//더해진 코인표시
	}
	else if (EXTI->PR & 0x0200)
	{
		EXTI->PR |= 0x0200;
		BEEP();
		LCD_SetBrushColor(RGB_YELLOW);
		LCD_DrawFillRect(8 * 12, 13 * 4 + 1, 7, 9);		//50원 네모 노란색 칠
		DelayMS(1000);
		LCD_SetBrushColor(RGB_GRAY);
		LCD_DrawFillRect(8 * 12, 13 * 4 + 1, 7, 9);		//50원 네모 회색 칠
		in50 += 50;
		COIN();
	}
}

/* EXTI15 ISR */
void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & 0x1000)	//sw4
	{
		EXTI->PR |= 0x1000;
		RF();		//리필 함수 동작
	}
	else if (EXTI->PR & 0x0800)	//sw3
	{
		EXTI->PR |= 0x0800;
		CHANGE();		//잔돈반환 함수 동작
	}
	else if (EXTI->PR & 0x2000)	//sw5
	{
		EXTI->PR |= 0x2000;
		CLEAR();		//매출, 판매수 리셋 함수 동작
	}

}

void COIN(void)		//입력된 코인 표시하는 함수
{
	in1050 = in1050 + in10 + in50 - spent;		//in1050은 총 돈 in10은 10원 in50은 50원 spent는 커피를 선택하고 만들때 사용 한 돈
	in10 = 0, in50 = 0, spent = 0;		//리셋
	if (in1050 > 200) in1050 = 200;		//200원이 넘으면 200원을 표시하기 위한 조건문
	Fram_Write(602, in1050);		//fram602에 총 돈 입력
	IN = Fram_Read(602);		//fram602 읽어서 IN에 입력
	IN100 = IN / 100;		//IN 100의자리
	IN10 = IN % 100 / 10;	//IN 10의자리
	IN1 = IN % 10;			//IN 1의자리 (사실상 필요 없음)
	LCD_SetTextColor(RGB_YELLOW);	//색 설정
	LCD_SetBackColor(RGB_BLACK);
	LCD_DisplayChar(1, 15, IN100 + 0x30);	//IN 100의자리
	LCD_DisplayChar(1, 16, IN10 + 0x30);	//IN 10의자리
	LCD_DisplayChar(1, 17, IN1 + 0x30);		//IN 1의자리 (사실상 필요 없음)
}

void TOTAL(void)		//매출 표시하는 함수
{
	if (tot >= 99) tot = 99;		//매출이 990을 넘으면 990을 표시하기 위한 함수
	Fram_Write(603, tot);		//fram603에 매출 입력
	MONEY = Fram_Read(603);		//fram603 읽어 MONEY에 입력
	MONEY10 = MONEY % 100 / 10;		//TOT 100의 자리
	MONEY1 = MONEY % 10;			//TOT 10의 자리
	LCD_SetTextColor(RGB_YELLOW);	//색 설정
	LCD_SetBackColor(RGB_BLACK);
	LCD_DisplayChar(3, 15, MONEY10 + 0x30);	//TOT 표시
	LCD_DisplayChar(3, 16, MONEY1 + 0x30);
	LCD_DisplayChar(3, 17, 0x30);
}

void COFFEERESET(void)		//커피 상태를 리셋 하는 함수
{
	COFFEESTATE = 'Q';		//커피를 다 만들고 다시 선택을 하지 않고 워킹을 눌렀을때 동작하지 않게 하기 위한 임의의 상태
	LCD_SetBackColor(RGB_BLUE);	//색 설정
	LCD_SetTextColor(RGB_WHITE);
	LCD_DisplayChar(2, 3, 'S');	//초기상태로 되돌림
	LCD_DisplayChar(3, 4, 'M');
	LCD_DisplayChar(3, 2, 'B');
}

void NoC(void)		//총 판매수를 표시하는 함수
{
	LCD_SetBackColor(RGB_YELLOW);		//색 설정
	LCD_SetTextColor(RGB_BLACK);
	if (noc > 50) noc = 50;		//총 판매수가 50을 넘으면 50을 표시
	Fram_Write(604, noc);		//총 판매수 fram에 입력
	NUMC = Fram_Read(604);		//fram읽어 NUMC에 입력
	NUMC100 = NUMC / 100;		//NoC 100의 자리
	NUMC10 = NUMC % 100 / 10;	//NoC 10의 자리
	NUMC1 = NUMC % 10;			//Noc 1의 자리
	LCD_DisplayChar(8, 15, NUMC100 + 0x30);	//NoC 표시
	LCD_DisplayChar(8, 16, NUMC10 + 0x30);
	LCD_DisplayChar(8, 17, NUMC1 + 0x30);
}

void STOCK(void)		//재고를 나타내는 함수
{
	LCD_SetBackColor(RGB_WHITE);	//색 설정
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(6, 2, Cup + 0x30);	//재고 표시
	LCD_DisplayChar(6, 5, Sugar + 0x30);
	LCD_DisplayChar(6, 8, Milk + 0x30);
	LCD_DisplayChar(6, 11, Coffee + 0x30);
	if (Cup == 0 | Sugar == 0 | Milk == 0 | Coffee == 0)		//만약 재고중 하나라도 0이 된다면
	{
		EXTI->IMR &= ~0x0300;		//SW 1, 2 disable
		//RCC->AHB1ENR &= ~0x0100;		//조이스틱 디스에이블 하려 했지만 이렇게 하면 TOT와 NoC가 0으로 표시되는 버그발생 이유는 못찾음 결국 다른 방식(main에 조건문 넣는)으로 해결
		LCD_SetBrushColor(RGB_RED);						//RF 네모
		LCD_DrawFillRect(8 * 17, 13 * 6, 7, 12);		//RF 네모 칠
	}
}

void RF(void)		//재고 리필 함수
{
	BEEP();
	DelayMS(500);
	BEEP();
	if (Cup == 0)
	{
		Cup = 9;
	}
	if (Sugar == 0)
	{
		Sugar = 5;
	}
	if (Milk == 0)
	{
		Milk = 5;
	}
	if (Coffee == 0)
	{
		Coffee = 9;
	}
	STOCK();		//리필한 수량 GLCD에 표시
	LCD_SetBrushColor(RGB_GREEN);						//RF 네모
	LCD_DrawFillRect(8 * 17, 13 * 6, 7, 12);					//RF 네모 칠
	EXTI->IMR |= 0x0B00; 		//EXTI 다시 ON
}

void WORKING(void)		//커피 만드는 함수
{
	COIN();
	if (IN >= 20 & COFFEESTATE == 'S')		//돈이 20원이상 있고 커피를 S를 골랐을 경우
	{
		BEEP();
		Cup -= 1;		//해당하는 수량들 감소
		Sugar -= 1;
		Coffee -= 1;
		spent += 20;	//사용한 돈 증가
		tot += 2;		//매출 증가
		noc += 1;		//총 판매수 증가
		MAKING();		//커피 만드는 동작 함수 실행
		COFFEERESET();	//커피선택 리셋
	}
	else if (IN >= 10 & COFFEESTATE == 'B')		//커피 B 선택시 이하 동일
	{
		BEEP();
		Cup -= 1;
		Coffee -= 1;
		spent += 10;
		tot += 1;
		noc += 1;
		MAKING();
		COFFEERESET();
	}
	else if (IN >= 30 & COFFEESTATE == 'M')
	{
		BEEP();
		Cup -= 1;
		Sugar -= 1;
		Coffee -= 1;
		Milk -= 1;
		spent += 30;
		tot += 3;
		noc += 1;
		MAKING();
		COFFEERESET();
	}
	//else if ( COFFEESTATE == 'Q')
	//{DelayMS(1);}
	COIN();			//커피를 만들고 난 후 바뀐 변수들 표시
	STOCK();
	TOTAL();
	NoC();
}

void MAKING(void)		//커피 만드는 동작 함수
{
	EXTI->IMR &= ~0x3B00;			//커피 제조중 모든 동작 멈춤
	GPIOG->ODR |= 0x00FF;			//이후로는 동작들
	LCD_SetBackColor(RGB_RED);
	LCD_SetTextColor(RGB_WHITE);	
	LCD_DisplayChar(4, 3, '0');
	DelayMS(1000);
	LCD_DisplayChar(4, 3, '1');
	DelayMS(1000);
	LCD_DisplayChar(4, 3, '2');
	DelayMS(1000);
	LCD_DisplayChar(4, 3, 'W');
	GPIOG->ODR &= ~0x00FF;
	BEEP();
	DelayMS(500);
	BEEP();
	DelayMS(500);
	BEEP();
	EXTI->IMR |= 0x3B00; 		//EXTI 다시 ON
}

void CHANGE(void)		//잔돈 반환하는 함수
{
	in1050 = 0;		//돈 관련 변수들 0으로 초기화
	in10 = 0;
	in50 = 0;
	spent = 0;
	COIN();			//리셋된 잔돈 표시
}

void CLEAR(void)		//TOT와 NoC 리셋
{
	tot = 0;
	TOTAL();
	noc = 0;
	NoC();
}

/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if (key == 0xFF00)		// if no key, check key off
	{
		if (key_flag == 0)
			return key;
		else
		{
			DelayMS(10);
			key_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{
		if (key_flag != 0)	// if continuous key, treat as no key input
			return 0xFF00;
		else			// if new key,delay for debounce
		{
			key_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}
uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{
	uint16_t key;
	key = GPIOI->IDR & 0x03E0;	// any key pressed ?
	if (key == 0x03E0)		// if no key, check key off
	{
		if (joy_flag == 0)
			return key;
		else
		{
			DelayMS(10);
			joy_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{
		if (joy_flag != 0)	// if continuous key, treat as no key input
			return 03E0;
		else			// if new key,delay for debounce
		{
			joy_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}

/* Buzzer: Beep for 30 ms */
void BEEP(void)
{
	GPIOF->ODR |= 0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}
void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i = 0; i < wMS; i++)
		DelayUS(1000);	// 1000us => 1ms
}
void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS * 17;
	for (; Dly; Dly--);
}