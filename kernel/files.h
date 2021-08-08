#ifndef FILES_H
#define FILES_H

#include "../cpu/types.h"
#include "../lock/lock.h"
#include "../mem/mem.h"
#include "../proc/thread.h"
#include "../kernel/kernel.h"
#include "../kernel/scheduler.h"

#define FD_STREAM 0
#define FD_BLOCK 1

typedef struct file_descriptor {
  u8 type;
  u32 fd;
  char *name;
  Lock *lock;
  byte *buffer;
  u32 size;
  u32 read_ptr;
  u32 available;
  u32 write_ptr;
  List q;

  u32 (*write)(u32 fd, u32 count, byte *src);
  u32 (*read)(u32 fd, u32 count, byte *dst);
  u32 (*seek)(u32, u32);

} FD;

extern List file_descriptors;
extern FD *stdin;
extern FD *stdout;
extern FD *stderr;

extern void init_files();
extern FD *create_block_device(char *name, u8 page_size);
extern FD *create_char_device(char *name, u8 page_size);
extern u32 write_byte_stream(FD *file, byte b);
extern byte read_byte_stream(FD *file);
extern FD *create_device(char *name, u8 page_size, u8 type);
#endif