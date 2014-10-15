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

static void syscall_handler (struct intr_frame *);
typedef int pid_t;

void check_pointer(void *addr);
void halt (void);
void close (int fd);
unsigned tell (int fd);
void seek (int fd, unsigned position);
int write (int fd, const void *buffer, unsigned size);
int read (int fd, void *buffer, unsigned size);
int filesize (int fd);
int open (const char *file);
bool remove (const char *file);
bool create (const char *file, unsigned initial_size);
int wait (pid_t pid);
pid_t exec (const char *cmd_line);
void exit (int status);

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
  for(x=0; x<=16; x+=4){
    printf("0x%08x <esp+%d>\t0x%08x\n", p+x, x, *(int *)(p+x));
  }
  
  // printf("==================================\n");
  // terminating the offending process and freeing its resources. 
  // if pointer is invalid 
  // user can pass a null pointer
  // a pointer to unmapped virtual memory,
  //  get_page_dir()
  //  current thread has a pagedir
  // a pointer to kernel virtual address space (above PHYS_BASE)
  //  is_kernal_vaddr()

  /* Check user mem access */

  switch (sys_call_num){
  	/* 0. Halt the operating system. */
  	case SYS_HALT:
      halt();
  		break;
  	/* 1. Terminate this process. */
  	case SYS_EXIT:
      // Terminates the current user program, returning status to the kernel. If the process's parent waits for it (see below), this is the status that will be returned. Conventionally, a status of 0 indicates success and nonzero values indicate errors. 
  		check_pointer(p+4);
      int status = (int)(p+4);
      exit(status);
      break;
  	/* 2. Start another process. */
  	case SYS_EXEC:
      check_pointer(p+4);
      const char *cmd_line = (const char *)(p+4);
      exec(cmd_line);
  		break;
 	  /* 3. Wait for a child process to die. */
  	case SYS_WAIT:
      check_pointer(p+4);
      pid_t pid = (pid_t)(p+4);
      wait(pid);
  		break;
    /* 4. Create a file. */
    case SYS_CREATE:
      check_pointer(p+4);
      check_pointer(p+8);
      unsigned initial_size = (unsigned)(p+8);
      char *file = (char *)(p+4);
      create(file,initial_size);
    	break;
   	/* 5. Delete a file. */         
    case SYS_REMOVE:
      check_pointer(p+4);
      char *file_1 = (char *)(p+4);
      remove(file_1);
    	break;   
    /* 6. Open a file. */
    case SYS_OPEN:
      // Get the first arguement and check validity
      check_pointer(p+4);
      char *file_2 = (char *)(p+4);
      open(file_2);
   		break;
  	/* 7. Obtain a file's size. */
    case SYS_FILESIZE:
      check_pointer(p+4);
      int fd = (int)(p+4);
      filesize(fd);
      break;
    /* 8. Read from a file. */              
    case SYS_READ:
      check_pointer(p+4);
      check_pointer(p+8);
      check_pointer(p+12);
      unsigned size = (unsigned)(p+12);
      void *buffer = (p+8);
      int fd_1 = (int)(p+4);
      read(fd_1,buffer,size);
    	break;         
    /* 9. Write to a file. */
    case SYS_WRITE:
      check_pointer(p+4);
      check_pointer(p+8);
      check_pointer(p+12);
      unsigned size_1 = (unsigned)(p+12);
      void *buffer_1 = (p+8);
      int fd_2 = (int)(p+4);
      write(fd_2, buffer_1, size_1);
    	break;     
    /* 10. Change position in a file. */           
    case SYS_SEEK:
      check_pointer(p+4);
      check_pointer(p+8);
      unsigned position = (unsigned)(p+8);
      int fd_3 = (int)(p+4);
      seek(fd_3, position);
    	break; 
    /* 11. Report current position in a file. */
    case SYS_TELL:
      check_pointer(p+4);
      int fd_4 = (int)(p+4);
      tell(fd_4);
    	break; 
   	/* 12. Close a file. */      
    case SYS_CLOSE:
      check_pointer(p+4);
      int fd_5 = (int)(p+4);
      close(fd_5);
    	break;
  }
  thread_exit ();
}

void check_pointer(void *addr){
  struct thread *cur = thread_current ();
  uint32_t *pd = cur->pagedir;

  // Check the following: 
  // a pointer to unmapped virtual memory,
  // a pointer to kernel virtual address space (above PHYS_BASE)
  if(is_kernel_vaddr(addr) || pagedir_get_page(pd, addr) == NULL){    
    // Free resources: Locks, Page Directories
    if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
      // Terminate process
      thread_exit();
    }
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
  //wut
}

/* Creates a new file called file initially initial_size
   bytes in size. Returns true if successful, false 
   otherwise. Creating a new file does not open it: 
   opening the new file is a separate operation which 
   would require a open system call. */
bool 
create (const char *file, unsigned initial_size)
{
  //Creates a a file named file with size of initial_size
  return filesys_create(file, (off_t)initial_size);

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
  // File descriptor
  int fd = -1;

  // Get the current thread
  struct thread *cur = thread_current ();

  // Opens the file called file
  // Add the current file's file descriptor to a list
  if(filesys_open(file) == NULL)
  {
    fd = -1;
  }
  else
  {
    // Add the current file's file descriptor to a list
  }

  // Return file descriptor
  return fd;
}

/* Returns the size, in bytes, of the file open as fd. */
int
filesize (int fd)
{
  //file_length returns the size of File in bytes
  return file_length((struct file *)fd);
}

/* Reads size bytes from the file open as fd into buffer. 
   Returns the number of bytes actually read (0 at end of file), 
   or -1 if the file could not be read (due to a condition other 
   than end of file). fd 0 reads from the keyboard using 
   input_getc(). */
int 
read (int fd, void *buffer, unsigned size)
{
  //Not sure if this is correct? Don't feel like it is
  if(file_read((struct file *)fd, buffer,(off_t)size))
    return 0;
  else
    return -1;
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
  //file_write accomplishes this
  file_deny_write((struct file *)fd);
  int x = (int)file_write((struct file *)fd, buffer,(off_t)size);
  file_allow_write((struct file *)fd);
  return x;
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
  //file_close closes the FILE
  file_close((struct file *)fd);
}
