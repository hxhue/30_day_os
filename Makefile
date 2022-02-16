# External dependencies:
# - qemu-system-x86_64
# - haxm

# About incremental make:
# If dependencies change, `make clean` should be called manually.

# ipl ==> bootstrap ==> entry_point (0x00280000)
#  |          \ 
# 0x7c00     0x8000 + 0x4200 = 0xc200
#               |        |
#  ipl loading addr  floppy content offset

BIN           := bin
INCLUDE_FLAGS := -Iinclude -Iinclude/std
BUILD         := $(strip $(BUILD_DIR))
ifeq ($(BUILD),)
	BUILD := build
endif

NASK     := $(BIN)/nask.exe
CC1      := $(BIN)/cc1.exe $(INCLUDE_FLAGS) -Os -Wall -quiet
GAS2NASK := $(BIN)/gas2nask.exe -a
OBJ2BIM  := $(BIN)/obj2bim.exe
BIM2HRB  := $(BIN)/bim2hrb.exe
BIN2OBJ  := $(BIN)/bin2obj.exe
MAKEFONT := $(BIN)/makefont.exe
RULEFILE := haribote.rul
EDIMG    := $(BIN)/edimg.exe
IMGTOL   := $(BIN)/imgtol.com
COPY     := $(BIN)/cp.exe
DEL      := $(BIN)/rm.exe -f
CAT      := $(BIN)/cat.exe
# NASM     := $(BIN)/nasm.exe
FIND     := $(BIN)/find.exe
MKDIR    := $(BIN)/mkdir.exe
QEMU_IMG := $(BUILD)/os.img
QEMU_RUN  = qemu-system-x86_64 -L . -m 32 -rtc base=localtime -vga std \
			-drive "file=$(QEMU_IMG),format=raw,if=floppy" -accel hax

# Get all C files in this directory
SOURCES  := $(shell $(FIND) . -name "*.c")
C_OBJS   := $(patsubst ./%.c,$(BUILD)/%.obj,$(SOURCES))

default : $(BUILD) run

$(BUILD):
	$(MKDIR) -p $(BUILD)

# For all C files in project directory
$(C_OBJS) : $(BUILD)/%.obj : %.c
	$(MKDIR) -p $(dir $@)
	$(CC1) -o $(patsubst %.obj,%.gas,$@) $<
	$(GAS2NASK) $(patsubst %.obj,%.gas,$@) $(patsubst %.obj,%.nas,$@)
	$(NASK) $(patsubst %.obj,%.nas,$@) $@ $(patsubst %.obj,%.lst,$@)

# For special files
$(BUILD)/ipl10.bin : ipl10.nas
	$(NASK) ipl10.nas $(BUILD)/ipl10.bin

$(BUILD)/bootstrap.bin : bootstrap.nas
#	$(NASM) -o"$(BUILD)/bootstrap.bin" "bootstrap.nas"
	$(NASK) bootstrap.nas $(BUILD)/bootstrap.bin $(BUILD)/bootstrap.lst

$(BUILD)/inst.obj : inst.nas
	$(NASK) inst.nas $(BUILD)/inst.obj $(BUILD)/inst.lst

$(BUILD)/hankaku.bin : hankaku.txt
	$(MAKEFONT) hankaku.txt $(BUILD)/hankaku.bin

font $(BUILD)/hankaku.obj: $(BUILD)/hankaku.bin
	$(BIN2OBJ) $(BUILD)/hankaku.bin $(BUILD)/hankaku.obj _hankaku

# Combine all objects in floopy storage (from address 0x4200 of the floopy)
$(BUILD)/main.bim : $(C_OBJS) $(BUILD)/inst.obj $(BUILD)/hankaku.obj
	$(OBJ2BIM) @$(RULEFILE) out:$(BUILD)/main.bim stack:3136k map:$(BUILD)/main.map \
		$(BUILD)/inst.obj $(BUILD)/hankaku.obj $(C_OBJS)
# 3MB+64KB=3136KB

$(BUILD)/main.hrb : $(BUILD)/main.bim
	$(BIM2HRB) $(BUILD)/main.bim $(BUILD)/main.hrb 0

$(BUILD)/haribote.sys : $(BUILD)/bootstrap.bin $(BUILD)/main.hrb
	$(CAT) $(BUILD)/bootstrap.bin $(BUILD)/main.hrb > $(BUILD)/haribote.sys

# "./bin/fdimg0at.tek" causes error.
# "../bin/*" and "bin/*" do not.
img $(QEMU_IMG) : $(BUILD)/ipl10.bin $(BUILD)/haribote.sys
	$(EDIMG) imgin:bin/fdimg0at.tek \
		wbinimg src:$(BUILD)/ipl10.bin len:512 from:0 to:0 \
		copy from:$(BUILD)/haribote.sys to:@: \
		imgout:$(QEMU_IMG)

run : $(QEMU_IMG)
	$(QEMU_RUN)

# Not viable on win64
install : $(QEMU_IMG)
	$(IMGTOL) w a: $(QEMU_IMG)

clean :
	$(DEL) -r $(BUILD)
