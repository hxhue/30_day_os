# External dependencies:
# - qemu-system-x86_64
# - gcc  (gcc.exe (Rev5, Built by MSYS2 project) 11.2.0)
# - nasm (NASM version 2.15.05 compiled on Oct 24 2020)

# About incremental make:
# If dependencies change, `make clean` should be called manually.

# Memory Layout:
# 0x00007c00 - 0x00007dff	Boot sector (512 bytes)
# 0x00100000 - 0x00267fff	Floppy disk image (1440KB)
# 0x0026f800 - 0x0026ffff	IDT (2KB)
# 0x00270000 - 0x0027ffff	GDT (64KB)
# 0x00280000 - 0x002effff	Kernel program (bootpack.hrb, 512KB)
# 0x002f0000 - 0x002fffff	.data, .rodata
# 0x00300000 - 0x003fffff	Stack etc. (1MB)
# 0x00400000 - ...        Free (application area)

BIN        = bin
BUILD      = build/tree
RULEFILE   = src/haribote.rul

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
FAT12IMG  := ruby $(BIN)/fat12img.rb

# NASK      := $(BIN)/nask.exe
# GAS2NASK  := $(BIN)/gas2nask.exe -a
# CC1       := $(BIN)/cc1.exe -Isrc -Isrc/libc -nostdinc -Os -Wall -quiet \
# 							-std=c99 -Werror
# https://github.com/nativeos/i386-elf-toolchain
# LD := $(BIN)\i386-elf-binutils\bin\i386-elf-ld.exe -m elf_i386
# LD := ld

C_FLAGS   = -m32 -Isrc -Isrc/libc -nostdinc -std=c11 -static-libgcc -lgcc -O2 \
						-Wall -ffreestanding

QEMU_IMG  := $(BUILD)/os.img
QEMU_RUN  := qemu-system-x86_64 -m 64 -rtc base=localtime -vga std \
	-drive "file=$(QEMU_IMG),format=raw,if=floppy" -serial stdio --no-reboot \
	-accel whpx,kernel-irqchip=off

# Get all C files in src/*/*
C_SOURCES  := $(shell $(FIND) src/ -mindepth 2 -name "*.c")
C_OBJS     := $(patsubst src/%.c,$(BUILD)/%.obj,$(C_SOURCES))

# Get all NASM files in src/*/*
NAS_SOURCES := $(shell $(FIND) src/ -mindepth 2 -name "*.nas")
NAS_OBJS    := $(patsubst src/%.nas,$(BUILD)/%.obj,$(NAS_SOURCES))

all : run

.PHONY: clean

# Font
$(BUILD)/hankaku.obj: src/hankaku.txt
	$(MAKEFONT) src/hankaku.txt $(BUILD)/hankaku.bin
	$(BIN2OBJ) $(BUILD)/hankaku.bin $(BUILD)/hankaku.obj _hankaku

# C files
$(BUILD)/%.obj : src/%.c
	$(MKDIR) -p $(dir $@)
	gcc $(C_FLAGS) -c $< -o $@

# NASM files
$(BUILD)/%.obj : src/%.nas
	$(MKDIR) -p $(dir $@)
	nasm -f coff -o $@ $<

$(BUILD)/ipl10.bin : src/ipl10.nas
	$(MKDIR) -p $(dir $@)
	nasm -f bin -o $@ $<

$(BUILD)/bootstrap.bin : src/bootstrap.nas
	$(MKDIR) -p $(dir $@)
	nasm -f bin -o $@ $<

# Combine all objects in floopy storage (from address 0x4200 of the floopy)
# 3MB + 64KB = 3136KB
$(BUILD)/main.bin : $(NAS_OBJS) $(C_OBJS) $(BUILD)/hankaku.obj
	$(OBJ2BIM) @$(RULEFILE) out:$(BUILD)/main.bim stack:3136k map:$(BUILD)/main.map \
		$(BUILD)/hankaku.obj $(NAS_OBJS) $(C_OBJS)
	$(BIM2HRB) $(BUILD)/main.bim $(BUILD)/main.bin 0

# $(BUILD)/main.bin : $(NAS_OBJS) $(C_OBJS) $(BUILD)/hankaku.obj
# 	$(LD) -Map $(BUILD)/main.map -T src/harimain.ls -s -o $@ \
# 		$(NAS_OBJS) $(C_OBJS) $(BUILD)/hankaku.obj

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
# img $(QEMU_IMG) : $(BUILD)/ipl10.bin $(BUILD)/haribote.sys
# 	$(FAT12IMG) $(QEMU_IMG) format
# 	$(FAT12IMG) $(QEMU_IMG) write $(BUILD)/ipl10.bin 0
# 	$(FAT12IMG) $(QEMU_IMG) save $(BUILD)/haribote.sys

run : $(QEMU_IMG)
	$(QEMU_RUN)

clean :
	$(DEL) -r $(BUILD)
