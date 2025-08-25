# Makefile for STM32F407 RTOS
# 工具链配置
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

# 目标芯片
MCU = cortex-m4

# 编译标志
CFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -Wall -Wextra -O2 -g
CFLAGS += -DSTM32F407xx -DUSE_HAL_DRIVER
CFLAGS += -ffunction-sections -fdata-sections

# 汇编标志
ASFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# 链接标志
LDFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections -Wl,-Map=output.map
LDFLAGS += -T STM32F407VGTx_FLASH.ld

# 包含路径
INCLUDES = -I.

# 源文件
C_SOURCES = \
	main.c \
	rtos_kernel.c \
	rtos_sync.c \
	rtos_hw.c

ASM_SOURCES = \
	rtos_asm.s \
	startup_stm32f407xx.s

# 目标文件
OBJECTS = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.s=.o)

# 目标名称
TARGET = rtos_demo

# 默认目标
all: $(TARGET).elf $(TARGET).hex $(TARGET).bin size

# ELF文件生成
$(TARGET).elf: $(OBJECTS)
	@echo "链接 $@"
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@

# HEX文件生成
$(TARGET).hex: $(TARGET).elf
	@echo "生成 $@"
	$(OBJCOPY) -O ihex $< $@

# BIN文件生成
$(TARGET).bin: $(TARGET).elf
	@echo "生成 $@"
	$(OBJCOPY) -O binary $< $@

# C文件编译
%.o: %.c
	@echo "编译 $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 汇编文件编译
%.o: %.s
	@echo "汇编 $<"
	$(AS) $(ASFLAGS) $< -o $@

# 显示大小信息
size: $(TARGET).elf
	@echo "程序大小信息:"
	$(SIZE) $<

# 清理
clean:
	@echo "清理中..."
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).hex $(TARGET).bin output.map

# 下载到芯片
flash: $(TARGET).hex
	@echo "下载程序到STM32F407..."
	# 这里可以添加具体的下载命令，如使用ST-Link或J-Link
	# st-flash write $(TARGET).bin 0x8000000

# 调试
debug: $(TARGET).elf
	@echo "启动调试..."
	# arm-none-eabi-gdb $(TARGET).elf

# 防止文件名冲突
.PHONY: all clean flash debug size

# 依赖关系
main.o: main.c rtos_kernel.h
rtos_kernel.o: rtos_kernel.c rtos_kernel.h rtos_hw.h
rtos_sync.o: rtos_sync.c rtos_kernel.h
rtos_hw.o: rtos_hw.c rtos_hw.h rtos_kernel.h
rtos_asm.o: rtos_asm.s