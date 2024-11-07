/**
 * @file    stacktrace.c
 * @author  Theo Bessel
 * @brief   Source file for stacktrace
 * @date    20/07/2024
 *
 * @copyright Copyright (c) Theo Bessel 2024
 */

/******************************* Include Files *******************************/

#include <stdio.h>
#include "stacktrace.h"

/***************************** Macros Definitions ****************************/

#define EXIDX_CANTUNWIND 0x1

// Personality routine indexes
#define SU16 0x0
#define LU16 0x1
#define LU32 0x2

/*************************** Functions Declarations **************************/

/*************************** Variables Definitions ***************************/

/*************************** Functions Definitions ***************************/

/**
 * @brief This gets a word in a given offset of the section in parameter.
 */
void GetWord(uint8_t* section, uint32_t offset, uint32_t *word)
{
    uint8_t *field = section + offset;

    *word = field[0] | (field[1] << 8) | (field[2] << 16) | (field[3] << 24);
}

/**
 * @brief This decodes an offset with prel31 encoding.
 */
uint32_t DecodePrel31(uint32_t word, uint32_t where)
{
    uint32_t offset;

    // Get the 31 weak bits of the word
    offset = word & 0x7fffffff;

    // Use the 31st bit as sign bit
    if (offset & 0x40000000)
    {
        offset |= ~ (uint64_t) 0x7fffffff;
    }

    return offset + where;
}

/**
 * @brief This function decodes an entry in the Exidx (Exception Index) Table.
 */
void GetExidxEntry(uint8_t* section, uint32_t offset, uint32_t *exidx_fn, uint32_t *exidx_entry, uint32_t *fn)
{
    GetWord(section, offset, exidx_fn);
    GetWord(section, offset+4, exidx_entry);

    /**
     * (Section 6)
     * The first word contains a prel31 offset (see Relocations) to the start of a function, with bit 31 clear.
    */
    if (!(*exidx_fn & 0x80000000))
    {
        *fn = DecodePrel31(*exidx_fn, (uint32_t)section + offset);
    }
    else
    {
        // Error
        *fn = 0;
    }
}

/**
 * @brief This function fetches an unwind instruction given in parameter.
 */
uint32_t FetchSingleInstruction(uint32_t entry, uint32_t *word_ptr, uint8_t i)
{
    uint32_t instruction = 0x0;

    if ((i % 4) == 0)
    {
        GetWord((uint8_t *) entry, 4, word_ptr);
        instruction = (*word_ptr >> 24) & 0xff;
    }
    else
    {
        instruction = (*word_ptr >> ((i % 4 - 1) * 8)) & 0xff;
    }

    return instruction;
}

/**
 * @brief This function decodes unwind instructions based on ARM EHABI standard.
 */
