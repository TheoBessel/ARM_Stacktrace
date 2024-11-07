/**
 * @file    stacktrace.h
 * @author  Theo Bessel
 * @brief   Header file for stacktrace
 * @date    20/07/2024
 *
 * @copyright Copyright (c) Theo Bessel 2024
 */

#ifndef STACKTRACE_H
#define STACKTRACE_H

/******************************* Include Files *******************************/

#include <stdint.h>

/***************************** Macros Definitions ****************************/

#define CALL_STACK_SIZE 20u

/***************************** Types Definitions *****************************/

typedef struct __attribute__((packed))
{
    uint32_t r[15];
} generalRegisters_t;


/**
 * @struct  savedRegisters
 * @brief   Structure that saves the registers of the processor
 * @warning The value of sp is its value after the context was saved
 */
typedef struct __attribute__((packed))
{
    uint32_t r[3];
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} savedRegisters_t;

typedef struct __attribute__((packed))
{
    uint32_t size;
    uint32_t calls[CALL_STACK_SIZE];
} callStack_t;

typedef struct __attribute__((packed))
{
    savedRegisters_t *registers;
    uint32_t sp;
    uint32_t cfsr;
    uint32_t hfsr;
    callStack_t *call_stack;
} debugInfo_t;


/*************************** Variables Declarations **************************/

extern uint32_t __exidx_start;
extern uint32_t __exidx_end;

extern uint32_t __extab_start;
extern uint32_t __extab_end;

/*************************** Functions Declarations **************************/

void PrintStackTrace(debugInfo_t*, generalRegisters_t*);

#endif /* STACKTRACE_H */
