#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "lib/string.h"
#include "threads/palloc.h"

static void syscall_handler (struct intr_frame *);


//declaration of everything
void check_pointer(void *addr); 
void halt (void);
void close (int fd);
unsigned tell (int fd);
void seek (int fd, unsigned position);
int write (int fd, const void *buffer, unsigned size);
int read (int fd, void *buffer, unsigned size);
int filesize (int fd);
int open (const char *file);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int wait (pid_t pid);
pid_t exec (const char *cmd_line);

struct semaphore syscall_sema;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init (&syscall_sema,1);
}

static void
syscall_handler (struct intr_frame *f) 
{
  //grab stack pointer
  void *p = f->esp;
  //check if is a valid pointer
  check_pointer(p);
  //grab the call number
  int sys_call_num = *(int *)p;

  //p+4 is the first thing on the stack, p+8 is the second, p+12 is the third one
  //function returns are stored in eax 
  switch (sys_call_num){
    /* 0. Halt the operating system. */
    case SYS_HALT:
      halt();
      break;

    /* 1. Terminate this process. */
    case SYS_EXIT:
      check_pointer(p+4);
      exit(*(int *)(p+4));
      break;

    /* 2. Start another process. */
    case SYS_EXEC:
      check_pointer(*(char **)(p+4));
      f->eax = exec(*(char **)(p+4));
      break;

    /* 3. Wait for a child process to die. */
    case SYS_WAIT:
      check_pointer(p+4);
      f->eax = wait(*(pid_t *)(p+4));
      break;

    /* 4. Create a file. */
    case SYS_CREATE:
      check_pointer(*(char **)(p+4));
      check_pointer(p+8);
      f->eax = create(*(char **)(p+4),*(unsigned *)(p+8));
      break;
    /* 5. Delete a file. */
    case SYS_REMOVE:
      check_pointer(*(char **)(p+4));
      f->eax = remove(*(char **)(p+4));
      break;
    /* 6. Open a file. */
    case SYS_OPEN:
      check_pointer(*(char **)(p+4));
      f->eax = open(*(char **)(p+4));
      break;
    /* 7. Obtain a file's size. */
    case SYS_FILESIZE:
      check_pointer(p+4);
      f->eax = filesize(*(int *)(p+4));
      break;
    /* 8. Read from a file. */
    case SYS_READ:
      check_pointer(p+4);
      check_pointer(*(char **)(p+8));
      check_pointer(p+12);
      thread_current()->saved_esp = f->esp;
      f->eax = read(*(int *)(p+4),*(char **)(p+8),*(unsigned *)(p+12));
      break;
    /* 9. Write to a file. */
    case SYS_WRITE:
      check_pointer(p+4);
      check_pointer(*(void **)(p+8));
      check_pointer(p+12);
      thread_current()->saved_esp = f->esp;
      f->eax = write(*(int *)(p+4), *(void **)(p+8), *(unsigned *)(p+12));
      break;
    /* 10. Change position in a file. */
    case SYS_SEEK:
      check_pointer(p+4);
      check_pointer(p+8);
      seek(*(int *)(p+4), *(unsigned *)(p+8));
      break;
    /* 11. Report current position in a file. */
    case SYS_TELL:
      check_pointer(p+4);
      f->eax = tell(*(int *)(p+4));
      break;
    /* 12. Close a file. */
    case SYS_CLOSE:
      check_pointer(p+4);
      close(*(int *)(p+4));
      break;
  }
}

/* Check the following:
   a pointer to unmapped virtual memory,
   a pointer to kernel virtual address space (above PHYS_BASE) */
void
check_pointer(void *addr){
  struct thread *cur = thread_current ();
  uint32_t *pd = cur->pagedir;
  //if the given pointer is a kernel virtual address, or it is invalid the process exits
    if(is_kernel_vaddr(addr) ||
      pagedir_get_page(pd, addr) == NULL ||
      addr == NULL){
      exit(-1);
    }
}

/* Terminates Pintos by calling shutdown_power_off()
(declared in "devices/shutdown.h"). This should be
seldom used, because you lose some information
about possible deadlock situations, etc. */
void
halt (void)
{
  shutdown_power_off();
}

/* Terminates the current user program, returning status
to the kernel. If the process's parent waits for it
(see below), this is the status that will be returned.
Conventionally, a status of 0 indicates success and
nonzero values indicate errors. */
void
exit (int status){
  struct thread *cur = thread_current ();
  //this is so the parent can grab its child's status
  cur->exit_status = status;
  cur->exiting = true;
  printf("%s: exit(%d)\n",cur->name,status);
  thread_exit();
}

/* Runs the executable whose name is given in cmd_line,
passing any given arguments, and returns the new process's
program id (pid). Must return pid -1, which otherwise
should not be a valid pid, if the program cannot load or
run for any reason. Thus, the parent process cannot return
from the exec until it knows whether the child process
successfully loaded its executable. You must use appropriate
synchronization to ensure this. */
pid_t
exec (const char *cmd_line){
  // sema_down (&syscall_sema);
  int exec_pid = process_execute(cmd_line);
  // sema_up (&syscall_sema);
  return exec_pid;
}


/* Waits for a child process pid and retrieves the child's
exit status.

If pid is still alive, waits until it terminates.
Then, returns the status that pid passed to exit.
If pid did not call exit(), but was terminated by
the kernel (e.g. killed due to an exception),
wait(pid) must return -1. wait must also fail and 
return -1 immediately if pid does not refer to a 
direct child of the calling process.
*/
int
wait (pid_t pid)
{
  return process_wait(pid);
}

