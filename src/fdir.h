/**
 * @file    fdir.h
 * @author  Theo Bessel
 * @brief   Header file for FDIR
 * @date    25/10/2024
 *
 * @copyright Copyright (c) Theo Bessel 2024
 */

#ifndef FDIR_H
#define FDIR_H

/******************************* Include Files *******************************/

#include <stdint.h>

/***************************** Macros Definitions ****************************/

//#define CALL_STACK_SIZE 20u

/***************************** Types Definitions *****************************/




/*************************** Variables Declarations **************************/

/*************************** Functions Declarations **************************/

void Unwind_Stack(void);

void Trigger_FDIR(void);

#endif /* FDIR_H */
