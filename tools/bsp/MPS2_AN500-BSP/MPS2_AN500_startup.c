/**
 * @file    MPS2_AN500_startup.c
 * @author  Théo Bessel
 * @brief   Startup File for MPS2-AN500
 * @date    02/11/2024
 *
 * @copyright Copyright (c) Théo Bessel 2024
 */

/******************************* Include Files *******************************/

#include <stdint.h>

/***************************** Macros Definitions ****************************/

/*************************** Functions Declarations **************************/

/**
 * @brief Main function
 * @return int
 */
extern int main(void);

/**
 * @brief System Initialisation
 */
// extern void SystemInit(void);

/**
 * @brief Default Handler
 */
void Default_Handler(void);

/**
 * @brief Cortex-M exceptions
 */
 void Reset_Handler(void);
 void NMI_Handler(void)             __attribute__ ((alias("Default_Handler")));
 extern void HardFault_Handler(void);
 extern void MemManage_Handler(void);
 extern void BusFault_Handler(void);
 extern void UsageFault_Handler(void);
 void SVCall_Handler(void)          __attribute__ ((alias("Default_Handler")));
 void DebugMonitor_Handler(void)    __attribute__ ((alias("Default_Handler")));
 void PendSV_Handler(void)          __attribute__ ((alias("Default_Handler")));
 void SysTick_Handler(void)         __attribute__ ((alias("Default_Handler")));

/*************************** Variables Definitions ***************************/

extern uint32_t __stack_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

/**
 * @brief ISR Vector Table
 */
uint32_t isr_vectors[] __attribute__((section(".isr_vector"))) = {
  (uint32_t) &__stack_end__,
  (uint32_t) &Reset_Handler,
  (uint32_t) &NMI_Handler,
  (uint32_t) &HardFault_Handler,
  (uint32_t) &MemManage_Handler,
  (uint32_t) &BusFault_Handler,
  (uint32_t) &UsageFault_Handler,
  0,
  0,
  0,
  0,
  (uint32_t) &SVCall_Handler,
  (uint32_t) &DebugMonitor_Handler,
  0,
  (uint32_t) &PendSV_Handler,
  (uint32_t) &SysTick_Handler,
};

/*************************** Functions Definitions ***************************/

/**
 * @brief Reset Handler, called on reset.
 */
void Reset_Handler(void) {
  // Initialisation of the system
  // SystemInit();

  // Variable Initialisation
  uint32_t bss_size = (uint32_t) &__bss_end__ - (uint32_t) &__bss_start__;
  uint8_t *bss_ptr = (uint8_t *) &__bss_start__;

  // Initialise the .bss section with zero
  for (uint32_t i = 0; i < bss_size; i++) {
    *bss_ptr++ = 0;
  }

  // Finally goes to main
  main();
}

/**
 * @brief Default Handler
 */
void Default_Handler(void) {
  while (1) {}
}
