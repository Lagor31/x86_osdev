#include "files.h"
#include "../lib/strings.h"
#include "../kernel/scheduler.h"

extern Thread *pick_next_thread();

u32 fd = 0;
List file_descriptors;

FD *stdin;
FD *stdout;
FD *stderr;

u32 set_pos_block(FD *file, u32 pos) {
  get_lock(file->lock);
  if (pos >= file->size) {
    unlock(file->lock);
    return -1;
  }
  file->pos = pos;
  unlock(file->lock);
  return 1;
}

u32 write_byte_block(FD *file, byte b) {
  get_lock(file->lock);
  file->buffer[file->pos] = b;
  unlock(file->lock);
  return 1;
}

byte read_byte_block(FD *file) {
  get_lock(file->lock);
  get_lock(file->read_lock);
  char out = file->buffer[file->pos];
  unlock(file->read_lock);
  unlock(file->lock);
  return out;
}

void init_files() {
  LIST_INIT(&file_descriptors);
  stdin = create_char_device("stdin", 5);
  stdout = create_block_device("stdout", 2);
  stderr = create_block_device("stderr", 2);
}

FD *create_block_device(char *name, u8 page_size) {
  return create_device(name, page_size, DEV_BLOCK);
}

FD *create_char_device(char *name, u8 page_size) {
  return create_device(name, page_size, DEV_STREAM);
}

u32 write_byte_stream(FD *file, byte b) {
  get_lock(file->lock);

  append((char *)file->buffer, (char)b);
  file->available++;
  unlock(file->read_lock);
  unlock(file->lock);

  return 1;
}

byte read_byte_stream(FD *file) {
get_l:
  get_lock(file->read_lock);

  if (test_lock(file->lock) == LOCK_LOCKED) {
    file->read_lock->state = LOCK_FREE;
    reschedule();
    goto get_l;
  }
  /*
  We arrive here with stuff to read and with the global lock acquired
  */
  char out = file->buffer[file->last++];
  file->available--;
  if (file->available <= 0) {
    memset((byte *)file->buffer, '\0', PAGE_SIZE * 2);
    file->last = 0;
  } else {
    /*
    Still stuff to read? We wake up the others waiting on the read lock
    */
    unlock(file->read_lock);
  }
  // Always unlock the global lock
  unlock(file->lock);
  return out;
}

void copy_fd(FD *s, FD *d) {
  d->name = s->name;
  d->buffer = s->buffer;
  d->fd = s->fd;
  d->last = s->last;
  d->lock = s->lock;
  d->pos = s->pos;
  d->read = s->read;
  d->read_lock = s->read_lock;
  d->seek = s->seek;
  d->size = s->size;
  d->type = s->type;
  d->available = s->available;
  d->write = s->write;
}

FD *create_device(char *name, u8 page_size, u8 type) {
  FD *f = kernel_page_alloc(0);

  char *new_name = kernel_page_alloc(0);
  u32 name_length = strlen(name);
  memcopy((byte *)name, (byte *)new_name, name_length);
  new_name[name_length] = '\0';

  f->name = new_name;
  f->fd = fd++;
  f->buffer = (byte *)kernel_page_alloc(page_size);
  memset((byte *)f->buffer, 0, PAGE_SIZE << page_size);

  if (type == DEV_STREAM)
    f->available = 0;
  else
    f->available = PAGE_SIZE << page_size;

  f->last = 0;
  f->lock = make_lock();

  // Read is free when there's stuff to read
  f->read_lock = make_lock();
  f->read_lock->state = LOCK_LOCKED;

  f->lock->state = LOCK_FREE;
  f->type = type;
  f->size = PAGE_SIZE << page_size;

  f->write = NULL;
  f->read = NULL;
  f->seek = NULL;

  LIST_INIT(&f->kfdq);
  LIST_INIT(&f->q);

  list_add_head(&file_descriptors, &f->kfdq);

  return f;
}