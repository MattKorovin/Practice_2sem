#include "stm32f10x.h"

#define LED1_ON() GPIOB -> BSRR |= GPIO_BSRR_BS10 // Периметр
#define LED1_OFF() GPIOB -> BSRR |= GPIO_BSRR_BR10

#define LED2_ON() GPIOB -> BSRR |= GPIO_BSRR_BS11 // Центр
#define LED2_OFF() GPIOB -> BSRR |= GPIO_BSRR_BR11


/*
Необходимо поставить перемычки: PA8, PA9_2, PA10, PA11

Кнопки USER1-USER4 контролируют нижнюю подсветку платы:
  USER1 (PA8) - включить подсветку по центру
	USER2 (PA9) - выключить подсветку по центру
	USER3 (PA10) - включить подсветку по периметру
	USER4 (PA11) - выключить подсветку по периметру
Согласно электрической схеме, при подключении перемычек
пины уже будут подтянуты к + питания, а при нажатии кнопки
цепь будет замкнута на GND
Тогда настраиваем PA8-PA11 на Input Floating (Pull-Up делать не за чем)
Прерывания делаем на факт нажатия кнопок (1 -> 0)

Подсветка периметра контролируется пином PB10
Он подтянут к GND. При подаче + на PB10 открывается транзистор
и замыкает цепь светодиодов, подключенных к + питания, на GND
-> подсветка включается. Поэтому PB10 настраиваем как
Output Push-Pull
При подаче + на PB10 подсветка включается, 0 - выключается
Аналогично для подсветки центра пином PB11

Прерывания: если на соответствующем порту (PA8-PA11) обнаруживается перепад
уровня сигнала, то контроллер прерываний передаёт управление соответствующей
подпрограмме обработки вектора прерываний EXTI9_5_IRQHandler(void) или
EXTI15_10_IRQHandler(void), где она определяет источник прерываний (порт)
и в соответствии с портом назначает обработку события
*/



void setPinConf(void) {
	RCC -> APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC -> APB2ENR |= RCC_APB2ENR_IOPAEN;
	
	//PA8 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE8_0;     // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE8_1;     // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF8_0;       // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF8_1;      // CNF1 = 0 (CNF = 01 - Floating)
	
	//PA9 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE9_0;     // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE9_1;     // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF9_0;       // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF9_1;      // CNF1 = 0 (CNF = 01 - Floating)
	
	//PA10 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE10_0;    // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE10_1;    // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF10_0;      // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF10_1;     // CNF1 = 0 (CNF = 01 - Floating)
	
	//PA11 = Input Floating
	GPIOA -> CRH &= ~GPIO_CRH_MODE11_0;    // MODE0 = 0 (MODE = 00 - Input)
	GPIOA -> CRH &= ~GPIO_CRH_MODE11_1;    // MODE1 = 0 (MODE = 00 - Input)
	GPIOA -> CRH |= GPIO_CRH_CNF11_0;      // CNF0 = 1 (CNF = 01 - Floating)
	GPIOA -> CRH &= ~GPIO_CRH_CNF11_1;     // CNF1 = 0 (CNF = 01 - Floating)
	
	// PB10 = Output Push-Pull (LED периметр)
	GPIOB -> CRH &= ~GPIO_CRH_MODE10_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE10_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF10_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF10_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
	
	// PB11 = Output Push-Pull (LED центр)
	GPIOB -> CRH &= ~GPIO_CRH_MODE11_0;    // MODE0 = 0 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH |= GPIO_CRH_MODE11_1;     // MODE1 = 1 (MODE = 10 - Output 2MHz)
	GPIOB -> CRH &= ~GPIO_CRH_CNF11_0;     // CNF0 = 0 (CNF = 00 - Output Push-Pull)
	GPIOB -> CRH &= ~GPIO_CRH_CNF11_1;     // CNF1 = 0 (CNF = 00 - Output Push-Pull)
}



