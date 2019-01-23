#include "main.h"
#include <stdio.h>

typedef struct
{
    GPIO_TypeDef* gpio;
    uint32_t pin;
} PinMapEntry_t;

typedef enum {
    Pin_OnBoard_LED = 0,

    // input
    Pin_Input0_S0,
    Pin_Input0_S1,
    Pin_Input0_S2,
    Pin_Input0_Data,

    Pin_Input1_S0,
    Pin_Input1_S1,
    Pin_Input1_S2,
    Pin_Input1_Data,

    Pin_NumIds,
} PinId;

typedef struct
{
    uint32_t s0 : 1; // on MAX4558 = A
    uint32_t s1 : 1; // on MAX4558 = B
    uint32_t s2 : 1; // on MAX4558 = C
} InputChannelSelector_t;

// Selector bit settings to access a specific channel
// see schematics and datasheet for 74HC4051
static const InputChannelSelector_t g_inputChannelSelector[] = {
    {.s0 = 1, .s1 = 1, .s2 = 0 }, // Y3 -> Channel 0
    {.s0 = 0, .s1 = 0, .s2 = 0 }, // Y0 -> Channel 1
    {.s0 = 1, .s1 = 0, .s2 = 0 }, // Y1 -> Channel 2
    {.s0 = 0, .s1 = 1, .s2 = 0 }, // Y2 -> Channel 3
    {.s0 = 1, .s1 = 0, .s2 = 1 }, // Y5 -> Channel 4
    {.s0 = 1, .s1 = 1, .s2 = 1 }, // Y7 -> Channel 5
    {.s0 = 0, .s1 = 1, .s2 = 1 }, // Y6 -> Channel 6
    {.s0 = 0, .s1 = 0, .s2 = 1 }, // Y4 -> Channel 7
};

static const PinMapEntry_t g_PinMapping[] = {
    {.gpio = GPIOC, .pin = GPIO_PIN_13 }, // Pin_OnBoard_LED

    // input channels 0-7
    {.gpio = GPIOA, .pin = GPIO_PIN_11 }, // Pin_Input0_S0
    {.gpio = GPIOA, .pin = GPIO_PIN_12 }, // Pin_Input0_S1
    {.gpio = GPIOA, .pin = GPIO_PIN_15 }, // Pin_Input0_S2
    {.gpio = GPIOB, .pin = GPIO_PIN_4 },  // Pin_Input0_Data

    // input channels 8 - 15
    {.gpio = GPIOA, .pin = GPIO_PIN_10 }, // Pin_Input1_S0
    {.gpio = GPIOA, .pin = GPIO_PIN_9 },  // Pin_Input1_S1
    {.gpio = GPIOA, .pin = GPIO_PIN_8 },  // Pin_Input1_S2
    {.gpio = GPIOB, .pin = GPIO_PIN_3 },  // Pin_Input1_Data
};

void intialize_gpio_pin(PinId pinId, uint32_t gpioMode, uint32_t pullMode)
{
    GPIO_InitTypeDef gpio_init = {
        .Pin = g_PinMapping[pinId].pin, .Mode = gpioMode, .Pull = pullMode, .Speed = GPIO_SPEED_FREQ_LOW
    };

    HAL_GPIO_Init(g_PinMapping[pinId].gpio, &gpio_init);
}

void write_gpio_pin(PinId pinId, uint32_t value)
{
    HAL_GPIO_WritePin(g_PinMapping[pinId].gpio, g_PinMapping[pinId].pin,
                      value != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return;
    if (value == 0)
    {
        g_PinMapping[pinId].gpio->ODR &= ~g_PinMapping[pinId].pin;
    }
    else
    {
        g_PinMapping[pinId].gpio->ODR |= g_PinMapping[pinId].pin;
    }
}

uint32_t read_gpio_pin(PinId pinId)
{
    return (g_PinMapping[pinId].gpio->IDR & g_PinMapping[pinId].pin) != 0 ? 1 : 0;
}

void input_channels_initialize()
{
    // Input channels: 0-7, handled by MAX4558 at U2
    intialize_gpio_pin(Pin_Input0_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    intialize_gpio_pin(Pin_Input0_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    intialize_gpio_pin(Pin_Input0_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    intialize_gpio_pin(Pin_Input0_Data, GPIO_MODE_INPUT, GPIO_NOPULL);

    // Input channels: 8-15, handled by MAX4558 at U3
    intialize_gpio_pin(Pin_Input1_S0, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    intialize_gpio_pin(Pin_Input1_S1, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    intialize_gpio_pin(Pin_Input1_S2, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
    intialize_gpio_pin(Pin_Input1_Data, GPIO_MODE_INPUT, GPIO_NOPULL);
}

uint8_t read_input_channel(uint32_t channelIdx)
{
    InputChannelSelector_t sel = g_inputChannelSelector[channelIdx];

    write_gpio_pin(Pin_Input0_S0, sel.s0);
    write_gpio_pin(Pin_Input0_S1, sel.s1);
    write_gpio_pin(Pin_Input0_S2, sel.s2);

    uint32_t pin_status = read_gpio_pin(Pin_Input0_Data);
    return pin_status;
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

// Hooks called from kernel code

void user_initialize_gpio()
{
    input_channels_initialize();

    // GPIO Ports Clock Enable
    intialize_gpio_pin(Pin_OnBoard_LED, GPIO_MODE_OUTPUT_PP, GPIO_PULLUP);

    // Configure GPIO pin Output Level
    write_gpio_pin(Pin_OnBoard_LED, 1);
}

// Entry point
int user_main(void)
{
    printf("Initialized!\r\n");

    while (1)
    {
        write_gpio_pin(Pin_OnBoard_LED, 0);

        uint16_t input_state = read_input_channels();

        const uint32_t numInputChannels = 16;
        uint8_t channel[numInputChannels];

        for (int i = 0; i < numInputChannels; ++i)
        {
            channel[i] = (input_state >> i) & 0x1;
        }

        printf("Input Channel  %d%d%d%d %d%d%d%d %d%d%d%d %d%d%d%d\r\n", channel[0],
               channel[1], channel[2], channel[3], channel[4], channel[5],
               channel[6], channel[7], channel[8], channel[9], channel[10],
               channel[11], channel[12], channel[13], channel[14], channel[15]);

        HAL_Delay(500);
        write_gpio_pin(Pin_OnBoard_LED, 1);
        HAL_Delay(500);
    }
}
