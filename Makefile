# Makefile for Sinister-OS

# Compiler/Assembler settings
C_SOURCES = $(wildcard kernel/*.c drivers/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h fs/*.h include/*.h)
OBJ = ${C_SOURCES:.c=.o}

# Flags for compiling 32-bit code
CFLAGS = -m32 -fno-pic -fno-builtin -fno-stack-protector -nostdinc -nostdlib -ffreestanding -g -I./libc/include
LDFLAGS = -melf_i386 -Tlinker.ld

# Default make target
all: os-image

# Check if documentation is present
check-docs:
	@if [ ! -f wiki.txt ]; then \
		echo "Error: Documentation file wiki.txt is missing!"; \
		exit 1; \
	fi
	@echo "Documentation check passed."

# Run the operating system with QEMU
run: all
	qemu-system-i386 -fda os-image -boot a

# Create a disk image with our bootloader and kernel
os-image: check-docs boot/boot.bin kernel.bin
	cat boot/boot.bin kernel.bin > os-image
	# Show file size for debugging
	ls -la os-image

# Build the kernel binary
kernel.bin: boot/kernel_entry.o drivers/keyboard_asm.o ${OBJ}
	@echo "Building kernel binary..."
	ld $(LDFLAGS) -o $@ $^ --oformat binary
	ls -la $@

# Build the boot sector
boot/boot.bin: boot/boot.asm
	@echo "Building boot sector..."
	mkdir -p boot
	nasm $< -f bin -I boot/ -o $@
	# Show file size for debugging
	ls -la $@

# Build kernel entry ASM file
%.o: %.asm
	@echo "Building $<..."
	mkdir -p $(dir $@)
	nasm $< -f elf -o $@
	ls -la $@

# Build C object files
%.o: %.c ${HEADERS}
	@echo "Building $<..."
	mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@
	ls -la $@

# Clean up temporary files - more thorough cleaning
clean:
	rm -rf *.bin *.o os-image
	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o fs/*.o
	find . -name "*.o" -type f -delete
	find . -name "*.bin" -type f -delete

# Create a debug image for better debugging with GDB
debug: os-image
	qemu-system-i386 -fda os-image -boot a -gdb tcp::1234 -S

# Windows specific build and run targets
.PHONY: win-clean win-os-image win-run win-debug check-docs

# Windows-specific clean command - more thorough
win-clean:
	if exist *.bin del /F /Q *.bin
	if exist *.o del /F /Q *.o
	if exist os-image del /F /Q os-image
	if exist kernel\*.o del /F /Q kernel\*.o
	if exist boot\*.bin del /F /Q boot\*.bin
	if exist drivers\*.o del /F /Q drivers\*.o
	if exist fs\*.o del /F /Q fs\*.o
	for /R %%f in (*.o *.bin) do del /F /Q "%%f"

# Windows check docs
win-check-docs:
	@if not exist wiki.txt (echo Error: Documentation file wiki.txt is missing! && exit /b 1)
	@echo Documentation check passed.

# Helper target to merge binary files on Windows (using copy /b instead of cat)
win-os-image: win-check-docs boot/boot.bin kernel.bin
	copy /b boot\boot.bin+kernel.bin os-image

# Windows specific run (with forced floppy boot)
win-run: win-os-image
	qemu-system-i386 -fda os-image -boot a

# Windows debug target
win-debug: win-os-image
	qemu-system-i386 -fda os-image -boot a -gdb tcp::1234 -S
