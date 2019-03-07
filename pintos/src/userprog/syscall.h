#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"
#include "lib/user/syscall.h"

#define CLOSE_ALL -1
#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2

struct child_process {
	int pid;
	int load;
	bool wait;
	bool exit;
	int status;
	struct lock wait_lock;
	struct list_elem elem;
};

void syscall_init (void);

void syscall_halt(void);
void syscall_exit(int status);
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);
bool syscall_create(const char *file, unsigned initial_size);
bool syscall_remove(const char *file);
int syscall_open(const char *file);
int syscall_filesize(int fd);
int syscall_read(int fd, void *buffer, unsigned size);
int syscall_write(int fd, const void *buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);
//extra functions
struct child_process* get_child_process(int pid);
int process_add_file(struct file *f);
struct file* process_get_file(int fd);
void process_close_file(int fd);
void get_arg(struct intr_frame *f, int *arg, int n);
void check_valid_ptr(const void *vaddr);
void check_valid_buffer(void* buffer, unsigned size);
void remove_child_process(struct child_process *cp);
void remove_child_processes(void);
int user_to_kernel_ptr(const void *vaddr);


#endif /* userprog/syscall.h */
