/*
 * linker script for STM32F4xx with GNU ld
 * bernard.xiong 2009-10-14
 * flybreak      2018-11-19  Add support for RAM2
 */

/* Program Entry, set to mark it as "used" and avoid gc */
MEMORY
{
    CODE (rx) : ORIGIN = 0x08000000, LENGTH = 1024k /* 1024KB flash */
    RAM1 (rw) : ORIGIN = 0x20000000, LENGTH =  128k /* 128K sram */
    RAM2 (rw) : ORIGIN = 0x10000000, LENGTH =   64k /* 64K sram */
}
ENTRY(Reset_Handler)
_system_stack_size = 0x8000;

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        _stext = .;
        KEEP(*(.isr_vector))            /* Startup code */

        . = ALIGN(4);
        *(.text)                        /* remaining code */
        *(.text.*)                      /* remaining code */
        *(.rodata)                      /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t*)

        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;

        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;

        /* section information for utest */
        . = ALIGN(4);
        __rt_utest_tc_tab_start = .;
        KEEP(*(uTestTab))
        __rt_utest_tc_tab_end = .;
        
        /* section information for initial. */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;

        . = ALIGN(4);

        PROVIDE(__ctors_start__ = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE(__ctors_end__ = .);

        . = ALIGN(4);

        _etext = .;
    } > CODE = 0

    /* .ARM.exidx is sorted, so has to go in its own output section.  */
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)

        /* This is used by the startup in order to initialize the .data secion */
        _sidata = .;
    } > CODE
    __exidx_end = .;

    /* .data section which is used for initialized data */

    .data : AT (_sidata)
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;

        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)

        PROVIDE(__dtors_start__ = .);
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
        PROVIDE(__dtors_end__ = .);

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _edata = . ;
    } >RAM2

    .stack :
    {
        . = ALIGN(4);
        _sstack = .;
        . = . + _system_stack_size;
        . = ALIGN(4);
        _estack = .;
    } >RAM2

    __bss1_start = .;
    .bss1 :
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss1 = .;

        *dev_communication_interface.c.o (.bss .bss.* COMMON)
        *dev_log_flash.c.o (.bss .bss.* COMMON)
        *dev_motion_control_module_1.o (.bss .bss.* COMMON)
        *dev_motion_control_module_2.o (.bss .bss.* COMMON)
        *dev_motion_control_module_3.o (.bss .bss.* COMMON)
        *dev_motion_control_module_4.o (.bss .bss.* COMMON)
        *dev_motion_control_module_5.o (.bss .bss.* COMMON)
        *dev_parameter_eeprom.o (.bss .bss.* COMMON)
        *func_common.o (.bss .bss.* COMMON)
        *func1_motion_control_1.o (.bss .bss.* COMMON)
        *func2_motion_control_2.o (.bss .bss.* COMMON)
        *func3_motion_control_3.o (.bss .bss.* COMMON)
        *func4_motion_control_4.o (.bss .bss.* COMMON)
        *func5_motion_control_5.o (.bss .bss.* COMMON)
        *func6_miscellaneous.o (.bss .bss.* COMMON)
        *drv_pwm_pin.o (.bss .bss.* COMMON)
        *drv_stepmotor.o (.bss .bss.* COMMON)
        *drv_motion_control_components.o (.bss .bss.* COMMON)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _ebss1 = . ;

    } > RAM2
    __bss1_end = .;

    __bss2_start = .;
    .bss2 :
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss2 = .;

        EXCLUDE_FILE (*dev_communication_interface.c.o
                      *dev_log_flash.c.o
                      *dev_motion_control_module_1.o
                      *dev_motion_control_module_2.o
                      *dev_motion_control_module_3.o
                      *dev_motion_control_module_4.o
                      *dev_motion_control_module_5.o
                      *dev_parameter_eeprom.o
                      *func_common.o
                      *func1_motion_control_1.o
                      *func2_motion_control_2.o
                      *func3_motion_control_3.o
                      *func4_motion_control_4.o
                      *func5_motion_control_5.o
                      *func6_miscellaneous.o
                      *drv_pwm_pin.o
                      *drv_stepmotor.o
                      *drv_motion_control_components.o) *(.bss .bss.* COMMON)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _ebss2 = . ;

        /**(.bss.init)*/
    } > RAM1
    __bss2_end = .;

    _end = .;
    end = .;

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
     * Symbols in the DWARF debugging sections are relative to the beginning
     * of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
