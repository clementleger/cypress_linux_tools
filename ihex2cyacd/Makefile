BUILD_DIR := ./build
SRC_DIR := ./src

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(subst $(SRC_DIR),$(BUILD_DIR),$(patsubst %.c,%.o,$(SRC_FILES)))
HDR_FILES := $(wildcard $(SRC_DIR)/*.h)


CFLAGS := -g -Wall -I$(BUILD_DIR)
LFLAGS := 

all: check

$(BUILD_DIR)/ihex2cyacd_cmdline.c: $(SRC_DIR)/ihex2cyacd.ggo
	@mkdir -p $(dir $@)
	gengetopt -i $< -F ihex2cyacd_cmdline --output-dir=$(BUILD_DIR)/ --func-name=ihex2cyacd_cmdline_parser -a ihex2cyacd_args_info

$(BUILD_DIR)/ihex2cyacd_cmdline.h: $(BUILD_DIR)/ihex2cyacd_cmdline.c

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/ihex2cyacd_cmdline.h
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $^ $(CFLAGS) 

ihex2cyacd: $(OBJ_FILES) $(BUILD_DIR)/ihex2cyacd_cmdline.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

clean:
	rm -rf ihex2cyacd $(BUILD_DIR)

check: ihex2cyacd
	./ihex2cyacd -i test/Striplight_bootloadable.hex -o out.cyacd -b 4352
	diff out.cyacd test/Striplight_bootloadable.cyacd

install:
	cp ihex2cyacd /bin/

