all:
	nasm -f bin ./src/boot.asm -o ./bin/SinisterOS.bin

clean:
	rm -f ./bin/SinisterOS.bin

run: all
	qemu-system-x86_64 -drive format=raw,file=./bin/SinisterOS.bin