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

int number;
static struct frame *frames;
static struct lock frame_lock;
int frame_number;
// static bool DEBUG = false;

//so those outside frame.c can't lock and unlock
void lock_frame (void);
void unlock_frame (void);

void
lock_frame (){
	lock_acquire (&frame_lock);
}

void unlock_frame (){
	lock_release (&frame_lock);
}

void
frame_init (){
	number = init_ram_pages;
	frames = malloc(number * sizeof(struct frame));
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
		// lock_frame ();
		// evict ();
		// kpage = palloc_get_page ( PAL_USER | (zero ? PAL_ZERO : 0) );
		// unlock_frame ();
	}

	/* There is room, lets fill it */
	else if(frame_number < number){
		frame = (struct frame*) malloc (sizeof (struct frame));
		frame -> page = palloc_get_page (PAL_USER);
		frame -> thread = t;
		frame -> held = true;
		frame -> pinned = false;

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
