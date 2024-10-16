#include "page.h"
#include <stdint.h>
#include <stddef.h>

struct ppage physical_page_array[128]; //128 pages, each 2 mb in length covers 256 megs of memory
		
struct ppage *head;

extern int _end_kernel;
struct ppage *end_kernel_ptr = &_end_kernel;

struct page mp_pt[1024] __attribute__((aligned(0x1000)));
struct page page_table[1024] __attribute__((aligned(0x1000)));

struct page_directory_entry page_directory[1024] __attribute__((aligned(0x1000)));


void init_pfa_list(void) {
	// initialize list of free physical page structures
	// for size of physical page array
	// 	struct ppage, set next pointer to i+1
	// 	if NOT first page, set prev to i-1
	// 	page.next = &physical_page_array[i+1];
	int size = sizeof(physical_page_array)/sizeof(physical_page_array[0]);
	
	struct ppage *current;
	head = &physical_page_array[0];

	for (int i = 0; i < size; i++) {
		if (i == 0) {
			current = &physical_page_array[i];
			current->physical_addr = ((unsigned int) &_end_kernel & 0xFFFFF000);
			current->next = &physical_page_array[i+1];
			}
		else {
			current->next = &physical_page_array[i+1];
			current->physical_addr = ((unsigned int) &_end_kernel & 0xFFFFF000)+ 4096*i;
			current->prev = &physical_page_array[i-1];
		}
		current = current->next;	
	}
}

struct ppage *allocate_physical_pages(unsigned int npages) {
	// allocate physical pages
	// npages: how many pages to allocate
	// unlink npages pages
	// head point to page after npages
	// last of npages, next = null
	// page after npages, prev = null
	struct ppage *current = head;

	struct ppage *allocated = head;

	// set freed head to first page

	// point current to FIRST of NEW list
	for (int i = 0; i < npages; i++) {
		current = current->next;
	}	
	// head point to first of new list, current is first of new list
	head = current;

	// last of npages, next = null;
	current->prev->next = NULL;

	// first of new list, prev = null;
	current->prev = NULL;

	return allocated;
}

void free_physical_pages(struct ppage *ppage_list) {
	// free physical pages
	struct ppage *current = ppage_list;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = head;
	current->next->prev = current;
	head = ppage_list;
}

void load_page_directory(struct page_directory_entry *pd) {
	asm("mov %0, %%cr3"
		:
		:"r"(pd)
		:);
}

void enable_paging(void) {
	asm("mov %cr0, %eax\n"
		"or $0x80000001, %eax\n"
		"mov %eax, %cr0");
}

void map_pages_init(void) {
	//initialize page directory
	for (int i = 0; i < 1024; i++) {
		page_directory[i].present = 0;
		page_directory[i].rw = 0;
		page_directory[i].user = 0;
		page_directory[i].writethru = 0;
		page_directory[i].cachedisabled = 0;
		page_directory[i].accessed = 0;
		page_directory[i].pagesize = 0;
		page_directory[i].ignored = 0;
		page_directory[i].os_specific = 0;
		page_directory[i].frame = 0;
	}
	//initialize page table
	for (int i = 0; i < 1024; i++) {
		page_table[i].frame = i;
		page_table[i].present = 1;
		page_table[i].rw = 1;
		page_table[i].user = 0;
		page_table[i].accessed = 0;
		page_table[i].dirty = 0;
		page_table[i].unused = 0;
	}
	//point page_table to page_directory[0], present and rw to 1, while the others stay 0
	page_directory[0].frame = (unsigned int)page_table >> 12;
	page_directory[0].present = 1;
	page_directory[0].rw = 1;

	load_page_directory(page_directory);
	enable_paging();
}

void *map_pages(void *vaddr, struct ppage *ppages, struct page_directory_entry *pd) {
	// identity mapping page, where virtual address and physical addresses are the same
	uint32_t pdindex = (uint32_t)vaddr >> 22;
	uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x3ff;
	//zero out page table
	for (int i = 0; i < 1024; i++) {
		mp_pt[i].frame = 0;
		mp_pt[i].present = 0;
		mp_pt[i].rw = 0;
		mp_pt[i].user = 0;
		mp_pt[i].accessed = 0;
		mp_pt[i].dirty = 0;
		mp_pt[i].unused = 0;
	}
	//have the page_directory point to the page_table
	page_directory[pdindex].frame = (uint32_t)mp_pt >> 12;
	page_directory[pdindex].present = 1;
	page_directory[pdindex].rw = 1;
	//store page address to page_table entry
	mp_pt[ptindex].frame = (uint32_t)ppages->physical_addr >> 12;
	mp_pt[ptindex].present = 1;
	mp_pt[ptindex].rw = 1;
	return vaddr;
}

