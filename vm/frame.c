#include "vm/frame.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "lib/kernel/hash.h"
#include "threads/loader.h"
#include "lib/string.h"
#include "threads/vaddr.h"

int number=383;
static struct frame *frames;
static struct lock frame_lock;
int frame_number;
int free_frames;
// static bool DEBUG = false;

//so those outside frame.c can't lock and unlock
void lock_frame (void);
void unlock_frame (void);
int evict(void);

void
lock_frame (){
	lock_acquire (&frame_lock);
}

void unlock_frame (){
	lock_release (&frame_lock);
}

void
frame_init (){
	number = 383;
	frame_number = 0;
	frames = malloc(number * sizeof(struct frame));
	int index=0;
	struct frame *frame;
	for(;index<number;index++){
		frame = (struct frame*) malloc (sizeof (struct frame));
		frame -> page = palloc_get_page(PAL_USER);
		frames[index] = *frame;
	}
	lock_init (&frame_lock);
}

/*
Allocates a new page then adds to frame table

We were keeping a copy of the current page that the new page is created
from, but we decided on 11-7 that this was unnecessary and deleted this
to make progress elsewhere. May be a good idea, but who knows. Zach gets 
all credit/blame for thinking of storing it. Austin gets all credit/blame 
for deciding to delete it. 
*/
struct frame *
frame_get (){
	struct thread * t = thread_current ();
	struct frame *frame;
	/* no free mem, lets get some */
	if(frame_number >= number) {
		lock_frame ();
		int evindex = evict ();
		frame = &frames[evindex]; 
		// frame -> page = palloc_get_page ( PAL_USER /*| (zero ? PAL_ZERO : 0)*/ );
		memset(frame->page,0,PGSIZE);
		frame -> thread = t;
		frame -> held = true;
		frame -> pinned = false;
		unlock_frame ();
	}

	/* There is room, lets fill it */
	else if(frame_number < number){
		lock_frame();
		frame = &frames[frame_number];
		// frame = (struct frame*) malloc (sizeof (struct frame));
		// frame -> page = palloc_get_page(PAL_USER);
		frame -> thread = t;
		frame -> held = true;
		frame -> pinned = false;
		frame_number++;
		unlock_frame();
	}
	return frame;
}

/*
Get rid of the frame and all of its stuff
*/
bool
frame_free (struct frame *frame){

	if(frames[frame->frame_number].held == true){
		palloc_free_page (&frames[frame->frame_number].page); //Free physical memory
		frames[frame->frame_number].page = palloc_get_page(PAL_USER);
		frames[frame->frame_number].held = false;
		return true;
	} else {
		return false;
	}
}

/*
look for a frame with this address, if it isn't found return NULL
*/
struct frame *
frame_find_from_number (int index){
	return &frames[index];
}

uint8_t *
frame_corresponding_page(struct frame *frame){
	uint8_t *ret_val = frame->page;
	printf("Made it farther\n");
	return ret_val;
}

/* Returns the index of the frame we plan on evicting in the
	frame array. Uses the eviction algorithm we set up in this
	function.
*/
int evict(){
	uint32_t *pd = thread_current()->pagedir;
	int i;
	int evict = -1;
	for(i = 0; i < number; i++){
		if(!(&frames[i])->pinned){
			if(!pagedir_is_accessed(pd, (&frames[i])->page) && !pagedir_is_dirty(pd, (&frames[i])->page)){
				evict = i;
				break;
			}
		}
	}
	if(evict == -1){
		for(i = 0; i < number; i++){
			if(!(&frames[i])->pinned){
				if(!pagedir_is_accessed(pd,(&frames[i])->page)){
					evict = i;
				}
			}
		}
	}
	for(i = 0; i < evict; i++){
		pagedir_set_accessed(pd, (&frames[i])->page,false);
	}
	return evict;

}
