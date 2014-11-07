//need hash file, along with page and swap files we create
#include "lib/kernel/hash.h"
#include "vm/page.h"

struct frame {
	// void *paddr;					/* Physical address of the page */
	// void *uvaddr;					/* User virtual address of the page*/
	uint32_t frame_number;			/* Number corresponding to frame */
	struct void *page;				/* pointer to the page resident in the frame */
	struct thread *thread;			/* Thread the page belongs to*/
	// struct <think of name> *origin; /* Source of origin of the page*/
	bool pinned;					/* don't evict this */
	struct hash_elem hash_elem; 	/*to use the hash table*/
};

void frame_init (void);
void evict (void);
struct frame *frame_find (void *);
void* frame_get (void *, bool);
bool frame_free (void *);

void lock_frame (void);
void unlock_frame (void);
