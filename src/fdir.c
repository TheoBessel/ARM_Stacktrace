/**
 * @file    fdir.c
 * @author  Théo Bessel
 * @brief   Error Management functions
 *
 * @copyright Copyright (c) Théo Bessel 2024
 */

/******************************* Include Files *******************************/

#include "fdir.h"

/***************************** Macros Definitions ****************************/

// CantUnwind symbol
#define EXIDX_CANTUNWIND 0x1

// Personality routine indexes
#define SU16 0x0
#define LU16 0x1
#define LU32 0x2

// debugInfo_t manipulation
#define CFSR(debug_info)            (*debug_info)->cfsr
#define HFSR(debug_info)            (*debug_info)->hfsr
#define REGISTERS(debug_info)       (*debug_info)->registers
#define CALL_STACK(debug_info)      (*debug_info)->call_stack
#define LAST_CALL(call_stack)       call_stack->calls[call_stack->size]

// Masks
#define SIX_RIGHT_MASK(instruction) ((instruction & 0x3f) << 2)

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

// Exported functions
inline void __attribute__((always_inline)) SaveRegisters(debugInfo_t** debug_info);
inline void __attribute__((always_inline)) UnwindStack(debugInfo_t** debug_info);

void UnwindNextFrame(debugInfo_t** debug_info);
uint32_t DecodeFrame(uint32_t entry, uint32_t decoded_entry, uint32_t fp);
uint32_t DecodeCompactModelEntry(const uint32_t entry, const uint32_t word, const uint32_t fp, const uint8_t instr_count);
uint32_t GetInstruction(const uint32_t entry, const uint32_t word, const uint8_t offset);
exidxEntry_t GetExidxEntry(const uint8_t* const section, const uint32_t offset);
uint32_t DecodePrel31(const uint32_t word, const uint32_t where);
uint32_t GetWord(const uint8_t* const section, const uint32_t offset);

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
debugInfo_t *debug_info;

/************************ Unwind Functions Definitions ***********************/

/**
 * @brief This function saves the registers of the processor when an error occured
 * @param[out] debug_info         The structure where to store the saved registers
 * @return Nothing
 */
inline void __attribute__((always_inline)) SaveRegisters(debugInfo_t** debug_info)
{
    __asm volatile (
        "tst lr, #4             \n"     // Test bit 2 of EXC_RETURN; Z is set if lr[2] = 1
        "ite eq                 \n"     // If-Then-Else conditional execution
        "mrseq r0, msp          \n"     // If equal (Z=1), move the value of MSP to r0
        "mrsne r0, psp          \n"     // If not equal (Z=0), move the value of PSP to r0
        "str r0, %[sp_ptr]      \n"     // Store the value of r0 to the address of sp (pointed by sp_ptr)
        : [sp_ptr] "=m" (
            REGISTERS(debug_info)
        )                               // Output operands
        :                               // No input operands
        : "r0"                          // Clobbered register
    );

    CFSR(debug_info) = (uint32_t) CMSIS_CFSR;
    HFSR(debug_info) = (uint32_t) CMSIS_HFSR;
}

/**
 * @brief This function makes an unwind to compute the stacktrace from the program
 * counter variable.
 * @param[out] debug_info             The structure where to store the stracktrace
 * @return Nothing
 */
inline void __attribute__((always_inline)) UnwindStack(debugInfo_t** debug_info)
{
    CALL_STACK(debug_info)->size = 0;
    CALL_STACK(debug_info)->calls[0].lr = REGISTERS(debug_info)->lr;
    CALL_STACK(debug_info)->calls[0].fp = (uint32_t) __builtin_frame_address(0);

    __asm volatile (
        "mov lr, %[new_lr]\n"
        "add lr, lr, #1"
        :
        : [new_lr] "r" (REGISTERS(debug_info)->lr)
        : "lr"
    );

    while (CALL_STACK(debug_info)->size < CALL_STACK_SIZE && LAST_CALL(CALL_STACK(debug_info)).lr != 0xffffffff)
    {
        UnwindNextFrame(debug_info);
    }
    
}

/**
 * @brief This function unwind the frame following the last valid address stored
 * in (debug_info*)->call_stack
 * @param[out] debug_info     The structure where to store the frame computed lr
 * @return Nothing
 */
