#!/usr/bin/env python3
"""
RTOS硬件抽象层配置生成工具
面向对象的配置管理和代码生成
"""

import json
import os
import argparse
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, asdict
from enum import Enum

class PlatformType(Enum):
    """支持的平台类型"""
    STM32F1 = "stm32f1"
    STM32F4 = "stm32f4"
    STM32F7 = "stm32f7"
    RISC_V = "riscv"
    X86_64 = "x86_64"

@dataclass
class ClockConfig:
    """时钟配置"""
    system_clock_freq: int
    cpu_clock_freq: int
    apb1_clock_freq: int
    apb2_clock_freq: int
    timer_clock_freq: int
    timer_resolution_ns: int

@dataclass
class MemoryConfig:
    """内存配置"""
    flash_size_kb: int
    ram_size_kb: int
    ccm_size_kb: int
    heap_size_kb: int
    stack_size_kb: int

@dataclass
class PeripheralConfig:
    """外设配置"""
    gpio_port_count: int
    uart_count: int
    spi_count: int
    i2c_count: int
    adc_count: int
    dac_count: int
    timer_count: int
    dma_controller_count: int
    dma_stream_count: int

@dataclass
class FeatureConfig:
    """特性配置"""
    has_fpu: bool
    has_dsp: bool
    has_mpu: bool
    has_cache: bool
    has_dma: bool
    has_crypto: bool
    has_ethernet: bool
    has_usb: bool

@dataclass
class ModuleConfig:
    """模块配置"""
    power_management_enabled: bool
    memory_monitor_enabled: bool
    watchdog_enabled: bool
    gpio_abstraction_enabled: bool
    uart_abstraction_enabled: bool
    spi_abstraction_enabled: bool
    i2c_abstraction_enabled: bool
    adc_abstraction_enabled: bool
    dac_abstraction_enabled: bool
    dma_abstraction_enabled: bool
    security_enabled: bool
    network_enabled: bool
    ota_enabled: bool
    debug_tools_enabled: bool

@dataclass
class DebugConfig:
    """调试配置"""
    debug_enabled: bool
    error_check_enabled: bool
    performance_profiling_enabled: bool
    system_tracing_enabled: bool
    memory_leak_detection_enabled: bool

@dataclass
class PlatformConfig:
    """完整平台配置"""
    platform_type: PlatformType
    platform_name: str
    clock: ClockConfig
    memory: MemoryConfig
    peripheral: PeripheralConfig
    feature: FeatureConfig
    module: ModuleConfig
    debug: DebugConfig

