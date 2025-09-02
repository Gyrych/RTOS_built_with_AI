@echo off
REM =====================================================
REM RTOS硬件抽象层编译测试脚本
REM 逐个编译每个模块，检查编译错误
REM =====================================================

echo =====================================
echo RTOS硬件抽象层编译测试开始
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
set WARNINGS=-Wall -Wextra -Wno-unused-parameter -Wno-unused-function

REM 包含路径
set INCLUDES=-I. -Irtos -Irtos/core -Irtos/hw -Irtos/hw/security -Irtos/hw/network -Irtos/hw/ota -Irtos/hw/debug -Irtos/hw/platforms
set INCLUDES=%INCLUDES% -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include

REM 完整编译标志
set CFLAGS=%COMMON_FLAGS% %OPTIMIZATION% %DEFINES% %WARNINGS% %INCLUDES%

echo 编译标志: %CFLAGS%
echo.

REM 编译测试计数器
set TEST_COUNT=0
set PASS_COUNT=0
set FAIL_COUNT=0

echo 开始逐个模块编译测试...
echo.

REM 测试1: 核心类型模块
echo 测试1: 编译核心类型模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/core/types.c -o test_types.o 2>nul
if %ERRORLEVEL% equ 0 (
    echo   ✓ 核心类型模块编译成功
    set /a PASS_COUNT+=1
    del test_types.o 2>nul
) else (
    echo   ✗ 核心类型模块编译失败
    set /a FAIL_COUNT+=1
)

REM 测试2: 硬件抽象层基础模块
echo 测试2: 编译硬件抽象层基础模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/hw_abstraction.c -o test_hw_abstraction.o 2>test_hw_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 硬件抽象层基础模块编译成功
    set /a PASS_COUNT+=1
    del test_hw_abstraction.o 2>nul
) else (
    echo   ✗ 硬件抽象层基础模块编译失败
    echo   错误信息:
    type test_hw_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_hw_abstraction.err 2>nul

REM 测试3: 电源管理模块
echo 测试3: 编译电源管理模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/power_management.c -o test_power_management.o 2>test_power_management.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 电源管理模块编译成功
    set /a PASS_COUNT+=1
    del test_power_management.o 2>nul
) else (
    echo   ✗ 电源管理模块编译失败
    echo   错误信息:
    type test_power_management.err
    set /a FAIL_COUNT+=1
)
del test_power_management.err 2>nul

REM 测试4: 内存监控模块
echo 测试4: 编译内存监控模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/memory_monitor.c -o test_memory_monitor.o 2>test_memory_monitor.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 内存监控模块编译成功
    set /a PASS_COUNT+=1
    del test_memory_monitor.o 2>nul
) else (
    echo   ✗ 内存监控模块编译失败
    echo   错误信息:
    type test_memory_monitor.err
    set /a FAIL_COUNT+=1
)
del test_memory_monitor.err 2>nul

REM 测试5: 看门狗管理模块
echo 测试5: 编译看门狗管理模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/watchdog_manager.c -o test_watchdog_manager.o 2>test_watchdog_manager.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 看门狗管理模块编译成功
    set /a PASS_COUNT+=1
    del test_watchdog_manager.o 2>nul
) else (
    echo   ✗ 看门狗管理模块编译失败
    echo   错误信息:
    type test_watchdog_manager.err
    set /a FAIL_COUNT+=1
)
del test_watchdog_manager.err 2>nul

REM 测试6: GPIO抽象模块
echo 测试6: 编译GPIO抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/gpio_abstraction.c -o test_gpio_abstraction.o 2>test_gpio_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ GPIO抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_gpio_abstraction.o 2>nul
) else (
    echo   ✗ GPIO抽象模块编译失败
    echo   错误信息:
    type test_gpio_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_gpio_abstraction.err 2>nul

REM 测试7: UART抽象模块
echo 测试7: 编译UART抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/uart_abstraction.c -o test_uart_abstraction.o 2>test_uart_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ UART抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_uart_abstraction.o 2>nul
) else (
    echo   ✗ UART抽象模块编译失败
    echo   错误信息:
    type test_uart_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_uart_abstraction.err 2>nul

REM 测试8: DMA抽象模块
echo 测试8: 编译DMA抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/dma_abstraction.c -o test_dma_abstraction.o 2>test_dma_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ DMA抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_dma_abstraction.o 2>nul
) else (
    echo   ✗ DMA抽象模块编译失败
    echo   错误信息:
    type test_dma_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_dma_abstraction.err 2>nul

REM 测试9: SPI抽象模块
echo 测试9: 编译SPI抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/spi_abstraction.c -o test_spi_abstraction.o 2>test_spi_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ SPI抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_spi_abstraction.o 2>nul
) else (
    echo   ✗ SPI抽象模块编译失败
    echo   错误信息:
    type test_spi_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_spi_abstraction.err 2>nul

REM 测试10: I2C抽象模块
echo 测试10: 编译I2C抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/i2c_abstraction.c -o test_i2c_abstraction.o 2>test_i2c_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ I2C抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_i2c_abstraction.o 2>nul
) else (
    echo   ✗ I2C抽象模块编译失败
    echo   错误信息:
    type test_i2c_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_i2c_abstraction.err 2>nul

REM 测试11: ADC抽象模块
echo 测试11: 编译ADC抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/adc_abstraction.c -o test_adc_abstraction.o 2>test_adc_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ ADC抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_adc_abstraction.o 2>nul
) else (
    echo   ✗ ADC抽象模块编译失败
    echo   错误信息:
    type test_adc_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_adc_abstraction.err 2>nul

REM 测试12: DAC抽象模块
echo 测试12: 编译DAC抽象模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/dac_abstraction.c -o test_dac_abstraction.o 2>test_dac_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ DAC抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_dac_abstraction.o 2>nul
) else (
    echo   ✗ DAC抽象模块编译失败
    echo   错误信息:
    type test_dac_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_dac_abstraction.err 2>nul

REM 测试13: 安全模块
echo 测试13: 编译安全模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/security/secure_boot.c -o test_secure_boot.o 2>test_secure_boot.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 安全启动模块编译成功
    set /a PASS_COUNT+=1
    del test_secure_boot.o 2>nul
) else (
    echo   ✗ 安全启动模块编译失败
    echo   错误信息:
    type test_secure_boot.err
    set /a FAIL_COUNT+=1
)
del test_secure_boot.err 2>nul

