/**
 * @file    fdir.h
 * @author  Théo Bessel
 * @brief   Error Management functions
 *
 * @copyright Copyright (c) Théo Bessel 2024
 */

#ifndef FDIR_H
#define FDIR_H

/******************************* Include Files *******************************/

#include <stdint.h>

/***************************** Macros Definitions ****************************/

#define CALL_STACK_SIZE 20u

/***************************** Types Definitions *****************************/

/**
 * @struct  savedRegisters_t
 * @brief   Structure that saves the registers of the processor when an error occurs
 */
typedef struct __attribute__((packed))
{
    uint32_t r[4];
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} savedRegisters_t;

/**
 * @struct  call_t
 * @brief   Structure that saves the `lr` and the `fp` of a frame
 */
typedef struct __attribute__((packed))
{
    uint32_t lr;
    uint32_t fp;
} call_t;

/**
 * @struct  callStack_t
 * @brief   Structure that saves the call stack when an error occurs
 */
typedef struct __attribute__((packed))
{
    uint32_t size;
    call_t calls[CALL_STACK_SIZE];
} callStack_t;

/**
 * @struct  debugInfo_t
 * @brief   Structure that saves the general debug information when an error occurs
 * Note : sp is the address pointed by `registers`
 */
typedef struct
{
    savedRegisters_t *registers;
    uint32_t cfsr;
    uint32_t hfsr;
    callStack_t *call_stack;
} debugInfo_t;

/**
 * @struct  exidxEntry_t
 * @brief   Structure that handle exidx raw and decoded entries
 */
typedef struct __attribute__((packed))
{
    uint32_t exidx_entry;
    uint32_t exidx_fn;
    uint32_t decoded_entry;
    uint32_t decoded_fn;
} exidxEntry_t;


/*************************** Variables Declarations **************************/

extern uint32_t __exidx_start;
extern uint32_t __exidx_end;

extern uint32_t __extab_start;
extern uint32_t __extab_end;

/*************************** Functions Declarations **************************/

extern void InitFDIR(void);
extern void SaveRegisters(debugInfo_t** debug_info);
extern void UnwindStack(debugInfo_t** debug_info);

#endif /* FDIR_H */