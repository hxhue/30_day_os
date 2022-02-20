# External dependencies:
# - qemu-system-x86_64
# - haxm
# - gcc

# About incremental make:
# If dependencies change, `make clean` should be called manually.

# ipl          0x7c00
# bootstrap    0xc200 = 0x7c00 + 0x4200
# entry_point  0x00280000
# stack        0x00300000
# free         0x00400000

BIN        = bin
BUILD      = build
RULEFILE   = src/haribote.rul

NASK      := $(BIN)/nask.exe
GAS2NASK  := $(BIN)/gas2nask.exe -a
OBJ2BIM   := $(BIN)/obj2bim.exe
BIM2HRB   := $(BIN)/bim2hrb.exe
BIN2OBJ   := $(BIN)/bin2obj.exe
MAKEFONT  := $(BIN)/makefont.exe
EDIMG     := $(BIN)/edimg.exe
IMGTOL    := $(BIN)/imgtol.com
COPY      := $(BIN)/cp.exe
DEL       := $(BIN)/rm.exe -f
CAT       := $(BIN)/cat.exe
FIND      := $(BIN)/find.exe
MKDIR     := $(BIN)/mkdir.exe
CC1       := $(BIN)/cc1.exe -Isrc -Isrc/libc -nostdinc -Os -Wall -quiet -std=c99 -Werror

C_FLAGS   := -m32 -Isrc -Isrc/libc -nostdinc -std=c99 -Os -Wall
CXX_FLAGS := -m32 -Isrc -std=c++17 -Os -Wall

QEMU_IMG   = $(BUILD)/os.img
QEMU_RUN   = qemu-system-x86_64 -L . -m 32 -rtc base=localtime -vga std \
			-drive "file=$(QEMU_IMG),format=raw,if=floppy" -accel hax -serial stdio

# Get all C files in src/*/*
C_SOURCES  := $(shell $(FIND) src/ -mindepth 2 -name "*.c")
C_OBJS     := $(patsubst src/%.c,$(BUILD)/%.obj,$(C_SOURCES))

# Get all CPP files in src/*/*
#CPP_SOURCES := $(shell $(FIND) src/ -mindepth 2 -name "*.cpp")
#CPP_OBJS    := $(patsubst src/%.cpp,$(BUILD)/%.obj,$(CPP_SOURCES))

# Get all NASM files in src/*/*
NAS_SOURCES := $(shell $(FIND) src/ -mindepth 2 -name "*.nas")
NAS_OBJS    := $(patsubst src/%.nas,$(BUILD)/%.obj,$(NAS_SOURCES))

all : run

# Test util functions
.PHONY: test clean
test:
	clang -Isrc -Isrc/libc -Dxassert=assert -DXLIBC_H -o $(BUILD)/test.exe $(shell $(FIND) test -name "*.c") $(TEST_SRC)
	@echo "======== Running $(BUILD)/test.exe ========"
	@$(BUILD)/test.exe

# Font
$(BUILD)/hankaku.obj: src/hankaku.txt
	$(MAKEFONT) src/hankaku.txt $(BUILD)/hankaku.bin
	$(BIN2OBJ) $(BUILD)/hankaku.bin $(BUILD)/hankaku.obj _hankaku

# C files
$(C_OBJS) : $(BUILD)/%.obj : src/%.c
	$(MKDIR) -p $(dir $@)
	gcc $(C_FLAGS) -c $< -o $@
#	$(CC1) -o $(patsubst %.obj,%.gas,$@) $<
#	$(GAS2NASK) $(patsubst %.obj,%.gas,$@) $(patsubst %.obj,%.nas,$@)
#	$(NASK) $(patsubst %.obj,%.nas,$@) $@ $(patsubst %.obj,%.lst,$@)

# CPP files
#$(CPP_OBJS) : $(BUILD)/%.obj : src/%.cpp
#	$(MKDIR) -p $(dir $@)
#	g++ $(CXX_FLAGS) -c $< -o $@

# NASM files
$(NAS_OBJS) : $(BUILD)/%.obj : src/%.nas
	$(MKDIR) -p $(dir $@)
	$(NASK) $< $@ $(patsubst %.obj,%.lst,$@)

$(BUILD)/%.bin : src/%.nas
	$(MKDIR) -p $(dir $@)
	$(NASK) $< $@ $(patsubst %.bin,%.lst,$@)

# Combine all objects in floopy storage (from address 0x4200 of the floopy)
# 3MB + 64KB = 3136KB
$(BUILD)/main.bin : $(C_OBJS) $(NAS_OBJS) $(CPP_OBJS) $(BUILD)/hankaku.obj
	$(OBJ2BIM) @$(RULEFILE) out:$(BUILD)/main.bim stack:3136k map:$(BUILD)/main.map \
		$(BUILD)/hankaku.obj $(NAS_OBJS) $(C_OBJS) $(CPP_OBJS)
	$(BIM2HRB) $(BUILD)/main.bim $(BUILD)/main.bin 0

# Create .sys
$(BUILD)/haribote.sys : $(BUILD)/bootstrap.bin $(BUILD)/main.bin
	$(CAT) $^ > $(BUILD)/haribote.sys

# Create image
# "./bin/fdimg0at.tek" causes error.
# "../bin/*" and "bin/*" do not.
img $(QEMU_IMG) : $(BUILD)/ipl10.bin $(BUILD)/haribote.sys
	$(EDIMG) imgin:bin/fdimg0at.tek wbinimg src:$(BUILD)/ipl10.bin \
		len:512 from:0 to:0 \
		copy from:$(BUILD)/haribote.sys to:@: \
		imgout:$(QEMU_IMG)

run : $(QEMU_IMG)
	$(QEMU_RUN)

# Not viable on win64
# install : $(QEMU_IMG)
# 	$(IMGTOL) w a: $(QEMU_IMG)

clean :
	$(DEL) -r $(BUILD)
