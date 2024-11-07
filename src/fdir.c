/**
 * @file    fdir.c
 * @author  Theo Bessel
 * @brief   Failure Detection Identification and Recovery
 * @date    25/10/2024
 *
 * @copyright Copyright (c) Theo Bessel 2024
 */


/******************************* Include Files *******************************/

#include "fdir.h"
#include "stacktrace.h"

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

inline void __attribute__((always_inline)) Unwind_Stack(void);

extern void HardFault_Handler(void);
extern void MemManage_Handler(void);
extern void BusFault_Handler(void);
extern void UsageFault_Handler(void);

/*************************** Variables Definitions ***************************/

uint32_t sp;
debugInfo_t *debugInfo;
generalRegisters_t *generalRegisters;

/*************************** Functions Definitions ***************************/

inline void __attribute__((always_inline)) Unwind_Stack(void)
{
    __asm volatile (
        "tst lr, #4                         \n" // Test bit 2 of EXC_RETURN; Z is set if lr[2] = 1
        "ite eq                             \n" // If-Then-Else conditional execution
        "mrseq r0, msp                      \n" // If equal (Z=1), move the value of MSP to r0
        "mrsne r0, psp                      \n" // If not equal (Z=0), move the value of PSP to r0
        "add r0, r0, #12                    \n" // Add 8 to r0 to skip over the saved registers
        "mov %[sp], r0                      \n" // Move the value of r0 to sp (for debugging)
        : [sp] "=r" (sp)                        // Output operands
        :                                       // No input operands
        : "r0"                                  // Clobbered register
    );

    debugInfo->registers = (savedRegisters_t *)sp;

    debugInfo->sp = sp;

    debugInfo->cfsr = (uint32_t) *(uint32_t *) 0xE000ED28;
    debugInfo->hfsr = (uint32_t) *(uint32_t *) 0xE000ED2C;

    // Save the general registers
    register uint32_t r7 __asm("r7");
    generalRegisters->r[7] = *(uint32_t *) r7;

    register uint32_t lr __asm("lr");
    generalRegisters->r[14] = lr;

    // Print StackTrace
    PrintStackTrace(debugInfo, generalRegisters);
}

/*************************** Interruption Handlers ***************************/

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
    Unwind_Stack();

    while (1)
    {
    }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
    Unwind_Stack();

    while (1)
    {
    }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
    Unwind_Stack();

    while (1)
    {
    }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
    Unwind_Stack();

    while (1)
    {
    }
}
