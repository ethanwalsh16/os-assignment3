#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 10
#define PAGE_SIZE 256
#define OFFSET_MASK PAGE_SIZE - 1
#define OFFSET_BITS 8
#define PAGES 256
#define PHYSICAL_SIZE 32768
#define LOGICAL_SIZE 65536

signed char mem[PHYSICAL_SIZE] = {0};
int memidx = 0;
signed char *bkstoreptr;
void writemem(signed char *data);

int total_addrs = 0;
int total_pf = 0;

int main() {
	FILE *fptr = fopen("addresses.txt", "r");
	int bkstore_fd = open("BACKING_STORE.bin", O_RDONLY);
	bkstoreptr = mmap(0, LOGICAL_SIZE, PROT_READ, MAP_PRIVATE, bkstore_fd, 0);

	char buff[BUFFER_SIZE];
	int page_table[PAGES] = {-1};

	while(fgets(buff, BUFFER_SIZE, fptr) != NULL) {
		total_addrs++;
		int logical_addr = atoi(buff);
		int pg_num = logical_addr >> OFFSET_BITS;
		int pg_offset = logical_addr & OFFSET_MASK;
		int frm_num = page_table[pg_num];

		if (frm_num == -1) {
			total_pf++;
			writemem(bkstoreptr + logical_addr);
			frm_num = memidx - 1;
			page_table[pg_num] = frm_num;
		}

		int frm_addr = frm_num << OFFSET_BITS;
		int phys_addr = frm_addr | pg_offset;
		printf("Logical addr: %d, Page num: %d, Frame num: %d, Physical addr: %d, Value: %d\n", logical_addr, pg_num, frm_num, phys_addr, mem[phys_addr]);
	}

	munmap(bkstoreptr, LOGICAL_SIZE);
	fclose(fptr);

	printf("Total addresses: %d\n", total_addrs);
	printf("Page faults: %d\n", total_pf);

	return 0;
}

void writemem(signed char *data) {
	memcpy(mem + 256*memidx, data, PAGE_SIZE);
	memidx++;
	if (memidx == 128) memidx = 0;
}
