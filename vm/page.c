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

void
page_init (){
	hash_init (&pages, page_hash, page_less, NULL);
	lock_init (&page_lock);
}

/*
Allocates a new page then adds to page table
*/
struct hash_elem
page_get (){
	struct page * page = (struct page*) malloc (sizeof (struct page));

	page -> page = palloc_get_page(PAL_USER);
	page -> addr = (uint32_t)&page->page;
	page -> pinned = false;
	page -> framed = false;

	lock_page ();
	hash_insert (&pages, &page -> hash_elem);
	unlock_page ();

	return page->hash_elem;
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
		palloc_free_page (&page->addr); //Free physical memory
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
struct page *
page_find (struct hash_elem find_me){
	struct page * page;
	struct hash_elem * found_page;

	found_page = hash_find (&pages,&find_me);
	if(found_page != NULL){
		page = hash_entry (found_page, struct page, hash_elem);
		return page;
	} else {
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

bool
page_pinned (struct hash_elem pinned_1){
	struct page * page;
	struct hash_elem * found_page;

	found_page = hash_find(&pages, &pinned_1);
	if(found_page != NULL){
		page = hash_entry(found_page, struct page, hash_elem);
		return page->pinned;
	}
	else
		return false;
}


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
	return a->addr < b->addr;
}

/*
Can't use a hash table without a hash function
*/
unsigned
page_hash(const struct hash_elem *hash_this, void *aux UNUSED){
	const struct page * page = hash_entry (hash_this, struct page, hash_elem);
	return hash_bytes (&page->addr,sizeof page->addr);
}