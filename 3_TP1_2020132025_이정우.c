////////////////////////////////////////////////////////////////////////////////////
//	������: TP1. Ŀ�� �ڵ��Ǹű�
//	��������: Ŀ�Ǹ� �ڵ����� ���� �� �Ǹ��ϴ� Ŀ�����Ǳ� ���� ���α׷��� �ۼ���
//	����� �ϵ����(���): GPIO, joy_stick, EXTI, GLCD, LED, Buzzer, Switch
//	������: 2024. 6. 15
//	������ Ŭ����: ����Ϲ�
//	�й�: 2020132025
//	�̸�: ������
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
void COIN(void);				//������� �Էµ� ���� ���� ��Ÿ���� �Լ�
void TOTAL(void);				//��������� ������ ��Ÿ���� �Լ�
void COFFEERESET(void);			//Ŀ�Ǹ� ���̽�ƽ���� ���� ���¸� Ŀ�Ǹ� �� ����� ���¸� �ʱ�ȭ �ϴ� �Լ�
void NoC(void);					//������� �� Ŀ���� ���� ��Ÿ���� �Լ�
void STOCK(void);				//���� ���� Ŀ��, ����, ����, ���� ������ ��Ÿ���� �Լ�
void RF(void);					//��� 0�� ������ ä��� �Լ�
void WORKING(void);				//Ŀ�Ǹ� ����� �Լ�
void MAKING(void);				//Ŀ�Ǹ� ����� ������ �ϴ� �Լ�
void CHANGE(void);				//�ܵ��� ��ȯ�ϴ� �Լ�
void CLEAR(void);				//����� �Ǹż����� �ʱ�ȭ �ϴ� �Լ�
uint8_t  IN;		//���Ե� ������ �� FRAM�� �о��
uint8_t  in10 = 0, in50 = 0, in1050 = 0, spent = 0;		//���Ե� ������ ����ϱ� ���� ���� /in1050�� �� �� in10�� 10�� in50�� 50�� spent�� Ŀ�Ǹ� �����ϰ� ���鶧 ��� �� ��
uint8_t  IN100 = 0, IN10 = 0, IN1 = 0;		//������ GLCD�� ǥ���ϱ� ���� ����
uint8_t   tot = 0, MONEY100 = 0, MONEY10 = 0, MONEY1 = 0, MONEY = 0;		//������ ǥ���ϱ� ���� ����
uint8_t  noc = 0, NUMC100 = 0, NUMC10 = 0, NUMC1 = 0, NUMC = 0;		//�Ǹŵ� Ŀ�Ǹ� ǥ���ϱ� ���� ����

uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
uint8_t	SW0_Flag, SW1_Flag; 	// Switch �Է��� Ȧ����°����? ¦����°����? �� �˱����� ���� 
uint8_t  Cup, Sugar, Milk, Coffee;		//��� ǥ���ϱ� ���� ����
char COFFEESTATE;		//Ŀ�� ���¸� ��Ÿ���� ���� ����

