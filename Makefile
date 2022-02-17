# External dependencies:
# - qemu-system-x86_64
# - haxm

# About incremental make:
# If dependencies change, `make clean` should be called manually.

# ipl ==> bootstrap ==> entry_point (0x00280000)
#  |            | 
# 0x7c00     0x8000 + 0x4200 = 0xc200
#               |        |
#  ipl loading addr  floppy content offset

BIN           := bin
INCLUDE_FLAGS := -Isrc -Isrc/libc
BUILD         := build
RULEFILE      := src/haribote.rul

NASK     := $(BIN)/nask.exe
CC1      := $(BIN)/cc1.exe $(INCLUDE_FLAGS) -Os -Wall -quiet -std=c99
GAS2NASK := $(BIN)/gas2nask.exe -a
OBJ2BIM  := $(BIN)/obj2bim.exe
BIM2HRB  := $(BIN)/bim2hrb.exe
BIN2OBJ  := $(BIN)/bin2obj.exe
MAKEFONT := $(BIN)/makefont.exe
EDIMG    := $(BIN)/edimg.exe
IMGTOL   := $(BIN)/imgtol.com
COPY     := $(BIN)/cp.exe
DEL      := $(BIN)/rm.exe -f
CAT      := $(BIN)/cat.exe
FIND     := $(BIN)/find.exe
MKDIR    := $(BIN)/mkdir.exe

QEMU_IMG := $(BUILD)/os.img
QEMU_RUN  = qemu-system-x86_64 -L . -m 32 -rtc base=localtime -vga std \
			-drive "file=$(QEMU_IMG),format=raw,if=floppy" -accel hax

# Get all C files in src
C_SOURCES := $(shell $(FIND) src/* -name "*.c")
C_OBJS    := $(patsubst src/%.c,$(BUILD)/%.obj,$(C_SOURCES))

# Get all NASM files in src
NAS_SOURCES := $(shell $(FIND) src/* -name "*.nas")
NAS_OBJS    := $(patsubst src/%.nas,$(BUILD)/%.obj,$(NAS_SOURCES))

default : run

# C files
$(C_OBJS) : $(BUILD)/%.obj : src/%.c
	$(MKDIR) -p $(dir $@)
	$(CC1) -o $(patsubst %.obj,%.gas,$@) $<
	$(GAS2NASK) $(patsubst %.obj,%.gas,$@) $(patsubst %.obj,%.nas,$@)
	$(NASK) $(patsubst %.obj,%.nas,$@) $@ $(patsubst %.obj,%.lst,$@)

# NASM files
$(NAS_OBJS) : $(BUILD)/%.obj : src/%.nas
	$(MKDIR) -p $(dir $@)
	$(NASK) $< $@ $(patsubst %.obj,%.lst,$@)

# Font
$(BUILD)/hankaku.obj: src/hankaku.txt
	$(MAKEFONT) src/hankaku.txt $(BUILD)/hankaku.bin
	$(BIN2OBJ) $(BUILD)/hankaku.bin $(BUILD)/hankaku.obj _hankaku

# Combine all objects in floopy storage (from address 0x4200 of the floopy)
# 3MB + 64KB = 3136KB
$(BUILD)/main.hrb : $(C_OBJS) $(BUILD)/inst.obj $(BUILD)/hankaku.obj
	$(OBJ2BIM) @$(RULEFILE) out:$(BUILD)/main.bim stack:3136k \
		map:$(BUILD)/main.map \
		$(BUILD)/inst.obj $(BUILD)/hankaku.obj \
		$(C_OBJS)
	$(BIM2HRB) $(BUILD)/main.bim $(BUILD)/main.hrb 0

# Create .sys
$(BUILD)/haribote.sys : $(BUILD)/bootstrap.obj $(BUILD)/main.hrb
	$(CAT) $(BUILD)/bootstrap.obj $(BUILD)/main.hrb > $(BUILD)/haribote.sys

# Create image
# "./bin/fdimg0at.tek" causes error.
# "../bin/*" and "bin/*" do not.
img $(QEMU_IMG) : $(BUILD)/ipl10.obj $(BUILD)/haribote.sys
	$(EDIMG) imgin:bin/fdimg0at.tek wbinimg src:$(BUILD)/ipl10.obj \
		len:512 from:0 to:0 \
		copy from:$(BUILD)/haribote.sys to:@: \
		imgout:$(QEMU_IMG)

run : $(QEMU_IMG)
	$(QEMU_RUN)

# Not viable on win64
install : $(QEMU_IMG)
	$(IMGTOL) w a: $(QEMU_IMG)

clean :
	$(DEL) -r $(BUILD)
