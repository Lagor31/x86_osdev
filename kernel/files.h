#ifndef FILES_H
#define FILES_H

#include "../lib/list.h"

#include "../cpu/types.h"
#include "../lock/lock.h"

#include "../proc/thread.h"
#include "../lock/lock.h"
#include "../mem/mem.h"
#include "../kernel/kernel.h"

#define DEV_STREAM 0
#define DEV_BLOCK 1

typedef struct FD {
  u8 type;
  u32 fd;
  char *name;
  Lock *lock;
  Lock *read_lock;
  byte *buffer;
  u32 size;
  u32 pos;
  u32 last;
  u32 available;
  List q;
  List kfdq;
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
extern u32 write_byte_block(FD *file, byte b);
extern u32 set_pos_block(FD *file, u32 pos);
extern byte read_byte_stream(FD *file);
extern byte read_byte_block(FD *file);
extern void copy_fd(FD *s, FD *d);
//extern u32 set_pos_block_nb(FD *file, u32 pos);
extern FD *create_device(char *name, u8 page_size, u8 type);
extern void init_files();
#endif