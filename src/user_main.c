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

// globals
TIM_HandleTypeDef g_Timer2Hndl;

void initialize_timer(TIM_HandleTypeDef* timerHndl)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    timerHndl->Instance = TIM2;

    timerHndl->Init.Prescaler = 40000;
    timerHndl->Init.CounterMode = TIM_COUNTERMODE_UP;
    timerHndl->Init.Period = 500;
    timerHndl->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timerHndl->Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(timerHndl);
    HAL_TIM_Base_Start_IT(timerHndl);

    // enable interrupts
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

int poll_timer(TIM_HandleTypeDef* timerHndl)
{
    return __HAL_TIM_GET_COUNTER(timerHndl);
}

void TIM2_IRQHandler()
{
    if (__HAL_TIM_GET_FLAG(&g_Timer2Hndl, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&g_Timer2Hndl, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_FLAG(&g_Timer2Hndl, TIM_FLAG_UPDATE);

            HAL_TIM_IRQHandler(&g_Timer2Hndl);

            toggle_gpio_pin(Pin_OnBoard_LED);
        }
    }
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

uint16_t handle_input_stage()
{
    uint16_t input_state = read_input_channels();

    const int numInputChannels = 16;
    uint8_t channel[numInputChannels];

    int numActive = 0;
    for (int i = 0; i < numInputChannels; ++i)
    {
        channel[i] = (input_state >> i) & 0x1;

        if (channel[i] != 0)
        {
            printf("Channel %d,", i);
            ++numActive;
        }
    }

    if (numActive > 0)
    {
        printf("\r\n");
    }
    return input_state;
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

typedef enum {
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
    initialize_timer(&g_Timer2Hndl);

    printf("Initialized!\r\n");

    write_gpio_pin(Pin_OnBoard_LED, 0);

    while (1)
    {
        uint16_t input_state = handle_input_stage();
        //switch_direct_input_to_output(input_state);
        switch_on_all_outputs();

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