void setInterrupt(void) {
	RCC -> APB2ENR |= RCC_APB2ENR_AFIOEN;        // Interrupt ON
	
	// Init PA8
	EXTI -> PR |= EXTI_PR_PR8;                   // Снять статус "прервано"
	EXTI -> IMR |= EXTI_IMR_MR8;                 // Снять маску (разрешить прерывания)
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI8;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI8_PA;  // port 8 = A
	
	// Init PA9
	EXTI -> PR |= EXTI_PR_PR9;                   // Снять статус "прервано"
	EXTI -> IMR |= EXTI_IMR_MR9;                 // Снять маску (разрешить прерывания)
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI9;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI9_PA;  // port 9 = A
	
	// Init PA10
	EXTI -> PR |= EXTI_PR_PR10;                   // Снять статус "прервано"
	EXTI -> IMR |= EXTI_IMR_MR10;                 // Снять маску (разрешить прерывания)
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI10;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI10_PA;  // port 10 = A
	
	// Init PA11
	EXTI -> PR |= EXTI_PR_PR11;                   // Снять статус "прервано"
	EXTI -> IMR |= EXTI_IMR_MR11;                 // Снять маску (разрешить прерывания)
	AFIO -> EXTICR[2] &= ~AFIO_EXTICR3_EXTI11;
	AFIO -> EXTICR[2] |= AFIO_EXTICR3_EXTI11_PA;  // port 11 = A
	
	// Setup PA8-PA9
	EXTI -> FTSR |= EXTI_FTSR_TR8;                // Interrupt 1 -> 0
	EXTI -> FTSR |= EXTI_FTSR_TR9;                // Interrupt 1 -> 0
	NVIC_EnableIRQ(EXTI9_5_IRQn);                 // Enable Interrupt 1
	NVIC_SetPriority(EXTI9_5_IRQn, 0);            // Set Priority EXTI_9_5 -> 0
	
	// Setup PA10-PA11
	EXTI -> FTSR |= EXTI_FTSR_TR10;               // Interrupt 1 -> 0
	EXTI -> FTSR |= EXTI_FTSR_TR11;               // Interrupt 1 -> 0
	NVIC_EnableIRQ(EXTI15_10_IRQn);               // Enable Interrupt 1
	NVIC_SetPriority(EXTI15_10_IRQn, 1);          // Set Priority EXTI_15_10 -> 1
}



// Начальные настройки
void setMode(void) {
	LED1_OFF(); // Периметр выключен
	LED2_ON(); // Центр включен
	EXTI -> IMR &= ~EXTI_IMR_MR8;  // Запретить прерывание на ВКЛючение центра
	EXTI -> IMR |= EXTI_IMR_MR9;   // Разрешить прерывание на ВЫКЛючение центра
	EXTI -> IMR |= EXTI_IMR_MR10;  // Разрешить прерывание на ВКЛючение периметра
	EXTI -> IMR &= ~EXTI_IMR_MR11; // Запретить прерывание на ВЫКЛючение периметра
}

// Функция обработки вектора прерываний (9_5) (Центр)
void EXTI9_5_IRQHandler(void) {
	if ((EXTI -> PR & EXTI_PR_PR8) & (EXTI -> IMR & EXTI_IMR_MR8)) {
		EXTI -> PR |= EXTI_PR_PR8;
		LED2_ON();
		EXTI -> IMR &= ~EXTI_IMR_MR8;
		EXTI -> IMR |= EXTI_IMR_MR9;
	}
	else if ((EXTI -> PR & EXTI_PR_PR9) & (EXTI -> IMR & EXTI_IMR_MR9)) {
		EXTI -> PR |= EXTI_PR_PR9;
		LED2_OFF();
		EXTI -> IMR &= ~EXTI_IMR_MR9;
		EXTI -> IMR |= EXTI_IMR_MR8;
	}
}

// Функция обработки вектора прерываний (15_10) (Перметр)
void EXTI15_10_IRQHandler(void) {
	if ((EXTI -> PR & EXTI_PR_PR10) & (EXTI -> IMR & EXTI_IMR_MR10)) {
		EXTI -> PR |= EXTI_PR_PR10;
		LED1_ON();
		EXTI -> IMR &= ~EXTI_IMR_MR10;
		EXTI -> IMR |= EXTI_IMR_MR11;
	}
	else if ((EXTI -> PR & EXTI_PR_PR11) & (EXTI -> IMR & EXTI_IMR_MR11)) {
		EXTI -> PR |= EXTI_PR_PR11;
		LED1_OFF();
		EXTI -> IMR &= ~EXTI_IMR_MR11;
		EXTI -> IMR |= EXTI_IMR_MR10;
	}
}


                     
int main(void) {
	setPinConf();                                                  // Настройка конфигурации портов ввода-вывода
	setInterrupt();                                                // Настройка обработчиков прерываний (контроллера NVIC)
	setMode();                                                     // Начальные настройки
	while(1) {                                                     // Основной цикл - тут у нас НИЧЕГО НЕ ДЕЛАЕТСЯ, поскольку управление светодиодом
		                                                             // происходит  в обработчиках прерываний (асинхронно и независимо от основного цикла  
	}
}

