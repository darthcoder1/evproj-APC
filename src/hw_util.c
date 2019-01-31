
#include "hw_util.h"
#include <assert.h>

////////////////////////////////////////////////////////////
// Timer related utils

// NO_IRQ is used for the 'irqToEnable' argument of 'initialize_timer'
const IRQn_Type NO_IRQ = (IRQn_Type)0xffff;

// The clock for the passed 'systemTimer' (TIM1, TIM2, TIMn) must be enabled with
// prior to calling 'initialize_timer' with __HAL_RCC_TIMn_CLK_ENABLE()
void HWU_InitializeTimer(TIM_HandleTypeDef* timerHndl, TIM_TypeDef* systemTimer, int periodInMs, IRQn_Type irqToEnable)
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

int HWU_PollTimer(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);
    return __HAL_TIM_GET_COUNTER(timerHndl);
}

void HWU_ResetTimer(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);

    HAL_TIM_Base_Stop_IT(timerHndl);
    __HAL_TIM_CLEAR_IT(timerHndl, TIM_IT_UPDATE);

    // reset the counter
    timerHndl->Instance->CNT = 0;

    HAL_TIM_Base_Start_IT(timerHndl);
}

void HWU_DisableTimer(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);

    HAL_TIM_Base_Stop_IT(timerHndl);
    __HAL_TIM_CLEAR_IT(timerHndl, TIM_IT_UPDATE);

    // reset the counter
    timerHndl->Instance->CNT = 0;
}