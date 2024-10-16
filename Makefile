


all: hello rootfs.img
	mcopy -i rootfs.img@@1M hello ::/
	mcopy -i rootfs.img@@1M TEST2.TXT ::/
	mmd -i rootfs.img@@1M boot 
	mcopy -i rootfs.img@@1M grub.cfg ::/boot

hello: hello.c
	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall hello.c
	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall rprintf.c
	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall page.c 
	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall page.h
	nasm -f elf32 -g ide.s -o ide.o
	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall fat.c
	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall fat.h
#	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall ide.s
#	gcc -c -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall ide.h
	ld -T kernel.ld -e main -melf_i386 ide.o fat.o hello.o rprintf.o page.o -o hello


rootfs.img:
	dd if=/dev/zero of=rootfs.img bs=1M count=32
#	/usr/local/grub-i386/bin/grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos
	grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos
#	dd if=/usr/local/grub-i386/lib/grub/i386-pc/boot.img  of=rootfs.img conv=notrunc
	dd if=/usr/lib/grub/i386-pc/boot.img  of=rootfs.img conv=notrunc
	dd if=grub.img of=rootfs.img conv=notrunc seek=1
	echo 'start=2048, type=83, bootable' | sfdisk rootfs.img
	mkfs.vfat --offset 2048 -F16 rootfs.img
#	sudo mount rootfs.img /mnt/disk
#	sudo echo "this is test2" >> /test2.txt
#	sudo umount /mnt/disk

debug:
	screen -S qemu -d -m qemu-system-i386 -S -s -hda rootfs.img -monitor stdio
	TERM=xterm gdb -x gdb_os.txt && killall qemu-system-i386

clean:
	rm -f grub.img hello hello.o rprintf.o page.o ide.o fat.o rootfs.img