void UnwindNextFrame(debugInfo_t** debug_info)
{
    /**
     * @brief Total number of entries in the unwind table
     */
    uint8_t entries_count = (&__exidx_end - &__exidx_start) / 2;

    /**
     * @brief Unwind tables entries
     */
    uint32_t extab_entry = 0x0;
    exidxEntry_t entry;

    /**
     * @brief Frame pointer
     */
    uint32_t fp = LAST_CALL(CALL_STACK(debug_info)).fp;
    uint32_t new_fp = 0x0;

    /**
     * Perform a binary search to find the entry in the unwind table
     * corresponding to the last return address unwound.
     * The complexity is now O(log_2(N)) because of the sorted addresses.
     */
    uint8_t low = 0;
    uint8_t high = entries_count - 1;

    while (low <= high) {
        uint8_t mid = low + (high - low) / 2;
        entry = GetExidxEntry((uint8_t *) &__exidx_start, 8 * mid);

        if (entry.decoded_fn < LAST_CALL(CALL_STACK(debug_info)).lr) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    /**
     * Save the correct return address (the one that is less than the current frame address)
     */
    LAST_CALL(CALL_STACK(debug_info)).lr = entry.decoded_fn;

    /**
     * Move to the next call array place.
     */
    CALL_STACK(debug_info)->size += 1;

    /**
     * (Section 6)
     * The second word contains one of:
     *   - The prel31 offset of the start of the table entry for this function, with bit 31 clear.
     *   - The exception-handling table entry itself with bit 31 set, if it can be encoded in 31 bits
     *     (see The Arm-defined compact model).
     *   - The special bit pattern EXIDX_CANTUNWIND (0x1), indicating to run-time support code that associated
     *     frames cannot be unwound. On encountering this pattern the language-independent unwinding routines
     *     return a failure code to their caller, which should take an appropriate action such as calling
     *     terminate() or abort(). See Phase 1 unwinding and Phase 2 unwinding.
     */
    if (entry.exidx_entry == EXIDX_CANTUNWIND)      // Special pattern 0x1 EXIDX_CANTUNWIND
    {
        LAST_CALL(CALL_STACK(debug_info)).lr = 0xffffffff;
        LAST_CALL(CALL_STACK(debug_info)).fp = 0xffffffff;
    }
    else if (entry.exidx_entry & 0x80000000)        // Bit 31 set --> compact model
    {
        new_fp = DecodeFrame(entry.exidx_entry, entry.decoded_entry, fp);

        /**
         * The `lr` register is pushed just before the `fp` register, then we can get it by accessing `fp + 4`
         */
        LAST_CALL(CALL_STACK(debug_info)).fp = *((uint32_t *) new_fp);
        LAST_CALL(CALL_STACK(debug_info)).lr = *((uint32_t *) (new_fp + 4)) - 1;
    }
    else                                            // Bit 31 is clear
    {
        extab_entry = GetWord((uint8_t *) entry.decoded_entry, 0);

        if (extab_entry & 0x80000000)
        {
            new_fp = DecodeFrame(extab_entry, entry.decoded_entry, fp);

            /**
             * The `lr` register is pushed just before the `fp` register, then we can get it by accessing `fp + 4`
             */
            LAST_CALL(CALL_STACK(debug_info)).fp = *((uint32_t *) new_fp);
            LAST_CALL(CALL_STACK(debug_info)).lr = *((uint32_t *) (new_fp + 4)) - 1;
        }
    }
}

/**
 * @brief This function execute the personnality routine for a given entry and
 * return the computed frame pointer
 */
uint32_t DecodeFrame(uint32_t entry, uint32_t decoded_entry, uint32_t fp) {
    /**
     * (Section 10.2)
     * The first word is as described in The Arm-defined compact model.
     * The most significant byte encodes the index of the personality routine (PR) used
     * to interpret what follows :
     *   0 - Su16 : Short frame unwinding description followed by descriptors with 16-bit scope.
     *   1 - Lu16 : Long frame unwinding description followed by descriptors with 16-bit scope.
     *   2 - Lu32 : Long frame unwinding description followed by descriptors with 32-bit scope.
     */
    uint32_t word = (entry & 0xffffff);

    /**
     * @brief Number of instructions to decode the frame
     */
    uint32_t instr_count = (word >> 16) & 0xff;

    /**
     * (Section 7.3)
     * An exception-handling table entry for the compact model looks like:
     * 31 | 30 - 28 | 27 - 24 | 23 ------------------------------- 0 |
     *  1 |       0 |   index |  Data for personalityRoutine[index]. |
     *
     * The personality routine is the set of instructions required to unwind the stack.
     *
     * (Section 7.3)
     * Arm has allocated index numbers 0, 1 and 2 for use by C and C++.
     * Arm-defined personality routines and table formats for C and C++ details the mapping
     * from index numbers to personality routines and explains how to use them.
     * Index numbers 3-15 are reserved for future use.
     *
     *
     * Thus, personality index for `entry` is `(uint8_t) ((entry >> 24) & 0xf)`
     */
    switch ((uint8_t) ((entry >> 24) & 0xf))
    {
        /**
            * (Section 10.2)
            * Short 3 unwinding instructions in bits 16-23, 8-15, and 0-7 of the first word. Any of the instructions can be Finish.
            */
        case SU16:
            fp = DecodeCompactModelEntry(decoded_entry, word, fp, 3);
            break;
        /**
            * (Section 10.2)
            * Long Bits 16-23 contain a count N of the number of additional 4-byte words that contain unwinding instructions.
            * The sequence of unwinding instructions is packed into bits 8-15, 0-7, and the following N words.
            * Spare trailing bytes in the last word should be filled with Finish instructions.
            */
        case LU16:
            fp = DecodeCompactModelEntry(decoded_entry, word, fp, 2 + 4 * instr_count);
            break;
        case LU32:
            fp = DecodeCompactModelEntry(decoded_entry, word, fp, 2 + 4 * instr_count);
            break;
        default:
            // error
            break;
    }

    return fp;
}


/**
 * @brief This function decodes unwind instructions based on ARM EHABI standard.
 * @warning Some instructions are not implemented yet because they are not used to
 * unwind the stack on ARM Cortex-M. (ex : the frame pointer is r7 in any case)
 */
uint32_t DecodeCompactModelEntry(const uint32_t entry, const uint32_t word, const uint32_t fp, const uint8_t instr_count)
{
    uint32_t instr1 = 0x0;
    uint32_t instr2 = 0x0;

    uint8_t instr_index = 0;

    uint32_t new_fp = fp;

    while (instr_index < instr_count)
    {
        // Fetch the two first instructions
        instr1 = instr_index < instr_count ? GetInstruction(entry, word, instr_index + 2) : instr1;
        instr2 = instr_index < instr_count - 1 ? GetInstruction(entry, word, instr_index + 3) : instr2;

        if      ((instr1 & 0xc0) == 0x00)
        {
            /**
             * @brief 00xxxxxx
             * vsp = vsp + (xxxxxx << 2) + 4. Covers range 0x04-0x100 inclusive
             */
            new_fp += SIX_RIGHT_MASK(instr1) + 4;
        }
        else if ((instr1 & 0xc0) == 0x40)
        {
            /**
             * @brief 01xxxxxx
             * vsp = vsp – (xxxxxx << 2) - 4. Covers range 0x04-0x100 inclusive
             */
            new_fp -= SIX_RIGHT_MASK(instr1) - 4;
        }
        else if ((instr_index > 1) && (instr1 == 0x80) && (instr2 == 0x00))             { instr_index++; }
        else if ((instr_index > 1) && (instr1 & 0xf0) == 0x80)                          { instr_index++; }
        else if ((instr1 == 0x9d))                                                      { }
        else if ((instr1 == 0x9f))                                                      { }
        else if ((instr1 & 0xf0) == 0x90)                                               { }
        else if ((instr1 & 0xf8) == 0xa0)                                               { }
        else if ((instr1 & 0xf8) == 0xa8)                                               { }
        else if ((instr1 == 0xb0))                                                      { }
        else if ((instr_index > 1) && (instr1 == 0xb1) && (instr2 == 0x00))             { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xb1) && ((instr2 & 0xf0) == 0x00))    { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xb1))                                 { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xb2))
        {
            /**
             * @brief 10110010 uleb128
             * vsp = vsp + 0x204+ (uleb128 << 2) (for vsp increments of 0x104-0x200, use 00xxxxxx twice)
             */
            new_fp += 0x204 + (instr2 << 2);
            instr_index++;
        }
        else if ((instr_index > 1) && (instr1 == 0xb3))                                 { instr_index++; }
        else if ((instr1 == 0xb4))                                                      { }
        else if ((instr1 & 0xf8) == 0xb8)                                               { }
        else if ((instr1 & 0xf8) == 0xc0)                                               { }
        else if ((instr_index > 1) && (instr1 == 0xc6))                                 { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xc7) && (instr2 == 0x00))             { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xc7) && ((instr2 & 0xf0) == 0x00))    { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xc7))                                 { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xc8))                                 { instr_index++; }
        else if ((instr_index > 1) && (instr1 == 0xc9))                                 { instr_index++; }
        else if ((instr1 & 0xf8) == 0xc8)                                               { }
        else if ((instr1 & 0xf8) == 0xd0)                                               { }
        else if ((instr1 & 0xc0) == 0xc0)                                               { }
        else {}

        instr_index++;
    }

    return new_fp;
}


