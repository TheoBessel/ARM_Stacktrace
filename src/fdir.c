/**
 * @file    fdir.c
 * @author  Théo Bessel
 * @brief   Interface for Failure Detection, Identification and Recovery (FDIR).
 *
 * @copyright Copyright (c) Théo Bessel 2024
 */

/******************************* Include Files *******************************/

#include "fdir.h"

/***************************** Macros Definitions ****************************/

// CMSIS Macros
#define CMSIS_CFSR (*((volatile uint32_t *) 0xE000ED28))
#define CMSIS_HFSR (*((volatile uint32_t *) 0xE000ED2C))

#define CMSIS_SHCSR *((volatile uint32_t *) 0xE000ED24)
#define CMSIS_SHCSR_MEMFAULTENA_Msk (1 << 16)
#define CMSIS_SHCSR_BUSFAULTENA_Msk (1 << 17)
#define CMSIS_SHCSR_USGFAULTENA_Msk (1 << 18)

#define CMSIS_CCR *((volatile uint32_t *) 0xE000ED14)
#define CMSIS_CCR_DIV_0_TRP_Msk (1 << 4)
#define CMSIS_CCR_UNALIGN_TRP_Msk (1 << 3)

/*************************** Functions Declarations **************************/

inline void __attribute__((always_inline)) SaveRegisters(debugInfo_t *debug_info);

inline void __attribute__((always_inline)) PrepareUnwind(call_t* last_call);

/*************************** Handlers Declarations ***************************/

extern void Reset_Handler(void);
extern void HardFault_Handler(void);
extern void MemManage_Handler(void);
extern void BusFault_Handler(void);
extern void UsageFault_Handler(void);

/*************************** Variables Definitions ***************************/

/**
 * @brief Contains all the debugging informations
 */
debugInfo_t debug_info = {0};

/**
 * @brief Contains the last call (fp + lr)
 */
call_t last_call = {0};

/*************************** Functions Definitions ***************************/

/**
 * @brief  This function initialises the FDIR
 * @return Nothing
 */
void InitFDIR(void)
{
    // Enables memory management, bus fault and usage fault exceptions
    CMSIS_SHCSR |= CMSIS_SHCSR_MEMFAULTENA_Msk | CMSIS_SHCSR_BUSFAULTENA_Msk | CMSIS_SHCSR_USGFAULTENA_Msk;
    CMSIS_CCR |= CMSIS_CCR_DIV_0_TRP_Msk | CMSIS_CCR_UNALIGN_TRP_Msk;
}

/**
 * @brief This function saves the registers of the processor when an error occured
 * @param[out] debug_info         The structure where to store the saved registers
 * @return Nothing
 */
inline void __attribute__((always_inline)) SaveRegisters(debugInfo_t* debug_info)
{
    __asm volatile (
        "tst lr, #4         \n" // Test bit 2 of EXC_RETURN; Z is set if lr[2] = 1
        "ite eq             \n" // If-Then-Else conditional execution
        "mrseq %[sp], msp   \n" // If equal (Z=1), move the value of MSP to r1
        "mrsne %[sp], psp   \n" // If not equal (Z=0), move the value of PSP to r1
        : [sp] "=r" (
            (*debug_info).registers
        )                       // Output operands
        :                       // No input operands
        :                       // Clobbered register
    );

    (*debug_info).cfsr = (uint32_t) CMSIS_CFSR;
    (*debug_info).hfsr = (uint32_t) CMSIS_HFSR;
}

/**
 * @brief This function save the unwind base context when executed in an error handler
 * @param[out] last_call        The context to save
 * @return Nothing
 */
inline void __attribute__((always_inline)) PrepareUnwind(call_t* last_call) {
    __asm volatile (
        "str r7, %[call_fp]        \n"
        "tst lr, #4                \n"
        "ite eq                    \n"  // If-Then-Else conditional execution
        "mrseq r0, msp             \n"  // If equal (Z=1), move the value of MSP to r1
        "mrsne r0, psp             \n"  // If not equal (Z=0), move the value of PSP to r1
        "ldr %[call_lr], [r0, #20] \n"  // Save lr (=*r0+20) into call_lr, #20 is the offset from the start of the frame
        : [call_fp] "=m" (
            last_call->fp
        ), [call_lr] "=r" (
            last_call->lr
        )
        // Output operands
        :                               // No input operands
        : "r0"                          // No clobbered register
    );
}

/*************************** Interruption Handlers ***************************/

/**
 * @brief This function handles Hard fault interrupt.
 */
void __attribute__((naked)) HardFault_Handler(void)
{
    while (1);
}

/**
 * @brief This function handles Memory management fault.
 */
void __attribute__((naked)) MemManage_Handler(void)
{
    while (1);
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void __attribute__((naked)) BusFault_Handler(void)
{
    while (1);
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void __attribute__((naked)) UsageFault_Handler(void)
{
    // Save the registers
    SaveRegisters(&debug_info);

    // Unwind the stack to etablish a stacktrace
    PrepareUnwind(&last_call);
    UnwindStack(&(debug_info.call_stack), last_call);

    while (1);
}