LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/msm_shared

PLATFORM := mdm9x15

MEMBASE ?= 0x41700000
MEMSIZE ?= 0x00100000 # 1MB
SCRATCH_ADDR := BASE_ADDR+0x05000000
SCRATCH_REGION1 := BASE_ADDR+0x05000000
SCRATCH_REGION1_SIZE := 0x01700000 #23MB
SCRATCH_REGION2 := 0x40800000
SCRATCH_REGION2_SIZE := 0x00F00000 #15MB


BASE_ADDR        := 0x40800000

DEFINES += NO_KEYPAD_DRIVER=1

MODULES += \
	dev/keys \
	dev/ssbi \
	dev/pmic/pm8921 \
	lib/ptable

DEFINES += \
	SDRAM_SIZE=$(MEMSIZE) \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
	BASE_ADDR=$(BASE_ADDR) \
	TAGS_ADDR=$(TAGS_ADDR) \
	KERNEL_ADDR=$(KERNEL_ADDR) \
	RAMDISK_ADDR=$(RAMDISK_ADDR) \
	SCRATCH_ADDR=$(SCRATCH_ADDR) \
	SCRATCH_REGION1=$(SCRATCH_REGION1) \
	SCRATCH_REGION2=$(SCRATCH_REGION2) \
	SCRATCH_REGION1_SIZE=$(SCRATCH_REGION1_SIZE) \
	SCRATCH_REGION2_SIZE=$(SCRATCH_REGION2_SIZE)

OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/atags.o \
	$(LOCAL_DIR)/keypad.o
