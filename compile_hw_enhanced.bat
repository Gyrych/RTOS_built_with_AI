@echo off
REM =====================================================
REM RTOS硬件抽象层增强版编译脚本
REM 编译包含所有新增模块的完整硬件抽象层
REM =====================================================

echo =====================================
echo RTOS硬件抽象层增强版编译开始
echo =====================================

REM 设置编译器路径
set GCC_PATH=arm-none-eabi-gcc
set OBJCOPY_PATH=arm-none-eabi-objcopy
set SIZE_PATH=arm-none-eabi-size

REM 检查编译器是否存在
where %GCC_PATH% >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo 错误: 未找到ARM GCC编译器，请检查工具链安装
    pause
    exit /b 1
)

REM 编译参数
set COMMON_FLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
set OPTIMIZATION=-Os -g3
set DEFINES=-DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000
set WARNINGS=-Wall -Wextra -Wno-unused-parameter

REM 包含路径
set INCLUDES=-I. -Irtos -Irtos/core -Irtos/hw -Irtos/task -Irtos/sync -Irtos/time -Irtos/memory -Irtos/config
set INCLUDES=%INCLUDES% -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include

REM 完整编译标志
set CFLAGS=%COMMON_FLAGS% %OPTIMIZATION% %DEFINES% %WARNINGS% %INCLUDES%

echo 编译标志: %CFLAGS%
echo.

REM 编译固件库源文件
echo 编译STM32F4xx固件库...

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_rcc.c -o fwlib/src/stm32f4xx_rcc.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_tim.c -o fwlib/src/stm32f4xx_tim.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_gpio.c -o fwlib/src/stm32f4xx_gpio.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_usart.c -o fwlib/src/stm32f4xx_usart.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_pwr.c -o fwlib/src/stm32f4xx_pwr.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_adc.c -o fwlib/src/stm32f4xx_adc.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_iwdg.c -o fwlib/src/stm32f4xx_iwdg.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_wwdg.c -o fwlib/src/stm32f4xx_wwdg.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_dma.c -o fwlib/src/stm32f4xx_dma.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_exti.c -o fwlib/src/stm32f4xx_exti.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/stm32f4xx_syscfg.c -o fwlib/src/stm32f4xx_syscfg.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c fwlib/src/misc.c -o fwlib/src/misc.o
if %ERRORLEVEL% neq 0 goto compile_error

echo 固件库编译完成
echo.

REM 编译RTOS核心模块
echo 编译RTOS核心模块...

%GCC_PATH% %CFLAGS% -c rtos/core/types.c -o rtos/core/types.o 2>nul
%GCC_PATH% %CFLAGS% -c rtos/system.c -o rtos/system.o
if %ERRORLEVEL% neq 0 goto compile_error

echo RTOS核心模块编译完成
echo.

REM 编译硬件抽象层模块
echo 编译硬件抽象层模块...

%GCC_PATH% %CFLAGS% -c rtos/hw/hw_abstraction.c -o rtos/hw/hw_abstraction.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c rtos/hw/interrupt_handler.c -o rtos/hw/interrupt_handler.o
if %ERRORLEVEL% neq 0 goto compile_error

echo 硬件抽象层基础模块编译完成
echo.

REM 编译新增的硬件抽象模块
echo 编译新增硬件抽象模块...

%GCC_PATH% %CFLAGS% -c rtos/hw/power_management.c -o rtos/hw/power_management.o
if %ERRORLEVEL% neq 0 goto compile_error
echo   电源管理模块编译完成

%GCC_PATH% %CFLAGS% -c rtos/hw/memory_monitor.c -o rtos/hw/memory_monitor.o
if %ERRORLEVEL% neq 0 goto compile_error
echo   内存监控模块编译完成

%GCC_PATH% %CFLAGS% -c rtos/hw/watchdog_manager.c -o rtos/hw/watchdog_manager.o
if %ERRORLEVEL% neq 0 goto compile_error
echo   看门狗管理模块编译完成

%GCC_PATH% %CFLAGS% -c rtos/hw/gpio_abstraction.c -o rtos/hw/gpio_abstraction.o
if %ERRORLEVEL% neq 0 goto compile_error
echo   GPIO抽象模块编译完成

%GCC_PATH% %CFLAGS% -c rtos/hw/uart_abstraction.c -o rtos/hw/uart_abstraction.o
if %ERRORLEVEL% neq 0 goto compile_error
echo   UART抽象模块编译完成

%GCC_PATH% %CFLAGS% -c rtos/hw/hw_comprehensive_test.c -o rtos/hw/hw_comprehensive_test.o
if %ERRORLEVEL% neq 0 goto compile_error
echo   综合测试模块编译完成

echo 新增硬件抽象模块编译完成
echo.

REM 编译其他RTOS模块
echo 编译其他RTOS模块...

%GCC_PATH% %CFLAGS% -c rtos/time/tickless.c -o rtos/time/tickless.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c rtos/time/dynamic_delay.c -o rtos/time/dynamic_delay.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c rtos/memory/mempool.c -o rtos/memory/mempool.o
if %ERRORLEVEL% neq 0 goto compile_error

