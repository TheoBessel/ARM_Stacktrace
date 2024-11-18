/**
 * @file    main.c
 * @author  Théo Bessel
 * @brief   FDIR Test Application
 *
 * @copyright Copyright (c) Théo Bessel 2024
 */

#include <stdio.h>
#include <stdint.h>
#include "fdir.h"

void __attribute__((noinline)) function_c(uint32_t c) {
    // Random operation to have frames with registers pushed on the stack
    volatile uint32_t a = c + 43;
    volatile uint32_t b = 0;

//    printf("%d", (int) a);

    // Causes UsageFault
    volatile uint32_t result = a / b;

    (void) result;
}

void __attribute__((noinline)) function_b(uint32_t b) {
    // Random operation to have frames with registers pushed on the stack
    volatile uint32_t c = 32 - b;

    // Calling function_c (which causes UsageFault)
    function_c(c);
}

void __attribute__((noinline)) function_a(uint32_t a) {
    // Random operation to have frames with registers pushed on the stack

    // Calling function_b (which calls function_c that causes UsageFault)
    function_b(a - 10);
}


int main(void) {
    InitFDIR();

    // Causes UsageFault (division by zero)
    function_a(13);

    while (1);
    return 0;
}
