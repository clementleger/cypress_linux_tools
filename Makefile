HOST_BOOTLOADER_DIR := ./host_bootloader_src
BUILD_DIR := ./build
SRC_DIR := ./src

SRC_FILES := $(wildcard $(HOST_BOOTLOADER_DIR)/*.c)
OBJ_FILES := $(subst $(HOST_BOOTLOADER_DIR),$(BUILD_DIR),$(patsubst %.c,%.o,$(SRC_FILES)))
HDR_FILES := $(wildcard $(BUILD_DIR)/*.h)

ifeq ($(SRC_FILES),)
	dummy := $(error Please copy the host bootloader sources into $(HOST_BOOTLOADER_DIR))
endif

CFLAGS := -I$(HOST_BOOTLOADER_DIR) -DCALL_CON= -g
LFLAGS := -lrt

all: cyhostboot

$(BUILD_DIR)/%.o: $(HOST_BOOTLOADER_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $^ $(CFLAGS) 

cyhostboot: $(OBJ_FILES) $(SRC_DIR)/cyhostboot.c
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -rf cyhostboot $(BUILD_DIR)