int main(void)
{
	BEEP();
	DelayMS(100);
	_GPIO_Init(); 		// GPIO (LED, SW, Buzzer) �ʱ�ȭ
	LCD_Init();		// LCD ��� �ʱ�ȭ
	_EXTI_Init();
	GPIOG->ODR &= ~0x00FF;	// LED �ʱⰪ: LED0~7 Off
	DisplayInitScreen();	// LCD �ʱ�ȭ��


	while (1)
	{
		if (Cup != 0 & Sugar != 0 & Milk != 0 & Coffee != 0) {		//��� �ϳ��� 0�̸� ���̽�ĵ ������ ���� ���ϰ� �ϱ� ���� ���ǹ�
			switch (JOY_Scan())
			{
			case 0x01E0:	// NAVI_LEFT
				COFFEERESET();		//���ο� Ŀ�Ǹ� �����ϸ� �� �� �������� ����� �Լ�
				LCD_SetTextColor(RGB_RED);
				LCD_DisplayChar(3, 2, 'B');
				COFFEESTATE = 'B';		//Ŀ�� ���¸� �������� �°� �ٲ�
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
		switch (KEY_Scan())	// �Էµ� Switch ���� �з� 
		{
		case 0xFB00:		//SW2�� ������
			if (Cup != 0 & Sugar != 0 & Milk != 0 & Coffee != 0)		//��� 0�� �ƴ϶��
				WORKING();		//Ŀ�� ����� ����
			break;
		}
	}
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) �ʱ� ����	*/
void _GPIO_Init(void)		//���� ��
{
	// LED (GPIO G) ���� : Output mode
	RCC->AHB1ENR |= 0x0040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER &= ~0xFFFF;	// GPIOG 0~7 : Clear (0b00)						
	GPIOG->MODER |= 0x5555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER &= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR &= ~0xFFFF;	// GPIOG 0~7 : Clear (0b00) 
	GPIOG->OSPEEDR |= 0x5555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 

	// SW (GPIO H) ���� : Input mode 
	RCC->AHB1ENR |= 0x0080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER &= ~0x03FF0000;	// GPIOH 8~12 : Input mode (reset state)
	GPIOH->PUPDR &= ~0x03FF0000;	// GPIOH 8, 9, 10~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) ���� : Output mode
	RCC->AHB1ENR |= 0x0020;	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER &= ~0x000C0000;	// GPIOF 9 : Clear (0b00)						
	GPIOF->MODER |= 0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER &= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR &= ~0x000C0000;	// GPIOF 9 : Clear (0b00) 
	GPIOF->OSPEEDR |= 0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 

	// Joy_Stick (GPIO I) ���� : Input mode 
	RCC->AHB1ENR |= 0x0100;	// RCC_AHB1ENR GPIOI Enable	���̽�ƽ �ٿ��� ����ϱ� ����
	GPIOI->MODER &= ~0x000F3000;	// GPIOI 6, 8, 9 : Input mode (reset state)
	GPIOI->PUPDR &= ~0x000F3000;	// GPIOI 6, 8, 9: Floating input (No Pull-up, pull-down) (reset state)

}

/* GLCD �ʱ�ȭ�� ���� �Լ� */
void DisplayInitScreen(void)
{
	Fram_Init();                    // FRAM �ʱ�ȭ H/W �ʱ�ȭ
	Fram_Status_Config();   // FRAM �ʱ�ȭ S/W �ʱ�ȭ

	LCD_Clear(RGB_WHITE);		// ȭ�� Ŭ����
	LCD_SetFont(&Gulim8);		// ��Ʈ : ���� 8
	LCD_SetBackColor(RGB_YELLOW);	//���  �����
	LCD_SetTextColor(RGB_BLACK);	//���ڻ� ����
	LCD_DisplayText(0, 0, "LJW coffee");
	noc = Fram_Read(604);		//�ʱ� ���¿��� ���� ���¸� �о� ���� ���� Fram �о��
	NoC();		//NoC ǥ��
	LCD_SetBackColor(RGB_WHITE);		//�۾��� ����� ����̸� �����۾��� ��� �ۼ�
	LCD_DisplayText(0, 15, "IN");		//���ķδ� GLCD �⺻ ���õ�
	LCD_DisplayText(1, 12, "10");
	LCD_DisplayChar(1, 11, 0x5c);		//\�� ǥ���ϱ� ���� �ƽ�Ű �ڵ� ���
	LCD_DisplayText(2, 15, "TOT");
	LCD_DisplayText(3, 11, "\\50");		//�ƽ�Ű �ڵ尡 �ƴ� \�� �ι� ���� ����� ���
	//	LCD_DisplayChar(3, 11, 0x5c);
	Cup = 9;
	Sugar = 5;
	Milk = 5;
	Coffee = 9;
	STOCK();		//���ǥ��
	LCD_DisplayText(6, 15, "RF");
	LCD_DisplayText(7, 1, "cp sg mk cf");
	LCD_DisplayText(7, 15, "NoC");
	LCD_SetTextColor(RGB_WHITE);		//Ŀ�� ���¸� ��Ÿ���� ĭ���� ���� �ٸ�
	LCD_SetBackColor(RGB_BLUE);
	LCD_DisplayChar(2, 3, 'S');
	LCD_DisplayChar(3, 2, 'B');
	LCD_DisplayChar(3, 4, 'M');
	LCD_SetBackColor(RGB_RED);		//W�� ������
	LCD_DisplayChar(4, 3, 'W');
	LCD_SetTextColor(RGB_YELLOW);		//IN �� TOT�� ���� ��濡 ����� �۾�
	LCD_SetBackColor(RGB_BLACK);
	in1050 = Fram_Read(602);		//���� ���� ������ ���� �о����
	COIN();		//���� ǥ��
	tot = Fram_Read(603);		//���� ���� ������ ���� �о����
	TOTAL();		//���� ǥ��

	LCD_SetPenColor(RGB_BLACK);
	LCD_DrawRectangle(0, 0, 80, 12);		//�̸��κ� ���� �׸�

	LCD_SetPenColor(RGB_GREEN);		//�ʷϻ� �׸�ĭ��
	LCD_DrawRectangle(8 * 15 - 1, 13 * 1 - 1, 8 * 3 + 1, 13 * 1);	//in �κ� ���� �׸�

	LCD_DrawRectangle(8 * 3 - 1, 13 * 2 - 1, 9 * 1, 13 * 1);		//S
	LCD_DrawRectangle(8 * 2 - 1, 13 * 3 - 1, 9 * 1, 13 * 1);		//B
	LCD_DrawRectangle(8 * 4 - 1, 13 * 3 - 1, 9 * 1, 13 * 1);		//N
	LCD_DrawRectangle(8 * 3 - 1, 13 * 4 - 1, 9 * 1, 13 * 1);		//W
	LCD_DrawRectangle(8 * 15 - 1, 13 * 3 - 1, 8 * 3 + 1, 13 * 1);	//TOT �κ� ���� �׸�


	LCD_DrawRectangle(8 * 2 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//9
	LCD_DrawRectangle(8 * 5 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//5
	LCD_DrawRectangle(8 * 8 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//5
	LCD_DrawRectangle(8 * 11 - 1, 13 * 6 - 1, 9 * 1, 13 * 1);		//9

	LCD_DrawRectangle(8 * 15 - 1, 13 * 8 - 1, 8 * 3 + 1, 13 * 1);		//NoC
	LCD_DrawRectangle(8 * 17 - 1, 13 * 6 - 1, 8 * 1, 13 * 1);		//RF
	LCD_SetBrushColor(RGB_GREEN);						//RF �׸� ��
	LCD_DrawFillRect(8 * 17, 13 * 6, 7, 12);					//RF �׸� ĥ

	LCD_DrawRectangle(8 * 12 - 1, 13 * 2, 8 * 1, 10 * 1);		//10�� �׸�
	LCD_DrawRectangle(8 * 12 - 1, 13 * 4, 8 * 1, 10 * 1);		//50�� �׸�
	LCD_SetBrushColor(RGB_GRAY);		//ȸ��
	LCD_DrawFillRect(8 * 12, 13 * 2 + 1, 7, 9);		//10�� �׸� ȸ�� ĥ
	LCD_DrawFillRect(8 * 12, 13 * 4 + 1, 7, 9);		//50�� �׸� ȸ�� ĥ

}

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) �ʱ� ����  */
void _EXTI_Init(void)
{
	RCC->APB2ENR |= 0x00004000;	// Enable System Configuration Controller Clock
	// EXTI 8,9,11����
	SYSCFG->EXTICR[2] |= 0x7077; 	// EXTI8,9,11�� ���� �ҽ� �Է��� GPIOH�� ����
					// EXTI8 <- PH8, EXTI9 <- PH9 
					// EXTICR3(EXTICR[2])�� �̿� 
					// reset value: 0x0000
	// EXTI12~13 ����
	SYSCFG->EXTICR[3] |= 0x0077; 	// EXTI12, 13�� ���� �ҽ� �Է��� GPIOH�� ����
					// EXTI12 <- PH12
					// EXTICR4(EXTICR[3])�� �̿� 
					// reset value: 0x0000	

	EXTI->FTSR |= 0x3B00;		// EXTI����: Falling Trigger Enable 8~9, 11~15
	EXTI->IMR |= 0x3B00;		// EXTI IMR ����

	NVIC->ISER[0] |= (1 << 23);	// 0x00800000
					// Enable 'Global Interrupt EXTI8, 9'
					// Vector table Position ����
	NVIC->ISER[1] |= (1 << (40 - 32));// 0x00000100
					// Enable 'Global Interrupt EXTI11~13'
					// Vector table Position ����
}

/* EXTI5~9 ISR */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & 0x0100)			//EXTI8 Interrupt Pending(�߻�) ����?
	{
		EXTI->PR |= 0x0100;		// Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
		BEEP();
		LCD_SetBrushColor(RGB_YELLOW);
		LCD_DrawFillRect(8 * 12, 13 * 2 + 1, 7, 9);		//10�� �׸� ����� ĥ
		DelayMS(1000);
		LCD_SetBrushColor(RGB_GRAY);
		LCD_DrawFillRect(8 * 12, 13 * 2 + 1, 7, 9);		//10�� �׸� ȸ�� ĥ
		in10 += 10;		//IN +10
		COIN();		//������ ����ǥ��
	}
	else if (EXTI->PR & 0x0200)
	{
		EXTI->PR |= 0x0200;
		BEEP();
		LCD_SetBrushColor(RGB_YELLOW);
		LCD_DrawFillRect(8 * 12, 13 * 4 + 1, 7, 9);		//50�� �׸� ����� ĥ
		DelayMS(1000);
		LCD_SetBrushColor(RGB_GRAY);
		LCD_DrawFillRect(8 * 12, 13 * 4 + 1, 7, 9);		//50�� �׸� ȸ�� ĥ
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
		RF();		//���� �Լ� ����
	}
	else if (EXTI->PR & 0x0800)	//sw3
	{
		EXTI->PR |= 0x0800;
		CHANGE();		//�ܵ���ȯ �Լ� ����
	}
	else if (EXTI->PR & 0x2000)	//sw5
	{
		EXTI->PR |= 0x2000;
		CLEAR();		//����, �Ǹż� ���� �Լ� ����
	}

}

void COIN(void)		//�Էµ� ���� ǥ���ϴ� �Լ�
{
	in1050 = in1050 + in10 + in50 - spent;		//in1050�� �� �� in10�� 10�� in50�� 50�� spent�� Ŀ�Ǹ� �����ϰ� ���鶧 ��� �� ��
	in10 = 0, in50 = 0, spent = 0;		//����
	if (in1050 > 200) in1050 = 200;		//200���� ������ 200���� ǥ���ϱ� ���� ���ǹ�
	Fram_Write(602, in1050);		//fram602�� �� �� �Է�
	IN = Fram_Read(602);		//fram602 �о IN�� �Է�
	IN100 = IN / 100;		//IN 100���ڸ�
	IN10 = IN % 100 / 10;	//IN 10���ڸ�
	IN1 = IN % 10;			//IN 1���ڸ� (��ǻ� �ʿ� ����)
	LCD_SetTextColor(RGB_YELLOW);	//�� ����
	LCD_SetBackColor(RGB_BLACK);
	LCD_DisplayChar(1, 15, IN100 + 0x30);	//IN 100���ڸ�
	LCD_DisplayChar(1, 16, IN10 + 0x30);	//IN 10���ڸ�
	LCD_DisplayChar(1, 17, IN1 + 0x30);		//IN 1���ڸ� (��ǻ� �ʿ� ����)
}

void TOTAL(void)		//���� ǥ���ϴ� �Լ�
{
	if (tot >= 99) tot = 99;		//������ 990�� ������ 990�� ǥ���ϱ� ���� �Լ�
	Fram_Write(603, tot);		//fram603�� ���� �Է�
	MONEY = Fram_Read(603);		//fram603 �о� MONEY�� �Է�
	MONEY10 = MONEY % 100 / 10;		//TOT 100�� �ڸ�
	MONEY1 = MONEY % 10;			//TOT 10�� �ڸ�
	LCD_SetTextColor(RGB_YELLOW);	//�� ����
	LCD_SetBackColor(RGB_BLACK);
	LCD_DisplayChar(3, 15, MONEY10 + 0x30);	//TOT ǥ��
	LCD_DisplayChar(3, 16, MONEY1 + 0x30);
	LCD_DisplayChar(3, 17, 0x30);
}

void COFFEERESET(void)		//Ŀ�� ���¸� ���� �ϴ� �Լ�
{
	COFFEESTATE = 'Q';		//Ŀ�Ǹ� �� ����� �ٽ� ������ ���� �ʰ� ��ŷ�� �������� �������� �ʰ� �ϱ� ���� ������ ����
	LCD_SetBackColor(RGB_BLUE);	//�� ����
	LCD_SetTextColor(RGB_WHITE);
	LCD_DisplayChar(2, 3, 'S');	//�ʱ���·� �ǵ���
	LCD_DisplayChar(3, 4, 'M');
	LCD_DisplayChar(3, 2, 'B');
}

void NoC(void)		//�� �Ǹż��� ǥ���ϴ� �Լ�
{
	LCD_SetBackColor(RGB_YELLOW);		//�� ����
	LCD_SetTextColor(RGB_BLACK);
	if (noc > 50) noc = 50;		//�� �Ǹż��� 50�� ������ 50�� ǥ��
	Fram_Write(604, noc);		//�� �Ǹż� fram�� �Է�
	NUMC = Fram_Read(604);		//fram�о� NUMC�� �Է�
	NUMC100 = NUMC / 100;		//NoC 100�� �ڸ�
	NUMC10 = NUMC % 100 / 10;	//NoC 10�� �ڸ�
	NUMC1 = NUMC % 10;			//Noc 1�� �ڸ�
	LCD_DisplayChar(8, 15, NUMC100 + 0x30);	//NoC ǥ��
	LCD_DisplayChar(8, 16, NUMC10 + 0x30);
	LCD_DisplayChar(8, 17, NUMC1 + 0x30);
}

void STOCK(void)		//��� ��Ÿ���� �Լ�
{
	LCD_SetBackColor(RGB_WHITE);	//�� ����
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(6, 2, Cup + 0x30);	//��� ǥ��
	LCD_DisplayChar(6, 5, Sugar + 0x30);
	LCD_DisplayChar(6, 8, Milk + 0x30);
	LCD_DisplayChar(6, 11, Coffee + 0x30);
	if (Cup == 0 | Sugar == 0 | Milk == 0 | Coffee == 0)		//���� ����� �ϳ��� 0�� �ȴٸ�
	{
		EXTI->IMR &= ~0x0300;		//SW 1, 2 disable
		//RCC->AHB1ENR &= ~0x0100;		//���̽�ƽ �𽺿��̺� �Ϸ� ������ �̷��� �ϸ� TOT�� NoC�� 0���� ǥ�õǴ� ���׹߻� ������ ��ã�� �ᱹ �ٸ� ���(main�� ���ǹ� �ִ�)���� �ذ�
		LCD_SetBrushColor(RGB_RED);						//RF �׸�
		LCD_DrawFillRect(8 * 17, 13 * 6, 7, 12);		//RF �׸� ĥ
	}
}

void RF(void)		//��� ���� �Լ�
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
	STOCK();		//������ ���� GLCD�� ǥ��
	LCD_SetBrushColor(RGB_GREEN);						//RF �׸�
	LCD_DrawFillRect(8 * 17, 13 * 6, 7, 12);					//RF �׸� ĥ
	EXTI->IMR |= 0x0B00; 		//EXTI �ٽ� ON
}

void WORKING(void)		//Ŀ�� ����� �Լ�
{
	COIN();
	if (IN >= 20 & COFFEESTATE == 'S')		//���� 20���̻� �ְ� Ŀ�Ǹ� S�� ����� ���
	{
		BEEP();
		Cup -= 1;		//�ش��ϴ� ������ ����
		Sugar -= 1;
		Coffee -= 1;
		spent += 20;	//����� �� ����
		tot += 2;		//���� ����
		noc += 1;		//�� �Ǹż� ����
		MAKING();		//Ŀ�� ����� ���� �Լ� ����
		COFFEERESET();	//Ŀ�Ǽ��� ����
	}
	else if (IN >= 10 & COFFEESTATE == 'B')		//Ŀ�� B ���ý� ���� ����
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
	COIN();			//Ŀ�Ǹ� ����� �� �� �ٲ� ������ ǥ��
	STOCK();
	TOTAL();
	NoC();
}

void MAKING(void)		//Ŀ�� ����� ���� �Լ�
{
	EXTI->IMR &= ~0x3B00;			//Ŀ�� ������ ��� ���� ����
	GPIOG->ODR |= 0x00FF;			//���ķδ� ���۵�
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
	EXTI->IMR |= 0x3B00; 		//EXTI �ٽ� ON
}

void CHANGE(void)		//�ܵ� ��ȯ�ϴ� �Լ�
{
	in1050 = 0;		//�� ���� ������ 0���� �ʱ�ȭ
	in10 = 0;
	in50 = 0;
	spent = 0;
	COIN();			//���µ� �ܵ� ǥ��
}

void CLEAR(void)		//TOT�� NoC ����
{
	tot = 0;
	TOTAL();
	noc = 0;
	NoC();
}

/* Switch�� �ԷµǾ������� ���ο� � switch�� �ԷµǾ������� ������ return�ϴ� �Լ�  */
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