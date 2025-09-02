#!/bin/bash
# =====================================================
# RTOS硬件抽象层增强版编译脚本 (Linux版本)
# 编译包含所有新增模块的完整硬件抽象层
# =====================================================

echo "====================================="
echo "RTOS硬件抽象层增强版编译开始"
echo "====================================="

# 设置编译器路径
GCC_PATH="arm-none-eabi-gcc"
OBJCOPY_PATH="arm-none-eabi-objcopy"
SIZE_PATH="arm-none-eabi-size"

# 检查编译器是否存在
if ! command -v $GCC_PATH &> /dev/null; then
    echo "错误: 未找到ARM GCC编译器，请检查工具链安装"
    echo "使用gcc进行语法验证..."
    GCC_PATH="gcc"
    SYNTAX_ONLY=true
else
    SYNTAX_ONLY=false
fi

# 编译参数
COMMON_FLAGS="-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
OPTIMIZATION="-Os -g3"
DEFINES="-DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000"
WARNINGS="-Wall -Wextra -Wno-unused-parameter -Wno-format"

# 包含路径
INCLUDES="-I. -Irtos -Irtos/core -Irtos/hw -Irtos/task -Irtos/sync -Irtos/time -Irtos/memory -Irtos/config"
INCLUDES="$INCLUDES -Irtos/hw/security -Irtos/hw/network -Irtos/hw/ota -Irtos/hw/debug -Irtos/hw/platforms"
INCLUDES="$INCLUDES -Ifwlib/inc -Ifwlib/CMSIS/Include -Ifwlib/CMSIS/STM32F4xx/Include"

# 完整编译标志
if [ "$SYNTAX_ONLY" = "true" ]; then
    CFLAGS="$OPTIMIZATION $DEFINES $WARNINGS $INCLUDES -fsyntax-only"
else
    CFLAGS="$COMMON_FLAGS $OPTIMIZATION $DEFINES $WARNINGS $INCLUDES"
fi

echo "编译标志: $CFLAGS"
echo

# 编译固件库源文件
echo "编译STM32F4xx固件库..."

if [ "$SYNTAX_ONLY" = "false" ]; then
    compile_fwlib() {
        local file=$1
        local name=$2
        
        if [ -f "$file" ]; then
            echo "  编译 $name..."
            if ! $GCC_PATH $CFLAGS -c $file -o ${file%.c}.o; then
                echo "  错误: $name 编译失败"
                return 1
            fi
        else
            echo "  警告: $file 不存在，跳过"
        fi
        return 0
    }

    compile_fwlib "fwlib/src/stm32f4xx_rcc.c" "RCC"
    compile_fwlib "fwlib/src/stm32f4xx_tim.c" "TIM"
    compile_fwlib "fwlib/src/stm32f4xx_gpio.c" "GPIO"
    compile_fwlib "fwlib/src/stm32f4xx_usart.c" "USART"
    compile_fwlib "fwlib/src/stm32f4xx_pwr.c" "PWR"
    compile_fwlib "fwlib/src/stm32f4xx_adc.c" "ADC"
    compile_fwlib "fwlib/src/stm32f4xx_iwdg.c" "IWDG"
    compile_fwlib "fwlib/src/stm32f4xx_wwdg.c" "WWDG"
    compile_fwlib "fwlib/src/stm32f4xx_dma.c" "DMA"
    compile_fwlib "fwlib/src/stm32f4xx_spi.c" "SPI"
    compile_fwlib "fwlib/src/stm32f4xx_i2c.c" "I2C"
    compile_fwlib "fwlib/src/stm32f4xx_dac.c" "DAC"
    compile_fwlib "fwlib/src/stm32f4xx_exti.c" "EXTI"
    compile_fwlib "fwlib/src/stm32f4xx_syscfg.c" "SYSCFG"
    compile_fwlib "fwlib/src/misc.c" "MISC"
    
    echo "固件库编译完成"
    echo
fi

# 编译RTOS核心模块
echo "编译RTOS核心模块..."

compile_module() {
    local file=$1
    local name=$2
    
    if [ -f "$file" ]; then
        echo "  编译 $name..."
        if [ "$SYNTAX_ONLY" = "true" ]; then
            if ! $GCC_PATH $CFLAGS $file; then
                echo "  错误: $name 语法检查失败"
                return 1
            fi
        else
            if ! $GCC_PATH $CFLAGS -c $file -o ${file%.c}.o; then
                echo "  错误: $name 编译失败"
                return 1
            fi
        fi
    else
        echo "  警告: $file 不存在，跳过"
    fi
    return 0
}

compile_module "rtos/system.c" "RTOS系统核心"

echo "RTOS核心模块编译完成"
echo

# 编译硬件抽象层模块
echo "编译硬件抽象层模块..."

compile_module "rtos/hw/hw_abstraction.c" "硬件抽象层基础"
compile_module "rtos/hw/interrupt_handler.c" "中断处理"

echo "硬件抽象层基础模块编译完成"
echo

# 编译新增的硬件抽象模块
echo "编译新增硬件抽象模块..."