/**
 * @brief This function fetches an unwind instruction given in parameter.
 * @param[in] entry
 * @param[in] word
 * @param[in] offset the offset in the section
 */
uint32_t __attribute__((pure)) GetInstruction(const uint32_t entry, const uint32_t word, const uint8_t offset)
{
    uint32_t instr = 0x0;
    uint32_t new_word = word;

    // Calculate which word we need to access based on the offset
    if (offset >= 4)
    {
        // Fetch a new word from memory using GetWord when offset crosses word boundaries
        new_word = GetWord((uint8_t *)entry, 4 * (offset / 4));
    }
    
    instr = (new_word >> (24 - (offset % 4) * 8)) & 0xff;

    return instr;
}

/**
 * @brief This function decodes an entry in the Exidx (Exception Index) Table.
 * @warning This function is annotated with the `pure` attribute for
 * performance purpose, it must remain pure if it's changed
 * @param[in] section
 * @param[in] offset
 * @return The exidx entry in both raw and decoded forms (exidxEntry_t)
 */
exidxEntry_t __attribute__((pure)) GetExidxEntry(const uint8_t* const section, const uint32_t offset)
{
    exidxEntry_t entry;
    entry.exidx_fn = GetWord(section, offset);
    entry.exidx_entry = GetWord(section, offset+4);

    /**
     * (Section 6)
     * The first word contains a prel31 offset (see Relocations) to the start of a function, with bit 31 clear.
    */
    entry.decoded_fn = entry.exidx_fn & 0x80000000
        ? 0
        : DecodePrel31(entry.exidx_fn, (uint32_t) section + offset);

    entry.decoded_entry = entry.exidx_entry & 0x80000000
        ? entry.exidx_entry
        : DecodePrel31(entry.exidx_entry, (uint32_t) section + offset + 4);

    return entry;
}

