
#pragma once

#include "main.h"
#include "stm32f1xx_hal_tim.h"

////////////////////////////////////////////////////////////
// Timer related utils

// Initializes the timer to pulse the specified interrupt
// timerHndl        pointer to timer handle storage
// systemTimer      the system timer to setup (TIM2, TIM3, TIMn)
// periodInMs       period of the pulse
// irqToEnable      either HWU_IRQ_NONE or one of the appropriate IRQn_Type
// Notes            The clock for the passed 'systemTimer' (TIM1, TIM2, TIMn) must
//                  be enabled with __HAL_RCC_TIMn_CLK_ENABLE() prior to calling 'initialize_timer'.
//                  Don't forget to implement the according IRQ callback.
void HWU_InitializeTimer(TIM_HandleTypeDef* timerHndl, TIM_TypeDef* systemTimer, int periodInMs, IRQn_Type irqToEnable);

// Returns the current count of the timerHndl
int HWU_PollTimer(TIM_HandleTypeDef* timerHndl);

// Resets the timer and the interrupt and starts over again
void HWU_ResetTimer(TIM_HandleTypeDef* timerHndl);

// Disables the timer, so no further interrupts are triggered until
// HWU_ResetTimer() will be called on this timer.
void HWU_DisableTimer(TIM_HandleTypeDef* timerHndl);