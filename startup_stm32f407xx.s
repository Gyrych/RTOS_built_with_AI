/**
 * @file startup_stm32f407xx.s
 * @brief STM32F407启动代码
 */

.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

/* 栈顶地址 */
.word _estack

/* 中断向量表 */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word _estack                    /* 栈顶指针 */
  .word Reset_Handler              /* 复位中断 */
  .word NMI_Handler                /* NMI中断 */
  .word HardFault_Handler          /* 硬错误中断 */
  .word MemManage_Handler          /* 内存管理错误 */
  .word BusFault_Handler           /* 总线错误 */
  .word UsageFault_Handler         /* 使用错误 */
  .word 0                          /* 保留 */
  .word 0                          /* 保留 */
  .word 0                          /* 保留 */
  .word 0                          /* 保留 */
  .word SVC_Handler                /* SVC中断 */
  .word DebugMon_Handler           /* 调试监控 */
  .word 0                          /* 保留 */
  .word PendSV_Handler             /* PendSV中断 */
  .word SysTick_Handler            /* SysTick中断 */

  /* 外部中断 */
  .word WWDG_IRQHandler            /* 窗口看门狗 */
  .word PVD_IRQHandler             /* PVD中断 */
  .word TAMP_STAMP_IRQHandler      /* 篡改和时间戳 */
  .word RTC_WKUP_IRQHandler        /* RTC唤醒 */
  .word FLASH_IRQHandler           /* FLASH中断 */
  .word RCC_IRQHandler             /* RCC中断 */
  .word EXTI0_IRQHandler           /* EXTI0中断 */
  .word EXTI1_IRQHandler           /* EXTI1中断 */
  .word EXTI2_IRQHandler           /* EXTI2中断 */
  .word EXTI3_IRQHandler           /* EXTI3中断 */
  .word EXTI4_IRQHandler           /* EXTI4中断 */
  .word DMA1_Stream0_IRQHandler    /* DMA1 Stream0 */
  .word DMA1_Stream1_IRQHandler    /* DMA1 Stream1 */
  .word DMA1_Stream2_IRQHandler    /* DMA1 Stream2 */
  .word DMA1_Stream3_IRQHandler    /* DMA1 Stream3 */
  .word DMA1_Stream4_IRQHandler    /* DMA1 Stream4 */
  .word DMA1_Stream5_IRQHandler    /* DMA1 Stream5 */
  .word DMA1_Stream6_IRQHandler    /* DMA1 Stream6 */
  .word ADC_IRQHandler             /* ADC1, ADC2, ADC3 */
  .word CAN1_TX_IRQHandler         /* CAN1 TX */
  .word CAN1_RX0_IRQHandler        /* CAN1 RX0 */
  .word CAN1_RX1_IRQHandler        /* CAN1 RX1 */
  .word CAN1_SCE_IRQHandler        /* CAN1 SCE */
  .word EXTI9_5_IRQHandler         /* EXTI9-5 */
  .word TIM1_BRK_TIM9_IRQHandler   /* TIM1 Break, TIM9 */
  .word TIM1_UP_TIM10_IRQHandler   /* TIM1 Update, TIM10 */
  .word TIM1_TRG_COM_TIM11_IRQHandler /* TIM1 Trigger, TIM11 */
  .word TIM1_CC_IRQHandler         /* TIM1 Capture Compare */
  .word TIM2_IRQHandler            /* TIM2 */
  .word TIM3_IRQHandler            /* TIM3 */
  .word TIM4_IRQHandler            /* TIM4 */
  .word I2C1_EV_IRQHandler         /* I2C1 Event */
  .word I2C1_ER_IRQHandler         /* I2C1 Error */
  .word I2C2_EV_IRQHandler         /* I2C2 Event */
  .word I2C2_ER_IRQHandler         /* I2C2 Error */
  .word SPI1_IRQHandler            /* SPI1 */
  .word SPI2_IRQHandler            /* SPI2 */
  .word USART1_IRQHandler          /* USART1 */
  .word USART2_IRQHandler          /* USART2 */
  .word USART3_IRQHandler          /* USART3 */
  .word EXTI15_10_IRQHandler       /* EXTI15-10 */
  .word RTC_Alarm_IRQHandler       /* RTC Alarm */
  .word OTG_FS_WKUP_IRQHandler     /* USB OTG FS Wakeup */
  .word TIM8_BRK_TIM12_IRQHandler  /* TIM8 Break, TIM12 */
  .word TIM8_UP_TIM13_IRQHandler   /* TIM8 Update, TIM13 */
  .word TIM8_TRG_COM_TIM14_IRQHandler /* TIM8 Trigger, TIM14 */
  .word TIM8_CC_IRQHandler         /* TIM8 Capture Compare */
  .word DMA1_Stream7_IRQHandler    /* DMA1 Stream7 */
  .word FSMC_IRQHandler            /* FSMC */
  .word SDIO_IRQHandler            /* SDIO */
  .word TIM5_IRQHandler            /* TIM5 */
  .word SPI3_IRQHandler            /* SPI3 */
  .word UART4_IRQHandler           /* UART4 */
  .word UART5_IRQHandler           /* UART5 */
  .word TIM6_DAC_IRQHandler        /* TIM6, DAC1, DAC2 */
  .word TIM7_IRQHandler            /* TIM7 */
  .word DMA2_Stream0_IRQHandler    /* DMA2 Stream0 */
  .word DMA2_Stream1_IRQHandler    /* DMA2 Stream1 */
  .word DMA2_Stream2_IRQHandler    /* DMA2 Stream2 */
  .word DMA2_Stream3_IRQHandler    /* DMA2 Stream3 */
  .word DMA2_Stream4_IRQHandler    /* DMA2 Stream4 */
  .word ETH_IRQHandler             /* Ethernet */
  .word ETH_WKUP_IRQHandler        /* Ethernet Wakeup */
  .word CAN2_TX_IRQHandler         /* CAN2 TX */
  .word CAN2_RX0_IRQHandler        /* CAN2 RX0 */
  .word CAN2_RX1_IRQHandler        /* CAN2 RX1 */
  .word CAN2_SCE_IRQHandler        /* CAN2 SCE */
  .word OTG_FS_IRQHandler          /* USB OTG FS */
  .word DMA2_Stream5_IRQHandler    /* DMA2 Stream5 */
  .word DMA2_Stream6_IRQHandler    /* DMA2 Stream6 */
  .word DMA2_Stream7_IRQHandler    /* DMA2 Stream7 */
  .word USART6_IRQHandler          /* USART6 */
  .word I2C3_EV_IRQHandler         /* I2C3 Event */
  .word I2C3_ER_IRQHandler         /* I2C3 Error */
  .word OTG_HS_EP1_OUT_IRQHandler  /* USB OTG HS EP1 OUT */
  .word OTG_HS_EP1_IN_IRQHandler   /* USB OTG HS EP1 IN */
  .word OTG_HS_WKUP_IRQHandler     /* USB OTG HS Wakeup */
  .word OTG_HS_IRQHandler          /* USB OTG HS */
  .word DCMI_IRQHandler            /* DCMI */
  .word CRYP_IRQHandler            /* CRYP Crypto */
  .word HASH_RNG_IRQHandler        /* Hash, RNG */
  .word FPU_IRQHandler             /* FPU */

