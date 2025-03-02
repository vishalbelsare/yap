/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		readline.c *
* Last rev:	5/2/88							 *
* mods:									 *
* comments:	Input/Output C implemented predicates			 *
*									 *
*************************************************************************/

/**
 * @file   console.c
 * @author VITOR SANTOS COSTA <vsc@VITORs-MBP.lan>
 * @date   Wed Jan 20 00:56:23 2016
 *
 * @brief
 *
 *
 */
/**
 * @defgroup console Support for console-based interaction.
 * @ingroup InputOutput

 * This file includes the interface to the console IO, tty style. Refer also
 * to
 * the readline library.
 @{
 */

#include "sysbits.h"

static Int prompt(USES_REGS1);
static Int prompt1(USES_REGS1);



bool Yap_DoPrompt(StreamDesc *s) {
  CACHE_REGS
  if (LOCAL_newline) {
    return true;
  }
  return false;
}


/* check if we read a newline or an EOF */
int console_post_process_read_char(int ch, StreamDesc *s) {
  CACHE_REGS
  /* the character is also going to be output by the console handler */
  console_count_output_char(ch, GLOBAL_Stream + LOCAL_c_error_stream);
  return post_process_read_char( ch, s);
}

bool is_same_tty(FILE *f1, FILE *f2) {
#if HAVE_TTYNAME
  char buf1[MAX_PATH],  buf2[MAX_PATH];
  return ttyname_r(fileno(f1), buf1, MAX_PATH - 1) ==
         ttyname_r(fileno(f2), buf2, MAX_PATH - 1);
#endif
  // assume a single console, for now
  return true;
}

static Int is_same_tty2(USES_REGS1) { /* 'prompt(Atom)                 */
  int sni = Yap_CheckStream(ARG1, Input_Stream_f, "put/2");
  int sno = Yap_CheckStream(ARG2, Output_Stream_f, "put/2");
  bool out = (GLOBAL_Stream[sni].status & Tty_Stream_f) &&
             (GLOBAL_Stream[sno].status & Tty_Stream_f) &&
             is_same_tty(GLOBAL_Stream[sno].file, GLOBAL_Stream[sni].file);
  return out;
}

void Yap_ConsoleOps(StreamDesc *s) {
  /* the putc routine only has to check it is putting out a newline */
  s->stream_putc = ConsolePutc;
  s->stream_getc = ConsoleGetc;
#if USE_READLINE
  /* if a tty have a special routine to call readline */
  if ((s->status & Readline_Stream_f) && trueGlobalPrologFlag(READLINE_FLAG)) {
    if (Yap_ReadlineOps(s))
      return;
  }
#endif
}

/* static */
 int ConsolePutc(int sno, int ch) {
  CACHE_REGS
  StreamDesc *s = &GLOBAL_Stream[sno];
  if (ch == 10) {
#if MAC || _WIN32
    fputs("\n", s->file);
#else
    putc('\n', s->file);
#endif
    LOCAL_newline = true;
  } else
    putc(ch, s->file);
#if MAC || _WIN32
  fflush( NULL );
#endif
  console_count_output_char(ch, s);
  return ((int)ch);
}

/* send a prompt, and use the system for internal buffering. Speed is
   not of the essence here !!! */
