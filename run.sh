qemu-system-x86_64 -m 32 -rtc base=localtime -vga std \
	-drive "file=build/os.img,format=raw,if=floppy" -serial stdio --no-reboot \
	-accel whpx,kernel-irqchip=off