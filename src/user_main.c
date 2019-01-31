#include "main.h"
#include "pin_map.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include "usb_handler.h"

#include <assert.h>
#include <stdio.h>

// Initialize the pin with the specified settings
void initialize_gpio_pin(PinId pinId, uint32_t gpioMode, uint32_t pullMode)
{
    GPIO_InitTypeDef gpio_init = {
        .Pin = g_PinMapping[pinId].pin, .Mode = gpioMode, .Pull = pullMode, .Speed = GPIO_SPEED_FREQ_LOW
    };

    HAL_GPIO_Init(g_PinMapping[pinId].gpio, &gpio_init);
}

// sets pin according to 'value'
void write_gpio_pin(PinId pinId, uint32_t value)
{
    HAL_GPIO_WritePin((GPIO_TypeDef*)g_PinMapping[pinId].gpio,
                      g_PinMapping[pinId].pin, value != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void toggle_gpio_pin(PinId pinId)
{
    HAL_GPIO_TogglePin((GPIO_TypeDef*)g_PinMapping[pinId].gpio,
                       g_PinMapping[pinId].pin);
}

// returns state of the pin
uint32_t read_gpio_pin(PinId pinId)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef*)g_PinMapping[pinId].gpio,
                            g_PinMapping[pinId].pin) == GPIO_PIN_SET
               ? 1
               : 0;
}

// initialize input channels
void input_channels_initialize()
{
    // Input channels: 0-7, handled by MAX4558 at U2
    initialize_gpio_pin(Pin_Input0_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Input0_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Input0_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Input0_Data, GPIO_MODE_INPUT, GPIO_NOPULL);

    // Input channels: 8-15, handled by MAX4558 at U3
    initialize_gpio_pin(Pin_Input1_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Input1_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Input1_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Input1_Data, GPIO_MODE_INPUT, GPIO_NOPULL);
}

void output_channels_initialize()
{
    // initialize output channel GPIOs
    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        initialize_gpio_pin(Pin_Output_Ch(i), GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    }

    initialize_gpio_pin(Pin_Outout_Diag_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Outout_Diag_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Outout_Diag_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Outout_Diag_S3, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    initialize_gpio_pin(Pin_Output_Diag_Data, GPIO_MODE_INPUT, GPIO_NOPULL);
}

uint16_t read_input_channels()
{
    uint16_t ret = 0;
    for (int i = 0; i < 8; ++i)
    {
        InputChannelSelector_t sel = g_inputChannelSelector[i];

        write_gpio_pin(Pin_Input0_S0, sel.s0);
        write_gpio_pin(Pin_Input0_S1, sel.s1);
        write_gpio_pin(Pin_Input0_S2, sel.s2);
        ret |= read_gpio_pin(Pin_Input0_Data) << i;
    }

    for (int i = 0; i < 8; ++i)
    {
        InputChannelSelector_t sel = g_inputChannelSelector[i];
        write_gpio_pin(Pin_Input1_S0, sel.s0);
        write_gpio_pin(Pin_Input1_S1, sel.s1);
        write_gpio_pin(Pin_Input1_S2, sel.s2);
        ret |= read_gpio_pin(Pin_Input1_Data) << (8 + i);
    }

    return ret;
}

uint16_t read_output_diag_status()
{
    uint16_t ret = 0;
    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        OutputDiagChannelSelector_t sel = g_outputDiagChannelSelector[i];

        write_gpio_pin(Pin_Outout_Diag_S0, sel.s0);
        write_gpio_pin(Pin_Outout_Diag_S1, sel.s1);
        write_gpio_pin(Pin_Outout_Diag_S2, sel.s2);
        write_gpio_pin(Pin_Outout_Diag_S3, sel.s3);

        ret |= read_gpio_pin(Pin_Output_Diag_Data) << i;
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

const IRQn_Type NO_IRQ = (IRQn_Type)0xffff;

// The clock for the passed 'systemTimer' (TIM1, TIM2, TIMn) must be enabled with
// prior to calling 'initialize_timer' with __HAL_RCC_TIMn_CLK_ENABLE()
void initialize_timer(TIM_HandleTypeDef* timerHndl, TIM_TypeDef* systemTimer, int periodInMs, IRQn_Type irqToEnable)
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

int poll_timer(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);
    return __HAL_TIM_GET_COUNTER(timerHndl);
}

void reset_timer(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);

    HAL_TIM_Base_Stop_IT(timerHndl);
    __HAL_TIM_CLEAR_IT(timerHndl, TIM_IT_UPDATE);

    // reset the counter
    timerHndl->Instance->CNT = 0;

    HAL_TIM_Base_Start_IT(timerHndl);
}