class ConfigGenerator:
    """配置生成器类"""
    
    def __init__(self):
        self.platforms = {}
        self._init_default_platforms()
    
    def _init_default_platforms(self):
        """初始化默认平台配置"""
        
        # STM32F407配置
        self.platforms[PlatformType.STM32F4] = PlatformConfig(
            platform_type=PlatformType.STM32F4,
            platform_name="STM32F407VGT6",
            clock=ClockConfig(
                system_clock_freq=168000000,
                cpu_clock_freq=168000000,
                apb1_clock_freq=84000000,
                apb2_clock_freq=168000000,
                timer_clock_freq=84000000,
                timer_resolution_ns=11
            ),
            memory=MemoryConfig(
                flash_size_kb=1024,
                ram_size_kb=128,
                ccm_size_kb=64,
                heap_size_kb=64,
                stack_size_kb=16
            ),
            peripheral=PeripheralConfig(
                gpio_port_count=9,
                uart_count=6,
                spi_count=3,
                i2c_count=3,
                adc_count=3,
                dac_count=2,
                timer_count=14,
                dma_controller_count=2,
                dma_stream_count=8
            ),
            feature=FeatureConfig(
                has_fpu=True,
                has_dsp=True,
                has_mpu=True,
                has_cache=False,
                has_dma=True,
                has_crypto=False,
                has_ethernet=True,
                has_usb=True
            ),
            module=ModuleConfig(
                power_management_enabled=True,
                memory_monitor_enabled=True,
                watchdog_enabled=True,
                gpio_abstraction_enabled=True,
                uart_abstraction_enabled=True,
                spi_abstraction_enabled=True,
                i2c_abstraction_enabled=True,
                adc_abstraction_enabled=True,
                dac_abstraction_enabled=True,
                dma_abstraction_enabled=True,
                security_enabled=False,
                network_enabled=True,
                ota_enabled=True,
                debug_tools_enabled=True
            ),
            debug=DebugConfig(
                debug_enabled=True,
                error_check_enabled=True,
                performance_profiling_enabled=True,
                system_tracing_enabled=True,
                memory_leak_detection_enabled=True
            )
        )
        
        # STM32F103配置
        self.platforms[PlatformType.STM32F1] = PlatformConfig(
            platform_type=PlatformType.STM32F1,
            platform_name="STM32F103VET6",
            clock=ClockConfig(
                system_clock_freq=72000000,
                cpu_clock_freq=72000000,
                apb1_clock_freq=36000000,
                apb2_clock_freq=72000000,
                timer_clock_freq=72000000,
                timer_resolution_ns=13
            ),
            memory=MemoryConfig(
                flash_size_kb=512,
                ram_size_kb=64,
                ccm_size_kb=0,
                heap_size_kb=32,
                stack_size_kb=8
            ),
            peripheral=PeripheralConfig(
                gpio_port_count=7,
                uart_count=5,
                spi_count=3,
                i2c_count=2,
                adc_count=2,
                dac_count=0,
                timer_count=8,
                dma_controller_count=2,
                dma_stream_count=7
            ),
            feature=FeatureConfig(
                has_fpu=False,
                has_dsp=False,
                has_mpu=False,
                has_cache=False,
                has_dma=True,
                has_crypto=False,
                has_ethernet=False,
                has_usb=True
            ),
            module=ModuleConfig(
                power_management_enabled=True,
                memory_monitor_enabled=True,
                watchdog_enabled=True,
                gpio_abstraction_enabled=True,
                uart_abstraction_enabled=True,
                spi_abstraction_enabled=True,
                i2c_abstraction_enabled=True,
                adc_abstraction_enabled=True,
                dac_abstraction_enabled=False,
                dma_abstraction_enabled=True,
                security_enabled=False,
                network_enabled=False,
                ota_enabled=False,
                debug_tools_enabled=True
            ),
            debug=DebugConfig(
                debug_enabled=True,
                error_check_enabled=True,
                performance_profiling_enabled=False,
                system_tracing_enabled=True,
                memory_leak_detection_enabled=True
            )
        )
        
        # STM32F767配置
        self.platforms[PlatformType.STM32F7] = PlatformConfig(
            platform_type=PlatformType.STM32F7,
            platform_name="STM32F767VIT6",
            clock=ClockConfig(
                system_clock_freq=216000000,
                cpu_clock_freq=216000000,
                apb1_clock_freq=54000000,
                apb2_clock_freq=108000000,
                timer_clock_freq=108000000,
                timer_resolution_ns=9
            ),
            memory=MemoryConfig(
                flash_size_kb=2048,
                ram_size_kb=512,
                ccm_size_kb=0,
                heap_size_kb=256,
                stack_size_kb=32
            ),
            peripheral=PeripheralConfig(
                gpio_port_count=11,
                uart_count=8,
                spi_count=6,
                i2c_count=4,
                adc_count=3,
                dac_count=2,
                timer_count=17,
                dma_controller_count=2,
                dma_stream_count=8
            ),
            feature=FeatureConfig(
                has_fpu=True,
                has_dsp=True,
                has_mpu=True,
                has_cache=True,
                has_dma=True,
                has_crypto=True,
                has_ethernet=True,
                has_usb=True
            ),
            module=ModuleConfig(
                power_management_enabled=True,
                memory_monitor_enabled=True,
                watchdog_enabled=True,
                gpio_abstraction_enabled=True,
                uart_abstraction_enabled=True,
                spi_abstraction_enabled=True,
                i2c_abstraction_enabled=True,
                adc_abstraction_enabled=True,
                dac_abstraction_enabled=True,
                dma_abstraction_enabled=True,
                security_enabled=True,
                network_enabled=True,
                ota_enabled=True,
                debug_tools_enabled=True
            ),
            debug=DebugConfig(
                debug_enabled=True,
                error_check_enabled=True,
                performance_profiling_enabled=True,
                system_tracing_enabled=True,
                memory_leak_detection_enabled=True
            )
        )
    
    def generate_config_header(self, platform: PlatformType, output_file: str) -> bool:
        """生成配置头文件"""
        
        if platform not in self.platforms:
            print(f"错误: 不支持的平台 {platform}")
            return False
        
        config = self.platforms[platform]
        
        header_content = f'''/**
 * @file hw_config_generated.h
 * @brief RTOS硬件配置 - 自动生成 ({config.platform_name})
 * @author Config Generator Tool
 * @date 2024
 */

#ifndef __RTOS_HW_CONFIG_GENERATED_H__
#define __RTOS_HW_CONFIG_GENERATED_H__

/* 平台标识 */
#define RTOS_PLATFORM_NAME                "{config.platform_name}"
#define RTOS_PLATFORM_TYPE                {platform.value.upper()}

/* 时钟配置 */
#define RTOS_HW_SYSTEM_CLOCK_FREQ          {config.clock.system_clock_freq}
#define RTOS_HW_CPU_CLOCK_FREQ             {config.clock.cpu_clock_freq}
#define RTOS_HW_APB1_CLOCK_FREQ            {config.clock.apb1_clock_freq}
#define RTOS_HW_APB2_CLOCK_FREQ            {config.clock.apb2_clock_freq}
#define RTOS_HW_TIMER_CLOCK_FREQ           {config.clock.timer_clock_freq}
#define RTOS_HW_TIMER_RESOLUTION_NS        {config.clock.timer_resolution_ns}

/* 内存配置 */
#define RTOS_HW_FLASH_SIZE                 {config.memory.flash_size_kb}
#define RTOS_HW_SRAM_SIZE                  {config.memory.ram_size_kb}
#define RTOS_HW_CCM_SIZE                   {config.memory.ccm_size_kb}
#define RTOS_HW_HEAP_SIZE                  {config.memory.heap_size_kb}
#define RTOS_HW_STACK_SIZE                 {config.memory.stack_size_kb}

/* 外设配置 */
#define RTOS_HW_GPIO_PORT_COUNT            {config.peripheral.gpio_port_count}
#define RTOS_HW_UART_COUNT                 {config.peripheral.uart_count}
#define RTOS_HW_SPI_COUNT                  {config.peripheral.spi_count}
#define RTOS_HW_I2C_COUNT                  {config.peripheral.i2c_count}
#define RTOS_HW_ADC_COUNT                  {config.peripheral.adc_count}
#define RTOS_HW_DAC_COUNT                  {config.peripheral.dac_count}
#define RTOS_HW_TIMER_COUNT                {config.peripheral.timer_count}
#define RTOS_HW_DMA_CONTROLLER_COUNT       {config.peripheral.dma_controller_count}
#define RTOS_HW_DMA_STREAM_COUNT           {config.peripheral.dma_stream_count}

/* 硬件特性配置 */
#define RTOS_HW_SUPPORT_FPU                {1 if config.feature.has_fpu else 0}
#define RTOS_HW_SUPPORT_DSP                {1 if config.feature.has_dsp else 0}
#define RTOS_HW_SUPPORT_MPU                {1 if config.feature.has_mpu else 0}
#define RTOS_HW_SUPPORT_CACHE              {1 if config.feature.has_cache else 0}
#define RTOS_HW_SUPPORT_DMA                {1 if config.feature.has_dma else 0}
#define RTOS_HW_SUPPORT_CRYPTO             {1 if config.feature.has_crypto else 0}
#define RTOS_HW_SUPPORT_ETHERNET           {1 if config.feature.has_ethernet else 0}
#define RTOS_HW_SUPPORT_USB                {1 if config.feature.has_usb else 0}

/* 模块使能配置 */
#define RTOS_HW_ENABLE_POWER_MGMT          {1 if config.module.power_management_enabled else 0}
#define RTOS_HW_ENABLE_MEMORY_MONITOR      {1 if config.module.memory_monitor_enabled else 0}
#define RTOS_HW_ENABLE_WATCHDOG            {1 if config.module.watchdog_enabled else 0}
#define RTOS_HW_ENABLE_GPIO                {1 if config.module.gpio_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_UART                {1 if config.module.uart_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_SPI                 {1 if config.module.spi_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_I2C                 {1 if config.module.i2c_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_ADC                 {1 if config.module.adc_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_DAC                 {1 if config.module.dac_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_DMA                 {1 if config.module.dma_abstraction_enabled else 0}
#define RTOS_HW_ENABLE_SECURITY            {1 if config.module.security_enabled else 0}
#define RTOS_HW_ENABLE_NETWORK             {1 if config.module.network_enabled else 0}
#define RTOS_HW_ENABLE_OTA                 {1 if config.module.ota_enabled else 0}
#define RTOS_HW_ENABLE_DEBUG_TOOLS         {1 if config.module.debug_tools_enabled else 0}

/* 调试配置 */
#define RTOS_HW_DEBUG                      {1 if config.debug.debug_enabled else 0}
#define RTOS_HW_ERROR_CHECK                {1 if config.debug.error_check_enabled else 0}
#define RTOS_PERFORMANCE_PROFILING_ENABLED {1 if config.debug.performance_profiling_enabled else 0}
#define RTOS_SYSTEM_TRACING_ENABLED        {1 if config.debug.system_tracing_enabled else 0}
#define RTOS_MEMORY_LEAK_DETECTION         {1 if config.debug.memory_leak_detection_enabled else 0}

/* 模块特定调试配置 */
#define RTOS_POWER_DEBUG                   RTOS_HW_DEBUG
#define RTOS_POWER_ERROR_CHECK             RTOS_HW_ERROR_CHECK
#define RTOS_MEMORY_DEBUG                  RTOS_HW_DEBUG
#define RTOS_MEMORY_ERROR_CHECK            RTOS_HW_ERROR_CHECK
#define RTOS_WATCHDOG_DEBUG                RTOS_HW_DEBUG
#define RTOS_WATCHDOG_ERROR_CHECK          RTOS_HW_ERROR_CHECK
#define RTOS_GPIO_DEBUG                    RTOS_HW_DEBUG
#define RTOS_GPIO_ERROR_CHECK              RTOS_HW_ERROR_CHECK
#define RTOS_UART_DEBUG                    RTOS_HW_DEBUG
#define RTOS_UART_ERROR_CHECK              RTOS_HW_ERROR_CHECK
#define RTOS_SPI_DEBUG                     RTOS_HW_DEBUG
#define RTOS_SPI_ERROR_CHECK               RTOS_HW_ERROR_CHECK
#define RTOS_I2C_DEBUG                     RTOS_HW_DEBUG
#define RTOS_I2C_ERROR_CHECK               RTOS_HW_ERROR_CHECK
#define RTOS_ADC_DEBUG                     RTOS_HW_DEBUG
#define RTOS_ADC_ERROR_CHECK               RTOS_HW_ERROR_CHECK
#define RTOS_DAC_DEBUG                     RTOS_HW_DEBUG
#define RTOS_DAC_ERROR_CHECK               RTOS_HW_ERROR_CHECK
#define RTOS_DMA_DEBUG                     RTOS_HW_DEBUG
#define RTOS_DMA_ERROR_CHECK               RTOS_HW_ERROR_CHECK

/* 平台特定宏定义 */
'''

        if platform == PlatformType.STM32F4:
            header_content += '''
#define STM32F40_41xxx                     1
#define USE_STDPERIPH_DRIVER               1
#define HSE_VALUE                          25000000
'''
        elif platform == PlatformType.STM32F1:
            header_content += '''
#define STM32F10X_HD                       1
#define USE_STDPERIPH_DRIVER               1
#define HSE_VALUE                          8000000
'''
        elif platform == PlatformType.STM32F7:
            header_content += '''
#define STM32F767xx                        1
#define USE_HAL_DRIVER                     1
#define HSE_VALUE                          25000000
'''
        
        header_content += '''
#endif /* __RTOS_HW_CONFIG_GENERATED_H__ */
'''
        
        try:
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(header_content)
            print(f"配置头文件生成成功: {output_file}")
            return True
        except Exception as e:
            print(f"生成配置头文件失败: {e}")
            return False
    
    def generate_makefile(self, platform: PlatformType, output_file: str) -> bool:
        """生成Makefile"""
        
        if platform not in self.platforms:
            print(f"错误: 不支持的平台 {platform}")
            return False
        
        config = self.platforms[platform]
        
        makefile_content = f'''# RTOS硬件抽象层Makefile - 自动生成 ({config.platform_name})
# Generated by Config Generator Tool

# 平台配置
PLATFORM = {platform.value}
PLATFORM_NAME = {config.platform_name}

# 编译器配置
'''

        if platform in [PlatformType.STM32F1, PlatformType.STM32F4, PlatformType.STM32F7]:
            makefile_content += '''CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# CPU配置
CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
'''
        elif platform == PlatformType.RISC_V:
            makefile_content += '''CC = riscv64-unknown-elf-gcc
OBJCOPY = riscv64-unknown-elf-objcopy
SIZE = riscv64-unknown-elf-size

# CPU配置
CPU = -march=rv32imac -mabi=ilp32
'''
        
        makefile_content += f'''
# 编译标志
CFLAGS = $(CPU) -Os -g3 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000

# 包含路径
INCLUDES = -I. -Irtos -Irtos/core -Irtos/hw -Irtos/hw/security -Irtos/hw/network -Irtos/hw/ota -Irtos/hw/debug
INCLUDES += -Ifwlib/inc -Ifwlib/CMSIS/STM32F4xx/Include -Ifwlib/CMSIS/Include

# 源文件
SOURCES = main_hw_enhanced.c system_support.c
SOURCES += rtos/system.c
SOURCES += rtos/hw/hw_abstraction.c rtos/hw/interrupt_handler.c

# 新增模块源文件
'''

        # 根据模块配置添加源文件
        if config.module.power_management_enabled:
            makefile_content += "SOURCES += rtos/hw/power_management.c\n"
        
        if config.module.memory_monitor_enabled:
            makefile_content += "SOURCES += rtos/hw/memory_monitor.c\n"
        
        if config.module.watchdog_enabled:
            makefile_content += "SOURCES += rtos/hw/watchdog_manager.c\n"
        
        if config.module.gpio_abstraction_enabled:
            makefile_content += "SOURCES += rtos/hw/gpio_abstraction.c\n"
        
        if config.module.uart_abstraction_enabled:
            makefile_content += "SOURCES += rtos/hw/uart_abstraction.c\n"
        
        if config.module.spi_abstraction_enabled:
            makefile_content += "SOURCES += rtos/hw/spi_abstraction.c\n"
        
        if config.module.i2c_abstraction_enabled:
            makefile_content += "SOURCES += rtos/hw/i2c_abstraction.c\n"
        
        if config.module.dma_abstraction_enabled:
            makefile_content += "SOURCES += rtos/hw/dma_abstraction.c\n"
        
        makefile_content += '''
# 固件库源文件
FWLIB_SOURCES = fwlib/src/stm32f4xx_rcc.c fwlib/src/stm32f4xx_tim.c
FWLIB_SOURCES += fwlib/src/stm32f4xx_gpio.c fwlib/src/stm32f4xx_usart.c
FWLIB_SOURCES += fwlib/src/stm32f4xx_pwr.c fwlib/src/stm32f4xx_adc.c
FWLIB_SOURCES += fwlib/src/stm32f4xx_iwdg.c fwlib/src/stm32f4xx_wwdg.c
FWLIB_SOURCES += fwlib/src/stm32f4xx_dma.c fwlib/src/stm32f4xx_spi.c
FWLIB_SOURCES += fwlib/src/stm32f4xx_i2c.c fwlib/src/stm32f4xx_dac.c
FWLIB_SOURCES += fwlib/src/misc.c

# 目标文件
OBJECTS = $(SOURCES:.c=.o) $(FWLIB_SOURCES:.c=.o) startup_stm32f407xx.o

# 链接脚本
LDSCRIPT = STM32F407VGTx_FLASH.ld

# 链接标志
LDFLAGS = $(CPU) -specs=nano.specs -specs=nosys.specs
LDFLAGS += -T$(LDSCRIPT) -Wl,--gc-sections -Wl,--print-memory-usage

# 目标
TARGET = rtos_hw_enhanced

.PHONY: all clean

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJECTS)
\t$(CC) $(LDFLAGS) $^ -o $@ -lm
\t$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
\t$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
\t$(OBJCOPY) -O ihex $< $@

%.o: %.c
\t$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.s
\t$(CC) $(CFLAGS) -c $< -o $@

clean:
\t@echo "清理编译文件..."
\trm -f $(OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

info:
\t@echo "平台: $(PLATFORM_NAME)"
\t@echo "编译器: $(CC)"
\t@echo "源文件数: $(words $(SOURCES))"
\t@echo "固件库文件数: $(words $(FWLIB_SOURCES))"
'''
        
        try:
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(makefile_content)
            print(f"Makefile生成成功: {output_file}")
            return True
        except Exception as e:
            print(f"生成Makefile失败: {e}")
            return False
    
    def generate_cmake_file(self, platform: PlatformType, output_file: str) -> bool:
        """生成CMakeLists.txt"""
        
        if platform not in self.platforms:
            print(f"错误: 不支持的平台 {platform}")
            return False
        
        config = self.platforms[platform]
        
        cmake_content = f'''# RTOS硬件抽象层CMake配置 - 自动生成 ({config.platform_name})
# Generated by Config Generator Tool

cmake_minimum_required(VERSION 3.16)

# 项目配置
project(rtos_hw_enhanced
    VERSION 1.0.0
    DESCRIPTION "RTOS Hardware Abstraction Layer Enhanced"
    LANGUAGES C ASM
)

# 平台配置
set(PLATFORM_TYPE {platform.value})
set(PLATFORM_NAME {config.platform_name})

# 编译器配置
'''

        if platform in [PlatformType.STM32F1, PlatformType.STM32F4, PlatformType.STM32F7]:
            cmake_content += '''set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# CPU配置
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
'''
        
        cmake_content += f'''
# 编译标志
set(CMAKE_C_FLAGS "${{CPU_FLAGS}} -Os -g3 -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER -DHSE_VALUE=25000000")

# 包含目录
include_directories(
    .
    rtos
    rtos/core
    rtos/hw
    rtos/hw/security
    rtos/hw/network
    rtos/hw/ota
    rtos/hw/debug
    fwlib/inc
    fwlib/CMSIS/STM32F4xx/Include
    fwlib/CMSIS/Include
)

# 源文件
set(SOURCES
    main_hw_enhanced.c
    system_support.c
    rtos/system.c
    rtos/hw/hw_abstraction.c
    rtos/hw/interrupt_handler.c
'''

        # 根据模块配置添加源文件
        if config.module.power_management_enabled:
            cmake_content += "    rtos/hw/power_management.c\n"
        
        if config.module.memory_monitor_enabled:
            cmake_content += "    rtos/hw/memory_monitor.c\n"
        
        if config.module.watchdog_enabled:
            cmake_content += "    rtos/hw/watchdog_manager.c\n"
        
        if config.module.gpio_abstraction_enabled:
            cmake_content += "    rtos/hw/gpio_abstraction.c\n"
        
        if config.module.uart_abstraction_enabled:
            cmake_content += "    rtos/hw/uart_abstraction.c\n"
        
        if config.module.dma_abstraction_enabled:
            cmake_content += "    rtos/hw/dma_abstraction.c\n"
        
        cmake_content += ''')

# 固件库源文件
set(FWLIB_SOURCES
    fwlib/src/stm32f4xx_rcc.c
    fwlib/src/stm32f4xx_tim.c
    fwlib/src/stm32f4xx_gpio.c
    fwlib/src/stm32f4xx_usart.c
    fwlib/src/stm32f4xx_pwr.c
    fwlib/src/stm32f4xx_adc.c
    fwlib/src/stm32f4xx_iwdg.c
    fwlib/src/stm32f4xx_wwdg.c
    fwlib/src/stm32f4xx_dma.c
    fwlib/src/misc.c
)

# 创建可执行文件
add_executable(${PROJECT_NAME}.elf
    ${SOURCES}
    ${FWLIB_SOURCES}
    startup_stm32f407xx.s
)

# 链接配置
set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    LINK_FLAGS "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -T${CMAKE_SOURCE_DIR}/STM32F407VGTx_FLASH.ld -Wl,--gc-sections -Wl,--print-memory-usage"
)

target_link_libraries(${PROJECT_NAME}.elf m)

# 生成二进制文件
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "生成二进制文件和显示大小信息"
)
'''
        
        try:
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(cmake_content)
            print(f"CMakeLists.txt生成成功: {output_file}")
            return True
        except Exception as e:
            print(f"生成CMakeLists.txt失败: {e}")
            return False
    
    def save_config_json(self, platform: PlatformType, output_file: str) -> bool:
        """保存配置为JSON文件"""
        
        if platform not in self.platforms:
            return False
        
        config = self.platforms[platform]
        
        try:
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(asdict(config), f, indent=2, ensure_ascii=False)
            print(f"配置JSON文件保存成功: {output_file}")
            return True
        except Exception as e:
            print(f"保存配置JSON文件失败: {e}")
            return False
    
    def load_config_json(self, input_file: str) -> Optional[PlatformConfig]:
        """从JSON文件加载配置"""
        
        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                config_dict = json.load(f)
            
            # 转换为PlatformConfig对象
            config = PlatformConfig(**config_dict)
            print(f"配置JSON文件加载成功: {input_file}")
            return config
        except Exception as e:
            print(f"加载配置JSON文件失败: {e}")
            return None
    
    def list_platforms(self):
        """列出所有支持的平台"""
        print("支持的平台:")
        for platform_type, config in self.platforms.items():
            print(f"  {platform_type.value}: {config.platform_name}")
            print(f"    时钟: {config.clock.system_clock_freq//1000000}MHz")
            print(f"    内存: Flash {config.memory.flash_size_kb}KB, RAM {config.memory.ram_size_kb}KB")
            print(f"    特性: FPU={config.feature.has_fpu}, DSP={config.feature.has_dsp}, Cache={config.feature.has_cache}")
            print()

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='RTOS硬件抽象层配置生成工具')
    parser.add_argument('--platform', type=str, choices=[p.value for p in PlatformType],
                       help='目标平台')
    parser.add_argument('--output-dir', type=str, default='generated',
                       help='输出目录')
    parser.add_argument('--config-file', type=str,
                       help='配置JSON文件路径')
    parser.add_argument('--list-platforms', action='store_true',
                       help='列出所有支持的平台')
    parser.add_argument('--generate-all', action='store_true',
                       help='生成所有平台的配置')
    
    args = parser.parse_args()
    
    generator = ConfigGenerator()
    
    if args.list_platforms:
        generator.list_platforms()
        return
    
    if args.generate_all:
        print("生成所有平台配置...")
        for platform in PlatformType:
            platform_dir = os.path.join(args.output_dir, platform.value)
            
            # 生成配置头文件
            header_file = os.path.join(platform_dir, "hw_config_generated.h")
            generator.generate_config_header(platform, header_file)
            
            # 生成Makefile
            makefile = os.path.join(platform_dir, "Makefile")
            generator.generate_makefile(platform, makefile)
            
            # 生成CMakeLists.txt
            cmake_file = os.path.join(platform_dir, "CMakeLists.txt")
            generator.generate_cmake_file(platform, cmake_file)
            
            # 保存配置JSON
            json_file = os.path.join(platform_dir, "platform_config.json")
            generator.save_config_json(platform, json_file)
        
        print("所有平台配置生成完成！")
        return
    
    if not args.platform:
        print("错误: 请指定目标平台或使用 --list-platforms 查看支持的平台")
        return
    
    platform = PlatformType(args.platform)
    platform_dir = os.path.join(args.output_dir, platform.value)
    
    # 如果指定了配置文件，先加载自定义配置
    if args.config_file:
        custom_config = generator.load_config_json(args.config_file)
        if custom_config:
            generator.platforms[platform] = custom_config
    
    print(f"为平台 {platform.value} 生成配置...")
    
    # 生成配置头文件
    header_file = os.path.join(platform_dir, "hw_config_generated.h")
    generator.generate_config_header(platform, header_file)
    
    # 生成Makefile
    makefile = os.path.join(platform_dir, "Makefile")
    generator.generate_makefile(platform, makefile)
    
    # 生成CMakeLists.txt
    cmake_file = os.path.join(platform_dir, "CMakeLists.txt")
    generator.generate_cmake_file(platform, cmake_file)
    
    # 保存配置JSON
    json_file = os.path.join(platform_dir, "platform_config.json")
    generator.save_config_json(platform, json_file)
    
    print(f"平台 {platform.value} 配置生成完成！")
    print(f"输出目录: {platform_dir}")

if __name__ == "__main__":
    main()