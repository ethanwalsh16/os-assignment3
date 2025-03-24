#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 10 // Max size of each address in addresses.txt
#define PAGE_SIZE 256 // Virtual memory page size
#define OFFSET_MASK PAGE_SIZE - 1 // Offset mask used to extract offset bits
#define OFFSET_BITS 8 // # of offset bits
#define PAGES 256 // # of pages
#define PHYSICAL_SIZE 32768 // simulated memory physical size (bytes)
#define LOGICAL_SIZE 65536 // simulated memory logical size (bytes)
#define TLB_SIZE 16

struct TLBentry {
	int page;
	int frame;
};

int TLBidx = 0;

typedef struct TLBentry TLBentry;

int search_TLB(struct TLBentry* TLBlist, int target);
void add_TLB(struct TLBentry* TLBlist, int frame, int page);
void update_TLB(struct TLBentry* TLBlist, int new_frame, int page);

void writemem(signed char *data);

signed char mem[PHYSICAL_SIZE] = {0};
int memidx = 0;
signed char *bkstoreptr;
bool memfull = false;

int total_addrs = 0;
int total_pf = 0;
int total_tlbh = 0;

int main() {
	// open addresses file and open backing store as a mem mapped file
	FILE *fptr = fopen("addresses.txt", "r");
	int bkstore_fd = open("BACKING_STORE.bin", O_RDONLY);
	bkstoreptr = mmap(0, LOGICAL_SIZE, PROT_READ, MAP_PRIVATE, bkstore_fd, 0);

	char buff[BUFFER_SIZE];
	int page_table[PAGES];

	// initialize page table as empty
	for (int i = 0; i < PAGES; i++) {
		page_table[i] = -1;
	}

	struct TLBentry TLB[TLB_SIZE];
	for (int i = 0; i < TLB_SIZE; i++) {
		TLB[i].page = -1;
		TLB[i].frame = -1;
	}

	// process all addresses in addresses.txt
	while(fgets(buff, BUFFER_SIZE, fptr) != NULL) {
		total_addrs++;

		// convert logical address to a physical frame number
		int logical_addr = atoi(buff);
		int pg_num = logical_addr >> OFFSET_BITS;
		int pg_offset = logical_addr & OFFSET_MASK;
		int frm_num = search_TLB(TLB, pg_num);

		if (frm_num != -1) {
			//TLB hit
			total_tlbh++;
		} else {
			// If not in TLB, find in page table
			frm_num = page_table[pg_num];
		}

		// If not in page table or TLB, page fault
		if (frm_num == -1) {
			total_pf++;

			if (memfull) { // if memory has already been filled up, we must overwrite old memory
				for (int i = 0; i < PAGES; i++) {
					if (page_table[i] == memidx){ // find old entry in page table that we're going to overwrite

						// Resetting values of TLB and page table when replacing page in physical memory.
						if(search_TLB(TLB, page_table[i]) != -1){
							update_TLB(TLB,pg_num,-1);
						}

						page_table[i] = -1; // reset value in page table that we've replaced in physical memory
						break;
					}
				}
			}

			// write new value to mem, and update TLB + page table
			frm_num = memidx;
			add_TLB(TLB,frm_num,pg_num);
			writemem(bkstoreptr + pg_num*256);
			page_table[pg_num] = frm_num;
		}

		// get physical address from from number
		int frm_addr = frm_num << OFFSET_BITS;
		int phys_addr = frm_addr | pg_offset;
		printf("Virtual address: %d, Physical address: %d, Value: %d\n", logical_addr, phys_addr, mem[phys_addr]);
	}

	// close files
	munmap(bkstoreptr, LOGICAL_SIZE);
	fclose(fptr);

	printf("Total addresses: %d\n", total_addrs);
	printf("Page faults: %d\n", total_pf);
	printf("TLB Hits: %d\n", total_tlbh);

	return 0;
}

// write data to the next physical address in memory
void writemem(signed char *data) {
	memcpy(mem + 256*memidx, data, PAGE_SIZE);
	memidx++;
	if (memidx == 128) {
		memidx = 0;
		memfull = true;
	}
}

int search_TLB(struct TLBentry* TLBlist, int target) {
	for(int i=0; i < TLB_SIZE; i++){
		if(TLBlist[i].page == target){
			return TLBlist[i].frame;
		}
	}
	return -1;
}

void add_TLB(struct TLBentry* TLBlist, int frame, int page){
	struct TLBentry new_TLB = {page,frame};
	TLBlist[TLBidx] = new_TLB;
	TLBidx++;
	if(TLBidx == TLB_SIZE){
		TLBidx = 0;
	}
}

void update_TLB(struct TLBentry* TLBlist, int new_frame, int page){
	for(int i=0; i < TLB_SIZE; i++){
		if(TLBlist[i].page == page){
			TLBlist[i].frame = new_frame;
			TLBlist[i].page = -1;
			return;
		}
	}

}