/* 复位处理程序 */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack          /* 设置栈指针 */

/* 复制数据段到RAM */
  movs  r1, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r3, =_sidata
  ldr   r3, [r3, r1]
  str   r3, [r0, r1]
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr   r0, =_sdata
  ldr   r3, =_edata
  adds  r2, r0, r1
  cmp   r2, r3
  bcc   CopyDataInit
  ldr   r2, =_sbss
  b     LoopFillZerobss

/* 零初始化BSS段 */
FillZerobss:
  movs  r3, #0
  str   r3, [r2], #4

LoopFillZerobss:
  ldr   r3, = _ebss
  cmp   r2, r3
  bcc   FillZerobss

/* 调用系统初始化 */
  bl  SystemInit

/* 调用主函数 */
  bl  main
  
/* 无限循环 */
LoopForever:
    b LoopForever

.size Reset_Handler, .-Reset_Handler

/* 默认中断处理程序 */
.section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/* 弱定义的中断处理程序 */
.weak NMI_Handler
.thumb_set NMI_Handler,Default_Handler

.weak HardFault_Handler
.thumb_set HardFault_Handler,Default_Handler

.weak MemManage_Handler
.thumb_set MemManage_Handler,Default_Handler

.weak BusFault_Handler
.thumb_set BusFault_Handler,Default_Handler

.weak UsageFault_Handler
.thumb_set UsageFault_Handler,Default_Handler

.weak DebugMon_Handler
.thumb_set DebugMon_Handler,Default_Handler

.weak SysTick_Handler
.thumb_set SysTick_Handler,Default_Handler

/* 外部中断弱定义 */
.weak WWDG_IRQHandler
.thumb_set WWDG_IRQHandler,Default_Handler

.weak PVD_IRQHandler
.thumb_set PVD_IRQHandler,Default_Handler

.weak TAMP_STAMP_IRQHandler
.thumb_set TAMP_STAMP_IRQHandler,Default_Handler