/**
 * @brief This decodes an offset with prel31 encoding.
 * @warning This function is annotated with the `pure` attribute for
 * performance purpose, it must remain pure if it's changed
 * @param[in] word
 * @param[in] where
 * @return The prel31 offset of the word given in parameter
 */
uint32_t __attribute__((pure)) DecodePrel31(const uint32_t word, const uint32_t where)
{
    // Get the 31 weak bits of the word
    uint32_t offset = word & 0x7fffffff;

    // Use the 31st bit as sign bit
    if (offset & 0x40000000)
    {
        offset |= ~ (uint64_t) 0x7fffffff;
    }

    // Add the relocation
    offset += where;

    return offset;
}

/**
 * @brief This gets a word in a given offset of the section in parameter.
 * @warning This function is annotated with the `pure` attribute for
 * performance purpose, it must remain pure if it's changed
 * @param[in] section
 * @param[in] offset
 * @return The 32-bit word at the specified offset.
 */
uint32_t __attribute__((pure)) GetWord(const uint8_t* const section, const uint32_t offset)
{
    uint8_t const* const field = section + offset;

    return (
        field[0]
        | (field[1] << 8)
        | (field[2] << 16)
        | (field[3] << 24)
    );
}

/*************************** Functions Definitions ***************************/

/**
 *  @fn     InitFDIR(void)
 *  @brief  Function that initialises the FDIR
 */
void InitFDIR(void)
{
    // Enables memory management, bus fault and usage fault exceptions
    CMSIS_SHCSR |= CMSIS_SHCSR_MEMFAULTENA_Msk | CMSIS_SHCSR_BUSFAULTENA_Msk | CMSIS_SHCSR_USGFAULTENA_Msk;
    CMSIS_CCR |= CMSIS_CCR_DIV_0_TRP_Msk;
}

/*************************** Interruption Handlers ***************************/

/**
 * @brief This function handles Hard fault interrupt.
 */
void __attribute__((naked)) HardFault_Handler(void)
{
    SaveRegisters(&debug_info);
    UnwindStack(&debug_info);

    while (1);
}

/**
 * @brief This function handles Memory management fault.
 */
void __attribute__((naked)) MemManage_Handler(void)
{
    SaveRegisters(&debug_info);
    UnwindStack(&debug_info);

    while (1);
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void __attribute__((naked)) BusFault_Handler(void)
{
    SaveRegisters(&debug_info);
    UnwindStack(&debug_info);

    while (1);
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void __attribute((naked)) UsageFault_Handler(void)
{
    SaveRegisters(&debug_info);
    UnwindStack(&debug_info);

    while (1);
}
