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


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  // struct thread *cur = thread_current ();
  // printf("%s\n", cur->name);

  void *p = f->esp;
  check_pointer(p);
  int sys_call_num = *(int *)p;
  // printf("==================================\n");
  // printf("%d\n", sys_call_num);
  // printf("==================================\n");


  switch (sys_call_num){
    /* 0. Halt the operating system. */
    case SYS_HALT:
      // halt();
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
      // check_pointer(p+4);
      // char *file_1 = (char *)(p+4);
      // f->eax = remove(file_1);
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
      f->eax = read(*(int *)(p+4),*(char **)(p+8),*(unsigned *)(p+12));
      break;
    /* 9. Write to a file. */
    case SYS_WRITE:
      check_pointer(p+4);
      check_pointer(*(void **)(p+8));
      check_pointer(p+12);
      f->eax = write(*(int *)(p+4), *(void **)(p+8), *(unsigned *)(p+12));
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
exit (int status)
{
  struct thread *cur = thread_current ();
  cur->exit_status = status;
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
exec (const char *cmd_line)
{
  // printf("%s\n",cmd_line );
  return process_execute(cmd_line);
}


/* Waits for a child process pid and retrieves the child's
exit status.

If pid is still alive, waits until it terminates.
Then, returns the status that pid passed to exit.
If pid did not call exit(), but was terminated by
the kernel (e.g. killed due to an exception),
wait(pid) must return -1. It is perfectly legal
for a parent process to wait for child processes
that have already terminated by the time the parent
calls wait, but the kernel must still allow the
parent to retrieve its child's exit status or
learn that the child was terminated by the kernel.

wait must fail and return -1 immediately if any of
the following conditions are true:

pid does not refer to a direct child of the calling
process.pid is a direct child of the calling process
if and only if the calling process received pid as
a return value from a successful call to exec.

Note that children are not inherited:
if A spawns child B and B spawns child process C,
then A cannot wait for C, even if B is dead.
A call to wait(C) by process A must fail. Similarly,
orphaned processes are not assigned to a new parent
if their parent process exits before they do.

The process that calls wait has already called wait
on pid. That is, a process may wait for any given
child at most once.

Processes may spawn any number of children, wait for
them in any order, and may even exit without having
waited for some or all of their children. Your
design should consider all the ways in which waits can
occur. All of a process's resources, including its struct
thread, must be freed whether its parent ever waits for
it or not, and regardless of whether the child exits
before or after its parent.

You must ensure that Pintos does not terminate until the
initial process exits. The supplied Pintos code tries to
do this by calling process_wait() (in "userprog/process.c")
from main() (in "threads/init.c"). We suggest that you
implement process_wait() according to the comment at the
top of the function and then implement the wait system call
in terms of process_wait().

Implementing this system call requires
considerably more work than any of the rest. */
int
wait (pid_t pid)
{
  // printf("%s\n",thread_current()->name );
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
  return filesys_create(file,(off_t)initial_size);
}

/* Deletes the file called file. Returns true if
successful, false otherwise. A file may be removed
regardless of whether it is open or closed, and
removing an open file does not close it. See Removing
an Open File, for details. */
bool
remove (const char *file)
{
  /*
  have to go through each process to see what is still accessing
  the file/file descriptor and wait for the process to finish.
  */
  //Only the last part

  /*
  if(look through process list(has file descriptor))
  wait for it to terminate
  then return filesys_remove(file);
  */
  return filesys_remove(file);
}

/* Opens the file called file. Returns a nonnegative
integer handle called a "file descriptor" (fd) or -1
if the file could not be opened.

File descriptors numbered 0 and 1 are reserved for the
console: fd 0 (STDIN_FILENO) is standard input,
fd 1 (STDOUT_FILENO) is standard output. The open system
call will never return either of these file descriptors,
which are valid as system call arguments only as
explicitly described below.

Each process has an independent set of file descriptors.
File descriptors are not inherited by child processes.

When a single file is opened more than once, whether by
a single process or different processes, each open
returns a new file descriptor. Different file descriptors
for a single file are closed independently in separate
calls to close and they do not share a file position. */
int
open (const char *file)
{
  // printf("====%s\n", file);
  struct thread *cur = thread_current ();
  struct file *nfile = filesys_open(file);

  // Opens the file called file
  // Add the current file's file descriptor to a list
  if(nfile == NULL)
  {
    return -1;
  }
  else
  {
    // Add the current file's file descriptor to a list
    cur->files[cur->fd_count-2] = nfile;
  }

  // // Return file descriptor
  return cur->fd_count++;
}

/* Returns the size, in bytes, of the file open as fd. */
int
filesize (int fd)
{
  struct thread *cur = thread_current();
  //file_length returns the size of File in bytes
  return file_length(cur->files[fd-2]);
}

/* Reads size bytes from the file open as fd into buffer.
Returns the number of bytes actually read (0 at end of file),
or -1 if the file could not be read (due to a condition other
than end of file). fd 0 reads from the keyboard using
input_getc(). */
int
read (int fd, void *buffer, unsigned size)
{
  int size_1 = size;
  char c;

  struct thread *cur = thread_current();
  if(fd == 0)
  {
    // while(size_1 != 0){
      uint8_t b = input_getc();
      // c = (char)b;
      // memcpy(buffer, &c, sizeof(char));
      // ++buffer;
      // --size_1;
    // }
    return b;
  }

  if(fd == 1){
  //   uint8_t buf = input_getc();
  //   return buf;
    exit(-1);
  }

  if(fd > cur->fd_count || fd < 0 || cur->files[fd-2] == NULL)
    return -1;
   
  int x = file_read(cur->files[fd-2], buffer,(off_t)size);
  // printf("%d\n",x);
  return x;

}

/* Writes size bytes from buffer to the open file fd.
Returns the number of bytes actually written, which
may be less than size if some bytes could not be written.
Writing past end-of-file would normally extend the
file, but file growth is not implemented by the basic
file system. The expected behavior is to write as many
bytes as possible up to end-of-file and return the actual
number written, or 0 if no bytes could be written at all.

fd 1 writes to the console. Your code to write to the
console should write all of buffer in one call to putbuf(),
at least as long as size is not bigger than a few
hundred bytes. (It is reasonable to break up larger
buffers.) Otherwise, lines of text output by different
processes may end up interleaved on the console, confusing
both human readers and our grading scripts. */
int
write (int fd, const void *buffer, unsigned size)
{

  // struct thread *cur = thread_current ();
  // printf("%d\n",cur->tid);
	
  // int x = 0;
	// char *buffer_copy = palloc_get_page(0);
  // strlcpy((char *)&buffer_copy,(char *)buffer,strlen(buffer)+1);

  // printf("%d\n", fd);

  if(fd == 1){
    putbuf((char *)buffer, size);
  }else{

  }
  // else if(find_file(fd)!=-1){
  //   if()
  //     x = (int)file_write((struct file *)fd, buffer,(off_t)size);
  // }
  // file_allow_write((struct file *)fd);
	return size;
}

/* Changes the next byte to be read or written in open file
fd to position, expressed in bytes from the beginning of
the file. (Thus, a position of 0 is the file's start.)

A seek past the current end of a file is not an error.
A later read obtains 0 bytes, indicating end of file.
A later write extends the file, filling any unwritten
gap with zeros. (However, in Pintos, files will have a
fixed length until project 4 is complete, so writes past
end of file will return an error.) These semantics are
implemented in the file system and do not require any
special effort in system call implementation. */
void
seek (int fd, unsigned position)
{
  //file_seek sets the current position in FILE to a new position
  file_seek((struct file *)fd, (off_t)position);
}

/* Returns the position of the next byte to be read or written
in open file fd, expressed in bytes from the beginning of the
file. */
unsigned
tell (int fd)
{
  //file_tell takes in a file and returns the current position in file
  return file_tell((struct file *)fd);
}

/* Closes file descriptor fd. Exiting or terminating a process
implicitly closes all its open file descriptors, as if by
calling this function for each one. */
void
close (int fd)
{
  struct thread *cur = thread_current ();
  //file_close closes the FILE
  if(fd < 2 || fd > cur->fd_count)
      exit(-1);
  file_close(cur->files[fd-2]);
  cur->files[fd-2] = NULL;
}
