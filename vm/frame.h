#ifndef VM_FRAME_H
#define VM_FRAME_H

//need hash file, along with page and swap files we create
#include "lib/kernel/hash.h"
#include "vm/page.h"
#include "threads/thread.h"

struct frame {
	uint32_t frame_number;				/* Number corresponding to frame */
	uint8_t *page;						/* pointer to the page resident in the frame */
	struct thread *thread;				/* Thread the page belongs to*/
	bool pinned;						/* don't evict this */
	bool held;							/* Set to true if the frame is currently held by a thread */
};

void frame_init (void);
struct frame *frame_get (void);
bool frame_free (struct frame *);
struct frame *frame_find_from_number (int);

#endif
