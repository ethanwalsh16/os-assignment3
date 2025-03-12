#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#define BUFFER_SIZE 10
#define PAGE_SIZE 256
#define OFFSET_MASK PAGE_SIZE - 1
#define OFFSET_BITS 8
#define PAGES 256
#define PHYSICAL_SIZE 32768
#define LOGICAL_SIZE 65536
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

signed char mem[PHYSICAL_SIZE] = {0};
int memidx = 0;
signed char *bkstoreptr;
void writemem(signed char *data);
bool memfull = false;

int total_addrs = 0;
int total_pf = 0;
int total_tlbh = 0;

int main() {
	FILE *fptr = fopen("addresses.txt", "r");
	int bkstore_fd = open("BACKING_STORE.bin", O_RDONLY);
	bkstoreptr = mmap(0, LOGICAL_SIZE, PROT_READ, MAP_PRIVATE, bkstore_fd, 0);

	char buff[BUFFER_SIZE];
	int page_table[PAGES];

	for (int i = 0; i < PAGES; i++) {
		page_table[i] = -1;
	}

	struct TLBentry TLB[TLB_SIZE];
	for (int i = 0; i < TLB_SIZE; i++) {
		TLB[i].page = -1;
		TLB[i].frame = -1;
	}

	while(fgets(buff, BUFFER_SIZE, fptr) != NULL) {
		bool updated = false;
		total_addrs++;
		int logical_addr = atoi(buff);
		int pg_num = logical_addr >> OFFSET_BITS;
		int pg_offset = logical_addr & OFFSET_MASK;
		int frm_num = search_TLB(TLB, pg_num);
		
		if(frm_num != -1){
			//TLB hit
			total_tlbh++;
		}else{
			frm_num = page_table[pg_num];
		}
		
		if (frm_num == -1) {
			if (memfull) {
				for (int i = 0; i < PAGES; i++) {
					if (page_table[i] == memidx){
						if(search_TLB(TLB, page_table[i]) != -1){
							update_TLB(TLB,pg_num,memidx);
							updated = true;
						}
						page_table[i] = -1;
						break;
					}
				}
			}
			total_pf++;
			frm_num = memidx;
			
			writemem(bkstoreptr + pg_num*256);
			page_table[pg_num] = frm_num;
		}
		if(updated == false){
			add_TLB(TLB,frm_num,pg_num);
		}
		int frm_addr = frm_num << OFFSET_BITS;
		int phys_addr = frm_addr | pg_offset;
		printf("%d - Logical addr: %d, Page num: %d, Frame num: %d, Physical addr: %d, Value: %d\n", total_addrs, logical_addr, pg_num, frm_num, phys_addr, mem[phys_addr]);
	}

	munmap(bkstoreptr, LOGICAL_SIZE);
	fclose(fptr);

	printf("Total addresses: %d\n", total_addrs);
	printf("Page faults: %d\n", total_pf);
	printf("TLB Hits: %d\n", total_tlbh);

	return 0;
}

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
			return;
		}
	}

}