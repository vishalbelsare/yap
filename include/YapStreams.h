/**************************************************************************
 *									 *
 * File:		iopreds.h *
 * Last rev:	5/2/88							 *
 * mods: *
 * comments:	Input/Output C implemented predicates			 *
 *									 *
 *************************************************************************/

#ifndef YAPSTREAMS_H
#define YAPSTREAMS_H 1

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if THREADS
#include <locks_pthread.h>
#endif


#define YAP_ERROR NIL

#define MaxStreams 256

#define EXPAND_FILENAME 0x000080

#define StdInStream 0
#define StdOutStream 1
#define StdErrStream 2

#define ALIASES_BLOCK_SIZE 8

#if _WIN32
#ifndef USE_SOCKET
#define USE_SOCKET 1
#endif
#define HAVE_SOCKET 1
#endif

//#include "Atoms.h"
//#include "Yap.h"
#include <stdlib.h>

/*
 * This file defines main data-structure for stream management,
 *
 */

#if defined(_MSC_VER) || defined(__MINGW32__)

#include <windows.h>

#endif

#include <wchar.h>

/************ SWI compatible support for unicode representations  ************/
typedef struct yap_io_position {
  int64_t byteno;       /* byte-position in file */
  int64_t charno;       /* character position in file */
  long int lineno;      /* lineno in file */
  long int linepos;     /* position in line */
  intptr_t reserved[2]; /* future extensions */
} yapIOPOS;

typedef struct yapchlookahead {
    intptr_t charcount;       /* character position in file */
    intptr_t linecount;      /* lineno in file */
    intptr_t linepos;     /* position in line */
    intptr_t ch; /* future extensions */
    struct yapchlookahead *next;
} yapStreamLookahead;

extern int PopCode(int sno);

#ifndef _PL_STREAM_H
typedef struct {
  YAP_Atom file;     /* current source file */
  yapIOPOS position; /* Line, line pos, char and byte */
} yapSourceLocation;
#endif

#define RD_MAGIC 0xefebe128

typedef struct vlist_struct_t {
  struct VARSTRUCT *ve;
  struct vlist_struct_t *next;
} vlist_t;

typedef struct qq_struct_t {
  unsigned char *text;
  yapIOPOS start, mid, end;
  vlist_t *vlist;
  struct qq_struct_t *next;
} qq_t;

typedef struct read_data_t {
  unsigned char *here;        /* current character */
  unsigned char *base;        /* base of clause */
  unsigned char *end;         /* end of the clause */
  unsigned char *token_start; /* start of most recent read token */

  int magic; /* RD_MAGIC */
  struct stream_desc *stream;
  FILE *f;           /* file. of known */
  YAP_Term position; /* Line, line pos, char and byte */
  void *posp;        /* position pointer */
  size_t posi;       /* position number */

  YAP_Term subtpos;                /* Report Subterm positions */
  bool cycles;                     /* Re-establish cycles */
  yapSourceLocation start_of_term; /* Position of start of term */
  struct mod_entry *module;        /* Current source module */
  unsigned int flags;              /* Module syntax flags */
  int styleCheck;                  /* style-checking mask */
  bool backquoted_string;          /* Read `hello` as string */

  int *char_conversion_table; /* active conversion table */

  YAP_Atom on_error; /* Handling of syntax errors */
  int has_exception; /* exception is raised */

  YAP_Term exception; /* raised exception */
  YAP_Term variables; /* report variables */
  YAP_Term singles;   /* Report singleton variables */
  YAP_Term varnames;  /* Report variables+names */
  int strictness;     /* Strictness level */

#ifdef O_QUASIQUOTATIONS
  YAP_Term quasi_quotations; /* User option quasi_quotations(QQ) */
  YAP_Term qq;               /* Quasi quoted list */
  YAP_Term qq_tail;          /* Tail of the quoted stuff */
#endif

  YAP_Term comments; /* Report comments */

} read_data, *ReadData;


#if __ANDROID__
//extern FILE * fmemopen(void *buf, size_t size, const char *mode);
#define HAVE_FMEMOPEN 1
#define HAVE_OPEN_MEMSTREAM 1
#endif

#if HAVE_FMEMOPEN
#define MAY_READ 1
#endif

#if HAVE_OPEN_MEMSTREAM
#define MAY_READ 1
#define MAY_WRITE 1
#endif

#if _WIN32
#undef MAY_WRITE
#undef MAY_READ
#endif

typedef struct mem_desc {
  char *buf;        /* where the file is being read from/written to */
  int src;          /* where the space comes from, 0 code space, 1 malloc */
  YAP_Int max_size; /* maximum buffer size (may be changed dynamically) */
  YAP_UInt pos;     /* cursor */
  volatile void *error_handler;
} memHandle;

#if HAVE_SOCKET
typedef enum { /* in YAP, sockets may be in one of 4 possible status */
               new_socket,
               server_socket,
               client_socket,
               server_session_socket,
               closed_socket
} socket_info;

typedef enum { /* we accept two domains for the moment, IPV6 may follow */
               af_inet, /* IPV4 */
               af_unix  /* or AF_FILE */
} socket_domain;

#endif

typedef struct stream_desc {
  YAP_Atom name;
  YAP_Term user_name;
  FILE *file;
  // useful in memory streams
  char *nbuf;
  size_t nsize;
  struct {
    struct {
#define PLGETC_BUF_SIZE 4096
      unsigned char *buf, *ptr;
      int left;
    } file;
    memHandle mem_string;
    struct {
      int fd;
    } pipe;
#if HAVE_SOCKET
    struct {
      socket_domain domain;
      socket_info flags;
      int fd;
    } socket;
#endif
     struct {
      const unsigned char *buf, *ptr;
    } irl;
     struct {
       unsigned char *buf, *ptr;
    } w_irl;
    void *private_data;
  } u;
    struct {
        bool on;
        int ch;
        intptr_t pos, line, lpos;
    } buf;

    int charcount, linecount, linestart;
      int ocharcount, olinecount, olinestart;
  stream_flags_t status;
#if defined(YAPOR) || defined(THREADS)
  lockvar streamlock; /* protect stream access */
#endif
  int (*stream_putc)(
      int, int); /** function the stream uses for writing a single octet */
  int (*stream_wputc)(
      int, wchar_t); /** function the stream uses for writing a character */
  int (*stream_getc)(int); /** function the stream uses for reading an octet. */
  int (*stream_wgetc)(
      int);         /** function the stream uses for reading a character. */
  struct vfs *vfs;  /** stream belongs to a space */
  void *vfs_handle; /** direct handle to stream in that space. */
    int (*stream_wgetc_for_read)(int);   /** function the stream uses for parser. It may be different
                          from above if the ISO  character conversion is on */
    int (*stream_peek)(int);   /** check if the next character is available. */
    int (*stream_wpeek)(int);   /**  check if the next wide character is available. */
  encoding_t encoding; /** current encoding for stream */
} StreamDesc;



extern bool Yap_set_stream_to_buf(StreamDesc *st, const char *bufi,
                                  size_t nchars
                                  #ifdef USES_REGS
                                   USES_REGS
                                   #endif
                                 );

#endif
