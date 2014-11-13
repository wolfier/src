#ifndef VM_PAGE_H
#define VM_PAGE_H
// prahjekt 3

//need hash file, along with page and swap files we create
#include "lib/kernel/hash.h"
#include "vm/frame.h"
#include "threads/thread.h"

struct page 
  {
  	uint32_t addr;
  	uint32_t *page;					/* The real life, no lie page */
    bool framed;					/* Is the page in a frame right now */
    bool swapped; 					/* Is the paged swapped out to memory right now */
	bool pinned;					/* don't evict this */
	struct hash_elem hash_elem; 	/* to use the hash table*/
};

void page_init (void);
struct page *page_find (struct hash_elem);
struct hash_elem page_get (void);
bool page_free (struct hash_elem);
bool page_in_frame(struct hash_elem);
bool page_pinned(struct hash_elem);
bool page_swapped(struct hash_elem);
void lock_page (void);
void unlock_page (void);
  
#endif  
