
#include "stm32f103xb.h"
#include <stdint.h>

// Mapping of STM32 hardware pins
typedef struct
{
    GPIO_TypeDef* gpio;
    uint32_t pin;
} PinMapEntry_t;

// PinId
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

    Pin_Output_Ch_0,
    Pin_Output_Ch_1,
    Pin_Output_Ch_2,
    Pin_Output_Ch_3,
    Pin_Output_Ch_4,
    Pin_Output_Ch_5,
    Pin_Output_Ch_6,
    Pin_Output_Ch_7,
    Pin_Output_Ch_8,
    Pin_Output_Ch_9,
    Pin_Output_Ch_10,
    Pin_Output_Ch_11,

    Pin_NumIds,
    Pin_Output_NumOutputs = (Pin_Output_Ch_11 - Pin_Output_Ch_0) + 1,
} PinId;

#define Pin_Output_Ch(channelIdx) ((PinId)((int)Pin_Output_Ch_0 + (channelIdx)))

// InputChannelSelector_t
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

// mapping of pins to channels
static const PinMapEntry_t g_PinMapping[] = {
    {.gpio = GPIOC, .pin = GPIO_PIN_13 }, // Pin_OnBoard_LED

    // input channels 0-7
    {.gpio = GPIOB, .pin = GPIO_PIN_15 }, // Pin_Input0_S0
    {.gpio = GPIOB, .pin = GPIO_PIN_14 }, // Pin_Input0_S1
    {.gpio = GPIOA, .pin = GPIO_PIN_15 }, // Pin_Input0_S2
    {.gpio = GPIOB, .pin = GPIO_PIN_4 },  // Pin_Input0_Data

    // input channels 8 - 15
    {.gpio = GPIOA, .pin = GPIO_PIN_10 }, // Pin_Input1_S0
    {.gpio = GPIOA, .pin = GPIO_PIN_9 },  // Pin_Input1_S1
    {.gpio = GPIOA, .pin = GPIO_PIN_8 },  // Pin_Input1_S2
    {.gpio = GPIOB, .pin = GPIO_PIN_3 },  // Pin_Input1_Data

    // output channels
    {.gpio = GPIOA, .pin = GPIO_PIN_6 },  // Pin_Output_Ch_0
    {.gpio = GPIOA, .pin = GPIO_PIN_7 },  // Pin_Output_Ch_1
    {.gpio = GPIOB, .pin = GPIO_PIN_0 },  // Pin_Output_Ch_2
    {.gpio = GPIOB, .pin = GPIO_PIN_1 },  // Pin_Output_Ch_3
    {.gpio = GPIOB, .pin = GPIO_PIN_10 }, // Pin_Output_Ch_4
    {.gpio = GPIOB, .pin = GPIO_PIN_11 }, // Pin_Output_Ch_5
    {.gpio = GPIOA, .pin = GPIO_PIN_5 },  // Pin_Output_Ch_6
    {.gpio = GPIOA, .pin = GPIO_PIN_4 },  // Pin_Output_Ch_7
    {.gpio = GPIOA, .pin = GPIO_PIN_3 },  // Pin_Output_Ch_8
    {.gpio = GPIOA, .pin = GPIO_PIN_2 },  // Pin_Output_Ch_9
    {.gpio = GPIOA, .pin = GPIO_PIN_1 },  // Pin_Output_Ch_10
    {.gpio = GPIOA, .pin = GPIO_PIN_0 },  // Pin_Output_Ch_11

};