/* Creates a new file called file initially initial_size
bytes in size. Returns true if successful, false
otherwise. Creating a new file does not open it:
opening the new file is a separate operation which
would require a open system call. */
bool
create (const char *file, unsigned initial_size)
{
  sema_down (&syscall_sema);
  bool create_bool = filesys_create(file,(off_t)initial_size);
  sema_up (&syscall_sema);
  return create_bool;
}

/* Deletes the file called file. Returns true if
successful, false otherwise. A file may be removed
regardless of whether it is open or closed, and
removing an open file does not close it. See Removing
an Open File, for details. */
bool
remove (const char *file)
{
  sema_down (&syscall_sema);
  bool remove_bool = filesys_remove(file);
  sema_up (&syscall_sema);
  return remove_bool;
}

/* Opens the file called file. Returns a nonnegative
integer handle called a "file descriptor" (fd) or -1
if the file could not be opened.

File descriptors numbered 0 and 1 are reserved for the
console: fd 0 (STDIN_FILENO) is standard input,
fd 1 (STDOUT_FILENO) is standard output. */
int
open (const char *file)
{
  sema_down (&syscall_sema);
  struct thread *cur = thread_current ();
  struct file *nfile = filesys_open(file);

  // Opens the file called file
  if(nfile == NULL)
  {
    sema_up (&syscall_sema);
    return -1;
  }
  else
  {
    // Set struct file pointer to files array
    cur->files[cur->fd_count-2] = nfile;
  }

  // Return file descriptor
  int open_fd = cur->fd_count++;
  sema_up (&syscall_sema);
  return open_fd;
}

/* Returns the size, in bytes, of the file open as fd. */
int
filesize (int fd)
{
  sema_down (&syscall_sema);
  struct thread *cur = thread_current();
  //file_length returns the size of File in bytes
  int file_size = file_length(cur->files[fd-2]);
  sema_up (&syscall_sema);
  return file_size;
}

/* Reads size bytes from the file open as fd into buffer.
Returns the number of bytes actually read (0 at end of file),
or -1 if the file could not be read (due to a condition other
than end of file). fd 0 reads from the keyboard using
input_getc(). */
int
read (int fd, void *buffer, unsigned size)
{
  sema_down (&syscall_sema);
  // printf("%s\n","_____________" );
  struct thread *cur = thread_current();

  //keyboard input
  if(fd == 0){
    // printf("0\n");
    uint8_t b = input_getc();
    sema_up (&syscall_sema);
    return b;
  }
  //standard output
  if(fd == 1){
    sema_up (&syscall_sema);
    exit(-1);
  }
  //if it isn't in the fd bounds, it fails
  if(fd > cur->fd_count || fd < 0 || cur->files[fd-2] == NULL){
    sema_up (&syscall_sema);
    return -1;
  }
  //it can begin reading

  // printf("offset: %u\n", file_get_offset(cur->files[fd-2]));//this call needs to be deleted from file.c
  int x = file_read(cur->files[fd-2], buffer,(off_t)size);  
  sema_up (&syscall_sema);
  return x;

}

/* Writes size bytes from buffer to the open file fd.
Returns the number of bytes actually written, which
may be less than size if some bytes could not be written.
Writing past end-of-file would normally extend the
file, but file growth is not implemented by the basic
file system. The expected behavior is to write as many
bytes as possible up to end-of-file and return the actual
number written, or 0 if no bytes could be written at all.*/
int
write (int fd, const void *buffer, unsigned size)
{
  sema_down (&syscall_sema);
  struct thread *cur = thread_current ();
	
  int x = 0;
  //fd 1 writes to the console 
  if(fd == 1){
    putbuf((char *)buffer, size);
  }
  //if it isn't a valid fd
  else if(fd == 0 || fd >= cur->fd_count){
    sema_up (&syscall_sema);
    exit(-1);
  }
  //let it write
  else{
    x = file_write(cur->files[fd-2],buffer,(off_t)size);
  }
	int write_size = (size > x) ? x : size ;
  sema_up (&syscall_sema);
  return write_size;
}

/* Changes the next byte to be read or written in open file
fd to position, expressed in bytes from the beginning of
the file. (Thus, a position of 0 is the file's start.)

A seek past the current end of a file is not an error.
A later read obtains 0 bytes, indicating end of file.
A later write extends the file, filling any unwritten
gap with zeros. These semantics are implemented in the
file system and did not require any special effort in
system call implementation. */
void
seek (int fd, unsigned position)
{
  sema_down (&syscall_sema);
  struct thread *cur = thread_current ();
  file_seek(cur->files[fd-2], (off_t)position);
  sema_up (&syscall_sema);
}

/* Returns the position of the next byte to be read or written
in open file fd, expressed in bytes from the beginning of the
file. */
unsigned
tell (int fd)
{
  sema_down (&syscall_sema);
  struct thread *cur = thread_current ();
  //file_tell takes in a file and returns the current position in file
  unsigned tell_size = file_tell(cur->files[fd-2]);
  sema_up (&syscall_sema);
  return tell_size;
}

/* Closes file descriptor fd. Exiting or terminating a process
implicitly closes all its open file descriptors, as if by
calling this function for each one. */
void
close (int fd)
{
  sema_down (&syscall_sema);
  struct thread *cur = thread_current ();
  if(fd < 2 || fd >= cur->fd_count){
      sema_up (&syscall_sema);
      exit(-1);
  }
  file_close(cur->files[fd-2]);
  cur->files[fd-2] = NULL;
  sema_up (&syscall_sema);
}