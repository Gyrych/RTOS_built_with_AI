# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an **STM32F407 RTOS project** using:
- **STM32F407VGTx** microcontroller
- **STM32F4 Standard Peripheral Library V1.8.0**
- **Custom RTOS implementation** (RT-Thread Nano inspired)
- **GCC toolchain** for ARM Cortex-M4
- **EIDE** (Embedded IDE) project configuration

## Project Structure

```
EIDE/
├── .eide/                 # EIDE configuration
│   ├── eide.json         # Main project configuration
│   └── *.ini            # Option bytes configuration
├── build/                # Build output directory
├── STM32F407VGTx_FLASH.ld # Linker script
└── .cmsis/include/       # CMSIS core headers

../
├── User/                 # Application code
│   ├── main.c           # Main application
│   └── config/stm32f4/  # STM32F4 configuration
│       ├── core/        # Startup and system files
│       └── config/      # Peripheral configuration
├── 01_fwlib/            # STM32F4 Standard Peripheral Library
├── 02_rtos/             # Custom RTOS implementation
└── DebugConfig/         # Debug configuration files
```

## Build System

This project uses **EIDE** (Embedded IDE) with **GCC for ARM** toolchain.

### Build Commands

**Build the project:**
```bash
# Using EIDE build system (recommended)
eide build

# Or using direct GCC commands (advanced)
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
  -DSTM32F40_41xxx -DUSE_STDPERIPH_DRIVER \
  -I. -I../../01_fwlib/inc -I../User -I../User/config/stm32f4/config \
  -I../User/config/stm32f4/core -I.cmsis/include -I../../02_rtos \
  -T STM32F407VGTx_FLASH.ld \
  -specs=nosys.specs -specs=nano.specs \
  -o build/output.elf \
  ../User/main.c ../User/config/stm32f4/core/stm32f4xx_it.c \
  ../User/config/stm32f4/core/system_stm32f4xx.c \
  ../../01_fwlib/src/*.c ../../02_rtos/core.c
```

**Clean build:**
```bash
eide clean
rm -rf build/*
```

**Flash to device (ST-Link):**
```bash
eide upload
```

## Key Components

### 1. RTOS Core (`../../02_rtos/`)
- **core.h**: Task management and scheduler interface
- **core.c**: RTOS implementation with:
  - Preemptive task scheduling
  - Priority-based scheduling (0=highest, 31=lowest)
  - Task states (READY, RUNNING, SUSPENDED)
  - Stack management for context switching

### 2. Application Code (`../User/`)
- **main.c**: Multi-task demonstration with:
  - LED blinking task (high priority)
  - Serial print task (medium priority) 
  - Button check task (low priority)
- **main.h**: Hardware definitions for Spark V1 development board

### 3. Hardware Abstraction
- **STM32F4 Standard Library**: Full peripheral driver support
- **CMSIS Core**: Cortex-M4 core support
- **Custom drivers**: LED control, delay functions

## Development Workflow

1. **Edit code** in `../User/` or `../../02_rtos/`
2. **Build** with `eide build`
3. **Flash** to STM32F407 device with `eide upload`
4. **Debug** using ST-Link or J-Link

## Target Hardware

- **MCU**: STM32F407VGTx
- **Board**: Spark V1 development board
- **LED**: GPIOF Pin 11
- **Debug**: ST-Link/V2 or J-Link

## Toolchain Requirements

- **ARM GCC**: arm-none-eabi-gcc
- **EIDE**: Embedded IDE extension for VSCode
- **ST-Link**: ST-Link utility for flashing
- **OpenOCD**: Optional for debugging

## Important Defines

- `STM32F40_41xxx`: MCU family definition
- `USE_STDPERIPH_DRIVER`: Enable Standard Peripheral Library
- `HSE_VALUE=8000000`: 8MHz external crystal

## Memory Layout

- **Flash**: 0x08000000 (1MB)
- **RAM**: 0x20000000 (192KB)
- **Linker Script**: STM32F407VGTx_FLASH.ld