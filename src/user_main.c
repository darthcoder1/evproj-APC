#include "hw_util.h"
#include "main.h"
#include "pin_map.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include "usb_handler.h"

#include <assert.h>
#include <stdio.h>

// initialize input channels
void input_channels_initialize()
{
    // Input channels: 0-7, handled by MAX4558 at U2
    HWU_GPIO_Initialize(Pin_Input0_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Input0_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Input0_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Input0_Data, GPIO_MODE_INPUT, GPIO_NOPULL);

    // Input channels: 8-15, handled by MAX4558 at U3
    HWU_GPIO_Initialize(Pin_Input1_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Input1_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Input1_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Input1_Data, GPIO_MODE_INPUT, GPIO_NOPULL);
}

void output_channels_initialize()
{
    // initialize output channel GPIOs
    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        HWU_GPIO_Initialize(Pin_Output_Ch(i), GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    }

    HWU_GPIO_Initialize(Pin_Outout_Diag_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Outout_Diag_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Outout_Diag_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Outout_Diag_S3, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    HWU_GPIO_Initialize(Pin_Output_Diag_Data, GPIO_MODE_INPUT, GPIO_NOPULL);
}

uint16_t read_input_channels()
{
    uint16_t ret = 0;
    for (int i = 0; i < 8; ++i)
    {
        InputChannelSelector_t sel = g_inputChannelSelector[i];

        HWU_GPIO_WritePin(Pin_Input0_S0, sel.s0);
        HWU_GPIO_WritePin(Pin_Input0_S1, sel.s1);
        HWU_GPIO_WritePin(Pin_Input0_S2, sel.s2);
        ret |= HWU_GPIO_ReadPin(Pin_Input0_Data) << i;
    }

    for (int i = 0; i < 8; ++i)
    {
        InputChannelSelector_t sel = g_inputChannelSelector[i];
        HWU_GPIO_WritePin(Pin_Input1_S0, sel.s0);
        HWU_GPIO_WritePin(Pin_Input1_S1, sel.s1);
        HWU_GPIO_WritePin(Pin_Input1_S2, sel.s2);
        ret |= HWU_GPIO_ReadPin(Pin_Input1_Data) << (8 + i);
    }

    return ret;
}

uint16_t read_output_diag_status()
{
    uint16_t ret = 0;
    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        OutputDiagChannelSelector_t sel = g_outputDiagChannelSelector[i];

        HWU_GPIO_WritePin(Pin_Outout_Diag_S0, sel.s0);
        HWU_GPIO_WritePin(Pin_Outout_Diag_S1, sel.s1);
        HWU_GPIO_WritePin(Pin_Outout_Diag_S2, sel.s2);
        HWU_GPIO_WritePin(Pin_Outout_Diag_S3, sel.s3);

        ret |= HWU_GPIO_ReadPin(Pin_Output_Diag_Data) << i;
    }

    return ret;
}

typedef struct
{
    // directly mapped from input
    uint32_t LeftTurnActive : 1;
    uint32_t RightTurnActive : 1;
    uint32_t HazardLightActive : 1;
} DriverInputState_t;

typedef struct
{
    uint32_t LeftTurnActive : 1;
    uint32_t RightTurnActive : 1;
    uint32_t HazardLightActive : 1;
} SystemRuntimeState_t;

// globals
TIM_HandleTypeDef g_LED_Timer;                // Timer for the 500s timer
TIM_HandleTypeDef g_TurnIndicatorPulse_Timer; // Turn inidcator pulse timer

SystemRuntimeState_t g_SystemState;

typedef void (*TimerCallbackFunc)(TIM_HandleTypeDef*);

void DefaultTimerHandler(TIM_HandleTypeDef* timerHndl, TimerCallbackFunc callback)
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

void BlinkLedLight(TIM_HandleTypeDef* timerHndl)
{
    HWU_GPIO_TogglePin(Pin_OnBoard_LED);
}

void PrintPing(TIM_HandleTypeDef* timerHndl)
{
    printf("PING\r\n");
}

void PulseTurnLight(TIM_HandleTypeDef* timerHndl)
{
    if (g_SystemState.LeftTurnActive)
    {
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Left_Back);
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Left_Front);
    }

    if (g_SystemState.RightTurnActive)
    {
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Right_Back);
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Right_Front);
    }

    if (g_SystemState.HazardLightActive)
    {
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Left_Back);
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Left_Front);
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Right_Back);
        HWU_GPIO_TogglePin(PinMap_TurnSignal_Right_Front);
    }
}

void TIM2_IRQHandler()
{
    DefaultTimerHandler(&g_LED_Timer, BlinkLedLight);
}

void TIM3_IRQHandler()
{
    DefaultTimerHandler(&g_TurnIndicatorPulse_Timer, PulseTurnLight);
}