void disable_timer(TIM_HandleTypeDef* timerHndl)
{
    assert(timerHndl);

    HAL_TIM_Base_Stop_IT(timerHndl);
    __HAL_TIM_CLEAR_IT(timerHndl, TIM_IT_UPDATE);

    // reset the counter
    timerHndl->Instance->CNT = 0;
}

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
    toggle_gpio_pin(Pin_OnBoard_LED);
}

void PrintPing(TIM_HandleTypeDef* timerHndl)
{
    printf("PING\r\n");
}

void PulseTurnLight(TIM_HandleTypeDef* timerHndl)
{
    if (g_SystemState.LeftTurnActive)
    {
        toggle_gpio_pin(PinMap_TurnSignal_Left_Back);
        toggle_gpio_pin(PinMap_TurnSignal_Left_Front);
    }

    if (g_SystemState.RightTurnActive)
    {
        toggle_gpio_pin(PinMap_TurnSignal_Right_Back);
        toggle_gpio_pin(PinMap_TurnSignal_Right_Front);
    }

    if (g_SystemState.HazardLightActive)
    {
        toggle_gpio_pin(PinMap_TurnSignal_Left_Back);
        toggle_gpio_pin(PinMap_TurnSignal_Left_Front);
        toggle_gpio_pin(PinMap_TurnSignal_Right_Back);
        toggle_gpio_pin(PinMap_TurnSignal_Right_Front);
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
    initialize_gpio_pin(Pin_OnBoard_LED, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP);

    // Configure GPIO pin Output Level
    write_gpio_pin(Pin_OnBoard_LED, 1);
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
        write_gpio_pin(Pin_Output_Ch(i), (input >> i) & 0x1);
    }

    // HACK: map input channel 12 to output channel 6. This is only needed for
    // 2nd proto v0.1 board due to accidentialy cut trace
    write_gpio_pin(Pin_Output_Ch_6, (input >> 12) & 0x1);
}

void switch_on_all_outputs()
{
    // directly map to output for testing

    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        write_gpio_pin(Pin_Output_Ch(i), 1);
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
            write_gpio_pin(PinMap_TurnSignal_Left_Front, 1);
            write_gpio_pin(PinMap_TurnSignal_Left_Front, 1);

            reset_timer(&g_TurnIndicatorPulse_Timer);
        }
        else
        {
            write_gpio_pin(PinMap_TurnSignal_Left_Back, 0);
            write_gpio_pin(PinMap_TurnSignal_Left_Front, 0);

            disable_timer(&g_TurnIndicatorPulse_Timer);
        }
    }

    if (driverInput.RightTurnActive != systemState->RightTurnActive)
    {
        systemState->RightTurnActive = driverInput.RightTurnActive;

        if (systemState->RightTurnActive)
        {
            write_gpio_pin(PinMap_TurnSignal_Right_Back, 1);
            write_gpio_pin(PinMap_TurnSignal_Right_Front, 1);

            reset_timer(&g_TurnIndicatorPulse_Timer);
        }
        else
        {
            write_gpio_pin(PinMap_TurnSignal_Right_Back, 0);
            write_gpio_pin(PinMap_TurnSignal_Right_Front, 0);

            disable_timer(&g_TurnIndicatorPulse_Timer);
        }
    }

    if (driverInput.HazardLightActive != systemState->HazardLightActive)
    {
        systemState->HazardLightActive = driverInput.HazardLightActive;

        if (systemState->HazardLightActive)
        {
            write_gpio_pin(PinMap_TurnSignal_Left_Back, 1);
            write_gpio_pin(PinMap_TurnSignal_Left_Front, 1);
            write_gpio_pin(PinMap_TurnSignal_Right_Back, 1);
            write_gpio_pin(PinMap_TurnSignal_Right_Front, 1);

            reset_timer(&g_TurnIndicatorPulse_Timer);
        }
        else
        {
            write_gpio_pin(PinMap_TurnSignal_Left_Back, 0);
            write_gpio_pin(PinMap_TurnSignal_Left_Front, 0);
            write_gpio_pin(PinMap_TurnSignal_Right_Back, 0);
            write_gpio_pin(PinMap_TurnSignal_Right_Front, 0);

            disable_timer(&g_TurnIndicatorPulse_Timer);
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
        write_gpio_pin(Pin_OnBoard_LED, (uint32_t)targetLedState);
    }
}

// Entry point
int user_main(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    initialize_timer(&g_LED_Timer, TIM2, 500, TIM2_IRQn);
    __HAL_RCC_TIM3_CLK_ENABLE();
    initialize_timer(&g_TurnIndicatorPulse_Timer, TIM3, 1000, TIM3_IRQn);

    printf("Initialized!\r\n");

    write_gpio_pin(Pin_OnBoard_LED, 0);

    while (1)
    {
        uint16_t inputState = read_input_channels();
        DriverInputState_t driverInputState = TransformLowLevelInput(inputState);

        ProcessDriverInput(driverInputState, &g_SystemState);
        //switch_direct_input_to_output(input_state);
        //switch_on_all_outputs();

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