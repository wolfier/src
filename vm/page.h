#ifndef VM_PAGE_H
#define VM_PAGE_H

//need hash file, along with page and swap files we create
#include "lib/kernel/hash.h"
#include "vm/frame.h"
#include "threads/thread.h"

struct page 
  {
    uint32_t vaddr;					/* Virtual address of the page */
    uint32_t paddr;					/* Physical address of the page */
    bool resident;					/* Is the page in a frame right now */
    bool fuck_you_Zach;				/* Has the page been accessed recently (Clock bit) */	
    bool fuck_you_Jae;				/* Has the page been written to and needs to be written back to disk */
	struct thread *thread;			/* Thread the page belongs to*/
	bool pinned;					/* don't evict this */
	struct hash_elem hash_elem; 	/* to use the hash table*/
};

void page_init (void);
struct page *page_find (void *);
uint32_t page_get (void);
bool page_free (void *);
void lock_page (void);
void unlock_page (void);
  
#endif  
