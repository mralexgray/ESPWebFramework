
# CONFIG

SPEED 			:= 230400 ## vs. 115200
# linking libgccirom.a instead of libgcc.a causes reset when working with flash memory (ie spi_flash_erase_sector)
# linking libcirom.a causes conflicts with come std c routines (like strstr, strchr...)
LINK_LIBS   := minic m gcc hal phy pp net80211 wpa main freertos lwip

# END CONFIG

          BUILD_DIR := build/debug
release:  BUILD_DIR := build/release

# homebrew installs the toolchain, but neglects to put it in your $PATH
ifeq ($(shell uname), Darwin)
	SHELL 		 	:= $(shell which zsh)
	PORT 			 	?= $(lastword $(wildcard /dev/tty.*))
	BIN_EXEC 	 	?= /opt/xtensa-lx106-elf/bin
	export PATH := $(BIN_EXEC)subst:$(PATH)
else
	PORT 			 	?= COM4
	# get SDK path from environment variable ESP8266SDK
	SDKBASE  	 	?= $(subst \,/,$(ESP8266SDK))
endif

# this directory
ROOT_DIR   :=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
SDKBASE		 ?= $(ROOT_DIR)/sdk
SRCDIR 		 := src

WWW_DIR		 := www.altº
WWW_BIN    := webcontent.bin
WWW_MAXSZ	 := 57344
COMPRESSOR := script/binarydir.py

# paths and executables
ESPTOOL    := python $(SDKBASE)/esptool.py
ESP_CMD 	 := $(ESPTOOL) --port $(PORT) --baud $(SPEED)

RTOS       := esp_iot_rtos_sdk
RTOS_SDK   := $(SDKBASE)/$(RTOS)
RTOS_BASE  := $(RTOS_SDK)/include

XELF 			 := xtensa-lx106-elf
AR         := $(BIN_EXEC)/$(XELF)-ar
CC         := $(BIN_EXEC)/$(XELF)-g++
CPP        := $(CC)
LD         := $(CC)
NM         := $(BIN_EXEC)/xt-nm
OBJCOPY    := $(BIN_EXEC)/$(XELF)-objcopy
OD         := $(BIN_EXEC)/$(XELF)-objdump

LIBS       := $(addprefix -l, $(LINK_LIBS))

# options/flags/inlcudes

INCLUDES   := -I include -I $(RTOS_BASE)
INCLUDES   += $(addprefix -I $(RTOS_BASE)/, espressif lwip lwip/lwip lwip/ipv4 lwip/ipv6)
INCLUDES   += -I $(RTOS_SDK)/extra_include
INCLUDES   += -I $(SDKBASE)/$(XELF)/xtensa-lx106-elf/include
FORCE_NO   := exceptions rtti inline-functions threadsafe-statics use-cxa-atexit
YES_WARN   := pointer-arith undef error
# don't change -Os (or add other -O options) otherwise FLASHMEM and FSTR data will be duplicated in RAM
CFLAGS      = -g -Os $(addprefix -W, $(YES_WARN)) -Wl,-EL \
              -nostdlib -mlongcalls -mtext-section-literals   \
              $(addprefix -fno-, $(FORCE_NO))                 \
              -DICACHE_FLASH -D__ets__
              
LDFLAGS     = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -Wl,--gc-sections
LD_SCRIPT   = eagle.app.v6.ld

SDK_LIBDIR := -L$(RTOS_SDK)/lib
ELF_LIBDIR := -L$(SDKBASE)/xtensa-lx106-elf/lib
SDK_LDDIR   = $(RTOS_SDK)/ld

TARGET_OUT := $(BUILD_DIR)/app.out
BINS       := $(addprefix $(TARGET_OUT),-0x00000.bin -0x11000.bin)
OBJ  			 := $(addprefix $(BUILD_DIR)/, 																									\
							$(addsuffix .o, user_main 																									\
							$(addprefix fdv_, serial sync utils flash printf debug strings network 			\
																collections confmanager datetime serialserv task gpio)))
WWW_CONTENT = $(BUILD_DIR)/$(WWW_BIN)
WWW_ADDRS		= 0x6D000

.PHONY: all flash clean flashweb flashdump flasherase fresh mkdirs submodules $(WWW_CONTENT) format

all: mkdirs $(BINS)

submodules: .gitmodules sdk/esp_iot_rtos_sdk
	@-git submodule update --init --recursive

mkdirs: submodules
	@-mkdir -p $(BUILD_DIR)

$(TARGET_OUT): $(BUILD_DIR)/libuser.a
	$(LD) $(ELF_LIBDIR) $(SDK_LIBDIR) -T$(SDK_LDDIR)/$(LD_SCRIPT) \
        -Wl,-M >$(BUILD_DIR)/out.map $(LDFLAGS)                 \
        -Wl,--start-group $(LIBS) $^                            \
	      -Wl,--end-group -o $@
	$(OD) -h -j .data -j .rodata -j .bss -j .text -j .irom0.text $@
	$(OD) -t -j .text $@ >$(BUILD_DIR)/_text_content.map
	$(OD) -t -j .irom0.text $@ > $(BUILD_DIR)/_irom0_text_content.map

$(BINS): $(TARGET_OUT)
	$(ESPTOOL) elf2image $^
	@echo "The binaries are done. \"make flash\" to burn them to device, if needed"

$(BUILD_DIR)/libuser.a: $(OBJ)
	$(AR) cru $@ $^
	
# $(BUILD_DIR)/%.o: $(SRCDIR)/%.c $(wildcard $(SRCDIR)/*.h)
# 	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

INCS := $(wildcard include/*)

$(BUILD_DIR)/%.o: $(SRCDIR)/%.cpp $(INCS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(WWW_CONTENT): mkdirs
	python $(COMPRESSOR) $(WWW_DIR) $@ $(WWW_MAXSZ)

flash: flashbins flashweb

flashbins: needs_port
	$(ESP_CMD) write_flash 0x11000 $(TARGET_OUT)-0x11000.bin 0x00000 $(TARGET_OUT)-0x00000.bin

flashweb: $(WWW_CONTENT)
	$(ESP_CMD) write_flash $(WWW_ADDRS) $^

flashdump: needs_port
	$(ESP_CMD) read_flash 0x0000 0x80000 flash.dump
	xxd flash.dump > flash.hex
	$(shell cat flash.hex)

flasherase: needs_port
	$(ESPTOOL) --port $(PORT) erase_flash

clean:
	# -$(shell if [[ -d $(BUILD_DIR) ]]; then rm -rf $(BUILD_DIR); fi)
	rm -rf $(BUILD_DIR) *.(dump|hex)
	$(shell git checkout -- build/release/)

fresh: clean flash

FORMATEES := $(wildcard $(SRCDIR)/*) $(INCS)

format: $(FORMATEES) .clang-format
	clang-format -i $^

needs_port:

#	ifeq ($(wildcard $(PORT)), )
#		$(warning [PORT] is $(PORT))
#		$(error must specify existant port!)
# endif
#	ifneq(,$(findstring Bluetooth-Incoming-Port,$(PORT))))
#		$(error Not that port!)

usage:
	@echo "make         	builds in build/debug (untracked)"
	@echo "make release 	builds (git tracked) bins for dist in build/release"
	@echo "make clean   	cleans build directory, resets release to checked out bins"
