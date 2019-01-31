
#pragma once

#include "main.h"
#include "pin_map.h"
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
void HWU_Timer_Initialize(TIM_HandleTypeDef* timerHndl, TIM_TypeDef* systemTimer, int periodInMs, IRQn_Type irqToEnable);

// Returns the current count of the timerHndl
int HWU_Timer_Poll(TIM_HandleTypeDef* timerHndl);

// Resets the timer and the interrupt and starts over again
void HWU_Timer_Reset(TIM_HandleTypeDef* timerHndl);

// Disables the timer, so no further interrupts are triggered until
// HWU_ResetTimer() will be called on this timer.
void HWU_Timer_Disable(TIM_HandleTypeDef* timerHndl);

// Timer callback function. Use this function type to pass a handler
// callback into DefaultTimerHandler
typedef void (*TimerCallbackFunc)(TIM_HandleTypeDef*);

// Default timer handler. This will check the interrupt flags and reset them
// correctly before and after the callback specified was called.
void HWU_Timer_DefaultHandler(TIM_HandleTypeDef* timerHndl, TimerCallbackFunc callback);

////////////////////////////////////////////////////////////
//GPIO utils

// Initialize the pin with the specified settings
void HWU_GPIO_Initialize(PinId pinId, uint32_t gpioMode, uint32_t pullMode);

// sets pin according to 'value'
void HWU_GPIO_WritePin(PinId pinId, uint32_t value);

void HWU_GPIO_TogglePin(PinId pinId);

// returns state of the pin
uint32_t HWU_GPIO_ReadPin(PinId pinId);