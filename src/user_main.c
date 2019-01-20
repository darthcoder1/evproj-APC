#include "main.h"
#include <stdio.h>


// Entry point
int user_main(void)
{
    printf("Initialized!\r\n");
    
    while (1) 
    {
        GPIOC->BRR |= 1<<13;
        for (int i = 0; i < 1000000; ++i) asm("nop");
        GPIOC->BSRR |= 1<<13;
        for (int i = 0; i <  500000; ++i) asm("nop");
    }
}