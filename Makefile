OBJS = \
	   kbd.o		\
	   exec.o		\
	   sysfile.o	\
	   pipe.o		\
	   file.o		\
	   fs.o			\
	   log.o		\
	   bio.o		\
	   sleeplock.o	\
	   ide.o		\
	   spinlock.o	\
	   timer.o		\
	   picirq.o		\
	   sysproc.o	\
	   syscall.o	\
	   vectors.o	\
	   trap.o		\
	   swtch.o		\
	   trapasm.o	\
	   proc.o		\
	   vm.o			\
	   kalloc.o		\
	   string.o		\
	   console.o	\
	   main.o

OBJDUMP=objdump
OBJCOPY=objcopy
QEMU=qemu-system-i386

CFLAGS=-static -nostdinc -fno-pic -fno-builtin -I. -O2 -m32 -ggdb

ASLAGS=-static -nostdinc -fno-pic -fno-builtin -I. -O2 -m32 -ggdb

.PHONY: run clean

default : xv6.img

%.o: %.S
	$(CC) $(CFLAGS) -c $^

OS_IMG=xv6.img
xv6.img : bootblock kernel fs.img
	dd if=/dev/zero of=${OS_IMG} count=10000
	dd if=bootblock of=${OS_IMG} conv=notrunc
	dd if=kernel of=${OS_IMG} seek=1 conv=notrunc
#	dd if=fs.img of=${OS_IMG} seek=1000 conv=notrunc

bootblock: bootasm.o bootmain.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
	./sign.pl bootblock

initcode: initcode.o
	$(LD) $(LDGLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

kernel: kernel.ld entry.o $(OBJS) initcode param.h
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel entry.o $(OBJS) -b binary initcode
	$(OBJDUMP) -S kernel > kernel.asm


QEMUOPTS=-drive file=fs.img,index=1,media=disk,format=raw -drive file=xv6.img,index=0,media=disk,format=raw -m 256 -smp cpus=1,cores=1


run: bootblock
	$(QEMU) $(QEMUOPTS)



clean:
	rm *.o *.asm *.sym kernel bootblock initcode initcode.out xv6.img fs.img $(ULIB) $(UPROGS) mkfs


qemu-gdb: xv6.img .gdbinit
	@echo "*** Now run 'gdb'." 1>&2
	$(QEMU) $(QEMUOPTS) -S -gdb tcp::1234

ULIB=usys.o\
	 ulib.o\
	 printf.o\
	 umalloc.o\

_%: %.o $(ULIB)
	$(LD) $(LDGLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ > $*.sym

UPROGS=\
	   _rm\
	   _cat\
	   _printargs\
	   _ls\
	   _init\
	   _sh\


mkfs: mkfs.c
	$(CC)  mkfs.c -o mkfs

fs.img: mkfs $(UPROGS)
	./mkfs fs.img $(UPROGS)

