#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/init.h"

#define MAX_ARGS 4
#define USER_VADDR_BOTTOM ((void *) 0x08048000)

struct lock filesys_lock;

struct process_file {
	struct file *file;
	int fd;
	struct list_elem elem;
};

int process_add_file(struct file *f);
struct file* process_get_file(int fd);

static void syscall_handler (struct intr_frame *);
int user_to_kernel_ptr(const void *vaddr);
void get_arg(struct intr_frame *f, int *arg, int n);
void check_valid_ptr(const void *vaddr);
void check_valid_buffer(void* buffer, unsigned size);

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int arg[MAX_ARGS];
	check_valid_ptr((const void*)f->esp);
	switch (*(int *)f->esp)
	{
		case SYS_HALT:
		{
			syscall_halt();
			break;
		}
		case SYS_EXIT:
		{
			get_arg(f, &arg[0], 1);
			syscall_exit(arg[0]);
			break;
		}
		case SYS_EXEC:
		{
			get_arg(f, &arg[0], 1);
			arg[0] = user_to_kernel_ptr((const void *)arg[0]);
			f->eax = syscall_exec((const char *)arg[0]);
			break;
		}
		case SYS_WAIT:
		{
			get_arg(f, &arg[0], 1);
			f->eax = syscall_wait(arg[0]);
			break;
		}
		case SYS_CREATE:
		{
			get_arg(f, &arg[0], 2);
			arg[0] = user_to_kernel_ptr((const void *)arg[0]);
			f->eax = syscall_create((const char *)arg[0], (unsigned)arg[1]);
			break;
		}
		case SYS_REMOVE:
		{
			get_arg(f, &arg[0], 1);
			arg[0] = user_to_kernel_ptr((const void *)arg[0]);
			f->eax = syscall_remove((const char *)arg[0]);
			break;
		}
		case SYS_OPEN:
		{
			get_arg(f, &arg[0], 1);
			arg[0] = user_to_kernel_ptr((const void *)arg[0]);
			f->eax = syscall_open((const char *)arg[0]);
			break;
		}
		case SYS_FILESIZE:
		{
			get_arg(f, &arg[0], 1);
			f->eax = syscall_filesize(arg[0]);
			break;
		}
		case SYS_READ:
		{
			get_arg(f, &arg[0], 3);
			check_valid_buffer((void *)arg[1], (unsigned)arg[2]);
			arg[1] = user_to_kernel_ptr((const void *)arg[1]);
			f->eax = syscall_read(arg[0], (void *)arg[1], (unsigned)arg[2]);
			break;
		}
		case SYS_WRITE:
		{
			get_arg(f, &arg[0], 3);
			check_valid_buffer((void *)arg[1], (unsigned)arg[2]);
			arg[1] = user_to_kernel_ptr((const void *)arg[1]);
			f->eax = syscall_write(arg[0], (const void *)arg[1],
				(unsigned)arg[2]);
			break;
		}
		case SYS_SEEK:
		{
			get_arg(f, &arg[0], 2);
			syscall_seek(arg[0], (unsigned)arg[1]);
			break;
		}
		case SYS_TELL:
		{
			get_arg(f, &arg[0], 1);
			f->eax = syscall_tell(arg[0]);
			break;
		}
		case SYS_CLOSE:
		{
			get_arg(f, &arg[0], 1);
			syscall_close(arg[0]);
			break;
		}
	}
}

void syscall_halt(void)
{
	shutdown_power_off();
}

void syscall_exit(int status)
{
	struct thread *cur = thread_current();
	if (thread_alive(cur->parent))
	{
		cur->cp->status = status;
	}
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

pid_t syscall_exec(const char *cmd_line)
{
	pid_t pid = process_execute(cmd_line);
	struct child_process *cp = get_child_process(pid);
	ASSERT(cp);
	while (cp->load == NOT_LOADED)
	{
		barrier();
	}
	if (cp->load == LOAD_FAIL) 
	{
		return ERROR;
	}
	return pid;
}

int syscall_wait(pid_t pid)
{
	return process_wait(pid);
}

bool syscall_create(const char *file, unsigned initial_size)
{
	lock_acquire(&filesys_lock);
	bool success = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return success;
}

bool syscall_remove(const char *file)
{
	lock_acquire(&filesys_lock);
	bool success = filesys_remove(file);
	lock_release(&filesys_lock);
	return success;
}

int syscall_open(const char *file)
{
	lock_acquire(&filesys_lock);
	struct file *f = filesys_open(file);
	if (!f)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int fd = process_add_file(f);
	lock_release(&filesys_lock);
	return fd;
}

int syscall_filesize(int fd)
{
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int size = file_length(f);
	lock_release(&filesys_lock);
	return size;
}

int syscall_read(int fd, void *buffer, unsigned size)
{
	if (fd == STDIN_FILENO)
	{
		unsigned i;
		uint8_t* local_buffer = (uint8_t *)buffer;
		for (i = 0; i < size; i++)
		{
			local_buffer[i] = input_getc();
		}
		return size;
	}
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int bytes = file_read(f, buffer, size);
	lock_release(&filesys_lock);
	return bytes;
}

int syscall_write(int fd, const void *buffer, unsigned size)
{
	if (fd == STDOUT_FILENO)
	{
		putbuf(buffer, size);
		return size;
	}
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	int bytes = file_write(f, buffer, size);
	lock_release(&filesys_lock);
	return bytes;
}

void syscall_seek(int fd, unsigned position)
{
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return;
	}
	file_seek(f, position);
	lock_release(&filesys_lock);
}

