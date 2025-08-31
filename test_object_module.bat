@echo off
REM 设置控制台编码为UTF-8
chcp 65001 >nul 2>&1

echo ========================================
echo RTOS对象模块功能验证脚本
echo ========================================
echo.

echo 检查生成的文件...
if exist object_test.elf (
    echo ✓ object_test.elf 文件已生成
    echo.
    echo 文件信息:
    echo   大小: %~z1 object_test.elf 字节
    echo   修改时间: %~t1 object_test.elf
    echo.
) else (
    echo ✗ object_test.elf 文件未找到
    echo 请先运行编译脚本
    pause
    exit /b 1
)

echo 检查源代码文件...
if exist rtos/core/object.c (
    echo ✓ object.c 源文件存在
) else (
    echo ✗ object.c 源文件不存在
)

if exist rtos/core/object.h (
    echo ✓ object.h 头文件存在
) else (
    echo ✗ object.h 头文件不存在
)

if exist rtos/core/object_test.c (
    echo ✓ object_test.c 测试文件存在
) else (
    echo ✗ object_test.c 测试文件不存在
)

echo.
echo 检查编译输出文件...
if exist rtos/core/object.o (
    echo ✓ object.o 目标文件存在
) else (
    echo ✗ object.o 目标文件不存在
)

if exist rtos/core/object_test.o (
    echo ✓ object_test.o 目标文件存在
) else (
    echo ✗ object_test.o 目标文件不存在
)

echo.
echo 显示ELF文件详细信息...
arm-none-eabi-objdump -h object_test.elf

echo.
echo 显示ELF文件符号表...
arm-none-eabi-nm object_test.elf

echo.
echo ========================================
echo 验证完成！
echo ========================================
echo.
echo 下一步建议:
echo   1. 在目标硬件上运行测试程序
echo   2. 使用ARM模拟器进行功能测试
echo   3. 检查生成的汇编代码
echo   4. 验证内存布局和符号
echo.
echo 按任意键退出...
pause