set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/security/crypto_abstraction.c -o test_crypto_abstraction.o 2>test_crypto_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 加密抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_crypto_abstraction.o 2>nul
) else (
    echo   ✗ 加密抽象模块编译失败
    echo   错误信息:
    type test_crypto_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_crypto_abstraction.err 2>nul

REM 测试14: 网络模块
echo 测试14: 编译网络模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/network/ethernet_abstraction.c -o test_ethernet_abstraction.o 2>test_ethernet_abstraction.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 以太网抽象模块编译成功
    set /a PASS_COUNT+=1
    del test_ethernet_abstraction.o 2>nul
) else (
    echo   ✗ 以太网抽象模块编译失败
    echo   错误信息:
    type test_ethernet_abstraction.err
    set /a FAIL_COUNT+=1
)
del test_ethernet_abstraction.err 2>nul

REM 测试15: OTA模块
echo 测试15: 编译OTA模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/ota/ota_manager.c -o test_ota_manager.o 2>test_ota_manager.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ OTA管理模块编译成功
    set /a PASS_COUNT+=1
    del test_ota_manager.o 2>nul
) else (
    echo   ✗ OTA管理模块编译失败
    echo   错误信息:
    type test_ota_manager.err
    set /a FAIL_COUNT+=1
)
del test_ota_manager.err 2>nul

REM 测试16: 调试工具模块
echo 测试16: 编译调试工具模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/debug/performance_profiler.c -o test_performance_profiler.o 2>test_performance_profiler.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 性能分析器模块编译成功
    set /a PASS_COUNT+=1
    del test_performance_profiler.o 2>nul
) else (
    echo   ✗ 性能分析器模块编译失败
    echo   错误信息:
    type test_performance_profiler.err
    set /a FAIL_COUNT+=1
)
del test_performance_profiler.err 2>nul

set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/debug/system_tracer.c -o test_system_tracer.o 2>test_system_tracer.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 系统跟踪器模块编译成功
    set /a PASS_COUNT+=1
    del test_system_tracer.o 2>nul
) else (
    echo   ✗ 系统跟踪器模块编译失败
    echo   错误信息:
    type test_system_tracer.err
    set /a FAIL_COUNT+=1
)
del test_system_tracer.err 2>nul

REM 测试17: 综合测试模块
echo 测试17: 编译综合测试模块...
set /a TEST_COUNT+=1
%GCC_PATH% %CFLAGS% -c rtos/hw/hw_comprehensive_test.c -o test_hw_comprehensive_test.o 2>test_hw_comprehensive_test.err
if %ERRORLEVEL% equ 0 (
    echo   ✓ 综合测试模块编译成功
    set /a PASS_COUNT+=1
    del test_hw_comprehensive_test.o 2>nul
) else (
    echo   ✗ 综合测试模块编译失败
    echo   错误信息:
    type test_hw_comprehensive_test.err
    set /a FAIL_COUNT+=1
)
del test_hw_comprehensive_test.err 2>nul

echo.
echo =====================================
echo 编译测试完成
echo =====================================
echo 总测试数: %TEST_COUNT%
echo 通过数: %PASS_COUNT%
echo 失败数: %FAIL_COUNT%

REM 计算通过率
set /a PASS_RATE=(%PASS_COUNT% * 100) / %TEST_COUNT%
echo 通过率: %PASS_RATE%%%

if %FAIL_COUNT% equ 0 (
    echo.
    echo ✓ 所有模块编译测试通过！
    echo   硬件抽象层代码质量良好，可以进行完整编译
) else (
    echo.
    echo ✗ 发现编译错误，需要修复
    echo   请检查上述错误信息并修复相关问题
)

echo.
pause