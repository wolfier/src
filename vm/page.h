#ifndef VM_PAGE_H
#define VM_PAGE_H
// prahjekt 3

//need hash file, along with page and swap files we create
#include "lib/kernel/hash.h"
#include "vm/frame.h"
#include "threads/thread.h"
#include "filesys/file.h"

struct page 
  {
  	uint32_t *vaddr;
  	// struct frame* frame;
    bool framed;					/* Is the page in a frame right now */
    bool swapped; 					/* Is the paged swapped out to memory right now */
	struct hash_elem hash_elem; 	/* to use the hash table*/
	size_t num_read_bytes;			/*number of ready bytes*/
	size_t num_zero_bytes;			/*number of zero bytes*/
	struct file *executable;				/*file to execute from*/
	bool writable;					/*should we be writing to this file*/
	off_t ofs;						/*offset of the file*/


};

struct hash * page_init (void);
uint8_t *page_find (struct hash_elem);
bool page_set_sup(void*,struct file*, off_t, size_t, size_t, bool);
bool page_free (struct hash_elem);
bool page_in_frame(struct hash_elem);
bool page_pinned(struct hash_elem);
bool page_swapped(struct hash_elem);
void lock_page (void);
void unlock_page (void);

// uint8_t page_get_vaddr(struct hash_elem);
// bool page_get_framed(struct hash_elem);
// bool page_get_swapped(struct hash_elem);
// size_t page_get_read(struct hash_elem);
// size_t page_get_zero(struct hash_elem);
// file* page_get_file(struct hash_elem);
// bool page_get_writable(struct hash_elem);
// off_t page_get_ofs(struct hash_elem);
  
#endif  
