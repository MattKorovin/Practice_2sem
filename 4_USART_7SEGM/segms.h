#ifndef SEGMS_H
#define SEGMS_H

#include "stm32f10x.h"



// Маски сегментов
#define SEGM_A ((uint8_t)0x01)
#define SEGM_B ((uint8_t)0x02)
#define SEGM_C ((uint8_t)0x04)
#define SEGM_D ((uint8_t)0x08)
#define SEGM_E ((uint8_t)0x10)
#define SEGM_F ((uint8_t)0x20)
#define SEGM_G ((uint8_t)0x40)
#define SEGM_T ((uint8_t)0x80) // Точка - DP



// Контроль сегментов
#define CTRL_SEGM_A GPIOB -> BSRR |= GPIO_BSRR_BS12
#define CTRL_SEGM_B GPIOB -> BSRR |= GPIO_BSRR_BS13
#define CTRL_SEGM_C GPIOB -> BSRR |= GPIO_BSRR_BS14
#define CTRL_SEGM_D GPIOB -> BSRR |= GPIO_BSRR_BS15
#define CTRL_SEGM_E GPIOA -> BSRR |= GPIO_BSRR_BS8
#define CTRL_SEGM_F GPIOA -> BSRR |= GPIO_BSRR_BS9
#define CTRL_SEGM_G GPIOA -> BSRR |= GPIO_BSRR_BS10
#define CTRL_SEGM_T GPIOA -> BSRR |= GPIO_BSRR_BS11 // Точка - DP

/*
Пример записи:
Операция << смещает бит на 16 мест влево

CTRL_SEGM_A << (16 * ((NUM[0] & SEGM_A) == 0))
Если   NUM[0] & SEGM_A == 0   то   выход - минус питания (BS12 << 16 это BR12)
Если   NUM[0] & SEGM_A != 0   то   выход - плюс питания

Если хотим, чтобы было наоборот, то делаем вот так:
CTRL_SEGM_A << (16 * ((NUM[0] & SEGM_A) || 0))
*/



// Контроль ячеек
typedef void (*ToggleFunction)(void);
extern const ToggleFunction toggleNum[4];

#endif
