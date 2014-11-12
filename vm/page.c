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
uint32_t
page_get (){
	struct thread * t = thread_current ();

	struct page * page = (struct page*) malloc (sizeof (struct page));
	page -> addr = kpage;
	page -> uvaddr = uvaddr;
	page -> thread = t;
	page -> pinned = false;
	page -> resident = false;
	page -> fuck_you_Zach = true;
	page -> fuck_you_Jae = false;

	lock_page ();
	hash_insert (&pages, &page -> hash_elem);
	unlock_page ();

	return uvaddr;
}

/*
Get rid of the page and all of its stuff
*/
bool
page_free (uint32_t uvaddr){
	struct page * page;
	struct hash_elem * found_this_page;
	struct page page_elem;
	page_elem.addr = addr;

	found_this_page = hash_find (&pages, &page_elem.hash_elem);
	if(found_this_page != NULL){
		page = hash_entry (found_this_page, struct page, hash_elem);

		palloc_free_page (page->addr); //Free physical memory
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
page_find (void * addr){
	struct page * page;
	struct hash_elem * found_page;
	struct page page_elem;
	page_elem.addr = addr;

	found_page = hash_find (&pages, &page_elem.hash_elem);
	if(found_page != NULL){
		page = hash_entry (found_page, struct page, hash_elem);
		return page;
	} else {
		return NULL;
	}
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
	return hash_int ((unsigned)page->addr);
}