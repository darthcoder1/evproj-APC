
#include "hw_util.h"
#include <assert.h>

////////////////////////////////////////////////////////////
// Timer related utils

// NO_IRQ is used for the 'irqToEnable' argument of 'initialize_timer'
const IRQn_Type NO_IRQ = (IRQn_Type)0xffff;

// The clock for the passed 'systemTimer' (TIM1, TIM2, TIMn) must be enabled with
// prior to calling 'initialize_timer' with __HAL_RCC_TIMn_CLK_ENABLE()
void HWU_Timer_Initialize(TIM_HandleTypeDef* timerHndl, TIM_TypeDef* systemTimer, int periodInMs, IRQn_Type irqToEnable)
{
    assert(timerHndl);
    assert(systemTimer);

    timerHndl->Instance = systemTimer;

    timerHndl->Init.Prescaler = 40000;
    timerHndl->Init.CounterMode = TIM_COUNTERMODE_UP;
    timerHndl->Init.Period = periodInMs;
    timerHndl->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timerHndl->Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(timerHndl);

    if (irqToEnable == NO_IRQ)
    {
        HAL_TIM_Base_Start(timerHndl);
    }
    else
    {
        HAL_TIM_Base_Start_IT(timerHndl);
        // enable interrupts
        HAL_NVIC_SetPriority(irqToEnable, 0, 1);
        HAL_NVIC_EnableIRQ(irqToEnable);
    }
}

int HWU_Timer_Poll(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);
    return __HAL_TIM_GET_COUNTER(timerHndl);
}

void HWU_Timer_Reset(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);

    HAL_TIM_Base_Stop_IT(timerHndl);
    __HAL_TIM_CLEAR_IT(timerHndl, TIM_IT_UPDATE);

    // reset the counter
    timerHndl->Instance->CNT = 0;

    HAL_TIM_Base_Start_IT(timerHndl);
}

void HWU_Timer_Disable(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);

    HAL_TIM_Base_Stop_IT(timerHndl);
    __HAL_TIM_CLEAR_IT(timerHndl, TIM_IT_UPDATE);

    // reset the counter
    timerHndl->Instance->CNT = 0;
}

void HWU_Timer_DefaultHandler(TIM_HandleTypeDef* timerHndl, TimerCallbackFunc callback)
{
    if (__HAL_TIM_GET_FLAG(timerHndl, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(timerHndl, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_FLAG(timerHndl, TIM_FLAG_UPDATE);

            HAL_TIM_IRQHandler(timerHndl);

            callback(timerHndl);
        }
    }
}

////////////////////////////////////////////////////////////
//GPIO utils

// Initialize the pin with the specified settings
void HWU_GPIO_Initialize(PinId pinId, uint32_t gpioMode, uint32_t pullMode)
{
    GPIO_InitTypeDef gpio_init = {
        .Pin = g_PinMapping[pinId].pin, .Mode = gpioMode, .Pull = pullMode, .Speed = GPIO_SPEED_FREQ_LOW
    };

    HAL_GPIO_Init(g_PinMapping[pinId].gpio, &gpio_init);
}

// sets pin according to 'value'
void HWU_GPIO_WritePin(PinId pinId, uint32_t value)
{
    HAL_GPIO_WritePin((GPIO_TypeDef*)g_PinMapping[pinId].gpio,
                      g_PinMapping[pinId].pin, value != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HWU_GPIO_TogglePin(PinId pinId)
{
    HAL_GPIO_TogglePin((GPIO_TypeDef*)g_PinMapping[pinId].gpio,
                       g_PinMapping[pinId].pin);
}

// returns state of the pin
uint32_t HWU_GPIO_ReadPin(PinId pinId)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef*)g_PinMapping[pinId].gpio,
                            g_PinMapping[pinId].pin) == GPIO_PIN_SET
               ? 1
               : 0;
}
