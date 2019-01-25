#include "main.h"
#include "pin_map.h"
#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_tim.h"
#include "usb_handler.h"

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

typedef struct
{
    TIM_HandleTypeDef internalHndl;
} timer_handle_t;

void initialize_timer(timer_handle_t* timerHndl)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    timerHndl->internalHndl.Instance = TIM2;

    timerHndl->internalHndl.Init.Prescaler = 40000;
    timerHndl->internalHndl.Init.CounterMode = TIM_COUNTERMODE_UP;
    // timerHndl->internalHndl.Init.Period = 500;
    timerHndl->internalHndl.Init.Period = 10000;
    timerHndl->internalHndl.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timerHndl->internalHndl.Init.RepetitionCounter = 0;

    HAL_TIM_Base_Init(&timerHndl->internalHndl);
    HAL_TIM_Base_Start(&timerHndl->internalHndl);
}

int poll_timer(timer_handle_t* timerHndl)
{
    return __HAL_TIM_GET_COUNTER(&timerHndl->internalHndl);
}

void ErrorLoop();

// Hooks called from kernel code
void user_initialize()
{
    // enable usb
    if (usb_handler_initialize() != USBD_OK)
        ErrorLoop();

    if (usb_handler_start() != USBD_OK)
        ErrorLoop();

    // enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // initialize input channel GPIOs
    input_channels_initialize();

    // initialize output channel GPIOs
    for (int i = 0; i < Pin_Output_NumOutputs; ++i)
    {
        initialize_gpio_pin(Pin_Output_Ch(i), GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    }

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

// Entry point
int user_main(void)
{
    timer_handle_t timeHandleStorage;

    timer_handle_t* timeHndl = &timeHandleStorage;
    initialize_timer(timeHndl);

    printf("Initialized!\r\n");

    while (1)
    {
        write_gpio_pin(Pin_OnBoard_LED, 0);

        uint16_t input_state = handle_input_stage();

        switch_direct_input_to_output(input_state);

        int timer = poll_timer(timeHndl);

        if (timer < 5000)
            write_gpio_pin(Pin_OnBoard_LED, 0);
        else // if (timer == 500)
            write_gpio_pin(Pin_OnBoard_LED, 1);

        write_gpio_pin(Pin_OnBoard_LED, 1);
        HAL_Delay(500);
    }

    usb_handler_shutdown();
}

void ErrorLoop()
{
    while (1)
    {
        write_gpio_pin(Pin_OnBoard_LED, 0);
        HAL_Delay(250);
        write_gpio_pin(Pin_OnBoard_LED, 1);
        HAL_Delay(250);
    }
}
