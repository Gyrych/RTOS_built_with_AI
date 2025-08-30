@echo off
echo 编译RTOS项目...

REM 设置编译器路径
set CC=arm-none-eabi-gcc
set AS=arm-none-eabi-as
set OBJCOPY=arm-none-eabi-objcopy
set SIZE=arm-none-eabi-size

REM 设置编译选项
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -std=c99 -O2 -g -ffunction-sections -fdata-sections -DUSE_STDPERIPH_DRIVER -DSTM32F407xx
set ASFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
set LDFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections -Wl,--print-memory-usage -TSTM32F407VGTx_FLASH.ld

REM 设置包含目录
set INC_DIRS=-I. -Irtos/core -Irtos/task -Irtos/sync -Irtos/time -Irtos/memory

echo 编译启动代码...
%AS% %ASFLAGS% startup_stm32f407xx.s -o startup_stm32f407xx.o

echo 编译主程序...
%CC% %CFLAGS% %INC_DIRS% -c main_refactored.c -o main_refactored.o

echo 编译系统支持...
%CC% %CFLAGS% %INC_DIRS% -c system_support.c -o system_support.o

echo 编译RTOS核心...
%CC% %CFLAGS% %INC_DIRS% -c rtos/system.c -o rtos/system.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/core/object.c -o rtos/core/object.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/task/task.c -o rtos/task/task.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/sync/semaphore.c -o rtos/sync/semaphore.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/sync/mutex.c -o rtos/sync/mutex.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/sync/queue.c -o rtos/sync/queue.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/sync/event.c -o rtos/sync/event.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/time/timer.c -o rtos/time/timer.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/time/tickless.c -o rtos/time/tickless.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/time/dynamic_delay.c -o rtos/time/dynamic_delay.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/memory/mempool.c -o rtos/memory/mempool.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/hw/hw_abstraction.c -o rtos/hw/hw_abstraction.o
%CC% %CFLAGS% %INC_DIRS% -c rtos/hw/interrupt_handler.c -o rtos/hw/interrupt_handler.o

echo 链接生成ELF文件...
%CC% %LDFLAGS% -o rtos_refactored.elf startup_stm32f407xx.o main_refactored.o system_support.o rtos/system.o rtos/core/object.o rtos/task/task.o rtos/sync/semaphore.o rtos/sync/mutex.o rtos/sync/queue.o rtos/sync/event.o rtos/time/timer.o rtos/time/tickless.o rtos/time/dynamic_delay.o rtos/memory/mempool.o rtos/hw/hw_abstraction.o rtos/hw/interrupt_handler.o

if exist rtos_refactored.elf (
    echo 生成HEX文件...
    %OBJCOPY% -O ihex rtos_refactored.elf rtos_refactored.hex
    
    echo 生成BIN文件...
    %OBJCOPY% -O binary rtos_refactored.elf rtos_refactored.bin
    
    echo 显示程序大小信息...
    %SIZE% rtos_refactored.elf
    
    echo 编译成功！
) else (
    echo 编译失败！
)

pause