echo 其他RTOS模块编译完成
echo.

REM 编译主程序和启动文件
echo 编译主程序...

%GCC_PATH% %CFLAGS% -c main_refactored.c -o main_refactored.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c system_support.c -o system_support.o
if %ERRORLEVEL% neq 0 goto compile_error

%GCC_PATH% %CFLAGS% -c startup_stm32f407xx.s -o startup_stm32f407xx.o
if %ERRORLEVEL% neq 0 goto compile_error

echo 主程序编译完成
echo.

REM 链接所有目标文件
echo 链接程序...

set LINK_FLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
set LINK_FLAGS=%LINK_FLAGS% -specs=nano.specs -specs=nosys.specs
set LINK_FLAGS=%LINK_FLAGS% -T STM32F407VGTx_FLASH.ld -Wl,--gc-sections -Wl,--print-memory-usage

set OBJECTS=main_refactored.o system_support.o startup_stm32f407xx.o
set OBJECTS=%OBJECTS% rtos/system.o
set OBJECTS=%OBJECTS% rtos/hw/hw_abstraction.o rtos/hw/interrupt_handler.o
set OBJECTS=%OBJECTS% rtos/hw/power_management.o rtos/hw/memory_monitor.o
set OBJECTS=%OBJECTS% rtos/hw/watchdog_manager.o rtos/hw/gpio_abstraction.o
set OBJECTS=%OBJECTS% rtos/hw/uart_abstraction.o rtos/hw/hw_comprehensive_test.o
set OBJECTS=%OBJECTS% rtos/time/tickless.o rtos/time/dynamic_delay.o
set OBJECTS=%OBJECTS% rtos/memory/mempool.o
set OBJECTS=%OBJECTS% fwlib/src/stm32f4xx_rcc.o fwlib/src/stm32f4xx_tim.o
set OBJECTS=%OBJECTS% fwlib/src/stm32f4xx_gpio.o fwlib/src/stm32f4xx_usart.o
set OBJECTS=%OBJECTS% fwlib/src/stm32f4xx_pwr.o fwlib/src/stm32f4xx_adc.o
set OBJECTS=%OBJECTS% fwlib/src/stm32f4xx_iwdg.o fwlib/src/stm32f4xx_wwdg.o
set OBJECTS=%OBJECTS% fwlib/src/stm32f4xx_dma.o fwlib/src/stm32f4xx_exti.o
set OBJECTS=%OBJECTS% fwlib/src/stm32f4xx_syscfg.o fwlib/src/misc.o

%GCC_PATH% %LINK_FLAGS% %OBJECTS% -o rtos_enhanced.elf -lm
if %ERRORLEVEL% neq 0 goto link_error

echo 链接完成
echo.

REM 生成二进制文件
echo 生成二进制文件...

%OBJCOPY_PATH% -O binary rtos_enhanced.elf rtos_enhanced.bin
if %ERRORLEVEL% neq 0 goto objcopy_error

%OBJCOPY_PATH% -O ihex rtos_enhanced.elf rtos_enhanced.hex
if %ERRORLEVEL% neq 0 goto objcopy_error

echo 二进制文件生成完成
echo.

REM 显示程序大小信息
echo 程序大小信息:
%SIZE_PATH% rtos_enhanced.elf
echo.

REM 生成反汇编文件（可选）
echo 生成反汇编文件...
arm-none-eabi-objdump -D rtos_enhanced.elf > rtos_enhanced.dis 2>nul

echo =====================================
echo 编译成功完成！
echo =====================================
echo.
echo 输出文件:
echo   ELF文件: rtos_enhanced.elf
echo   二进制文件: rtos_enhanced.bin
echo   HEX文件: rtos_enhanced.hex
echo   反汇编文件: rtos_enhanced.dis
echo.
echo 新增功能模块:
echo   ✓ 电源管理模块 (power_management)
echo   ✓ 内存监控模块 (memory_monitor)  
echo   ✓ 看门狗管理模块 (watchdog_manager)
echo   ✓ GPIO抽象模块 (gpio_abstraction)
echo   ✓ UART抽象模块 (uart_abstraction)
echo   ✓ 综合测试模块 (hw_comprehensive_test)
echo.
echo 编译完成，可以烧录到STM32F407开发板运行！

goto end

:compile_error
echo.
echo =====================================
echo 编译错误！
echo =====================================
echo 错误代码: %ERRORLEVEL%
echo 请检查源代码和编译环境配置
pause
exit /b 1

:link_error
echo.
echo =====================================
echo 链接错误！
echo =====================================
echo 错误代码: %ERRORLEVEL%
echo 请检查链接器脚本和目标文件
pause
exit /b 1

:objcopy_error
echo.
echo =====================================
echo 目标文件转换错误！
echo =====================================
echo 错误代码: %ERRORLEVEL%
pause
exit /b 1

:end
pause