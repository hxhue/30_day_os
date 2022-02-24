@set SDL_VIDEODRIVER=windib
@set QEMU_AUDIO_DRV=none
@set QEMU_AUDIO_LOG_TO_MONITOR=0
@REM qemu-system-x86_64 -L . -m 32 -rtc base=localtime -vga std -drive "file=fdimage0.bin,format=raw,if=floppy"
qemu.exe -L . -m 32 -localtime -std-vga -fda fdimage0.bin