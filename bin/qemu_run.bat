@set SDL_VIDEODRIVER=windib
@set QEMU_AUDIO_DRV=none
@set QEMU_AUDIO_LOG_TO_MONITOR=0
qemu-system-x86_64 -L . -m 32 -rtc base=localtime -vga std -drive "file=%1,format=raw,if=floppy"