/**
 * Entry Point definition
 *
 * Main for the moment but will be replaced by Reset_Handler
 */
ENTRY(Reset_Handler)

/**
 * Memories definition
 *
 * Referencing : DAI0500B_cortex_m7_on_v2m_mps2.pdf
 */
MEMORY {
    /**
     * Section 3.2 : External ZBT Synchronous SRAM (SSRAM_1)
     *
     * This section describes the Fast Program SRAM in the CODE region.
     * This is interfaced to two external 32-bit ZBT SSRAM in parallel, forming a 64-bit ZBT SSRAM.
     * 8MB of memory space are allocated, but only 4MB is used (each ZBT SSRAM is 2MB).
     *
     * This memory space connects through the ITCM interface.
     *
     * Flash memory in CMSDK example system is 64KB.
     */
    ITCM    (rwx) : ORIGIN = 0x0,           LENGTH = 64K

    /**
     * Section 3.3 : External ZBT Synchronous SRAM (SSRAM2 & SSRAM3)
     *
     * The Fast ZBT SSRAM in SRAM region is set up as two external ZBT SSRAMs, connected to two
     * independent ZBT interfaces. In the 8MB memory region, 4MB of ZBT are available.
     *
     * This memory space connects through the DTCM interface.
     */
    DTCM    (rw)  : ORIGIN = 0x20000000,    LENGTH = 64K

    /**
     * Section 3.4 : External PSRAM
     *
     * A 16MB 16-bit PSRAM area is available and the memory map allocates the address-range
     * 0x60000000 - 0x60FFFFFF. This enables large test programs to be used, for example uClinux, in
     * the External RAM region of the Cortex-M memory space.
     * Note: Running code from SRAM region is slower than from CODE region because the internal
     * bus structure is not optimized for running programs from this region.
     *
     * This memory space connects through the AXI interface.
     */
    PSRAM   (rw)  : ORIGIN = 0x60000000,    LENGTH = 16M
}

__stack_end__       = ORIGIN(DTCM) + LENGTH(DTCM);
__Min_Heap_Size__   = 0x200;
__Min_Stack_Size__  = 0x400;

/**
 * Sections definition
 */
SECTIONS {
    /**
     *  ITCM part :
     *  - .isr_vector
     *  - .text
     *  - .ARM.exidx
     */

    .isr_vector :
    {
        . = ALIGN(4);
        __isr_vector_start__ = .;
        KEEP(*(.isr_vector))
        . = ALIGN(4);
        __isr_vector_end__ = .;
    } > ITCM

    .text :
    {
        . = ALIGN(4);
        __text_start__ = .;
        *(.text)
        *(.text*)
        . = ALIGN(4);
        __text_end__ = .;
    } > ITCM

    .ARM.exidx :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        . = ALIGN(4);
        __exidx_end = .;
    } > ITCM

    /**
     *  DTCM part :
     *  - .rodata
     *  - .ARM.extab
     *  - .data
     *  - .bss
     *  - .heap
     *  - .stack
     */

    .rodata :
    {
        . = ALIGN(4);
        __rodata_start__ = .;
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        __rodata_end__ = .;
    } > DTCM

    .ARM.extab :
    {
        . = ALIGN(4);
        __extab_start = .;
        *(.ARM.extab*)
        . = ALIGN(4);
        __extab_end = .;
    } > DTCM

    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        __data_end__ = .;
    } > DTCM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        __bss_end__ = .;
    } > DTCM

    .heap_stack :
    {
        . = ALIGN(8);
        PROVIDE (end = .);
        PROVIDE (_end = .);
        . = . + __Min_Heap_Size__;
        . = . + __Min_Stack_Size__;
        . = ALIGN(8);
    } > DTCM
}
