#include "vm/page.h"
#include <stdio.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"

static struct hash pages;
static struct lock page_lock;

static bool DEBUG = false;

//so those outside page.c can't lock and unlock
void lock_page (void);
void unlock_page (void);
//check if a page is less than another page
bool page_less (const struct hash_elem *, const struct hash_elem *, void *);
//hash it
unsigned page_hash (const struct hash_elem *, void *);
//clear it
void page_dump (struct page *);
//pay it
void page_payup (void *, bool);
//determine class of page
int get_class (uint32_t * , const void *);

void
lock_page (){
	lock_acquire (&page_lock);
}

void unlock_page (){
	lock_release (&page_lock);
}

struct hash *
page_init (){
	hash_init (&pages, page_hash, page_less, NULL);
	lock_init (&page_lock);
	return &pages;
}

/*
Allocates a new page then adds to page table
*/
bool
page_set_sup(void *uaddr, struct file *file, off_t offset, size_t read, size_t zero, bool write){
	struct page * page = (struct page*) malloc (sizeof (struct page));
	page -> vaddr = uaddr;
	page -> executable = file;
	page -> num_read_bytes = read;
	page -> num_zero_bytes = zero;
	page -> writable = write;
	page -> ofs = offset;
	page -> framed = true;
	page -> swapped = false;

	lock_page ();
	hash_insert (&pages, &page -> hash_elem);
	// struct hash_elem *inserted = hash_find(&pages, &page->hash_elem);
	// struct page *truly_inserted = hash_entry(inserted,struct page, hash_elem);
	// printf("==============\n%x\n",truly_inserted->hash_elem);
	unlock_page ();

	return true;
}

/*
Get rid of the page and all of its stuff
*/
bool
page_free (struct hash_elem free_me){
	struct page *page;
	struct hash_elem *found_this_page;

	found_this_page = hash_find (&pages, &free_me);
	if(found_this_page != NULL){
		page = hash_entry (found_this_page, struct page, hash_elem);
		palloc_free_page (&page->vaddr); //Free physical memory
		hash_delete (&pages, &page->hash_elem); //Free entry in the page table
		free (page); //Delete the structure

		return true;
	} else {
		return false;
	}
}

/*
look for a page with this address, if it isn't found return NULL
*/
uint8_t *
page_find (struct hash_elem find_me){
	struct page * page;
	struct hash_elem * found_page = NULL;
	printf("1\n");
	found_page = hash_find (&pages,&find_me);
	// printf("==========\n%x\n%x\n",find_me,found_page);
	printf("2\n");
	if(&found_page != NULL){
		printf("3\n");
		page = hash_entry (found_page, struct page, hash_elem);
		printf("3.5\n");
		return page->vaddr;
	} else {
		printf("4\n");
		return NULL;
	}
}


bool
page_in_frame (struct hash_elem framed_1){
	struct page * page;
	struct hash_elem * found_page;

	found_page = hash_find(&pages, &framed_1);
	if(found_page != NULL){
		page = hash_entry(found_page, struct page, hash_elem);
		return page->framed;
	}
	else
		return false;
}

// bool
// page_pinned (struct hash_elem pinned_1){
// 	struct page * page;
// 	struct hash_elem * found_page;

// 	found_page = hash_find(&pages, &pinned_1);
// 	if(found_page != NULL){
// 		page = hash_entry(found_page, struct page, hash_elem);
// 		return page->pinned;
// 	}
// 	else
// 		return false;
// }


bool
page_swapped (struct hash_elem swapped_1){
	struct page * page;
	struct hash_elem * found_page;

	found_page = hash_find(&pages, &swapped_1);
	if(found_page != NULL){
		page = hash_entry(found_page, struct page, hash_elem);
		return page->swapped;
	}
	else
		return false;
}
/*
Comparision function, returns if A is less than B
*/
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_	, void *aux UNUSED){
	const struct page * a = hash_entry (a_, struct page, hash_elem);
	const struct page * b = hash_entry (b_, struct page, hash_elem);
	return a->vaddr < b->vaddr;
}

/*
Can't use a hash table without a hash function
*/
unsigned
page_hash(const struct hash_elem *hash_this, void *aux UNUSED){
	const struct page * page = hash_entry (hash_this, struct page, hash_elem);
	return hash_bytes (&page->vaddr,sizeof page->vaddr);
}
