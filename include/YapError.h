/*************************************************************************
 *									 *
 *	 YAP Prolog 	%W% %G% 					 *
 *	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
 *									 *
 * Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
 *									 *
 **************************************************************************
 *									 *
 * File:		YapError.h * mods:
 ** comments:	error header file for YAP				 *
 * version:      $Id: Yap.h,v 1.38 2008-06-18 10:02:27 vsc Exp $	 *
 *************************************************************************/

///
/// @file YapError.h
///
/// @brief The file  YapErrors.h defines the internal error handling API.
///
/// @defgroup ErrorC C API/Implementation of error handling
/// @ingroup YAPErrors
///
/// @{
///

#ifndef YAP_ERROR_H
#define YAP_ERROR_H 1

#define ECLASS(CL, A, B) CL,

#define E0(A, B, C) A,
#define E(A, B, C) A,
#define E1(A, B, C) A,
#define ES(A, B, C) A,
#define E2(A, B, C, D) A,

#define BEGIN_ERRORS() typedef enum {

#define END_ERRORS()                                                           \
  }                                                                            \
  yap_error_number

#define BEGIN_ERROR_CLASSES() typedef enum {

#define END_ERROR_CLASSES()                                                    \
  }                                                                            \
  yap_error_class_number

#include "YapErrors.h"

#define MAX_ERROR_MSG_SIZE 1024

extern void Yap_InitError__(const char *file, const char *function, int lineno,
                            yap_error_number e, YAP_Term g, const char *msg, ...);
extern struct yami *Yap_Error__(bool thrw, const char *file,
                                const char *function, int lineno,
                                yap_error_number err, YAP_Term wheret,const char *fmt, ...);

extern void Yap_do_warning__( const char *file,
                                const char *function, int lineno, 
                               yap_error_number err, YAP_Term wheret, const char *fmt, ...);

extern void Yap_ThrowError__(const char *file, const char *function, int lineno,
                             yap_error_number err, YAP_Term wheret, const char *msg, ...)
    ;

#define Yap_NilError(id, ...)                                                  \
Yap_Error__(false, __FILE__, __FUNCTION__, __LINE__, id, TermNil, __VA_ARGS__)
#define Yap_InitError(id, t,...)					\
  Yap_InitError__(__FILE__, __FUNCTION__, __LINE__, id, t, __VA_ARGS__)

#define Yap_Error(id, inp, ...)                                                \
  Yap_Error__(false, __FILE__, __FUNCTION__, __LINE__, id, inp, __VA_ARGS__)

#define Yap_ThrowError(id, inp, ...)					\
  Yap_ThrowError__(__FILE__, __FUNCTION__, __LINE__, id, inp, __VA_ARGS__)


#define Yap_syntax_error(sno, tokptr, toktide, msg, ...)			\
  Yap_syntax_error__(__FILE__, __FUNCTION__, __LINE__,sno,tokptr, toktide,msg, __VA_ARGS__ )



#define Yap_do_warning(id, inp,  ...)					\
  Yap_do_warning__(__FILE__, __FUNCTION__, __LINE__, id, inp,  __VA_ARGS__)

#ifdef YAP_TERM_H
/**
 * make sure next argument is a bound instance of type
 * atom.
 */
