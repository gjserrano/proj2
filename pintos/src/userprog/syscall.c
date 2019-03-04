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
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

static void
syscall_halt(void)
{
	return;
}

static void
syscall_exit(int status)
{
	return;
}

static pid_t
syscall_exec(const char *cmd_line)
{
	return;
}

static int
syscall_halt(pid_t pid)
{
	return;
}

static bool
syscall_create(const char *file, unsigned initial_size)
{
	return;
}

static bool
syscall_remove(const char *file)
{
	return;
}

static int
syscall_open(const char *file)
{
	return;
}

static int
syscall_filesize(int fd)
{
	return;
}

static int
syscall_read(int fd, void *buffer, unsigned size)
{
	return;
}

static int
syscall_write(int fd, const void *buffer, unsigned size)
{
	return;
}

static void
syscall_seek(int fd, unsigned position)
{
	return;
}

static unsigned
syscall_tell(int fd)
{
	return;
}

static void
syscall_close(int fd)
{
	return;
}