int ConsoleGetc(int sno) {
  CACHE_REGS
  register StreamDesc *s = &GLOBAL_Stream[sno];
  int ch;

  /*§ keep the prompt around, just in case, but don't actually
     show it in silent mode */
  if (Yap_DoPrompt(s)) {
    if (!silentMode()) {
      char *cptr = LOCAL_Prompt, ch;
    Yap_clearInput(LOCAL_c_error_stream);
    strncpy(LOCAL_Prompt, (char *)RepAtom(LOCAL_AtPrompt)->StrOfAE, MAX_PROMPT);
    /* use the default routine */
      while ((ch = *cptr++) != '\0') {
        GLOBAL_Stream[StdErrStream].stream_putc(StdErrStream, ch);
      }
    }
  }
#if 0
#if HAVE_SIGINTERRUPT
  siginterrupt(SIGINT, TRUE);
#endif
  LOCAL_PrologMode |= ConsoleGetcMode;
  ch = fgetc(s->file);
#if HAVE_SIGINTERRUPT
  siginterrupt(SIGINT, FALSE);
#endif
  if (LOCAL_PrologMode & InterruptMode) {
    Yap_external_signal(0, YAP_INT_SIGNAL);
    LOCAL_PrologMode &= ~ConsoleGetcMode;
    LOCAL_newline = TRUE;
    if (LOCAL_PrologMode & AbortMode) {
      Yap_Error(ABORT_EVENT, TermNil, "");
      LOCAL_ErrorMessage = "Abort";
      return EOF;
    }
    goto restart;
  } else {
    LOCAL_PrologMode &= ~ConsoleGetcMode;
  }
#else
  LOCAL_PrologMode |= ConsoleGetcMode;
  ch = fgetc(s->file);
    LOCAL_PrologMode |= ConsoleGetcMode;
#endif
    LOCAL_PrologMode &= ~ InterruptMode;
    return console_post_process_read_char(ch, s);
}

/** @pred prompt1(+ _A__)


Changes YAP input prompt for the .


*/

static Int prompt1(USES_REGS1) { /* prompt1(Atom)                 */
  Term t = Deref(ARG1);
  Atom a;
  if (IsVarTerm(t) || !IsAtomTerm(t))
    return (FALSE);
  LOCAL_AtPrompt = a = AtomOfTerm(t);
  if (strlen((char *)RepAtom(a)->StrOfAE) > MAX_PROMPT) {
    Yap_Error(SYSTEM_ERROR_INTERNAL, t, "prompt %s is too long",
              RepAtom(a)->StrOfAE);
    return (FALSE);
  }
  strncpy(LOCAL_Prompt, (char *)RepAtom(a)->StrOfAE, MAX_PROMPT);
  return (TRUE);
}

/** @pred prompt(- _A_,+ _B_)

Changes YAP input prompt from  _A_ to  _B_, active on *next* standard input
interaction.

*/
static Int prompt(USES_REGS1) { /* prompt(Old,New)       */
  Term t = Deref(ARG2);
  Atom a;
  if (!Yap_unify_constant(ARG1, MkAtomTerm(LOCAL_AtPrompt)))
    return (FALSE);
  if (IsVarTerm(t) || !IsAtomTerm(t))
    return (FALSE);
  a = AtomOfTerm(t);
  if (strlen(RepAtom(a)->StrOfAE) > MAX_PROMPT) {
    Yap_Error(SYSTEM_ERROR_INTERNAL, t, "prompt %s is too long",
              RepAtom(a)->StrOfAE);
    return false;
  }
  strncpy(LOCAL_Prompt, (char *)RepAtom(LOCAL_AtPrompt)->StrOfAE, MAX_PROMPT);
  LOCAL_AtPrompt = a;
  return (TRUE);
}

/** @pred ensure_prompting

Make sure we have a prompt at this point, even if we have to
introduce a new line.

*/
static Int ensure_prompting(USES_REGS1) /* prompt(Old,New)       */
  {
    if(GLOBAL_Stream[2].status &  Past_Eof_Stream_f) {
    return false;
   } else if (!LOCAL_newline) {
      GLOBAL_Stream[2].stream_wputc(2, 10); // hack!
    }
  return true;
}

void Yap_InitConsole(void) {
  CACHE_REGS

    LOCAL_newline = true;
     Yap_InitCPred("prompt", 1, prompt1, SafePredFlag | SyncPredFlag);
  Yap_InitCPred("prompt1", 1, prompt1, SafePredFlag | SyncPredFlag);
  Yap_InitCPred("$is_same_tty", 2, is_same_tty2,
                SafePredFlag | SyncPredFlag | HiddenPredFlag);
  Yap_InitCPred("prompt", 2, prompt, SafePredFlag | SyncPredFlag);
  Yap_InitCPred("$ensure_prompting", 0, ensure_prompting,
                SafePredFlag | SyncPredFlag);
}

/// @}