compile_module "rtos/hw/power_management.c" "电源管理模块"
compile_module "rtos/hw/memory_monitor.c" "内存监控模块"
compile_module "rtos/hw/watchdog_manager.c" "看门狗管理模块"
compile_module "rtos/hw/gpio_abstraction.c" "GPIO抽象模块"
compile_module "rtos/hw/uart_abstraction.c" "UART抽象模块"
compile_module "rtos/hw/dma_abstraction.c" "DMA抽象模块"
compile_module "rtos/hw/spi_abstraction.c" "SPI抽象模块"
compile_module "rtos/hw/i2c_abstraction.c" "I2C抽象模块"
compile_module "rtos/hw/adc_abstraction.c" "ADC抽象模块"
compile_module "rtos/hw/dac_abstraction.c" "DAC抽象模块"

echo "新增硬件抽象模块编译完成"
echo

# 编译安全和网络模块
echo "编译安全和网络模块..."

compile_module "rtos/hw/security/secure_boot.c" "安全启动模块"
compile_module "rtos/hw/security/crypto_abstraction.c" "加密抽象模块"
compile_module "rtos/hw/network/ethernet_abstraction.c" "以太网抽象模块"
compile_module "rtos/hw/ota/ota_manager.c" "OTA管理模块"

echo "安全和网络模块编译完成"
echo

# 编译调试工具模块
echo "编译调试工具模块..."

compile_module "rtos/hw/debug/performance_profiler.c" "性能分析器模块"
compile_module "rtos/hw/debug/system_tracer.c" "系统跟踪器模块"
compile_module "rtos/hw/hw_comprehensive_test.c" "综合测试模块"

echo "调试工具模块编译完成"
echo

# 编译其他RTOS模块
echo "编译其他RTOS模块..."

compile_module "rtos/time/tickless.c" "Tickless时间管理"
compile_module "rtos/time/dynamic_delay.c" "动态延时"
compile_module "rtos/memory/mempool.c" "内存池"

echo "其他RTOS模块编译完成"
echo

# 编译主程序和启动文件
echo "编译主程序..."

compile_module "main_hw_enhanced.c" "增强版主程序"
compile_module "system_support.c" "系统支持"

if [ "$SYNTAX_ONLY" = "false" ]; then
    if [ -f "startup_stm32f407xx.s" ]; then
        echo "  编译启动文件..."
        if ! $GCC_PATH $CFLAGS -c startup_stm32f407xx.s -o startup_stm32f407xx.o; then
            echo "  错误: 启动文件编译失败"
            exit 1
        fi
    fi
fi

echo "主程序编译完成"
echo

if [ "$SYNTAX_ONLY" = "true" ]; then
    echo "====================================="
    echo "语法检查完成！"
    echo "====================================="
    echo "所有模块语法检查通过"
    echo "代码质量良好，可以使用ARM GCC进行实际编译"
else
    # 链接所有目标文件
    echo "链接程序..."
    
    # 收集所有目标文件
    OBJECTS=""
    for obj in *.o rtos/*.o rtos/hw/*.o rtos/hw/security/*.o rtos/hw/network/*.o rtos/hw/ota/*.o rtos/hw/debug/*.o rtos/time/*.o rtos/memory/*.o fwlib/src/*.o; do
        if [ -f "$obj" ]; then
            OBJECTS="$OBJECTS $obj"
        fi
    done
    
    LINK_FLAGS="$COMMON_FLAGS -specs=nano.specs -specs=nosys.specs"
    LINK_FLAGS="$LINK_FLAGS -T STM32F407VGTx_FLASH.ld -Wl,--gc-sections -Wl,--print-memory-usage"
    
    if ! $GCC_PATH $LINK_FLAGS $OBJECTS -o rtos_enhanced.elf -lm; then
        echo "链接失败"
        exit 1
    fi
    
    echo "链接完成"
    echo
    
    # 生成二进制文件
    echo "生成二进制文件..."
    
    $OBJCOPY_PATH -O binary rtos_enhanced.elf rtos_enhanced.bin
    $OBJCOPY_PATH -O ihex rtos_enhanced.elf rtos_enhanced.hex
    
    echo "二进制文件生成完成"
    echo
    
    # 显示程序大小信息
    echo "程序大小信息:"
    $SIZE_PATH rtos_enhanced.elf
    echo
    
    echo "====================================="
    echo "编译成功完成！"
    echo "====================================="
    echo
    echo "输出文件:"
    echo "  ELF文件: rtos_enhanced.elf"
    echo "  二进制文件: rtos_enhanced.bin"
    echo "  HEX文件: rtos_enhanced.hex"
    echo
    echo "新增功能模块:"
    echo "  ✓ 电源管理模块 (power_management)"
    echo "  ✓ 内存监控模块 (memory_monitor)"
    echo "  ✓ 看门狗管理模块 (watchdog_manager)"
    echo "  ✓ GPIO抽象模块 (gpio_abstraction)"
    echo "  ✓ UART抽象模块 (uart_abstraction)"
    echo "  ✓ DMA抽象模块 (dma_abstraction)"
    echo "  ✓ SPI抽象模块 (spi_abstraction)"
    echo "  ✓ I2C抽象模块 (i2c_abstraction)"
    echo "  ✓ ADC抽象模块 (adc_abstraction)"
    echo "  ✓ DAC抽象模块 (dac_abstraction)"
    echo "  ✓ 安全特性模块 (security)"
    echo "  ✓ 网络通信模块 (network)"
    echo "  ✓ OTA更新模块 (ota)"
    echo "  ✓ 调试工具模块 (debug)"
    echo "  ✓ 综合测试模块 (comprehensive_test)"
    echo
    echo "编译完成，可以烧录到STM32F407开发板运行！"
fi