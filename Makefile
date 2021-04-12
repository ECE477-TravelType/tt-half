TARGET = tt-half

DEBUG = 1
OPT = -Og
BUILD_DIR = build

C_SOURCES = src/main.c \
			src/usart.c \
			src/fifo.c \
			src/syscalls.c \
	        src/system_stm32l0xx.c
	        # Src/stm32l0xx_it.c \
	        # Src/stm32l0xx_hal_msp.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_tim.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_tim_ex.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_i2c.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_i2c_ex.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_rcc.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_rcc_ex.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_flash_ramfunc.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_flash.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_flash_ex.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_gpio.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_dma.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_pwr.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_pwr_ex.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_cortex.c \
	        # Drivers/STM32L0xx_HAL_Driver/Src/stm32l0xx_hal_exti.c \

ASM_SOURCES = src/startup_stm32l053xx.s

CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
CP = arm-none-eabi-objcopy
SZ = arm-none-eabi-size

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

CPU = -mcpu=cortex-m0plus
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

AS_DEFS =

# C_DEFS = -DUSE_HAL_DRIVER \
#          -DSTM32L053xx
C_DEFS = -DSTM32L053xx

AS_INCLUDES =

# C_INCLUDES =  \
# -IInc \
# -IDrivers/STM32L0xx_HAL_Driver/Inc \
# -IDrivers/STM32L0xx_HAL_Driver/Inc/Legacy \
# -IDrivers/CMSIS/Device/ST/STM32L0xx/Include \
# -IDrivers/CMSIS/Include

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
# CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -ggdb3 -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

# link script
LDSCRIPT = STM32L053C8Tx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

run: $(BUILD_DIR)/$(TARGET).hex
	openocd -f board/stm32l0discovery.cfg -c "program $(BUILD_DIR)/$(TARGET).hex verify reset"