// Hooks called from kernel code
void user_initialize()
{
    // enable usb
    usb_handler_initialize();
    usb_handler_start();

    // enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // initialize input channel GPIOs
    input_channels_initialize();
    output_channels_initialize();

    // GPIO Ports Clock Enable
    HWU_GPIO_Initialize(Pin_OnBoard_LED, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP);

    // Configure GPIO pin Output Level
    HWU_GPIO_WritePin(Pin_OnBoard_LED, 1);
}

void DebugPrintInputState(uint16_t inputState)
{
    const int numInputChannels = 16;

    int numActive = 0;
    for (int i = 0; i < numInputChannels; ++i)
    {
        if (IsInputChannelActive(inputState, i))
        {
            printf("Channel %d,", i);
            ++numActive;
        }
    }

    if (numActive > 0)
    {
        printf("\r\n");
    }
}

DriverInputState_t TransformLowLevelInput(uint16_t lowLevelInput)
{
    DriverInputState_t driverInputState = {
        .LeftTurnActive = IsInputChannelActive(lowLevelInput, 0),
        .RightTurnActive = IsInputChannelActive(lowLevelInput, 1),
        .HazardLightActive = IsInputChannelActive(lowLevelInput, 2),
    };

    return driverInputState;
}

void switch_direct_input_to_output(uint16_t input)
{
    // directly map to output for testing

    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        HWU_GPIO_WritePin(Pin_Output_Ch(i), (input >> i) & 0x1);
    }

    // HACK: map input channel 12 to output channel 6. This is only needed for
    // 2nd proto v0.1 board due to accidentialy cut trace
    HWU_GPIO_WritePin(Pin_Output_Ch_6, (input >> 12) & 0x1);
}

void switch_on_all_outputs()
{
    // directly map to output for testing

    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        HWU_GPIO_WritePin(Pin_Output_Ch(i), 1);
    }
}

void ProcessDriverInput(DriverInputState_t driverInput, SystemRuntimeState_t* systemState)
{
    assert(systemState);
    assert(!driverInput.LeftTurnActive || !driverInput.RightTurnActive);

    if (driverInput.LeftTurnActive != systemState->LeftTurnActive)
    {
        systemState->LeftTurnActive = driverInput.LeftTurnActive;

        if (systemState->LeftTurnActive)
        {
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Front, 1);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Front, 1);

            HWU_Timer_Reset(&g_TurnIndicatorPulse_Timer);
        }
        else
        {
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Back, 0);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Front, 0);

            HWU_Timer_Disable(&g_TurnIndicatorPulse_Timer);
        }
    }

    if (driverInput.RightTurnActive != systemState->RightTurnActive)
    {
        systemState->RightTurnActive = driverInput.RightTurnActive;

        if (systemState->RightTurnActive)
        {
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Back, 1);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Front, 1);

            HWU_Timer_Reset(&g_TurnIndicatorPulse_Timer);
        }
        else
        {
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Back, 0);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Front, 0);

            HWU_Timer_Disable(&g_TurnIndicatorPulse_Timer);
        }
    }

    if (driverInput.HazardLightActive != systemState->HazardLightActive)
    {
        systemState->HazardLightActive = driverInput.HazardLightActive;

        if (systemState->HazardLightActive)
        {
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Back, 1);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Front, 1);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Back, 1);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Front, 1);

            HWU_Timer_Reset(&g_TurnIndicatorPulse_Timer);
        }
        else
        {
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Back, 0);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Left_Front, 0);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Back, 0);
            HWU_GPIO_WritePin(PinMap_TurnSignal_Right_Front, 0);

            HWU_Timer_Disable(&g_TurnIndicatorPulse_Timer);
        }
    }
}

typedef enum
{
    LedState_On = 0,
    LedState_Off = 1
} LedState;

void switch_led_to(LedState* ledState, LedState targetLedState)
{
    assert(ledState != NULL);

    if (*ledState != targetLedState)
    {
        *ledState = targetLedState;
        HWU_GPIO_WritePin(Pin_OnBoard_LED, (uint32_t)targetLedState);
    }
}

// Entry point
int user_main(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    HWU_Timer_Initialize(&g_LED_Timer, TIM2, 500, TIM2_IRQn);
    __HAL_RCC_TIM3_CLK_ENABLE();
    HWU_Timer_Initialize(&g_TurnIndicatorPulse_Timer, TIM3, 1000, TIM3_IRQn);

    printf("Initialized!\r\n");

    HWU_GPIO_WritePin(Pin_OnBoard_LED, 0);

    while (1)
    {
        uint16_t inputState = read_input_channels();
        DriverInputState_t driverInputState = TransformLowLevelInput(inputState);

        ProcessDriverInput(driverInputState, &g_SystemState);

        uint16_t output_diag_state = read_output_diag_status();
        if (output_diag_state != 0)
        {
            for (int i = 0; i < Pin_Output_NumOutputs; ++i)
            {
                if (((output_diag_state >> i) & 1) == 1)
                {
                    printf("Error on Output Ch[%d]\r\n", i);
                }
            }
        }
    }

    usb_handler_shutdown();
}