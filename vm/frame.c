#include "vm/frame.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "lib/kernel/hash.h"

static struct hash frames;
static struct lock frame_lock;

static bool DEBUG = false;

//so those outside frame.c can't lock and unlock
void lock_frame (void);
void unlock_frame (void);
//check if a frame is less than another frame
bool frame_less (const struct hash_elem *, const struct hash_elem *, void *);
//hash it
unsigned frame_hash (const struct hash_elem *, void *);
//clear it
void page_dump (struct frame *);
//pay it
void frame_payup (void *, bool);
//determine class of page
int get_class (uint32_t * , const void *);

void
lock_frame (){
	lock_acquire (&frame_lock);
}

void unlock_frame (){
	lock_release (&frame_lock);
}

void
frame_init (){
	hash_init (&frames, frame_hash, frame_less, NULL);
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
hash_elem
frame_get (bool zero){
	void * kpage = palloc_get_page ( PAL_USER | (zero ? PAL_ZERO : 0) );
	struct thread * t = thread_current ();

	/* no free mem, lets get some */
	if(kpage == NULL) {
		lock_frame ();
		evict ();
		kpage = palloc_get_page ( PAL_USER | (zero ? PAL_ZERO : 0) );
		unlock_frame ();
	}

	/* There is room, lets fill it */
	if(kpage != NULL){
		struct frame * frame = (struct frame*) malloc (sizeof (struct frame));
		frame -> page = kpage;
		frame -> thread = t;
		frame -> pinned = false;

		lock_frame ();
		hash_insert (&frames, &frame -> hash_elem);
		unlock_frame ();
	}

	return frame->hash_elem;
}

/*
Get rid of the frame and all of its stuff
*/
bool
frame_free (hash_elem kill){
	struct frame * frame;
	struct hash_elem * found_this_frame;
	struct frame frame_elem;

	found_this_frame = hash_find (&frames, &frame_elem.hash_elem);
	if(found_this_frame != NULL){
		frame = hash_entry (found_this_frame, struct frame, hash_elem);

		palloc_free_page (&frame->page); //Free physical memory
		hash_delete (&frames, &frame->hash_elem); //Free entry in the frame table
		free (frame); //Delete the structure

		return true;
	} else {
		return false;
	}
}

/*
look for a frame with this address, if it isn't found return NULL
*/
struct frame *
frame_find (hash_elem find_me){
	struct frame * frame;
	struct hash_elem * found_frame;
	struct frame frame_elem;
	//Assumption: Return of hash_find is the hash of the i'th element in the hash table
	found_frame = hash_find (&frames, find_me);
	if(found_frame != NULL){
		frame = hash_entry (found_frame, struct frame, hash_elem);
		return frame;
	} else {
		return NULL;
	}
}

/*
Comparision function, returns if A is less than B
*/
bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_	, void *aux UNUSED){
	const struct frame * a = hash_entry (a_, struct frame, hash_elem);
	const struct frame * b = hash_entry (b_, struct frame, hash_elem);
	return a->addr < b->addr;
}

/*
Can't use a hash table without a hash function
*/
unsigned
frame_hash(const struct hash_elem *hash_this, void *aux UNUSED){
	const struct frame * frame = hash_entry (hash_this, struct frame, hash_elem);
	return hash_int ((unsigned)frame->addr);
}