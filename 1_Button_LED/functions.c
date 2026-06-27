#include "functions.h"
#include "stm32f10x.h"

#define LED_ON() GPIOB -> BSRR |= GPIO_BSRR_BS2
#define LED_OFF() GPIOB -> BSRR |= GPIO_BSRR_BR2

uint8_t buttonState = 0;

void cycle(void) {
	buttonState = READ_BIT(GPIOA -> IDR, GPIO_IDR_IDR0);
	if(buttonState == 1) {
		LED_ON();
	}
	else {
		LED_OFF();
	}
}