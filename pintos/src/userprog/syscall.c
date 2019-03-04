#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
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
	return;
}

int syscall_halt(pid_t pid)
{
	return;
}

bool syscall_create(const char *file, unsigned initial_size)
{
	return;
}

bool syscall_remove(const char *file)
{
	return;
}

int syscall_open(const char *file)
{
	return;
}

int syscall_filesize(int fd)
{
	return;
}

int syscall_read(int fd, void *buffer, unsigned size)
{
	return;
}

int syscall_write(int fd, const void *buffer, unsigned size)
{
	return;
}

void syscall_seek(int fd, unsigned position)
{
	return;
}

unsigned syscall_tell(int fd)
{
	return;
}

void syscall_close(int fd)
{
	return;
}