void FetchMultipleInstructions(uint32_t entry, uint32_t *word, uint8_t n, uint32_t *vsp, generalRegisters_t *generalRegisters)
{
    uint32_t instruction = 0x0;
    uint32_t instruction2 = 0x0;
    uint32_t word_ptr = *word;

    uint8_t i = n;

    while (i > 0)
    {
        instruction = FetchSingleInstruction(entry, &word_ptr, i);

        if (i > 1)
        {
            instruction2 = FetchSingleInstruction(entry, &word_ptr, i - 1);
        }
        // Implemented : Mandatory for Cortex M
        if ((instruction & 0xc0) == 0x00) // 00xxxxxx
        {
            // printf("      0x%02lx      vsp = vsp + %d\n", instruction, (uint8_t) ((instruction & 0x3f) << 2) + 4);
            *vsp += ((instruction & 0x3f) << 2) + 4;
        }
        // Implemented : Mandatory for Cortex M
        else if ((instruction & 0xc0) == 0x40) // 01xxxxxx
        {
            // printf("      0x%02lx      vsp = vsp - %d\n", instruction, (uint8_t) ((instruction & 0x3f) << 2) - 4);
            *vsp -= ((instruction & 0x3f) << 2) - 4;
        }
        // Not Implemented : Optional for Cortex M
        else if ((i > 1) && (instruction == 0x80) && (instruction2 == 0x00)) // 10000000 00000000
        {
            // printf("      0x%02lx 0x%02lx Refuse to unwind\n", instruction, instruction2);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((i > 1) && (instruction & 0xf0) == 0x80) // 1000iiii iiiiiiii
        {
            // printf("      0x%02lx 0x%02lx pop {", instruction, instruction2);

            uint16_t mask = (((instruction & 0x0f) << 8) | instruction2);
            uint8_t first = 0x1;

            for (uint8_t j = 0; j < 12; j++)
            {
                if (mask & (1 << j))
                {
                    if (first)
                    {
                        first = 0x0;
                    }
                    else
                    {
                        // printf (", ");
                    }
                    // printf("r%d", j + 4);
                }
            }
            // printf("}\n");
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0x9d) // 10011101
        {
            // Reserved
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0x9f) // 10011111
        {
            // Reserved
        }
        // Implemented : Mandatory for Cortex M
        else if ((instruction & 0xf0) == 0x90) // 1001nnnn
        {
            // printf("      0x%02lx      vsp = r%d\n", instruction, (uint8_t) (instruction & 0x0f));
            //  Get the value of the register r[nnn] in the vsp
            *vsp = generalRegisters->r[instruction & 0x0f];
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xf8) == 0xa0) // 10100nnn
        {
            uint8_t end = 4 + (instruction & 0x07);
            uint8_t first = 0x1;

            // printf("      0x%02lx      pop {", instruction);

            for (uint8_t j = 4; j <= end; j++)
            {
                if (first)
                {
                    first = 0x0;
                }
                else
                {
                    // printf (", ");
                }
                // printf("r%d", j);
            }

            // printf("}\n");
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xf8) == 0xa8) // 10101nnn
        {
            uint8_t end = 4 + (instruction & 0x07);
            uint8_t first = 0x1;

            // printf("      0x%02lx      pop {", instruction);

            for (uint8_t j = 4; j <= end; j++)
            {
                if (first)
                {
                    first = 0x0;
                }
                else
                {
                    // printf (", ");
                }
                // printf("r%d", j);
            }

            // printf(", r14}\n");
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0xb0) // 10110000
        {
            // printf("      0x%02lx      finish\n", instruction);
        }
        // Not Implemented : Optional for Cortex M
        else if ((i > 1) && (instruction == 0xb1) && (instruction2 == 0x00)) // 10110001 00000000
        {
            // printf("      0x%02lx 0x%02lx [spare]\n", instruction, instruction2);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((i > 1) && (instruction == 0xb1) && ((instruction2 & 0xf0) == 0x00)) // 10110001 0000iiii
        {
            // printf("      0x%02lx 0x%02lx pop {", instruction, instruction2);

            uint8_t first = 0x1;

            for (uint8_t j = 0; j < 4; j++)
            {
                if (instruction2 & (1 << j))
                {
                    if (first)
                    {
                        first = 0x0;
                    }
                    else
                    {
                        // printf (", ");
                    }
                    // printf("r%d", j + 4);
                }
            }
            // printf("}\n");
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((i > 1) && (instruction == 0xb1)) // 10110001 xxxxyyyy
        {
            // printf("      0x%02lx [spare]\n", instruction);
            i--;
        }
        // Implemented : Mandatory for Cortex M
        else if ((i > 1) && (instruction == 0xb2)) // 10110010 uleb128
        {
            // printf("      0x%02lx 0x%02lx vsp = vsp + %d\n", instruction, instruction2, (uint8_t) (0x204 + (instruction2 << 2)));
            *vsp += 0x204 + (instruction2 << 2);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((i > 1) && (instruction == 0xb3)) // 10110011 sssscccc
        {
            // uint8_t first = instruction2 & 0xf0;
            // uint8_t last = instruction2 & 0x0f;
            // printf("      0x%02lx 0x%02lx pop {d%d-d%d}\n", instruction, instruction2, first, first + last);
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0xb4) // 10110100
        {
            // printf("      0x%02lx      pop {ra_auth_code}\n", instruction);
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xfc) == 0xb4) // 101101nn
        {
            // printf("      0x%02lx      [spare]\n", instruction);
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xf8) == 0xb8) // 10111nnn
        {
            // printf("      0x%02lx pop {d8-d%d}\n", instruction, (uint8_t) (8 + (instruction & 0x07)));
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xf8) == 0xc0) // 11000nnn
        {
            // printf("      0x%02lx pop {wr10-wr%d}\n", instruction, (uint8_t) (10 + (instruction & 0x07)));
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0xc6) // 11000110 sssscccc
        {
            // uint8_t first = instruction2 & 0xf0;
            // uint8_t last = instruction2 & 0x0f;
            // printf("      0x%02lx 0x%02lx pop {wr%d-wr%d}\n", instruction, instruction2, first, first + last);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction == 0xc7) && (instruction2 == 0x00)) // 11000111 00000000
        {
            // printf("      0x%02lx 0x%02lx [spare]\n", instruction, instruction2);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction == 0xc7) && ((instruction2 & 0xf0) == 0x00)) // 11000111 0000iiii
        {
            // printf("      0x%02lx 0x%02lx pop {", instruction, instruction2);

            uint8_t first = 0x1;

            for (uint8_t j = 0; j < 4; j++)
            {
                if (instruction2 & (1 << j))
                {
                    if (first)
                    {
                        first = 0x0;
                    }
                    else
                    {
                        // printf (", ");
                    }
                    // printf("wcgr%d", j + 4);
                }
            }
            // printf("}\n");
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0xc7) // 11000111 xxxxyyyy
        {
            // printf("      0x%02lx 0x%02lx [spare]\n", instruction, instruction2);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0xc8) // 11001000 sssscccc
        {
            // uint8_t first = instruction2 & 0xf0;
            // uint8_t last = instruction2 & 0x0f;
            // printf("      0x%02lx 0x%02lx pop {d%d-d%d}\n", instruction, instruction2, 16 + first, 16 + first + last);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if (instruction == 0xc9) // 11001001 sssscccc
        {
            // uint8_t first = instruction2 & 0xf0;
            // uint8_t last = instruction2 & 0x0f;
            // printf("      0x%02lx 0x%02lx pop {d%d-d%d}\n", instruction, instruction2, first, first + last);
            i--;
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xf8) == 0xc8) // 11001yyy
        {
            // printf("      0x%02lx      [spare]\n", instruction);
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xf8) == 0xd0) // 11010nnn
        {
            // printf("      0x%02lx pop {d8-d%d}\n", instruction, (uint8_t) (8 + (instruction & 0x07)));
        }
        // Not Implemented : Optional for Cortex M
        else if ((instruction & 0xc0) == 0xc0) // 11xxxyyy
        {
            // printf("      0x%02lx      [spare]\n", instruction);
        }
        // Not Implemented : Optional for Cortex M
        else
        {
            // printf("      0x%02lx\n", instruction);
            }

        i--;
    }
}

/**
 * @brief This function decodes the entries in Extab (Exception Table) Table
 */
void DecodeCompactModelEntry(uint32_t entry, uint32_t decoded_exidx_entry, uint32_t *vsp, generalRegisters_t *generalRegisters) {
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
     */
    uint8_t personality_index = (uint8_t) ((entry >> 24) & 0xf);
    //printf("      compact model index = %d\n", personality_index);

    uint32_t n = 0x0;

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

    switch (personality_index)
    {
        /**
            * (Section 10.2)
            * Short 3 unwinding instructions in bits 16-23, 8-15, and 0-7 of the first word. Any of the instructions can be Finish.
            */
        case SU16:
            FetchMultipleInstructions(decoded_exidx_entry, &word, 3, vsp, generalRegisters);
            break;
        /**
            * (Section 10.2)
            * Long Bits 16-23 contain a count N of the number of additional 4-byte words that contain unwinding instructions.
            * The sequence of unwinding instructions is packed into bits 8-15, 0-7, and the following N words.
            * Spare trailing bytes in the last word should be filled with Finish instructions.
            */
        case LU16:
            n = (word >> 16) & 0xff;
            FetchMultipleInstructions(decoded_exidx_entry, &word, 2 + 4 * n, vsp, generalRegisters);
            break;
        case LU32:
            n = (word >> 16) & 0xff;
            FetchMultipleInstructions(decoded_exidx_entry, &word, 2 + 4 * n, vsp, generalRegisters);
            break;
        default:
            // error
            break;
    }
}

/**
 * @brief This function use the personnal routine to unwind one frame of the stack.
 */
uint32_t GetFunctionPersonnalRoutine(uint32_t fn, uint32_t *vsp, generalRegisters_t *generalRegisters)
{

    //printf("    Personnal routine for function associated to p = %p is :\n\n", (void *) fn);

    uint8_t entries = (&__exidx_end - &__exidx_start) / 2;

    ////printf("    Unwind section contains %d entries\n\n", entries);

    uint32_t return_code = 0x0;

    uint32_t exidx_fn = 0x0;
    uint32_t exidx_entry = 0x0;

    uint32_t decoded_exidx_fn = 0x0;
    uint32_t decoded_exidx_entry = 0x0;

    uint8_t i = entries;

    do {
        i--;
        GetExidxEntry((uint8_t *)&__exidx_start, 8*i, &exidx_fn, &exidx_entry, &decoded_exidx_fn);
    } while (i > 0 && decoded_exidx_fn > fn);

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
    if (exidx_entry == EXIDX_CANTUNWIND)     // Special pattern 0x1 EXIDX_CANTUNWIND
    {
        decoded_exidx_entry = exidx_entry;

        //printf("    %p : %p [cantunwind]\n", (void *) decoded_exidx_fn, (void *) decoded_exidx_entry);

        return_code = 0xffffffff;
    }
    else if (!(exidx_entry & 0x80000000))    // Bit 31 is clear
    {
        decoded_exidx_entry = DecodePrel31(exidx_entry, (uint32_t)&__exidx_start + 8*i + 4);

        //printf("    %p : @%p\n", (void *) decoded_exidx_fn, (void *) decoded_exidx_entry);

        uint32_t extab_entry = 0x0;
        GetWord((uint8_t *) decoded_exidx_entry, 0, &extab_entry);

        if (extab_entry & 0x80000000)
        {
            DecodeCompactModelEntry(extab_entry, decoded_exidx_entry, vsp, generalRegisters);
        }
    }
    else                                     // Bit 31 set --> compact model
    {
        decoded_exidx_entry = exidx_entry;

        //printf("    %p : @%p\n", (void *) decoded_exidx_fn, (void *) decoded_exidx_entry);

        DecodeCompactModelEntry(exidx_entry, decoded_exidx_entry, vsp, generalRegisters);
    }

    //printf("\n");

    return return_code;
}

void PrintDebugInfo(debugInfo_t* debugInfo)
{
    printf("\n==================[Debug info]==================\n\n");
    printf("r0   = 0x%08lx\n", debugInfo->registers->r[0]);
    printf("r1   = 0x%08lx\n", debugInfo->registers->r[1]);
    printf("r2   = 0x%08lx\n", debugInfo->registers->r[2]);
    printf("r3   = 0x%08lx\n", debugInfo->registers->r[3]);
    printf("r12  = 0x%08lx\n", debugInfo->registers->r12);
    printf("lr   = 0x%08lx\n", debugInfo->registers->lr);
    printf("pc   = 0x%08lx\n", debugInfo->registers->pc);
    printf("xpsr = 0x%08lx\n", debugInfo->registers->xpsr);
    printf("sp   = 0x%08lx\n", debugInfo->sp);
    printf("cfsr = 0x%08lx\n", debugInfo->cfsr);
    printf("hfsr = 0x%08lx\n", debugInfo->hfsr);

    printf("\n");

    for (uint8_t i = 0; i < debugInfo->call_stack->size; i++)
    {
        printf("call_stack[%d] = 0x%lx\n", i, debugInfo->call_stack->calls[i]);
    }

    printf("\n================================================\n");
}

void PrintStackTrace(debugInfo_t *debugInfo, generalRegisters_t *generalRegisters)
{
    //printf("\n");
    //printf("==================[Stack trace]==================\n\n");


    //printf("exidx start: %p, end: %p, size = %d\n", (void *)&__exidx_start, (void *)&__exidx_end, (int)(&__exidx_end - &__exidx_start));
    //printf("extab start: %p, end: %p, size = %d\n\n", (void *)&__extab_start, (void *)&__extab_end, (int)(&__extab_end - &__extab_start));

    //printf("=================================================\n\n");

    callStack_t call_stack;

    uint8_t i = 0;

    uint32_t verification_code = 0x0;

    uint32_t ret = debugInfo->registers->pc;
    uint32_t vsp = 0x0;

    do
    {
        call_stack.calls[i] = ret;
        verification_code = GetFunctionPersonnalRoutine(ret, &vsp, generalRegisters);

        generalRegisters->r[7] = *((uint32_t *) vsp);
        ret = *((uint32_t *) (vsp + 4)) - 1;

        i++;
    } while (i < CALL_STACK_SIZE && verification_code != 0xffffffff);

    call_stack.size = i - 1;

    debugInfo->call_stack = &call_stack;

    PrintDebugInfo(debugInfo);

    //printf("=================================================\n\n");
}
