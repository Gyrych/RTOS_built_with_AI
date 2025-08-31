#!/bin/bash
# =====================================================
# RTOS硬件抽象层编译测试脚本 (Linux版本)
# 逐个编译每个模块，检查编译错误
# =====================================================

echo "====================================="
echo "RTOS硬件抽象层编译测试开始"
echo "====================================="

# 设置编译器路径
GCC_PATH="arm-none-eabi-gcc"

# 检查编译器是否存在
if ! command -v $GCC_PATH &> /dev/null; then
    echo "错误: 未找到ARM GCC编译器，使用gcc进行语法检查"
    GCC_PATH="gcc"
fi

# 编译参数
COMMON_FLAGS="-mcpu=cortex-m4 -mthumb"
OPTIMIZATION="-Os -g3"
DEFINES="-DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000"
DEFINES="$DEFINES -D__ARM_ARCH_7EM__"  # 添加架构定义
WARNINGS="-Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable"

# 包含路径
INCLUDES="-I. -Irtos -Irtos/core -Irtos/hw -Irtos/hw/security -Irtos/hw/network -Irtos/hw/ota -Irtos/hw/debug -Irtos/hw/platforms"
INCLUDES="$INCLUDES -Ifwlib/inc -Ifwlib/CMSIS/Include -Ifwlib/CMSIS/STM32F4xx/Include"

# 完整编译标志
if [ "$GCC_PATH" = "gcc" ]; then
    CFLAGS="$OPTIMIZATION $DEFINES $WARNINGS $INCLUDES -fsyntax-only"
else
    CFLAGS="$COMMON_FLAGS $OPTIMIZATION $DEFINES $WARNINGS $INCLUDES -fsyntax-only"
fi

echo "编译标志: $CFLAGS"
echo

# 编译测试计数器
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

echo "开始逐个模块编译测试..."
echo

# 测试函数
test_compile() {
    local file=$1
    local name=$2
    
    TEST_COUNT=$((TEST_COUNT + 1))
    echo "测试$TEST_COUNT: 编译$name..."
    
    if $GCC_PATH $CFLAGS $file 2>/tmp/compile_error.log; then
        echo "  ✓ $name编译成功"
        PASS_COUNT=$((PASS_COUNT + 1))
        return 0
    else
        echo "  ✗ $name编译失败"
        echo "  错误信息:"
        cat /tmp/compile_error.log | head -10
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
}

# 执行编译测试
test_compile "rtos/hw/hw_abstraction.c" "硬件抽象层基础模块"
test_compile "rtos/hw/power_management.c" "电源管理模块"
test_compile "rtos/hw/memory_monitor.c" "内存监控模块"
test_compile "rtos/hw/watchdog_manager.c" "看门狗管理模块"
test_compile "rtos/hw/gpio_abstraction.c" "GPIO抽象模块"
test_compile "rtos/hw/uart_abstraction.c" "UART抽象模块"
test_compile "rtos/hw/dma_abstraction.c" "DMA抽象模块"
test_compile "rtos/hw/spi_abstraction.c" "SPI抽象模块"
test_compile "rtos/hw/i2c_abstraction.c" "I2C抽象模块"
test_compile "rtos/hw/adc_abstraction.c" "ADC抽象模块"
test_compile "rtos/hw/dac_abstraction.c" "DAC抽象模块"
test_compile "rtos/hw/security/secure_boot.c" "安全启动模块"
test_compile "rtos/hw/security/crypto_abstraction.c" "加密抽象模块"
test_compile "rtos/hw/network/ethernet_abstraction.c" "以太网抽象模块"
test_compile "rtos/hw/ota/ota_manager.c" "OTA管理模块"
test_compile "rtos/hw/debug/performance_profiler.c" "性能分析器模块"
test_compile "rtos/hw/debug/system_tracer.c" "系统跟踪器模块"
test_compile "rtos/hw/hw_comprehensive_test.c" "综合测试模块"

echo
echo "====================================="
echo "编译测试完成"
echo "====================================="
echo "总测试数: $TEST_COUNT"
echo "通过数: $PASS_COUNT"
echo "失败数: $FAIL_COUNT"

# 计算通过率
PASS_RATE=$((PASS_COUNT * 100 / TEST_COUNT))
echo "通过率: ${PASS_RATE}%"

if [ $FAIL_COUNT -eq 0 ]; then
    echo
    echo "✓ 所有模块编译测试通过！"
    echo "  硬件抽象层代码质量良好，可以进行完整编译"
    exit 0
else
    echo
    echo "✗ 发现编译错误，需要修复"
    echo "  请检查上述错误信息并修复相关问题"
    exit 1
fi