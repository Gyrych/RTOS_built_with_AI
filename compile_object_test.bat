@echo off
REM 设置控制台编码为UTF-8，解决PowerShell中的中文乱码问题
chcp 65001 >nul 2>&1

echo ========================================
echo RTOS对象模块功能测试编译脚本
echo ========================================
echo.

REM 设置编译器路径
set CC=arm-none-eabi-gcc
set AS=arm-none-eabi-as
set OBJCOPY=arm-none-eabi-objcopy
set SIZE=arm-none-eabi-size

REM 设置编译选项 - 添加必要的预处理器定义
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -std=c99 -O2 -g -ffunction-sections -fdata-sections -DUSE_STDPERIPH_DRIVER -DSTM32F40_41xxx -DHSE_VALUE=25000000 -DOBJECT_TEST_MAIN
set ASFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
set LDFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wl,--gc-sections -Wl,--print-memory-usage -TSTM32F407VGTx_FLASH.ld

REM 设置包含目录 - 添加固件库和CMSIS路径
set INC_DIRS=-I. -Irtos/core -Irtos/task -Irtos/sync -Irtos/time -Irtos/memory -Irtos/hw -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include

echo 编译参数:
echo   CC: %CC%
echo   CFLAGS: %CFLAGS%
echo   INC_DIRS: %INC_DIRS%
echo.

echo 开始编译对象模块测试程序...
echo.

REM 编译对象模块测试程序
echo 编译对象模块核心...
%CC% %CFLAGS% %INC_DIRS% -c rtos/core/object.c -o rtos/core/object.o

echo 编译对象模块测试程序...
%CC% %CFLAGS% %INC_DIRS% -c rtos/core/object_test.c -o rtos/core/object_test.o

echo 链接生成ELF文件...
%CC% %LDFLAGS% -o object_test.elf rtos/core/object.o rtos/core/object_test.o

if exist object_test.elf (
    echo.
    echo ========================================
    echo 编译成功！
    echo ========================================
    echo.
    echo 生成的文件:
    echo   object_test.elf - ELF格式可执行文件
    echo.
    echo 显示程序大小信息...
    %SIZE% object_test.elf
    echo.
    echo 注意: 这是针对ARM Cortex-M4架构编译的测试程序
    echo 需要在目标硬件上运行，或使用ARM模拟器
    echo.
    echo 测试功能包括:
    echo   - 基本对象操作 (初始化、名称、类型、标志)
    echo   - 对象时间管理 (创建时间、年龄计算)
    echo   - 引用计数管理 (增加、减少、检查)
    echo   - 对象容器管理 (添加、移除、查找、清空)
    echo   - 等待队列管理 (添加、移除、清空)
    echo   - 等待节点操作 (初始化、数据设置、标志设置)
    echo   - 对象系统管理 (初始化、容器获取、统计)
    echo   - 对象销毁和统计信息
    echo.
) else (
    echo.
    echo ========================================
    echo 编译失败！
    echo ========================================
    echo.
    echo 请检查:
    echo   1. 源代码文件是否存在
    echo   2. 包含路径是否正确
    echo   3. 依赖的头文件是否完整
    echo   4. 编译工具链是否正确安装
    echo.
)

echo 按任意键退出...
pause
