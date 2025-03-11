#include <stdio.h>
#include <stdlib.h>

#define TLB_SIZE 16

struct TLBentry {
	int page;
	int frame;
};

int idx = 0;

typedef struct TLBentry TLBentry;

int search_TLB(struct TLBentry* TLBlist, int target);

int main() {
	struct TLBentry TLB[] = {
		{ 1, 4 },
		{ 3, 7 },
	};

	search_TLB(TLB, 3);
}

int search_TLB(struct TLBentry* TLBlist, int target) {

	for(int i=0; i < 2; i++){
		printf("%d, %d\n",i,TLBlist[i].page);
	}
}

int add_TLB(struct TLBentry* TLBlist, int new){

	struct TLBentry new_TLB = {1,4};
	TLBlist[idx] = new_TLB;

	if(idx < TLB_SIZE - 1){
		idx += 1;
	}else{
		idx = 0;
	}
}

int update_TLB(){

}

