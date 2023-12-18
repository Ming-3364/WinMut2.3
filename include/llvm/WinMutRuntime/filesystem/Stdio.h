#ifndef LLVM_STDIO_H
#define LLVM_STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __APPLE__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _IO_MTSAFE_IO
typedef struct {
  int lock;
  int cnt;
  void *owner;
} _IO_lock_t;

#endif
#include <sys/param.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <stdio.h>
#else

/* The tag name of this struct is _IO_FILE to preserve historic
   C++ mangled names for functions taking FILE* arguments.
   That name should not be used in new code.  */
struct _IO_FILE {
  int _flags; /* High-order word is _IO_MAGIC; rest is flags. */

  /* The following pointers correspond to the C++ streambuf protocol. */
  char *_IO_read_ptr;   /* Current read pointer */
  char *_IO_read_end;   /* End of get area. */
  char *_IO_read_base;  /* Start of putback+get area. */
  char *_IO_write_base; /* Start of put area. */
  char *_IO_write_ptr;  /* Current put pointer. */
  char *_IO_write_end;  /* End of put area. */
  char *_IO_buf_base;   /* Start of reserve area. */
  char *_IO_buf_end;    /* End of reserve area. */

  /* The following fields are used to support backing up and undo. */
  char *_IO_save_base;   /* Pointer to start of non-current get area. */
  char *_IO_backup_base; /* Pointer to first valid character of backup area */
  char *_IO_save_end;    /* Pointer to end of non-current get area. */

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _flags2;
  __off_t _old_offset; /* This used to be _offset but it's too small.  */

  /* 1+column number of pbase(); 0 is unknown. */
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  _IO_lock_t *_lock;
#ifdef _IO_USE_OLD_IO_FILE
};

struct _IO_FILE_complete {
  struct _IO_FILE _file;
#endif
  __off64_t _offset;
  /* Wide character stream stuff.  */
  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  size_t __pad5;
  int _mode;
  /* Make sure we don't get into trouble again.  */
  char _unused2[15 * sizeof(int) - 4 * sizeof(void *) - sizeof(size_t)];
};
typedef struct _IO_FILE FILE;
#define _USE_OWN_STDIO
#endif

#ifndef __APPLE__

typedef int64_t off64_t;

typedef void (*_IO_finish_t)(FILE *, int);

typedef int (*_IO_overflow_t)(FILE *, int);

typedef int (*_IO_underflow_t)(FILE *);

typedef int (*_IO_pbackfail_t)(FILE *, int);

typedef size_t (*_IO_xsputn_t)(FILE *FP, const void *DATA, size_t N);

typedef size_t (*_IO_xsgetn_t)(FILE *FP, void *DATA, size_t N);

typedef off64_t (*_IO_seekoff_t)(FILE *FP, off64_t OFF, int DIR, int MODE);

typedef off64_t (*_IO_seekpos_t)(FILE *, off64_t, int);

typedef FILE *(*_IO_setbuf_t)(FILE *, char *, ssize_t);

typedef int (*_IO_sync_t)(FILE *);

typedef int (*_IO_doallocate_t)(FILE *);

typedef ssize_t (*_IO_read_t)(FILE *, void *, ssize_t);

typedef ssize_t (*_IO_write_t)(FILE *, const void *, ssize_t);

typedef off64_t (*_IO_seek_t)(FILE *, off64_t, int);

typedef int (*_IO_close_t)(FILE *); /* finalize */

typedef int (*_IO_stat_t)(FILE *, void *);

typedef int (*_IO_showmanyc_t)(FILE *);

typedef void (*_IO_imbue_t)(FILE *, void *);

#endif

typedef int (*_unix_close_t)(int fd);

typedef ssize_t (*_unix_read_t)(int fd, void *buf, size_t size);

#ifndef __APPLE__
typedef off64_t (*_unix_seek_t)(int fd, off64_t offset, int whence);
#else
typedef fpos_t (*_unix_seek_t)(int fd, fpos_t offset, int whence);
#endif

typedef ssize_t (*_unix_write_t)(int fd, const void *buf, size_t size);

typedef int (*_unix_open_t)(const char *buf, int oflag, ...);

typedef int (*_unix_fcntl_t)(int fd, int flag, ...);

typedef int (*_unix_dup2_t)(int fd1, int fd2);

#ifndef __APPLE__
typedef int (*_unix_dup3_t)(int fd1, int fd2, int flags);
#endif

typedef int (*_unix_fstat_t)(int fd, struct stat *buf);

void inject_read(_unix_read_t func);

void inject_write(_unix_write_t func);

void inject_seek(_unix_seek_t func);

void inject_close(_unix_close_t func);

void inject_open(_unix_open_t func);

void inject_fcntl(_unix_fcntl_t func);

void inject_dup2(_unix_dup2_t func);

void reset_stdio();

#ifndef __APPLE__
void inject_dup3(_unix_dup3_t func);
#endif

void inject_fstat(_unix_fstat_t func);

#ifndef __APPLE__
void unload_stdio();
#endif

void init_stdio();

#ifdef __cplusplus
};
#endif

#endif // LLVM_STDIO_H
