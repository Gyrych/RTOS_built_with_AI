@echo off
echo ========================================
echo RTOS硬件定时器改进测试编译脚本
echo ========================================
echo.

echo 清理旧的编译文件...
if exist *.o del *.o
if exist *.elf del *.elf
if exist *.bin del *.bin
if exist *.hex del *.hex

echo.
echo 编译硬件抽象层...
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ^
    -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000 ^
    -I. -Irtos -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include ^
    -O2 -g -Wall -Wextra ^
    rtos/hw/hw_abstraction.c -o hw_abstraction.o

if %ERRORLEVEL% neq 0 (
    echo 错误: 编译硬件抽象层失败
    pause
    exit /b 1
)

echo 编译中断处理程序...
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ^
    -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000 ^
    -I. -Irtos -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include ^
    -O2 -g -Wall -Wextra ^
    rtos/hw/interrupt_handler.c -o interrupt_handler.o

if %ERRORLEVEL% neq 0 (
    echo 错误: 编译中断处理程序失败
    pause
    exit /b 1
)

echo 编译硬件定时器测试程序...
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ^
    -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000 ^
    -I. -Irtos -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include ^
    -O2 -g -Wall -Wextra ^
    rtos/hw/hw_timer_test.c -o hw_timer_test.o

if %ERRORLEVEL% neq 0 (
    echo 错误: 编译硬件定时器测试程序失败
    pause
    exit /b 1
)

echo.
echo 链接生成可执行文件...
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 ^
    -TSTM32F407VGTx_FLASH.ld ^
    -Wl,--gc-sections -Wl,--print-memory-usage ^
    hw_abstraction.o interrupt_handler.o hw_timer_test.o ^
    -o hw_test.elf

if %ERRORLEVEL% neq 0 (
    echo 错误: 链接失败
    pause
    exit /b 1
)

echo.
echo 生成二进制文件...
arm-none-eabi-objcopy -O binary hw_test.elf hw_test.bin
arm-none-eabi-objcopy -O ihex hw_test.elf hw_test.hex

echo.
echo 显示文件大小信息...
arm-none-eabi-size hw_test.elf

echo.
echo ========================================
echo 编译完成!
echo ========================================
echo.
echo 生成的文件:
echo   hw_test.elf - ELF可执行文件
echo   hw_test.bin - 二进制文件
echo   hw_test.hex - Intel HEX文件
echo.
echo 下一步: 将hw_test.hex烧录到STM32F407开发板进行测试
echo.

pause
