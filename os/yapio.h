/*************************************************************************
 *									 *
 *	 YAP Prolog 	%W% %G%
 *									 *
 *	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
 *									 *
 * Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-2003	 *
 *									 *
 **************************************************************************
 *									 *
 * File:		yapio.h * Last
 *rev:	22/1/03							 * mods:
 ** comments:	Input/Output information				 *
 *									 *
 *************************************************************************/

#ifndef YAPIO_H

#define YAPIO_H 1

#ifdef SIMICS
#undef HAVE_LIBREADLINE
#endif

#include <stdio.h>
#include <wchar.h>


#include "YapIOConfig.h"

#include "YapUTF8.h"

#include <VFS.h>

#include <Yatom.h>

#include "YapStreams.h"

#define WRITE_DEFS()                                                           \
  PAR("module", isatom, WRITE_MODULE)                                          \
  , PAR("attributes", isatom, WRITE_ATTRIBUTES),                               \
      PAR("cycles", booleanFlag, WRITE_CYCLES),                                \
      PAR("quoted", booleanFlag, WRITE_QUOTED),                                \
      PAR("ignore_ops", booleanFlag, WRITE_IGNORE_OPS),                        \
      PAR("max_depth", nat, WRITE_MAX_DEPTH),                                  \
      PAR("numbervars", booleanFlag, WRITE_NUMBERVARS),                        \
      PAR("name_variables", booleanFlag, WRITE_NAME_VARIABLES),                        \
      PAR("portrayed", booleanFlag, WRITE_PORTRAYED),                          \
      PAR("portray", booleanFlag, WRITE_PORTRAY),                              \
      PAR("priority", nat, WRITE_PRIORITY),                                    \
      PAR("character_escapes", booleanFlag, WRITE_CHARACTER_ESCAPES),          \
      PAR("backquotes", booleanFlag, WRITE_BACKQUOTES),                        \
      PAR("brace_terms", booleanFlag, WRITE_BRACE_TERMS),                      \
      PAR("conjunction", booleanFlag, WRITE_CONJUNCTION),                      \
      PAR("fullstop", booleanFlag, WRITE_FULLSTOP),                            \
      PAR("nl", booleanFlag, WRITE_NL),                                        \
      PAR("variable_names", ok, WRITE_VARIABLE_NAMES),                         \
      PAR(NULL, ok, WRITE_END)
#define PAR(x, y, z) z
typedef enum write_enum_choices { WRITE_DEFS() } write_choices_t;


#ifdef BEAM
int beam_write(USES_REGS1) {
  Yap_StartSlots();
  Yap_plwrite(ARG1, GLOBAL_Stream + LOCAL_c_output_stream, LOCAL_max_depth, 0,
              NULL);
  Yap_CloseSlots();
  Yap_RaiseException();
  return (TRUE);
}
#endif

#ifndef _PL_WRITE_

#define EOFCHAR EOF

#endif

/** an alias as stored in the Aliases vector. */
typedef struct AliasDescS {
  Atom name;
  int alias_stream;
} * AliasDesc;

#define MAX_ISO_LATIN1 255
/**
 *
 * @return a new VFS that will support /assets
 */

extern struct vfs *Yap_InitAssetManager(void);

/* routines in parser.c */
extern VarEntry *Yap_LookupVar(const char *,int,int);
extern Term Yap_VarNames(VarEntry *, Term);
extern Term Yap_Variables(VarEntry *, Term);
extern Term Yap_Singletons(VarEntry *, bool, Term);

/* routines in scanner.c */
extern void Yap_clean_tokenizer(void);
extern char *Yap_AllocScannerMemory(unsigned int);

/* routines in iopreds.c */
extern FILE *Yap_FileDescriptorFromStream(Term);
extern Int Yap_FirstLineInParse(void);
extern int Yap_CheckIOStream(Term, char *);
#if defined(YAPOR) || defined(THREADS)
extern void Yap_LockStream(void *);
extern void Yap_UnLockStream(void *);
#else
#define Yap_LockStream(X)
#define Yap_UnLockStream(X)
#endif
extern Int Yap_GetStreamFd(int);
extern void Yap_CloseStreams(void);
extern void Yap_CloseTemporaryStreams(int minstream);
extern int Yap_FirstFreeStreamD();
extern void Yap_FlushStreams(void);
extern void Yap_ReleaseStream(int);
extern int Yap_PlGetchar(void);
extern int Yap_PlGetWchar(void);
extern int Yap_PlFGetchar(void);
extern int Yap_GetCharForSIGINT(void);
extern Int Yap_StreamToFileNo(Term);
int Yap_OpenStream(Term tin, const char* io_mode, YAP_Term user_name, encoding_t enc);
extern int Yap_FileStream(FILE *, Atom, Term, estream_f, VFS_t *);
extern char *Yap_TermToBuffer(Term t, int flags);
extern char *Yap_HandleToString(yhandle_t l, size_t sz, size_t *length,
                                encoding_t *encoding, int flags);
extern int Yap_GetFreeStreamD(void);
extern int Yap_GetFreeStreamDForReading(void);

extern Term Yap_BufferToTerm(const char *s, Term opts);
extern Term Yap_UBufferToTerm(const unsigned char *s, Term opts);

extern Term Yap_WStringToList(wchar_t *);
extern Term Yap_WStringToListOfAtoms(wchar_t *);
extern Atom Yap_LookupWideAtom(const wchar_t *);


typedef enum mem_buf_source {
  MEM_BUF_MALLOC = 1,
  MEM_BUF_USER = 2
} memBufSource;

extern char *Yap_MemStreamBuf(int sno);

extern char *Yap_StrPrefix(const char *buf, size_t n);

extern Term Yap_StringToNumberTerm(const char *s, encoding_t *encp,
                                   bool error_on);
extern int Yap_FormatFloat(Float f, char **s, size_t sz);
extern int Yap_open_buf_read_stream(void *st, const char *buf, size_t nchars,
                                    encoding_t *encp, memBufSource src,
                                    Atom name, Term uname);
extern int Yap_open_buf_write_stream(int sno, encoding_t enc);
extern Term Yap_BufferToTerm(const char *s, Term opts);

extern X_API Term Yap_BufferToTermWithPrioBindings(const char *s, Term opts,
                                                   Term bindings, size_t sz,
                                                   int prio);
extern FILE *Yap_GetInputStream(Term t, const char *m);
extern FILE *Yap_GetOutputStream(Term t, const char *m);
extern FILE *Yap_GetBinaryOutputStream(Term t, const char *m);
extern Atom Yap_guessFileName( int sno, Atom n, Term un, size_t max);

extern int Yap_CheckSocketStream(Term stream, const char *error);
extern void Yap_init_socks(char *host, long interface_port);

extern bool Yap_flush(int sno);

extern uint64_t HashFunction(const unsigned char *);
extern uint64_t WideHashFunction(wchar_t *);

extern void Yap_InitAbsfPreds(void);

inline static Term MkCharTerm(Int c) {
  CACHE_REGS
  unsigned char cs[8];
  if (c==EOF)
    return TermEof;
  size_t n = put_xutf8(cs, c);
  if (n<0) n = 0;
  cs[n] =  0;
  return MkAtomTerm(Yap_ULookupAtom(cs));
}

extern char *GLOBAL_cwd;

extern char *Yap_VF(const char *path);

extern char *Yap_VFAlloc(const char *path);

/// UT when yap started
extern uint64_t Yap_StartOfWTimes;

extern bool Yap_HandleSIGINT(void);

extern void Yap_plwrite(Term, struct stream_desc *, CELL *, yhandle_t, write_flag_t, xarg *);


#endif
