/*
 * STM32F407VGTx_FLASH.ld
 * STM32F407VG链接脚本
 */

/* 内存布局 */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
  CCMRAM (rw)     : ORIGIN = 0x10000000, LENGTH = 64K
}

/* 栈大小 */
_Min_Heap_Size = 0x200;   /* 512字节 */
_Min_Stack_Size = 0x400;  /* 1K字节 */

/* 入口点 */
ENTRY(Reset_Handler)

/* 段定义 */
SECTIONS
{
  /* 中断向量表 */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* 程序代码 */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text段 */
    *(.text*)          /* .text*段 */
    *(.glue_7)         /* ARM到Thumb跳转代码 */
    *(.glue_7t)        /* Thumb到ARM跳转代码 */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* 代码段结束地址 */
  } >FLASH

  /* 常量数据 */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata段 */
    *(.rodata*)        /* .rodata*段 */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* 初始化数据加载地址 */
  _sidata = LOADADDR(.data);

  /* 初始化数据段 */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* 数据段开始地址 */
    *(.data)           /* .data段 */
    *(.data*)          /* .data*段 */

    . = ALIGN(4);
    _edata = .;        /* 数据段结束地址 */
  } >RAM AT> FLASH

  /* 未初始化数据段 */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;         /* BSS段开始地址 */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* BSS段结束地址 */
    __bss_end__ = _ebss;
  } >RAM

  /* 用户堆栈 */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* 栈顶地址 */
  _estack = ORIGIN(RAM) + LENGTH(RAM);

  /* 移除调试信息 */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}