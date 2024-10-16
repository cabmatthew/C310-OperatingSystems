#include "page.h"
#include <stdint.h>
#include <string.h>
#include "fat.h"


#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

unsigned int multiboot_header[] __attribute__((section(".multiboot")))  = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

extern int _end_kernel;

void outb (uint16_t _port, uint8_t val) {
	__asm__ __volatile__ ("outb %0, %1" : : "a" (val), "dN" (_port) );
}

unsigned char keyboard_map[128] =
{
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',     /* 9 */
 '9', '0', '-', '=', '\b',     /* Backspace */
 '\t',                 /* Tab */
 'q', 'w', 'e', 'r',   /* 19 */
 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
   0,                  /* 29   - Control */
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 39 */
'\'', '`',   0,                /* Left shift */
'\\', 'z', 'x', 'c', 'v', 'b', 'n',                    /* 49 */
 'm', ',', '.', '/',   0,                              /* Right shift */
 '*',
   0,  /* Alt */
 ' ',  /* Space bar */
   0,  /* Caps lock */
   0,  /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,  
   0,  /* < ... F10 */
   0,  /* 69 - Num lock*/
   0,  /* Scroll Lock */
   0,  /* Home key */
   0,  /* Up Arrow */
   0,  /* Page Up */
 '-',
   0,  /* Left Arrow */
   0,  
   0,  /* Right Arrow */
 '+',
   0,  /* 79 - End key*/
   0,  /* Down Arrow */
   0,  /* Page Down */
   0,  /* Insert Key */
   0,  /* Delete Key */
   0,   0,   0,  
   0,  /* F11 Key */
   0,  /* F12 Key */
   0,  /* All other keys are undefined */
};

int xCoord = 0;
int yCoord = 0;

void putc(int c) {
	// input c, put it into the appropriate memory slot for video memory
	// after putting a character, go to next x coord
	// if goes out of window to the right, go to next y coord
	// if goes out of bottom of window, stay at same y coord but shift everything up 1
	short *mem = 0xb8000;
	if (c == '\n') {
		yCoord++;
		xCoord = 0;
		
		if (yCoord > 24) {
			scrollOne();
			yCoord = 24;
		}		

		return;
	}
	
//	xCoord++;

	else {	
		mem[xCoord+80*yCoord] = (7 << 8) + c;
	}

	xCoord++;
	
	if (xCoord == 80) {
		xCoord = 0;
		yCoord++;
	}

	if (yCoord > 24) {
		scrollOne();
		xCoord = 0;
		yCoord = 24;
	}
}

void scrollOne() {

	// for each printed, starting at 2nd line, print it
	// at its y coord -1
	short *mem = 0xb8000;
	for (int j = 0; j < 80; j++) {
	for (int i = 1; i < 26; i++) {
		mem[j+80*(i-1)] = mem[j+80*i];
	}}
	for (int i = 0; i < 80; i++) {
		mem[i+80*24] = 0;
	}
}


/* FAT FS STUFF */


void main() {
	unsigned short *vram = (unsigned short*)0xb8000; // Base address of video mem
        const unsigned char color = 7; // gray text on black background
	// use screen -r in new terminal for qemu monitor
	// use info mem to display page table
	
	/* HW 4 FAT FS STUFF */

	fatInit();
	struct file current;
	fatOpen("/TEST2.TXT", &current);
	fatRead(current);


	/* HW 2 STUFF */
        map_pages_init();
	init_pfa_list();
        struct ppage *p = allocate_physical_pages(1);
        /* HW 3 */
	map_pages(0x80000000, p, 1);
	free_physical_pages(p);


	/*
        // HW 3 STUFF, notes	
	// Neil's way, hardcoding?
	void *va = 0x100000;
	for (unsigned int va = 0x100000; va < 0x108000; va += 0x1000) { // 0x108000 -> _end_kernel
		unsigned int vpn = ((unsigned int) va) >> 12;
	        unsigned int pd_idx = (vpn>>10);
		unsigned int pt_idx = (vpn) & 0x3ff; // bitmasking, twelve 1s
		pd[pd_idx] = (unsigned int) pt | 3; // pd[pd_idx] = address of pt it's pointing to
		pt[pt_idx] = (unsigned int) va | 3; //  or 3 -> or 011, TELL COMPUTER THAT THESE ARE MAPPED, SET RW & PRESENT TO 1
						// cant OR pointer, must cast as unsigned int
	}
*/
	// Assembly code to put address of PD into CR3

	// Enable paging , copies value cr0 into eax, ORS it into the value
	
	/*
	 * so far he did 1st part
	 * now have to point stuff	
	 * need to set CR3 to the base address of PD, inline assembly
	 *
	 */


	/* END HW 3 STUFF */

	/* KEYBOARD DRIVER */
    	char c = 0;
    	while(1) {
		if(inb(0x60)!=c) {
			c = inb(0x60);
			if (c>0) {
				putc(keyboard_map[c]);
			}
		}
    	}
}