.weak RTC_WKUP_IRQHandler
.thumb_set RTC_WKUP_IRQHandler,Default_Handler

.weak FLASH_IRQHandler
.thumb_set FLASH_IRQHandler,Default_Handler

.weak RCC_IRQHandler
.thumb_set RCC_IRQHandler,Default_Handler

.weak EXTI0_IRQHandler
.thumb_set EXTI0_IRQHandler,Default_Handler

.weak EXTI1_IRQHandler
.thumb_set EXTI1_IRQHandler,Default_Handler

.weak EXTI2_IRQHandler
.thumb_set EXTI2_IRQHandler,Default_Handler

.weak EXTI3_IRQHandler
.thumb_set EXTI3_IRQHandler,Default_Handler

.weak EXTI4_IRQHandler
.thumb_set EXTI4_IRQHandler,Default_Handler

.weak DMA1_Stream0_IRQHandler
.thumb_set DMA1_Stream0_IRQHandler,Default_Handler

.weak DMA1_Stream1_IRQHandler
.thumb_set DMA1_Stream1_IRQHandler,Default_Handler

.weak DMA1_Stream2_IRQHandler
.thumb_set DMA1_Stream2_IRQHandler,Default_Handler

.weak DMA1_Stream3_IRQHandler
.thumb_set DMA1_Stream3_IRQHandler,Default_Handler

.weak DMA1_Stream4_IRQHandler
.thumb_set DMA1_Stream4_IRQHandler,Default_Handler

.weak DMA1_Stream5_IRQHandler
.thumb_set DMA1_Stream5_IRQHandler,Default_Handler

.weak DMA1_Stream6_IRQHandler
.thumb_set DMA1_Stream6_IRQHandler,Default_Handler

.weak ADC_IRQHandler
.thumb_set ADC_IRQHandler,Default_Handler

.weak CAN1_TX_IRQHandler
.thumb_set CAN1_TX_IRQHandler,Default_Handler

.weak CAN1_RX0_IRQHandler
.thumb_set CAN1_RX0_IRQHandler,Default_Handler

.weak CAN1_RX1_IRQHandler
.thumb_set CAN1_RX1_IRQHandler,Default_Handler

.weak CAN1_SCE_IRQHandler
.thumb_set CAN1_SCE_IRQHandler,Default_Handler

.weak EXTI9_5_IRQHandler
.thumb_set EXTI9_5_IRQHandler,Default_Handler

.weak TIM1_BRK_TIM9_IRQHandler
.thumb_set TIM1_BRK_TIM9_IRQHandler,Default_Handler

.weak TIM1_UP_TIM10_IRQHandler
.thumb_set TIM1_UP_TIM10_IRQHandler,Default_Handler

.weak TIM1_TRG_COM_TIM11_IRQHandler
.thumb_set TIM1_TRG_COM_TIM11_IRQHandler,Default_Handler

.weak TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler,Default_Handler

/* TIM2中断处理程序 - 由RTOS使用 */
.weak TIM2_IRQHandler
.thumb_set TIM2_IRQHandler,Default_Handler

.weak TIM3_IRQHandler
.thumb_set TIM3_IRQHandler,Default_Handler

.weak TIM4_IRQHandler
.thumb_set TIM4_IRQHandler,Default_Handler

.weak I2C1_EV_IRQHandler
.thumb_set I2C1_EV_IRQHandler,Default_Handler

.weak I2C1_ER_IRQHandler
.thumb_set I2C1_ER_IRQHandler,Default_Handler

.weak I2C2_EV_IRQHandler
.thumb_set I2C2_EV_IRQHandler,Default_Handler

.weak I2C2_ER_IRQHandler
.thumb_set I2C2_ER_IRQHandler,Default_Handler

.weak SPI1_IRQHandler
.thumb_set SPI1_IRQHandler,Default_Handler

.weak SPI2_IRQHandler
.thumb_set SPI2_IRQHandler,Default_Handler

.weak USART1_IRQHandler
.thumb_set USART1_IRQHandler,Default_Handler

.weak USART2_IRQHandler
.thumb_set USART2_IRQHandler,Default_Handler

.weak USART3_IRQHandler
.thumb_set USART3_IRQHandler,Default_Handler

.weak EXTI15_10_IRQHandler
.thumb_set EXTI15_10_IRQHandler,Default_Handler

.weak RTC_Alarm_IRQHandler
.thumb_set RTC_Alarm_IRQHandler,Default_Handler

.weak OTG_FS_WKUP_IRQHandler
.thumb_set OTG_FS_WKUP_IRQHandler,Default_Handler

