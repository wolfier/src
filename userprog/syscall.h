#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void syscall_init (void);
void check_pointer(void *addr);
int wait(pid_t pid);
pid_t exec(const char* cmd_line);
void exit(int status);

#endif /* userprog/syscall.h */