#define Yap_ensure_atom(T0, TF)                                                                 \
  { if ( (TF = Yap_ensure_atom__(__FILE__, __FUNCTION__, __LINE__, T0  )  == 0L ) return false; \
  }

//INLINE_ONLY 
 static YAP_Term Yap_ensure_atom__(const char *fu, const char *fi, int line,
                                   YAP_Term in) {
  YAP_Term t = Deref(in);
  // YAP_Term Context = Deref(ARG2);
  if (!IsVarTerm(t) && IsAtomTerm(t))
    return t;
  if (IsVarTerm(t)) {
    Yap_Error__(false, fu, fi, line, INSTANTIATION_ERROR, t, NULL);
  } else {
    if (IsAtomTerm(t))
      return t;
    Yap_Error__(false, fu, fi, line, TYPE_ERROR_ATOM, t, NULL);
    return 0L;
  }

#endif

#define JMP_LOCAL_ERROR(v, LAB)                                                \
  if (H + 2 * (v) > ASP - 1024) {                                              \
    LOCAL_Error_TYPE = RESOURCE_ERROR_STACK;                                   \
    LOCAL_Error_Size = 2 * (v) * sizeof(CELL);                                 \
    goto LAB;                                                                  \
  }

#define LOCAL_ERROR(t, v)                                                      \
  if (HR + (v) > ASP - 1024) {                                                 \
    LOCAL_Error_TYPE = RESOURCE_ERROR_STACK;                                   \
    LOCAL_Error_Size = 2 * (v) * sizeof(CELL);                                 \
    return NULL;                                                               \
  }

#define LOCAL_TERM_ERROR(t, v)                                                 \
  if (HR + (v) > ASP - 1024) {                                                 \
    LOCAL_Error_TYPE = RESOURCE_ERROR_STACK;                          \
    LOCAL_Error_Size = 2 * (v) * sizeof(CELL);                                 \
    return 0L;                                                                 \
  }

#define AUX_ERROR(t, n, s, TYPE)                                               \
  if (s + (n + 1) > (TYPE *)AuxSp) {                                           \
    LOCAL_Error_TYPE = RESOURCE_ERROR_AUXILIARY_STACK;                         \
    LOCAL_Error_Size = n * sizeof(TYPE);                                       \
    return NULL;                                                               \
  }

#define AUX_TERM_ERROR(t, n, s, TYPE)                                          \
  if (s + (n + 1) > (TYPE *)AuxSp) {                                           \
    LOCAL_Error_TYPE = RESOURCE_ERROR_AUXILIARY_STACK;                         \
    LOCAL_Error_Size = n * sizeof(TYPE);                                       \
    return 0L;                                                                 \
  }

#define JMP_AUX_ERROR(n, s, t, TYPE, LAB)                                      \
  if (s + (n + 1) > (TYPE *)AuxSp) {                                           \
    LOCAL_Error_TYPE = RESOURCE_ERROR_AUXILIARY_STACK;                         \
    LOCAL_Error_Size = n * sizeof(TYPE);                                       \
    goto LAB;                                                                  \
  }

#define HEAP_ERROR(a, TYPE)                                                    \
  if (a == NIL) {                                                              \
    LOCAL_Error_TYPE = RESOURCE_ERROR_HEAP;                                    \
    LOCAL_Error_Size = n * sizeof(TYPE);                                       \
    return NULL;                                                               \
  }

#define HEAP_TERM_ERROR(a, TYPE, n)                                            \
  if (a == NIL) {                                                              \
    LOCAL_Error_TYPE = RESOURCE_ERROR_HEAP;                                    \
    LOCAL_Error_Size = n * sizeof(TYPE);                                       \
    return 0L;                                                                 \
  }

#define JMP_HEAP_ERROR(a, n, t, TYPE, LAB)                                     \
  if (a == NIL) {                                                              \
    LOCAL_Error_TYPE = RESOURCE_ERROR_HEAP;                                    \
    LOCAL_Error_Size = n * sizeof(TYPE);                                       \
    goto LAB;                                                                  \
  }

  /**
   * Error stages since it was initially processed.
   *
   * Notice that some of the stages may be active simultaneouly.
   */
  typedef enum yap_error_status {
    /// where we like to be
    YAP_NO_ERROR_STATUS = 0x0,
    /// Prolog discovered the problem
    YAP_ERROR_INITIATED_IN_PROLOG = 0x1,
    /// The problem was found before the system can cope
    YAP_ERROR_INITIATED_IN_BOOT = 0x2,
    /// C-helper like must_ found out the problem
    YAP_ERROR_INITIATED_IN_HELPER = 0x4,
    /// C-builtin crashed
    YAP_ERROR_INITIATED_IN_SYSTEM_C = 0x8,
    /// user code crashed
    YAP_ERROR_INITIATED_IN_USER_C = 0x10,
    /// ok, we put a throw to be dispatched
    YAP_THROW_THROWN = 0x20,
    /// someone caught it
    YAP_THROW_CAUGHT = 0x40,
    /// error became an exception (usually SWIG bridge)
    YAP_ERROR_EXPORTED_TO_CXX = 0x80,
    /// handle error in Prolog
    YAP_ERROR_BEING_PROCESSED_IN_PROLOG = 0x100
    /// go back t
  } yap_error_stage_t;

  /// a Prolog goal that caused a bug

  typedef struct yap_error_prolog_source {
    intptr_t prologPredCl;
    uintptr_t prologPredLine;
    uintptr_t prologPredFirstLine;
    uintptr_t prologPredLastLine;
    const char *prologPredName;
    uintptr_t prologPredArity;
    const char *prologPredModule;
    const char *prologPredFile;
    struct error_prolog_source *errorParent;
  } yap_error_prolog_source_t;

  /// all we need to know about an error/throw
  typedef struct s_yap_error_descriptor {
    /// error identifier
    yap_error_number errorNo;
    /// kind of error: derived from errorNo;
    yap_error_class_number errorClass;
    ///  errorNo as text
    char *errorAsText;
    char *errorAsText2;
    ///  errorClass as text
    char *classAsText;
    /// c-code that generated the error
    /// C-line
    intptr_t errorLine;
    /// C-function
    const char *errorFunction;
    /// C-file
    const char *errorFile;
    // struct error_prolog_source *errorSource;
    /// Prolog predicate that caused the error: name
    const char *prologPredName;
    /// Prolog predicate that caused the error:arity
    uintptr_t prologPredArity;
    /// Prolog predicate that caused the error:module    
    const char *prologPredModule;
    /// Prolog predicate that caused the error:line    
    const char *prologPredFile;
    /// line where error clause defined
    uintptr_t prologPredLine;
    /// syntax and other parsing errors
    uintptr_t parserPos;
    uintptr_t parserFirstPos;
    uintptr_t parserFirstLinePos;
    uintptr_t parserLastPos;
    uintptr_t parserLastLinePos;
    uintptr_t parserLine;
    uintptr_t parserSize;
    uintptr_t parserLinePos;
    uintptr_t parserFirstLine;
    uintptr_t parserLastLine;
    const char *parserTextA;
    const char * parserTextB;
    const char *parserFile;
    /// reading a clause, or called from read?
    bool parserReadingCode;
    ///  whether we are consulting
    bool prologConsulting;
    const char * culprit;
    YAP_Term errorUserTerm, culprit_t;
    /// Prolog stack at the time
    const char *prologStack;
     char *errorMsg;
    size_t errorMsgLen;
    const char *currentGoal;
    const char *alternativeGoal;
    const char *continuationGoal;
    struct s_yap_error_descriptor *top_error;
  } yap_error_descriptor_t;

/// compatibility with existing code..
#define LOCAL_Error_TYPE LOCAL_ActiveError->errorNo
#define LOCAL_Error_File LOCAL_ActiveError->errorFile
#define LOCAL_Error_Function LOCAL_ActiveError->errorFunction
#define LOCAL_Error_Lineno LOCAL_ActiveError->errorLine
#define LOCAL_Error_Size LOCAL_ActiveError->errorMsgLen
#define LOCAL_UserTerm LOCAL_ActiveError->errorUserTerm
#define LOCAL_ErrorFullTerm LOCAL_ActiveError->FullErrorTerm
#define LOCAL_ErrorMessage LOCAL_ActiveError->errorMsg

  extern void Yap_CatchError(void);
  extern void Yap_ThrowExistingError(void);
  extern YAP_Term Yap_MkPrologError(YAP_Term t, yap_error_descriptor_t *wi);
  extern YAP_Term MkSysError(yap_error_descriptor_t * r);
  extern YAP_Term Yap_MkFullError(yap_error_descriptor_t * r, yap_error_number e);
  extern bool Yap_MkErrorRecord(
      yap_error_descriptor_t * r, const char *file, const char *function,
      int lineno, yap_error_number type, YAP_Term where, YAP_Term extra, const char *msg);

  extern yap_error_descriptor_t *Yap_pc_add_location(
      yap_error_descriptor_t * t, void *pc0, void *b_ptr0, void *env0);
  extern yap_error_descriptor_t *Yap_env_add_location(
      yap_error_descriptor_t * t, void *cp0, void *b_ptr0, void *env0,
      YAP_Int ignore_first);

  extern yap_error_descriptor_t *Yap_prolog_add_culprit(yap_error_descriptor_t *t);

  extern yap_error_class_number Yap_errorClassNumber(const char *c);
  extern char *Yap_errorName(yap_error_number e);
  extern char *Yap_errorName2(yap_error_number e);
  extern yap_error_class_number Yap_errorClass(yap_error_number e);
  extern char *Yap_errorClassName(yap_error_class_number e);
  extern   yap_error_number Yap_errorNumber(yap_error_class_number, const char * e, const char * e2) ;
  
  extern bool Yap_get_exception(void);

  extern YAP_Term Yap_UserError(YAP_Term t, yap_error_descriptor_t *i);

extern yap_error_descriptor_t *Yap_pushErrorContext(bool pass,
						    yap_error_descriptor_t *new_error, yap_error_descriptor_t *old);
 extern yap_error_descriptor_t *Yap_popErrorContext(bool oerr, bool pass, yap_error_descriptor_t *);

extern YAP_Term Yap_MkErrorYAP_Term(struct s_yap_error_descriptor *t);
extern bool Yap_Warning(const char *s, ...);
extern bool Yap_PrintWarning(YAP_Term t, YAP_Term level);
extern bool Yap_HandleError__(const char *file, const char *function, int lineno,
                       const char *s, ...);
#define Yap_HandleError(...)                                                   \
  Yap_HandleError__(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
extern int Yap_SWIHandleError(const char *, ...);
extern void Yap_InitErrorPreds(void);
extern bool Yap_callable(YAP_Term t);
 extern bool Yap_must_be_callable(YAP_Term t, YAP_Term mod);

#include "ScannerTypes.h"
     
 extern char *Yap_syntax_error__(const char *file, const char *function, int lineno, YAP_Term t, int sno, TokEntry *start,
                              TokEntry *err, char *s,  ...);

#endif

/// @}