.weak TIM8_BRK_TIM12_IRQHandler
.thumb_set TIM8_BRK_TIM12_IRQHandler,Default_Handler

.weak TIM8_UP_TIM13_IRQHandler
.thumb_set TIM8_UP_TIM13_IRQHandler,Default_Handler

.weak TIM8_TRG_COM_TIM14_IRQHandler
.thumb_set TIM8_TRG_COM_TIM14_IRQHandler,Default_Handler

.weak TIM8_CC_IRQHandler
.thumb_set TIM8_CC_IRQHandler,Default_Handler

.weak DMA1_Stream7_IRQHandler
.thumb_set DMA1_Stream7_IRQHandler,Default_Handler

.weak FSMC_IRQHandler
.thumb_set FSMC_IRQHandler,Default_Handler

.weak SDIO_IRQHandler
.thumb_set SDIO_IRQHandler,Default_Handler

.weak TIM5_IRQHandler
.thumb_set TIM5_IRQHandler,Default_Handler

.weak SPI3_IRQHandler
.thumb_set SPI3_IRQHandler,Default_Handler

.weak UART4_IRQHandler
.thumb_set UART4_IRQHandler,Default_Handler

.weak UART5_IRQHandler
.thumb_set UART5_IRQHandler,Default_Handler

.weak TIM6_DAC_IRQHandler
.thumb_set TIM6_DAC_IRQHandler,Default_Handler

.weak TIM7_IRQHandler
.thumb_set TIM7_IRQHandler,Default_Handler

.weak DMA2_Stream0_IRQHandler
.thumb_set DMA2_Stream0_IRQHandler,Default_Handler

.weak DMA2_Stream1_IRQHandler
.thumb_set DMA2_Stream1_IRQHandler,Default_Handler

.weak DMA2_Stream2_IRQHandler
.thumb_set DMA2_Stream2_IRQHandler,Default_Handler

.weak DMA2_Stream3_IRQHandler
.thumb_set DMA2_Stream3_IRQHandler,Default_Handler

.weak DMA2_Stream4_IRQHandler
.thumb_set DMA2_Stream4_IRQHandler,Default_Handler

.weak ETH_IRQHandler
.thumb_set ETH_IRQHandler,Default_Handler

.weak ETH_WKUP_IRQHandler
.thumb_set ETH_WKUP_IRQHandler,Default_Handler

.weak CAN2_TX_IRQHandler
.thumb_set CAN2_TX_IRQHandler,Default_Handler

.weak CAN2_RX0_IRQHandler
.thumb_set CAN2_RX0_IRQHandler,Default_Handler

.weak CAN2_RX1_IRQHandler
.thumb_set CAN2_RX1_IRQHandler,Default_Handler

.weak CAN2_SCE_IRQHandler
.thumb_set CAN2_SCE_IRQHandler,Default_Handler

.weak OTG_FS_IRQHandler
.thumb_set OTG_FS_IRQHandler,Default_Handler

.weak DMA2_Stream5_IRQHandler
.thumb_set DMA2_Stream5_IRQHandler,Default_Handler

.weak DMA2_Stream6_IRQHandler
.thumb_set DMA2_Stream6_IRQHandler,Default_Handler

.weak DMA2_Stream7_IRQHandler
.thumb_set DMA2_Stream7_IRQHandler,Default_Handler

.weak USART6_IRQHandler
.thumb_set USART6_IRQHandler,Default_Handler

.weak I2C3_EV_IRQHandler
.thumb_set I2C3_EV_IRQHandler,Default_Handler

.weak I2C3_ER_IRQHandler
.thumb_set I2C3_ER_IRQHandler,Default_Handler

.weak OTG_HS_EP1_OUT_IRQHandler
.thumb_set OTG_HS_EP1_OUT_IRQHandler,Default_Handler

.weak OTG_HS_EP1_IN_IRQHandler
.thumb_set OTG_HS_EP1_IN_IRQHandler,Default_Handler

.weak OTG_HS_WKUP_IRQHandler
.thumb_set OTG_HS_WKUP_IRQHandler,Default_Handler

.weak OTG_HS_IRQHandler
.thumb_set OTG_HS_IRQHandler,Default_Handler

.weak DCMI_IRQHandler
.thumb_set DCMI_IRQHandler,Default_Handler

.weak CRYP_IRQHandler
.thumb_set CRYP_IRQHandler,Default_Handler

.weak HASH_RNG_IRQHandler
.thumb_set HASH_RNG_IRQHandler,Default_Handler

.weak FPU_IRQHandler
.thumb_set FPU_IRQHandler,Default_Handler

/* 系统初始化函数 */
.weak SystemInit
.thumb_set SystemInit,Default_Handler