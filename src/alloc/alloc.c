#include <std/int.h>
#include <mmu/mmu.h>
#include <std/string.h>
#include <shell/shell.h>

typedef struct huge_alloc_info_s {
	uint64_t starting_page;
	uint64_t pages;
}__attribute__((packed)) huge_alloc_info_t;

typedef struct tiny_alloc_page_s {
	uint64_t bitmask;
	uint64_t page_addr;
}__attribute__((packed)) tiny_alloc_page_t;

typedef struct head_page_s {
	tiny_alloc_page_t bitmask[16];
	uint64_t huge_alloc_count;
	uint64_t huge_alloc_next;
	huge_alloc_info_t allocations[239];
}__attribute__((packed)) head_page_t;



static head_page_t *head_page;

void *kmalloc_init() {
	head_page = (void *)starting_address(allocate_full_page());
	memset(head_page, 0, sizeof(head_page_t));

	for(size_t i=0; i<16; i++) {
		head_page->bitmask[i].page_addr = allocate_full_page();
	}
}

void *kmalloc_tiny() {
	for (int i=0; i<16; i++) {
		if (head_page->bitmask[i].bitmask != 0xFFFFFFFFFFFFFFFF) {
			for(int bit=0; bit < 64; bit++) {
				uint64_t off = 0x01 << bit;
				if (!(head_page->bitmask[i].bitmask & off)) {
					head_page->bitmask[i].bitmask |= off;
					char *page = (char *)(starting_address(head_page->bitmask[i].page_addr));
					return &page[bit*64];
				}
			}
		}
	}
	return (void *)0;
}

void *kmalloc_huge(uint64_t size) {
	int index = 0;
	for(int i=0; i<239; i++) {
		if (!head_page->allocations[i].starting_page) {
			index = i;
			goto allocate;
		}
	}
	return (void *)0;

allocate:
	(void)"We found an opening, now save to it";
	uint64_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	uint64_t p = alloc_contiguous_pages(pages_needed);
	head_page->allocations[index].starting_page = p;
	head_page->allocations[index].pages = pages_needed;
	return (void *)(starting_address(p));
}

void *kmalloc(uint64_t size) {
	void *x = NULL;
	if (size <= 64) {
		x = kmalloc_tiny();
	} else {
		x = kmalloc_huge(size);
	}
	return x;
}

void free_tiny(uint64_t addr, size_t index) {
	tiny_alloc_page_t *tap = &head_page->bitmask[index];
	if (addr & 0x3F) {
		// not an address we can free
		kprintf("PANIC\n");
		while(1);
		return;
	}
	uint64_t mask = ~(0x01 << ((addr % 4096) / 64));
	tap->bitmask &= mask;
}

void kfree(void *_ptr) {
	uint64_t ptr = (uint64_t)_ptr;
	for(size_t i=0; i<16; i++) {
		uint64_t page_start = starting_address(head_page->bitmask[i].page_addr);
		if (ptr>page_start && ptr<page_start+PAGE_SIZE) {
			free_tiny(ptr, i);
			memset(_ptr, 0xBF, 64); // we asan now
			return;
		}
	}
	//release full page
	//free_huge

	for(int i=0; i<239; i++) {
		uint64_t sp = head_page->allocations[i].starting_page;
		uint64_t mp = containing_address((uint64_t)_ptr);
		if (sp == mp) {
			for(int j=0; j<head_page->allocations[i].pages; j++) {
				release_full_page(sp+j);
			}
			head_page->allocations[i].starting_page = 0;
			head_page->allocations[i].pages = 0;
		}
	}
}

void *kcmalloc(uint64_t size, uint64_t nmemb) {
	size *= nmemb;
	void *res = kmalloc(size);
	if (res) {
		memset(res, 0, size);
	}
	return res;
}