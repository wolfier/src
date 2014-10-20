#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "userprog/pagedir.h"
#include "filesys/file.h";
#include "userprog/process.h"

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
  printf("%d\n", sys_call_num);
  printf("==================================\n");


  switch (sys_call_num){
    /* 0. Halt the operating system. */
    case SYS_HALT:
      // halt();
      break;

    /* 1. Terminate this process. */
    case SYS_EXIT:
      // Terminates the current user program, returning status to the kernel.
      // If the process's parent waits for it (see below), this is the status
      // that will be returned. Conventionally, a status of 0 indicates success
      // and nonzero values indicate errors.
      check_pointer(p+4);
      int status = (int)(p+4);
      exit(status);
      break;

    /* 2. Start another process. */
    case SYS_EXEC:
      // check_pointer(p+4);
      // const char *cmd_line = (const char *)(p+4);
      // f->eax = exec(cmd_line);
      break;

    /* 3. Wait for a child process to die. */
    case SYS_WAIT:
      // check_pointer(p+4);
      // pid_t pid = (pid_t)(p+4);
      // f->eax = wait(pid);
      break;

    /* 4. Create a file. */
    case SYS_CREATE:
      // check_pointer(p+4);
      // check_pointer(p+8);
      // unsigned initial_size = (unsigned)(p+8);
      // char *file = (char *)(p+4);
      // f->eax = create(file,initial_size);
      break;
    /* 5. Delete a file. */
    case SYS_REMOVE:
      // check_pointer(p+4);
      // char *file_1 = (char *)(p+4);
      // f->eax = remove(file_1);
      break;
    /* 6. Open a file. */
    case SYS_OPEN:
      // Get the first arguement and check validity
      // check_pointer(p+4);
      // char *file_2 = (char *)(p+4);
      // f->eax = open(file_2);
      break;
    /* 7. Obtain a file's size. */
    case SYS_FILESIZE:
      // check_pointer(p+4);
      // int fd = (int)(p+4);
      // f->eax = filesize(fd);
      break;
    /* 8. Read from a file. */
    case SYS_READ:
      // check_pointer(p+4);
      // check_pointer(p+8);
      // check_pointer(p+12);
      // unsigned size = (unsigned)(p+12);
      // void *buffer = (p+8);
      // int fd_1 = (int)(p+4);
      // f->eax = read(fd_1,buffer,size);
      break;
    /* 9. Write to a file. */
    case SYS_WRITE:
      // check_pointer(p+4);
      // check_pointer(p+8);
      // check_pointer(p+12);
      // unsigned size_1 = (unsigned)(p+12);
      // void *buffer_1 = (p+8);
      // int fd_2 = (int)(p+4);
      // f->eax = write(fd_2, buffer_1, size_1);
      break;
    /* 10. Change position in a file. */
    case SYS_SEEK:
      // check_pointer(p+4);
      // check_pointer(p+8);
      // unsigned position = (unsigned)(p+8);
      // int fd_3 = (int)(p+4);
      // seek(fd_3, position);
      break;
    /* 11. Report current position in a file. */
    case SYS_TELL:
      // check_pointer(p+4);
      // int fd_4 = (int)(p+4);
      // f->eax = tell(fd_4);
      break;
    /* 12. Close a file. */
    case SYS_CLOSE:
      // check_pointer(p+4);
      // int fd_5 = (int)(p+4);
      // close(fd_5);
      break;
  }
}

void check_pointer(void *addr){
  struct thread *cur = thread_current ();
  uint32_t *pd = cur->pagedir;

    // Check the following:
    // a pointer to unmapped virtual memory,
    // a pointer to kernel virtual address space (above PHYS_BASE)
    if(is_kernel_vaddr(addr) || pagedir_get_page(pd, addr) == NULL || addr == NULL){
      thread_exit();
    }
}

void
exit (int status)
{
  struct thread *cur = thread_current ();
  cur->exit_status = status;
  printf("%d\n", status);
  thread_exit();
}

pid_t
exec (const char *cmd_line)
{
  int x = process_execute(cmd_line);
  return x;
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}
