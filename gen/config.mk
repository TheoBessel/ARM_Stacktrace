#############   Project   ############
PROJ_VERSION = 1.0
PROJ_NAME    = cortex-m7-fdir
WORKSPACE 	 = .
######################################


##############   Path    #############
SRC_DIR		 = $(WORKSPACE)/src
BUILD_DIR	 = $(WORKSPACE)/build
BSPs_DIR	 = $(WORKSPACE)/tools/bsp
HALs_DIR	 = $(WORKSPACE)/tools/hal
######################################


##############   Tools   #############
CC      	 = arm-none-eabi-gcc
AR      	 = arm-none-eabi-ar
SIZE    	 = arm-none-eabi-size
READELF 	 = arm-none-eabi-readelf
STRIP   	 = arm-none-eabi-strip
GDB     	 = arm-none-eabi-gdb
EMU			 = qemu-system-arm
######################################


##############   Board   #############
BOARD 		 = MPS2_AN500
LOAD_MEMORY  = ram
VERSION 	 = debug

CHIP 		 = CMSDK_CM7
CHIP_VENDOR  = ARM
CHIP_FAMILLY = CMSDK
MACH 		 = cortex-m7
CORE_SELECT  = -DCORE_CM7
######################################


#############  HAL & BSP  ############
HAL_DIR		 = $(HALs_DIR)/$(CHIP_FAMILLY)_hal_conf.mk
BSP_DIR		 = $(BSPs_DIR)/$(BOARD)-BSP
LINKER		 = $(BSP_DIR)/$(BOARD).ld
######################################


############  Compilation  ###########
BUILD_DIR    = $(WORKSPACE)/build
TARGET 		 = $(BUILD_DIR)/target/$(PROJ_NAME)-$(PROJ_VERSION).elf

CC_FLAGS  	 = -c -mcpu=$(MACH) -std=gnu11 -Werror -Wall -Wextra -pedantic -mthumb
CC_FLAGS 	+= -D$(BOARD) -D$(CHIP)
CC_FLAGS 	+= -g3 -DDEBUG -O0
CC_FLAGS	+= -I$(SRC_DIR)
# Unwind specific
CC_FLAGS 	+= -fexceptions
#CC_FLAGS 	+= -fno-omit-frame-pointer

LD_FLAGS     = -mcpu=$(MACH) -T $(LINKER) -static -Wall -Wextra -pedantic -mthumb
LD_FLAGS	+= --specs=nosys.specs
LD_FLAGS 	+= -D$(BOARD) -D$(CHIP)
######################################
