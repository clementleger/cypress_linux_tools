HOST_BOOTLOADER_DIR := ./host_bootloader_src
BUILD_DIR := ./build
SRC_DIR := ./src

SRC_FILES := $(wildcard $(HOST_BOOTLOADER_DIR)/*.c)
OBJ_FILES := $(subst $(HOST_BOOTLOADER_DIR),$(BUILD_DIR),$(patsubst %.c,%.o,$(SRC_FILES)))
HDR_FILES := $(wildcard $(BUILD_DIR)/*.h)

ifeq ($(SRC_FILES),)
	dummy := $(error Please copy the host bootloader sources into $(HOST_BOOTLOADER_DIR))
endif

CFLAGS := -I$(HOST_BOOTLOADER_DIR) -I$(BUILD_DIR) -DCALL_CON= -g -Wall
LFLAGS := -lrt

all: cyhostboot

$(BUILD_DIR)/cyhostboot_cmdline.c: $(SRC_DIR)/cyhostboot.ggo
	gengetopt -i $< -F cyhostboot_cmdline --output-dir=$(BUILD_DIR)/ --func-name=cyhostboot_cmdline_parser -a cyhostboot_args_info

$(BUILD_DIR)/%.o: $(HOST_BOOTLOADER_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $^ $(CFLAGS)

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -o $@ $^ $(CFLAGS) 

cyhostboot: $(OBJ_FILES) $(SRC_DIR)/cyhostboot.c $(BUILD_DIR)/cyhostboot_cmdline.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -rf cyhostboot $(BUILD_DIR)

install:
	cp cyhostboot /bin/
	
