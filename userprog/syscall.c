#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf("==================================\n");
  void *p = f->esp;
  int sys_call_num = *(int *)p;
  int x;
  for(x=16; x>=0; x-=4){
  	char * c = palloc_get_page(0);
  	memcpy(c, p+x, sizeof(char*));
  	printf("%s\n", c);
  	// printf("0x%08x <esp+%d>\t0x%08x\n", p+x, x, *(int *)(p+x));
  }
  printf("==================================\n");

  /* Check user mem access */

  switch (sys_call_num){
  	/* Halt the operating system. */
  	case SYS_HALT:
  		break;
  	/* Terminate this process. */
  	case SYS_EXIT:
  		break;
  	/* Start another process. */
  	case SYS_EXEC:
  		break;
 	/* Wait for a child process to die. */
  	case SYS_WAIT:
  		break;
	/* Create a file. */
    case SYS_CREATE:
    	break;
   	/* Delete a file. */         
    case SYS_REMOVE:
    	break;   
    /* Open a file. */
    case SYS_OPEN:
   		break;
  	/* Obtain a file's size. */
    case SYS_FILESIZE:
    /* Read from a file. */              
    case SYS_READ:
    	break;         
    /* Write to a file. */
    case SYS_WRITE:
    	break;     
    /* Change position in a file. */           
    case SYS_SEEK:
    	break; 
    /* Report current position in a file. */
    case SYS_TELL:
    	break; 
   	/* Close a file. */      
    case SYS_CLOSE:
    	break;
  }
  thread_exit ();
}