unsigned syscall_tell(int fd)
{
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return ERROR;
	}
	off_t offset = file_tell(f);
	lock_release(&filesys_lock);
	return offset;
}

void syscall_close(int fd)
{
	lock_acquire(&filesys_lock);
	process_close_file(fd);
	lock_release(&filesys_lock);
}

//Extra functions outside syscall
struct child_process* get_child_process(int pid)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for (e = list_begin(&t->child_list); e != list_end(&t->child_list);
		e = list_next(e))
	{
		struct child_process *cp = list_entry(e, struct child_process, elem);
		if (pid == cp->pid)
		{
			return cp;
		}
	}
	return NULL;
}

int process_add_file(struct file *f)
{
	struct process_file *pf = malloc(sizeof(struct process_file));
	pf->file = f;
	pf->fd = thread_current()->fd;
	thread_current()->fd++;
	list_push_back(&thread_current()->file_list, &pf->elem);
	return pf->fd;
}

struct file* process_get_file(int fd)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for (e = list_begin(&t->file_list); e != list_end(&t->file_list);
		e = list_next(e))
	{
		struct process_file *pf = list_entry(e, struct process_file, elem);
		if (fd == pf->fd)
		{
			return pf->file;
		}
	}
	return NULL;
}

void process_close_file(int fd)
{
	struct thread *t = thread_current();
	struct list_elem *next, *e = list_begin(&t->file_list);

	while (e != list_end(&t->file_list))
	{
		next = list_next(e);
		struct process_file *pf = list_entry(e, struct process_file, elem);
		if (fd == pf->fd || fd == CLOSE_ALL)
		{
			file_close(pf->file);
			list_remove(&pf->elem);
			free(pf);
			if (fd != CLOSE_ALL)
			{
				return;
			}
		}
		e = next;
	}
}

void get_arg(struct intr_frame *f, int *arg, int n)
{
	int i;
	int *ptr;
	for (i = 0; i < n; i++)
	{
		ptr = (int *)f->esp + i + 1;
		check_valid_ptr((const void *)ptr);
		arg[i] = *ptr;
	}
}

void check_valid_ptr(const void *vaddr)
{
	if (!is_user_vaddr(vaddr) || vaddr < USER_VADDR_BOTTOM)
	{
		syscall_exit(ERROR);
	}
}

void check_valid_buffer(void* buffer, unsigned size)
{
	unsigned i;
	char* local_buffer = (char *)buffer;
	for (i = 0; i < size; i++)
	{
		check_valid_ptr((const void*)local_buffer);
		local_buffer++;
	}
}

void remove_child_process(struct child_process *cp)
{
	list_remove(&cp->elem);
	free(cp);
}

void remove_child_processes(void)
{
	struct thread *t = thread_current();
	struct list_elem *next, *e = list_begin(&t->child_list);

	while (e != list_end(&t->child_list))
	{
		next = list_next(e);
		struct child_process *cp = list_entry(e, struct child_process,
			elem);
		list_remove(&cp->elem);
		free(cp);
		e = next;
	}
}

int user_to_kernel_ptr(const void *vaddr)
{
	check_valid_ptr(vaddr);
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		syscall_exit(ERROR);
	}
	return (int)ptr;
}

struct child_process* add_child_process(int pid)
{
	struct child_process* cp = malloc(sizeof(struct child_process));
	cp->pid = pid;
	cp->load = NOT_LOADED;
	cp->wait = false;
	cp->exit = false;
	lock_init(&cp->wait_lock);
	list_push_back(&thread_current()->child_list,
		&cp->elem);
	return cp;
}