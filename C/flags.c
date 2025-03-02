/*************************************************************************
 *									 *
 *	 YAP Prolog 							 *
 *									 *
 *	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
 *									 *
 * Copyright L.Damas, V.S.Costa and Universidade do Porto 2015-		 *
 *									 *
 **************************************************************************
 *									 *
 * File:		flags.c *
 * Last rev:								 *
 * mods: *
 * comments:	abstract machine definitions				 *
 *									 *
 *************************************************************************/

/**
   @file C/flags.c
   @brief  Prolog parameter browsing and setting,
*/

/**
 * @defgroup YAPFlags Prolog Flags
 * @ingroup Builtins
 * @brief Flags can be used to configure and to query the Prolog engine.
 * @{
 * 
 */

// this is where  we define flags


#include "Yap.h"

#define INIT_FLAGS 1
#include "iopreds.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

 Term ro(Term inp);
 Term nat(Term inp);
 Term isatom(Term inp);
 Term booleanFlag(Term inp);
// static bool string( Term inp );
// static bool  list_atom( Term inp );
static Term list_option(Term inp);
static Term argv(Term inp);
static Term os_argv(Term inp);
static bool agc_threshold(Term inp);
static bool gc_margin(Term inp);
static Term executable(Term inp);
static Term sys_thread_id(Term inp);
static Term sys_pid(Term inp);
static bool mkprompt(Term inp);
static Term indexer(Term inp);
static Term stream(Term inp);
static bool getenc(Term inp);
static bool typein(Term inp);
static bool set_error_stream(Term inp);
static bool set_input_stream(Term inp);
static bool set_output_stream(Term inp);
static bool dollar_to_lc(Term inp);
static bool setSignals(Term inp);

static void newFlag(Term fl, Term val);
static Int set_prolog_flag(USES_REGS1);

#include "YapEval.h"
#include "Yatom.h"
#include "yapio.h"


static Term compiling(Term inp) {
  CACHE_REGS
  if (LOCAL_consult_level) return Yap_unify(inp,TermTrue);
  return Yap_unify(inp,TermFalse);
}

Term ro(Term inp) {
    if (IsVarTerm(inp)) {
        Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag: value must be %s",
                  "bound");
        return TermZERO;
    }
    Yap_ThrowError(PERMISSION_ERROR_READ_ONLY_FLAG, inp, "set_prolog_flag %s",
              "flag is read-only");
    return TermZERO;
}

Term aro(Term inp) {
    if (IsVarTerm(inp)) {
        Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag %s",
                  "value must be bound");
}
        return TermZERO;
    }


static Term mkplus(Term val)
{
  CACHE_REGS
  Term ts[2];
  ts[0] = LOCAL_flag;
  ts[1] = val;
  return Yap_MkApplTerm(FunctorPlus,2,ts);
    
}

Term booleanFlag(Term inp) {
    if (IsStringTerm(inp)) {
        inp =  MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
    }
    if (inp == TermTrue || inp == TermOn)
        return TermTrue;
    if (inp == TermFalse || inp == TermOff)
        return TermFalse;
    if (IsVarTerm(inp)) {
        Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag %s",
                  "value must be bound");
        ;
        return TermZERO;
    }
    if (IsAtomTerm(inp)) {
      Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(inp),
                  "set_prolog_flag in {true,false,on,off}");
        return TermZERO;
    }
    Yap_ThrowError(TYPE_ERROR_ATOM, inp, "set_prolog_flag in {true,false,on,off");
    return TermZERO;
}


 Term febooleanFlag(Term inp) {
    if (IsStringTerm(inp)) {
        inp = MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
    }
    if (inp == TermTrue || inp == TermError)
        return TermError;
    if (inp == TermFalse || inp == TermFail)
        return TermFail;
    if (IsVarTerm(inp)) {
        Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag %s",
                  "value must be bound");
        ;
        return TermZERO;
    }
    if (IsAtomTerm(inp)) {
      Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(inp),
                  "set_prolog_flag in {true,false,on,off}");
        return TermZERO;
    }
    Yap_ThrowError(TYPE_ERROR_ATOM, inp, "set_prolog_flag in {true,false,error,fail");
    return TermZERO;
}

static Term quote( mod_entry_flags_t  qt)
{
  switch(qt) {
  case SNGQ_ATOM:
    return TermAtom;
  case SNGQ_CODES:
    return  TermCodes;
  case SNGQ_CHARS:
    return  TermChars;
  case SNGQ_STRING:
    return TermString;
  case DBLQ_ATOM:
    return TermAtom;
  case DBLQ_CODES:
    return  TermCodes;
  case DBLQ_CHARS:
    return  TermChars;
  case DBLQ_STRING:
    return TermString;
  case BCKQ_ATOM:
    return TermAtom;
  case BCKQ_CODES:
    return  TermCodes;
  case BCKQ_CHARS:
    return  TermChars;
  case BCKQ_STRING:
    return TermString;
  default:
    return TermZERO;
  }
}

// used to overwrite singletons quoteFunc flag
static  Term snglq(Term val) {
   CACHE_REGS
    struct mod_entry *m = Yap_GetModuleEntry(CurrentModule);
  if (IsVarTerm(val)) {
    return Yap_unify(val,quote(m->flags&SNGQ_MASK));
  }
    if (IsStringTerm(val)) {
        val = MkAtomTerm(Yap_LookupAtom(StringOfTerm(val)));
    }
     if (val == TermAtom) {
    m->flags &= ~SNGQ_MASK;
       m->flags |= SNGQ_ATOM;
    } else if (val == TermCodes) {
   m->flags &= ~SNGQ_MASK;
        m->flags |= SNGQ_CODES;
    } else if (val == TermChars) { 
   m->flags &= ~SNGQ_MASK;
           m->flags |= SNGQ_CHARS;
            } else if (val == TermString) {
   m->flags &= ~SNGQ_MASK;
                m->flags |= SNGQ_STRING;
            } else {
                Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, val,
                               "valid flags for single_quotes prolog flag are one of atom, string, codes or chars");
            }
    GLOBAL_Flags[SINGLE_QUOTES_FLAG].at = val;
    return val;
}


// used to overwrite singletons quoteFunc flag
static inline Term dblq(Term val) {
  CACHE_REGS
  struct mod_entry *m = Yap_GetModuleEntry(CurrentModule);
  if (IsVarTerm(val)) {
    return Yap_unify(val,quote(m->flags&DBLQ_MASK));
  }
  if (IsStringTerm(val)) {
    val = MkAtomTerm(Yap_LookupAtom(StringOfTerm(val)));
  }
  if (val == TermAtom) {
    m->flags &= ~DBLQ_MASK;
    m->flags |= DBLQ_ATOM;
  } else if (val == TermCodes) {
      m->flags &= ~DBLQ_MASK;

      m->flags |= DBLQ_CODES;
  } else if (val == TermChars) {
  m->flags &= ~DBLQ_MASK;
    m->flags |= DBLQ_CHARS;
  } else if (val == TermString) {
  m->flags &= ~DBLQ_MASK;
    m->flags |= DBLQ_STRING;
  } else {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(val),
		   "valid flags for double_quotes prolog flag are one of atom, string, codes or chars");
  }
  GLOBAL_Flags[DOUBLE_QUOTES_FLAG].at = val;
  return val;
}


// used to overwrite singletons quoteFunc flag
static inline Term bckq(Term val) {
  CACHE_REGS
  struct mod_entry *m = Yap_GetModuleEntry(CurrentModule);
  if (IsVarTerm(val)) {
    return Yap_unify(val,quote(m->flags&BCKQ_MASK));
  }
  if (IsStringTerm(val)) {
    val = MkAtomTerm(Yap_LookupAtom(StringOfTerm(val)));
  }

  if (val == TermAtom) {
  m->flags &= ~BCKQ_MASK;
  m->flags |= BCKQ_ATOM;
  } else if (val == TermCodes) {
    m->flags &= ~BCKQ_MASK;
    m->flags |= BCKQ_CODES;
  } else if (val == TermChars) {
      m->flags &= ~BCKQ_MASK;
      m->flags |= BCKQ_CHARS;
  } else if (val == TermString) {
      m->flags &= ~BCKQ_MASK;
      m->flags |= BCKQ_STRING;
  } else {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(val),
		   "valid flags for back_quotes prolog flag are one of atom, string, codes or chars");
  }
  GLOBAL_Flags[BACK_QUOTES_FLAG].at = val;
  return val;
}

// used to overwrite singletons quoteFunc flag
static inline Term multil( Term val) {
  CACHE_REGS
  struct mod_entry *m = Yap_GetModuleEntry(CurrentModule); 
  if (IsVarTerm(val)) {
    return Yap_unify(val,m->flags& M_MULTILINE?TermTrue:TermFalse);
  }
 if (IsStringTerm(val)) {
    val = MkAtomTerm(Yap_LookupAtom(StringOfTerm(val)));
  }
  if (val == TermTrue || val == TermOn) {
    m->flags |= M_MULTILINE;
  } else if (val == TermFalse || val == TermOff) {
    m->flags &= ~M_MULTILINE;
  }  else {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(val),
		   "valid flags for multiline_quoted_text prolog flag are booleean");
  }
  GLOBAL_Flags[MULTILINE_QUOTED_TEXT_FLAG].at = val;
  return val;
}

// used to overwrite singletons quoteFunc flag
static Term undefph(Term val) {
  CACHE_REGS
  struct mod_entry *m = Yap_GetModuleEntry(CurrentModule);
  if (IsStringTerm(val)) {
    val = MkAtomTerm(Yap_LookupAtom(StringOfTerm(val)));
  }
  if (IsVarTerm(val)) {
    return Yap_unify(val,Yap_UnknownFlag(CurrentModule));
  }
  m->flags &= ~UNKNOWN_MASK;
  if (val == TermError) {
    m->flags |= UNKNOWN_ERROR;
  } else if (val == TermFail) {
    m->flags |= UNKNOWN_FAIL;
  } else if (val == TermFastFail) {
    m->flags |= UNKNOWN_FAST_FAIL;
  } else if (val == TermWarning) {
    m->flags |= UNKNOWN_WARNING;
  } else if (val == TermDAbort) {
    m->flags |= UNKNOWN_ABORT;
    //  } else if (val == TermHalt) {
    // m->flags |= UNKNOWN_HALT;
  } else {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(val),
		   "valid flags for unknown prolog flag are one of error, fail and warning");
  }
  GLOBAL_Flags[UNKNOWN_FLAG].at = val;
  return val;
}

Term synerr(Term inp){
  if (IsStringTerm(inp)) {
    inp = MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
  }
  if (inp == TermDec10 || inp == TermFail || inp == TermError ||
      inp == TermQuiet)
    return inp;

  if (IsAtomTerm(inp)) {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus( inp),
		   "set_prolog_flag in {dec10,error,fail,quiet}");
    return TermZERO;
  }
  Yap_ThrowError(TYPE_ERROR_ATOM, inp,
		 "syntax_error flag must be atom");
  return TermZERO;
}



#define YAP_FLAG(ID, NAME, WRITABLE, DEF, INIT, HELPER)    { NAME, WRITABLE, DEF, INIT, HELPER }

#define END_FLAG( )  , { NULL, false, NULL, NULL, NULL }

static flag_info local_flags_setup[] = {
#include "YapLFlagInfo.h"
};
static flag_info global_flags_setup[] = {
#include "YapGFlagInfo.h"
};

#undef YAP_FLAG
#undef END_FLAG
  
static Term indexer(Term inp) {
    if (IsStringTerm(inp)) {
      inp = MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
    }
  if (inp == TermOff || inp == TermSingle || inp == TermCompact ||
      inp == TermMulti || inp == TermOn || inp == TermMax)
    return inp;

  if (IsAtomTerm(inp)) {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(inp),
		   "set_prolog_flag index in {off,single,compact,multi,on,max}");
    return TermZERO;
  }
  Yap_ThrowError(TYPE_ERROR_ATOM, inp, "set_prolog_flag index to an atom");
  return TermZERO;
}

/**
 * set or reset signal handlers. The act is only performed if the flag changed values.
 *
 * @param inp Whether to enable or disable
 * @return always true
 *
 */
static bool setSignals(Term inp) {
  bool handle = (inp == TermTrue || inp == TermOn);
  if (handle !=  GLOBAL_PrologShouldHandleInterrupts) {
    Yap_InitSignals(0);
  }
  GLOBAL_PrologShouldHandleInterrupts = handle;
  return true;
}



static bool dollar_to_lc(Term inp) {
  if (inp == TermTrue || inp == TermOn) {
    Yap_chtype0['$'+1] = LC;
    return true;
  }
  if (inp == TermFalse || inp == TermOff) {
    Yap_chtype0['$'+1] = CC;
    return false;
  }
  Yap_ThrowError(TYPE_ERROR_BOOLEAN, inp,
		 "dollar_to_lower_case is a boolean flag");
  return TermZERO;
}

static Term isaccess(Term inp) {
    if (inp == TermReadWrite || inp == TermReadOnly)
      return inp;

  if (IsStringTerm(inp)) {
    inp = MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
  }
  if (IsAtomTerm(inp)) {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(inp),
		   "set_prolog_flag access in {read_write,read_only}");
    return TermZERO;
  }
  Yap_ThrowError(TYPE_ERROR_ATOM, inp,
		 "set_prolog_flag access to the {read_write,read_only}");
  return TermZERO;
}


static Term stream(Term inp) {
  if (IsVarTerm(inp))
    return  true;
  if (Yap_CheckStream(inp,
                      Input_Stream_f | Output_Stream_f | Append_Stream_f |
		      Socket_Stream_f,
                      "yap_flag/3") >= 0)
    return inp;
  return 0;
}

static bool set_input_stream(Term inp) {
  CACHE_REGS
    if (IsVarTerm(inp))
      return Yap_unify(inp, 
		       Yap_MkStream(LOCAL_c_input_stream));
  
  int sno =  Yap_CheckStream(inp,
			     Input_Stream_f  |
			     Socket_Stream_f,
			     "yap_flag/3") ;
  return Yap_AddAlias(AtomUserIn,sno);
}

static bool set_output_stream(Term inp) {
  CACHE_REGS
    int sno;
  if (IsVarTerm(inp))
    return Yap_unify(inp, 
		     Yap_MkStream(LOCAL_c_output_stream));
  
  sno =  Yap_CheckStream(inp,
			 Output_Stream_f | Append_Stream_f |
			 Socket_Stream_f,
			 "yap_flag/3") ;
  return Yap_AddAlias(AtomUserOut,sno);
}

static bool set_error_stream(Term inp) {
  CACHE_REGS
    int sno;
  if (IsVarTerm(inp))
    return Yap_unify(inp, 
		     Yap_MkStream(LOCAL_c_error_stream));
  sno =
    Yap_CheckStream(inp,
		    Output_Stream_f | Append_Stream_f |
		    Socket_Stream_f,
		    "yap_flag/3") ;
  return Yap_AddAlias(AtomUserErr,sno);
}


static Term isground(Term inp) {
  return Yap_IsGroundTerm(inp) ? inp : TermZERO;
}

static Term flagscope(Term inp) {
    if (inp == TermGlobal || inp == TermThread || inp == TermModule)
      return inp;

  if (IsStringTerm(inp)) {
    inp =MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp))); 
  }
  if (IsAtomTerm(inp)) {
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(inp),
		   "set_prolog_flag access in {global,module,thread}");
    return TermZERO;
  }
  Yap_ThrowError(TYPE_ERROR_ATOM, inp,
		 "set_prolog_flag access in {global,module,thread}");
  return TermZERO;
}

static bool mkprompt(Term inp) {
  CACHE_REGS
    if (IsVarTerm(inp)) {
      return Yap_unify(inp, MkAtomTerm(Yap_LookupAtom(LOCAL_Prompt)));
    }
  if (IsStringTerm(inp)) {
    inp =MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
  }
  if (!IsAtomTerm(inp)) {
    Yap_ThrowError(TYPE_ERROR_ATOM, mkplus(inp), "set_prolog_flag");
    return false;
  }
  strncpy(LOCAL_Prompt, (const char *)RepAtom(AtomOfTerm(inp))->StrOfAE,
          MAX_PROMPT);
  return true;
}

static bool getenc(Term inp) {
  CACHE_REGS
    if (IsVarTerm(inp)) {
      return  
	Yap_unify(inp, LOCAL_Flags[ENCODING_FLAG].at);
    }
  if (IsStringTerm(inp)) {
    LOCAL_Flags[ENCODING_FLAG].at = MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
    return true;
  }
  if (IsAtomTerm(inp)) {
    LOCAL_Flags[ENCODING_FLAG].at = inp;
    return true;
  }
  Yap_ThrowError(TYPE_ERROR_ATOM, inp, "get_encoding");
  return false;
}


/*
  static bool enablerl( Term inp ) {
  CACHE_REGS
  if (IsVarTerm(inp)) {
  return Yap_unify( inp, MkAtomTerm( Yap_LookupAtom( enc_name(LOCAL_encoding)) );
  }
  if (!IsAtomTerm(inp) ) {
  Yap_ThrowError(TYPE_ERROR_ATOM, inp, "set_prolog_flag");
  return false;
  }
  enc_id( RepAtom( AtomOfTerm( inp ) )->StrOfAE, ENC_OCTET );
  return true;
  }
*/

static bool typein(Term inp) {
  CACHE_REGS
    if (IsVarTerm(inp)) {
      Term tin = CurrentModule;
      if (tin == PROLOG_MODULE)
	tin = TermProlog;
      return Yap_unify(inp, tin);
    }
  if (IsStringTerm(inp)) {
    inp = MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
  }
  if (!IsAtomTerm(inp)) {
    Yap_ThrowError(TYPE_ERROR_ATOM, inp, "set_prolog_flag");
    return false;
  }
  CurrentModule = inp;
  if (inp == TermProlog)
    CurrentModule = PROLOG_MODULE;
  return true;
}

#if 0

static Int p_has_yap_or(USES_REGS1) {
#ifdef YAPOR
  return (TRUE);
#else
  return (FALSE);
#endif
}

static Int p_has_eam(USES_REGS1) {

#ifdef BEAM
  return (TRUE);
#else
  return (FALSE);
#endif
}

static Int p_has_jit(USES_REGS1) {
#ifdef HAS_JIT
  return (TRUE);
#else
  return (FALSE);
#endif
}

static bool tabling( Term inp ) {
  if (value == 0) { /* default */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      TabEnt_mode(tab_ent) = TabEnt_flags(tab_ent);
      tab_ent = TabEnt_next(tab_ent);
    }
    yap_flags[TA BLING_MODE_FLAG] = 0;
  } else if (value == 1) { /* batched */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_Batched(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_Batched(yap_flags[TABLING_MODE_FLAG]);
  } else if (value == 2) { /* local */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_Local(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_Local(yap_flags[TABLING_MODE_FLAG]);
  } else if (value == 3) { /* exec_answers */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_ExecAnswers(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_ExecAnswers(yap_flags[TABLING_MODE_FLAG]);
  } else if (value == 4) { /* load_answers */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_LoadAnswers(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_LoadAnswers(yap_flags[TABLING_MODE_FLAG]);
  } else if (value == 5) { /* local_trie */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_LocalTrie(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_LocalTrie(yap_flags[TABLING_MODE_FLAG]);
  } else if (value == 6) { /* global_trie */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_GlobalTrie(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_GlobalTrie(yap_flags[TABLING_MODE_FLAG]);
  } else if (value == 7) { /* CoInductive */
    tab_ent_ptr tab_ent = GLOBAL_root_tab_ent;
    while (tab_ent) {
      SetMode_CoInductive(TabEnt_mode(tab_ent));
      tab_ent = TabEnt_next(tab_ent);
    }
    SetMode_CoInductive(yap_flags[TABLING_MODE_FLAG]);
  }
}

static bool string( Term inp ) {
  if (IsVarTerm(inp)) {
    Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag in \"...\"");
    return false;
  }
  if (IsStringTerm( inp ))
    return true;
  Term inp0  = inp;
  if (IsPairTerm(inp)) {
    Term hd = HeadOfTerm(inp);
    if (IsAtomTerm(hd)) {
      do {
	Term hd = HeadOfTerm(inp);
	if (IsStringTerm(hd)) {
	  hd =  MkAtomTerm(Yap_LookupAtom(StringOfTerm(tflag)));
	}
	if (!IsAtomTerm(hd)) {
	  Yap_ThrowError(TYPE_ERROR_TEXT, inp0, "set_prolog_flag in \"...\"");
	  return false;
	}
      } while (IsPairTerm( inp ) );
    } else if (IsIntTerm(hd)) {
      do {
	Term hd = HeadOfTerm(inp);
	if (!IsIntTerm(hd)) {
	  Yap_ThrowError(TYPE_ERROR_TEXT, inp0, "set_prolog_flag in \"...\"");
	  return false;
	}
	if (IntOfTerm(hd) < 0) {
	  Yap_ThrowError(DOMAIN_ERROR_NOT_LESS_THAN_ZERO, inp0, "set_prolog_flag in 0...");
	  return false;
	}
      } while (IsPairTerm( inp ) );
    } else {
      Yap_ThrowError(TYPE_ERROR_TEXT, inp0, "set_prolog_flag in \"...\"");
      return false;
    }
  }
  if ( inp != TermNil ) {
    Yap_ThrowError(TYPE_ERROR_TEXT, inp0, "set_prolog_flag in \"...\"");
    return false;
  }
  return true;
}

  static bool list_atom( Term inp ) {
  if (IsVarTerm(inp)) {
    Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag in \"...\"");
    return false;
  }
  Term inp0  = inp;
  if (IsPairTerm(inp)) {
    Term hd = HeadOfTerm(inp);
    do {
      if (IsStringTerm(hd)) {
	hd = MkAtomTerm(Yap_LookupAtom(StringOfTerm(tflag)));
      }

      if (!IsAtomTerm(hd)) {
	Yap_ThrowError(TYPE_ERROR_ATOM, inp0, "set_prolog_flag in \"...\"");
	return false;
      }
    } while (IsPairTerm( inp ) );
  }
  if ( inp != TermNil ) {
    Yap_ThrowError(TYPE_ERROR_LIST, inp0, "set_prolog_flag in [...]");
    return false;
  }
  return true;
}
#endif

static Term list_option(Term inp) {
    if (IsVarTerm(inp)) {
      Yap_ThrowError(INSTANTIATION_ERROR, inp, "set_prolog_flag in \"...\"");
      return inp;
    }
  Term inp0 = inp;
  if (IsPairTerm(inp)) {
    do {
      Term hd = HeadOfTerm(inp);
      inp = TailOfTerm(inp);
      if (IsStringTerm(hd)) {
	hd =  MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
      }
      if (IsAtomTerm(hd)) {
        continue;
      }
      if (IsApplTerm(hd)) {
        Functor f = FunctorOfTerm(hd);
        if (!IsExtensionFunctor(f) && ArityOfFunctor(f) == 1 &&
            Yap_IsGroundTerm(hd)) {
          continue;
        }
        if (!Yap_IsGroundTerm(hd))
          Yap_ThrowError(INSTANTIATION_ERROR, hd, "set_prolog_flag in \"...\"");
        return TermZERO;
      }
    } while (IsPairTerm(inp));
    if (inp == TermNil) {
      return inp0;
    }
    Yap_ThrowError(TYPE_ERROR_LIST, inp0, "set_prolog_flag in [...]");
    return TermZERO;
  } else /* lone option */ {
    if (IsStringTerm(inp)) {
      inp =   MkAtomTerm(Yap_LookupAtom(StringOfTerm(inp)));
    }
    if (IsAtomTerm(inp)) {
      return inp;
    } else if (IsApplTerm(inp)) {
      Functor f = FunctorOfTerm(inp);
      if (!IsExtensionFunctor(f) && ArityOfFunctor(f) == 1 &&
          Yap_IsGroundTerm(ArgOfTerm(1, inp))) {
        return inp;
      }
    }
  }
  return TermZERO;
}

static bool agc_threshold(Term t) {
  t = Deref(t);
  if (IsVarTerm(t)) {
    CACHE_REGS
      return Yap_unify(t, MkIntegerTerm(GLOBAL_AGcThreshold));
  } else if (!IsIntegerTerm(t)) {
    Yap_ThrowError(TYPE_ERROR_INTEGER, t, "prolog_flag/2 agc_margin");
    return FALSE;
  } else {
    Int i = IntegerOfTerm(t);
    if (i < 0) {
      Yap_ThrowError(DOMAIN_ERROR_NOT_LESS_THAN_ZERO, t, "prolog_flag/2 agc_margin");
      return FALSE;
    } else {
      GLOBAL_AGcThreshold = i;
      return TRUE;
    }
  }
}

static bool gc_margin(Term t) {
  t = Deref(t);
  if (IsVarTerm(t)) {
    return Yap_unify(t, Yap_GetValue(AtomGcMargin));
  } else if (!IsIntegerTerm(t)) {
    Yap_ThrowError(TYPE_ERROR_INTEGER, t, "prolog_flag/2 agc_margin");
    return FALSE;
  } else {
    Int i = IntegerOfTerm(t);
    if (i < 0) {
      Yap_ThrowError(DOMAIN_ERROR_NOT_LESS_THAN_ZERO, t, "prolog_flag/2 gc_margin");
      return FALSE;
    } else {
      CACHE_REGS
	Yap_PutValue(AtomGcMargin, MkIntegerTerm(i));
      return true;
    }
  }
}

static Term mk_argc_list(USES_REGS1) {
  int i = 1;
  Term t = TermNil;
  while (i < GLOBAL_argc) {
    char *arg = GLOBAL_argv[i];
    /* check for -L -- */
    if (arg[0] == '-' && arg[1] == 'L') {
      arg += 2;
      while (*arg != '\0' && (*arg == ' ' || *arg == '\t'))
        arg++;
      if (*arg == '-' && arg[1] == '-' && arg[2] == '\0') {
        /* we found the separator */
        int j;
        for (j = GLOBAL_argc - 1; j > i + 1; --j) {
          t = MkPairTerm(MkAtomTerm(Yap_LookupAtom(GLOBAL_argv[j])), t);
        }
        return t;
      } else if (GLOBAL_argv[i + 1] && GLOBAL_argv[i + 1][0] == '-' &&
                 GLOBAL_argv[i + 1][1] == '-' &&
                 GLOBAL_argv[i + 1][2] == '\0') {
        /* we found the separator */
        int j;
        for (j = GLOBAL_argc - 1; j > i + 2; --j) {
          t = MkPairTerm(MkAtomTerm(Yap_LookupAtom(GLOBAL_argv[j])), t);
        }
        return t;
      }
    }
    if (arg[0] == '-' && arg[1] == '-' && arg[2] == '\0') {
      /* we found the separator */
      int j;
      for (j = GLOBAL_argc - 1; j > i; --j) {
        t = MkPairTerm(MkAtomTerm(Yap_LookupAtom(GLOBAL_argv[j])), t);
      }
      return (t);
    }
    i++;
  }
  return (t);
}

static Term mk_os_argc_list(USES_REGS1) {
  int i = 0;
  Term t = TermNil;
  for (i = GLOBAL_argc; i >0; ) {
    i--;
    const char *arg = GLOBAL_argv[i];
    t = MkPairTerm(MkAtomTerm(Yap_LookupAtom(arg)), t);
  }
  return (t);
}

static Term argv(Term inp) {
  CACHE_REGS
    return mk_argc_list(PASS_REGS1);
}

static Term os_argv(Term inp) {
  CACHE_REGS
    return mk_os_argc_list(PASS_REGS1);
}

static FlagEntry *
GetFlagProp(Atom a) { /* look property list of atom a for kind  */
  AtomEntry *ae = RepAtom(a);
  FlagEntry *pp;

  READ_LOCK(ae->ARWLock);

  pp = RepFlagProp(ae->PropsOfAE);
  while (!EndOfPAEntr(pp) && pp->KindOfPE != FlagProperty)
    pp = RepFlagProp(pp->NextOfPE);
  READ_UNLOCK(ae->ARWLock);

  return pp;
}

static void initFlag(flag_info *f, int fnum, bool global) {

  Atom name = Yap_LookupAtom(f->name);
  AtomEntry *ae = RepAtom(name);
  WRITE_LOCK(ae->ARWLock);
  FlagEntry *fprop = RepFlagProp(Yap_GetAPropHavingLock(name, FlagProperty));
  if (fprop == NULL) {
    fprop = (FlagEntry *)Yap_AllocAtomSpace(sizeof(FlagEntry));
    if (fprop == NULL) {
      WRITE_UNLOCK(ae->ARWLock);
      Yap_ThrowError(RESOURCE_ERROR_HEAP, TermNil,
		     "not enough space for new Flag %s", ae->StrOfAE);
      return;
    }
    fprop->KindOfPE = FlagProperty;
    fprop->FlagOfVE = fnum;
    fprop->rw = f->writable;
    fprop->global = global;
    fprop->scoped = fnum <= UNKNOWN_FLAG;
    fprop->type = f->def;
    fprop->helper = f->helper;
    AddPropToAtom(ae, AbsFlagProp(fprop));
  }
  WRITE_UNLOCK(ae->ARWLock);
}

static Term executable(Term inp) {

  return MkAtomTerm(Yap_LookupAtom(Yap_FindExecutable()));
}

static Term sys_thread_id(Term inp) {
  CACHE_REGS
    int pid;
#ifdef HAVE_GETTID_SYSCALL
  pid = syscall(__NR_gettid);
#elif defined(HAVE_GETTID_MACRO)
  pid = gettid();
#elif defined(__WINDOWS__)
  pid = GetCurrentThreadId();
#else
  pid = 0;
#endif

  return MkIntegerTerm(pid);
}

static Term sys_pid(Term inp) {
  CACHE_REGS
    int pid;
#if defined(__MINGW32__) || _MSC_VER
  pid = _getpid();
#else
  pid = getpid();
#endif

  return MkIntegerTerm(pid);
}



static Int cont_current_prolog_flag(USES_REGS1) {
  Term modt = CurrentModule;
  Term tflag = Yap_StripModule(ARG1, &CurrentModule);
  int i = IntOfTerm(EXTRA_CBACK_ARG(2, 1));
  while (i < GLOBAL_flagCount + LOCAL_flagCount) {
    int gmax = GLOBAL_flagCount;
    int lmax = LOCAL_flagCount;
    Term flag, f;

    if (i >= gmax + lmax) {
      CurrentModule=modt;
      cut_fail();
    } else if (i >= gmax) {
      Yap_unify(tflag, (f = MkAtomTerm(
				      Yap_LookupAtom(local_flags_setup[i - gmax].name))));
    } else {
      Yap_unify(tflag,
                (f = MkAtomTerm(Yap_LookupAtom(global_flags_setup[i].name))));
    }
    EXTRA_CBACK_ARG(2, 1) = MkIntTerm(++i);
    flag = getYapFlag(f);
    CurrentModule = modt;
    return Yap_unify(flag, ARG2);
  }
    CurrentModule = modt;
  cut_fail();
}


/** @pred current_prolog_flag(? _Flag_,- _Value_) is iso

    Obtain the value for a YAP Prolog flag. Equivalent to calling
    yap_flag/2 with the second argument unbound, and unifying the
    returned second argument with  _Value_.

*/
static Int  current_prolog_flag(USES_REGS1) {
  Term tflag = Deref(ARG1);
  Term tout = 0;
  FlagEntry *fv;
  flag_term *tarr;

  Term modt = CurrentModule;
  tflag = Yap_StripModule(tflag, &CurrentModule);
  if (IsVarTerm(tflag)) {
    EXTRA_CBACK_ARG(2, 1) = MkIntTerm(0);
    return cont_current_prolog_flag(PASS_REGS1);
  }
  do_cut(0);
  if (IsStringTerm(tflag)) {
    tflag=   MkAtomTerm(Yap_LookupAtom(StringOfTerm(tflag)));
  }
  if (!IsAtomTerm(tflag)) {
    Yap_ThrowError(TYPE_ERROR_ATOM, tflag, "current_prolog_flag/3");
    return false;
  }
  fv = GetFlagProp(AtomOfTerm(tflag));
  CurrentModule = modt;
  if (!fv) {
        return false;
  }
  if (fv->global)
    tarr = GLOBAL_Flags;
  else
    tarr = LOCAL_Flags;
  tout = tarr[fv->FlagOfVE].at;
  if (tout == TermZERO) {
    //    Yap_DebugPlWriteln(tflag);
    return false;
  }
  if (!IsAtomicTerm(tout)) {
    while ((tout = Yap_FetchTermFromDB(tarr[fv->FlagOfVE].DBT)) == 0) {
      /* oops, we are in trouble, not enough stack space */
	LOCAL_Error_TYPE = YAP_NO_ERROR;
	if (!Yap_dogc(PASS_REGS1)) {
	  Yap_ThrowError(RESOURCE_ERROR_STACK, TermNil, LOCAL_ErrorMessage);
	  //UNLOCK(ap->PELock);
	  return false;
	}
    }
  }
 return Yap_unify(ARG2, tout);
}


bool Yap_set_flag(Term tflag, Term t2) {
  CACHE_REGS
    FlagEntry *fv;
  Term tflag0 = tflag = Deref(tflag);
  t2 = Deref(t2);
  Term modt  = CurrentModule;
  tflag = Yap_StripModule(tflag, &CurrentModule);
  if (IsVarTerm(tflag)) {
        CurrentModule=modt;
    Yap_ThrowError(INSTANTIATION_ERROR, tflag, "yap_flag/2");
    return (FALSE);
  }
  if (IsStringTerm(tflag)) {
    tflag =  MkAtomTerm(Yap_LookupAtom(StringOfTerm(tflag)));
  }
  if (!IsAtomTerm(tflag)) {
        CurrentModule=modt;
    Yap_ThrowError(TYPE_ERROR_ATOM, tflag, "yap_flag/2");
    return (FALSE);
  }
  fv = GetFlagProp(AtomOfTerm(tflag));
  LOCAL_flag = tflag;
  if (!fv) {
    Term fl = GLOBAL_Flags[USER_FLAGS_FLAG].at;
    if (fl == TermSilent) {
      CACHE_REGS
	Term t2 = Deref(ARG2);
      newFlag(tflag, t2);
    } else if (fl == TermWarning) {
      Yap_Warning("Flag %s does not exist", RepAtom(AtomOfTerm(fl))->StrOfAE);
    } else {
              CurrentModule=modt;
	      Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(tflag),
		     "trying to set unknown flag \"%s\"",
		     AtomName(AtomOfTerm(tflag)));
    }
            CurrentModule=modt;
    return false;
  }
  flag_term *tarr;
  if (fv->global) {
    if (tflag == tflag0)
      fv->scoped = false;
    tarr = GLOBAL_Flags;
      } else {
    CACHE_REGS
      tarr = LOCAL_Flags;
  }
if (!(t2 = fv->type(t2))) {
          CurrentModule=modt;
    return false;
 }
 if (fv->helper && !(fv->helper(t2))) {
          CurrentModule=modt;
  return false;
 }
  Term tout = tarr[fv->FlagOfVE].at;
  if (IsVarTerm(tout))
    Yap_PopTermFromDB(tarr[fv->FlagOfVE].DBT);
if (IsAtomOrIntTerm(t2)) {
    tarr[fv->FlagOfVE].at = t2;
}  else {
    tarr[fv->FlagOfVE].DBT = Yap_StoreTermInDB(t2);
  }
         CurrentModule=modt;
return true;
}

Term Yap_UnknownFlag(Term mod) {
  if (mod == PROLOG_MODULE)
    mod = TermProlog;

  ModEntry *fv = Yap_GetModuleEntry(mod);
  if (fv == NULL)
    fv = Yap_GetModuleEntry(TermUser);
  if (fv->flags & UNKNOWN_ERROR)
    return TermError;
  if (fv->flags & UNKNOWN_WARNING)
    return TermWarning;
  return TermFail;
}

Term getYapFlag(Term tflag) {
  CACHE_REGS
    FlagEntry *fv;
  flag_term *tarr;
  tflag = Deref(tflag);
    Term tmod = CurrentModule;
    tflag = Yap_StripModule(tflag, &CurrentModule);
    if (IsStringTerm(tflag)) {
      tflag = MkAtomTerm(Yap_LookupAtom(StringOfTerm(tflag)));
    }
  if (IsVarTerm(tflag)) {
    Yap_ThrowError(INSTANTIATION_ERROR, tflag, "yap_flag/2");
    return (FALSE);
  }
  if (!IsAtomTerm(tflag)) {
    CurrentModule =  tmod;
    Yap_ThrowError(TYPE_ERROR_ATOM, tflag, "yap_flag/2");
    return (FALSE);
  }
  LOCAL_flag = tflag;
  fv = GetFlagProp(AtomOfTerm(tflag));
  if (!fv) {
    Term fl = GLOBAL_Flags[USER_FLAGS_FLAG].at;
    if (fl == TermSilent) {
    CurrentModule = tmod;
    return false;
    } else if (fl == TermWarning) {
      Yap_Warning("Flag ~s does not exist",
                  RepAtom(AtomOfTerm(tflag))->StrOfAE);
    } else {
        CurrentModule = tmod;
	Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(tflag),
		     "trying to use  unknown flag %s",
		     RepAtom(AtomOfTerm(tflag))->StrOfAE);
    }
        CurrentModule = tmod;
    return false;
  }
  if (fv->global) {
    tarr = GLOBAL_Flags;
  if (fv->scoped) {
    Term t2 = MkVarTerm();
    t2 = fv->type(t2);
        CurrentModule = tmod;
    return t2;
 }


  } else {
    CACHE_REGS
      tarr = LOCAL_Flags;
  }
      CurrentModule = tmod;
  Term tout = tarr[fv->FlagOfVE].at;
  if (IsVarTerm(tout))
    return Yap_FetchTermFromDB(tarr[fv->FlagOfVE].DBT);
  else
    return tout;
}

/** @pred set_prolog_flag(+ _Flag_,+ _Value_) is iso

    Set the value for YAP Prolog flag `Flag`. Equivalent to
    calling yap_flag/2 with both arguments bound.

*/
static Int set_prolog_flag(USES_REGS1) {
  Term tflag = Deref(ARG1), t2 = Deref(ARG2);
  return Yap_set_flag(tflag, t2);
}

/**   @pred source

      After executing this goal, YAP keeps information on the source
      of the predicates that will be consulted. This enables the use of
      listing/0, listing/1 and clause/2 for those
      clauses.

      The same as `source_mode(_,on)` or as declaring all newly defined
      static procedures as `public`.
*/
static Int source(USES_REGS1) {
  setBooleanGlobalPrologFlag(SOURCE_FLAG, true);
  return true;
}

/** @pred no_source
    The opposite to `source`.

    The same as `source_mode(_,off)`.

*/
static Int no_source(USES_REGS1) {
  setBooleanGlobalPrologFlag(SOURCE_FLAG, false);
  return true;
}

/**
   @pred source_mode(- _O_,+ _N_)

   The state of source mode can either be on or off. When the source mode
   is on, all clauses are kept both as compiled code and in a "hidden"
   database.  _O_ is unified with the previous state and the mode is set
   according to  _N_.

*/
static Int source_mode(USES_REGS1) {
  Term targ;
  bool current = trueGlobalPrologFlag(SOURCE_FLAG);
  if (current && !Yap_unify_constant(ARG1, TermTrue))
    return false;
  if (!current && !Yap_unify_constant(ARG1, TermFalse))
    return false;
  targ = Deref(ARG2);
  Yap_set_flag(TermSource, targ);
  return true;
}

static bool setInitialValue(bool bootstrap, flag_func f, const char *s,
                            flag_term *tarr) {
  errno = 0;
  const char *ss = (const char *)s;

  if (f == booleanFlag) {
    if (!bootstrap) {
      return 0;
    }
    const char *ss = (const char *)s;
    if (!strcmp(ss, "true")) {
      tarr->at = TermTrue;
      return true;
    }
    if (!strcmp(ss, "false")) {
      tarr->at = TermFalse;
      return true;
    }
    if (!strcmp(ss, "on")) {
      tarr->at = TermTrue;
      return true;
    }
    if (!strcmp(ss, "off")) {
      tarr->at = TermFalse;
      return true;
    }
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, TermNil,
		   "~s should be either true (on) or false (off)", s);
    return false;
  } else if (f == nat) {
    if (!bootstrap) {
      return 0;
    }
    UInt r = strtoul(ss, NULL, 10);
    Term t;
    if (errno) {
      Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, TermNil,
		     "~s should be a positive integer)", s);
      return false;
    }
    CACHE_REGS
      t = MkIntegerTerm(r);
    if (IsIntTerm(t))
      tarr->at = t;
    else {
      tarr->DBT = Yap_StoreTermInDB(t);
    }
    return true;
  } else if (f == at2n) {
    if (!bootstrap) {
      return false;
    }
    if (!strcmp(ss, "INT_MAX")) {
      tarr->at = MkIntTerm(Int_MAX);
      return true;
    }
    if (!strcmp(ss, "MAX_THREADS")) {
      tarr->at = MkIntTerm(MAX_THREADS);
      return true;
    }
    if (!strcmp(ss, "MAX_WORKERS")) {
      tarr->at = MkIntTerm(MAX_WORKERS);
      return true;
    }
    if (!strcmp(ss, "INT_MIN")) {
      tarr->at = MkIntTerm(Int_MIN);
      return true;
    }
    if (!strcmp(ss, "YAP_NUMERIC_VERSION")) {
      tarr->at = MkIntTerm(atol(YAP_NUMERIC_VERSION));
      return true;
    }
    Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, TermNil,
		   "~s should be either true (on) or false (off)", s);
    return false;
  } else if (f == isatom || f == synerr) {
    if (!bootstrap) {
      return false;
    }
    Atom r = Yap_LookupAtom(s);
    if (errno) {
      Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE , TermNil,
		     "~s should be a positive integer)", s);
      tarr->at = TermNil;
    }
    tarr->at = MkAtomTerm(r);
    return true;
  } else if (f == options) {
    CACHE_REGS
      char tmp[512];
    Term t0;
    if (bootstrap) {
      return true;
    }
    t0 = AbsPair(HR);
    while (true) {
      int i = 0, ch = s[0];
      while (ch != '\0' && ch != ';') {
        if (ch != ' ')
          tmp[i++] = ch;
        s++;
        ch = *s;
      }
      tmp[i] = '\0';
      HR += 2;
      HR[-2] = MkAtomTerm(Yap_LookupAtom(tmp));
      if (ch) {
        HR[-1] = AbsPair(HR);
        s++;
        continue;
      } else {
        HR[-1] = TermNil;
        tarr->DBT = Yap_StoreTermInDB(t0);
        return true;
      }
    }
  } else if (strcmp(ss, "@boot") == 0) {
    if (bootstrap) {
      return true;
    }

    Term t = f(TermZERO);
    if (t == TermZERO)
      return false;
    if (IsAtomOrIntTerm(t)) {
      tarr->at = t;
    } else {
      tarr->DBT = Yap_StoreTermInDB(t);
    }

  } else {
    Term t0;
    if (bootstrap) {
      return false;
    }
    CACHE_REGS
      const char *us = (const char *)s;
    t0 = Yap_BufferToTermWithPrioBindings(us, TermNil, 0L, strlen(s) + 1,
                                          GLOBAL_MaxPriority);
    if (!t0)
      return false;
    if (IsStringTerm(t0)) {
      t0 = MkStringTerm(RepAtom(AtomOfTerm(t0))->StrOfAE);
    }
    if (IsAtomTerm(t0) || IsIntTerm(t0)) {
      // do yourself flags
      if (t0 == MkAtomTerm(AtomQuery)) {
        f(TermNil);
      } else {
        tarr->at = t0;
      }
    } else {
      tarr->DBT = Yap_StoreTermInDB(t0);
    }
    return true;
  }
  return false;
}

#undef PAR

#define PROLOG_FLAG_PROPERTY_DEFS()					\
  PAR("access", isaccess, PROLOG_FLAG_PROPERTY_ACCESS, "read_write")	\
  , PAR("type", isground, PROLOG_FLAG_PROPERTY_TYPE, "term"),		\
    PAR("scope", flagscope, PROLOG_FLAG_PROPERTY_SCOPE, "global"),	\
    PAR("keep", booleanFlag, PROLOG_FLAG_PROPERTY_KEEP, "false"),	\
    PAR(NULL, ok, PROLOG_FLAG_PROPERTY_END, 0)

#define PAR(x, y, z, w) z

typedef enum prolog_flag_property_enum_choices {
  PROLOG_FLAG_PROPERTY_DEFS()
} prolog_flag_property_choices_t;

#undef PAR

#define PAR(x, y, z, w)				\
  { x, y, z, w }

static const param2_t prolog_flag_property_defs[] = {
  PROLOG_FLAG_PROPERTY_DEFS()};
#undef PAR

static Int
do_prolog_flag_property(Term tflag,
                        Term opts USES_REGS) { /* Init current_prolog_flag */
  FlagEntry *fv;
  xarg *args;
  prolog_flag_property_choices_t i;
  bool rc = true;
  args =
    Yap_ArgList2ToVector(opts, prolog_flag_property_defs,
			 PROLOG_FLAG_PROPERTY_END, DOMAIN_ERROR_FLAG_VALUE);
  if (args == NULL) {
    Yap_ThrowError(LOCAL_Error_TYPE, opts, NULL);
    return false;
  }
  Atom atflag;
  Term modt = CurrentModule;
  tflag = Yap_YapStripModule(tflag, &modt);
  if (IsStringTerm(tflag)) {
    atflag = Yap_LookupAtom(StringOfTerm(tflag));
  }  else if (IsAtomTerm(tflag)) {
    atflag = AtomOfTerm(tflag);
  } else {
    free(args);
    Yap_ThrowError(TYPE_ERROR_ATOM, tflag, "yap_flag/2");
    return (FALSE);
  }
  LOCAL_flag = MkAtomTerm(atflag);
  fv = GetFlagProp(atflag);
  if (!fv)
    return false;
  //Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE,MkAtomTerm(atflag),NULL);
  for (i = 0; i < PROLOG_FLAG_PROPERTY_END; i++) {
    if (args[i].used) {
      switch (i) {
      case PROLOG_FLAG_PROPERTY_ACCESS:
        if (fv->rw)
          rc = rc && Yap_unify(TermReadWrite,
                               args[PROLOG_FLAG_PROPERTY_ACCESS].tvalue);
        else
          rc = rc && Yap_unify(TermReadOnly,
                               args[PROLOG_FLAG_PROPERTY_ACCESS].tvalue);
        break;
      case PROLOG_FLAG_PROPERTY_TYPE:
        if (fv->type == booleanFlag)
          rc = rc &&
	    Yap_unify(TermBoolean, args[PROLOG_FLAG_PROPERTY_TYPE].tvalue);
        else if (fv->type == isatom)
          rc =
	    rc && Yap_unify(TermAtom, args[PROLOG_FLAG_PROPERTY_TYPE].tvalue);
        else if (fv->type == nat)
          rc = rc &&

	    Yap_unify(TermInteger, args[PROLOG_FLAG_PROPERTY_TYPE].tvalue);
        else if (fv->type == isfloat)
          rc = rc &&
	    Yap_unify(TermFloat, args[PROLOG_FLAG_PROPERTY_TYPE].tvalue);
        else
          rc =
	    rc && Yap_unify(TermTerm, args[PROLOG_FLAG_PROPERTY_TYPE].tvalue);
        break;
      case PROLOG_FLAG_PROPERTY_KEEP:
        rc = rc && false;
        break;
      case PROLOG_FLAG_PROPERTY_SCOPE:
        if (fv->global) {
          if (fv->FlagOfVE <= UNKNOWN_FLAG)
            Yap_unify(TermModule, args[PROLOG_FLAG_PROPERTY_SCOPE].tvalue);
          rc = rc &&
	    Yap_unify(TermGlobal, args[PROLOG_FLAG_PROPERTY_SCOPE].tvalue);
        } else
          rc = rc &&
	    Yap_unify(TermThread, args[PROLOG_FLAG_PROPERTY_SCOPE].tvalue);
        break;
      case PROLOG_FLAG_PROPERTY_END:
        /* break; */
        Yap_ThrowError(DOMAIN_ERROR_FLAG_VALUE, mkplus(opts), "Flag not supported by YAP");
      }
    }
  }
  // UNLOCK(GLOBAL_Prolog_Flag[sno].prolog_flaglock);
  free(args);
  return rc;
}

static Int cont_prolog_flag_property(USES_REGS1) { /* current_prolog_flag */
  int i = IntOfTerm(EXTRA_CBACK_ARG(2, 1));

  while (i < GLOBAL_flagCount + LOCAL_flagCount) {
    int gmax = GLOBAL_flagCount;
    int lmax = LOCAL_flagCount;
    Term lab;

    if (i >= gmax + lmax) {
      cut_fail();
    } else if (i >= gmax) {
      lab = MkAtomTerm(Yap_LookupAtom(local_flags_setup[i - gmax].name));
    } else {
        lab = MkAtomTerm(Yap_LookupAtom(global_flags_setup[i].name));
     }
    EXTRA_CBACK_ARG(2, 1) = MkIntTerm(++i);
    Yap_unify(ARG1, lab);
    return do_prolog_flag_property(lab, Deref(ARG2) PASS_REGS);
  }
  cut_fail();
}

/** @pred prolog_flag_property(+ _Flag_,+ _Prooperties_)

    Report a property for a YAP Prolog flag.  _Properties_ include

    * `type(+_Type_)` with _Type_ one of `boolean`, `integer`, `float`, `atom`
    and `term` (that is, any ground term)

    * `access(+_Access_)` with  _Access_ one of `read_only` or `read_write`

    * `scope(+_Scope_) the flag aplies to a `thread`, to a `module`, or is
    `global` to the system.

*/
static Int prolog_flag_property(USES_REGS1) { /* Initc urrent_prolog_flag */
  Term t1 = Deref(ARG1);
  /* make valgrind happy by always filling in memory */
  EXTRA_CBACK_ARG(2, 1) = MkIntTerm(0);
  if (IsStringTerm(t1)) {
    t1 = MkStringTerm(RepAtom(AtomOfTerm(t1))->StrOfAE);
  }
  if (IsVarTerm(t1)) {
    return (cont_prolog_flag_property(PASS_REGS1));
  } else {
    if (IsApplTerm(t1) && FunctorOfTerm(t1) == FunctorModule) {
      Term modt;
      t1 = Yap_StripModule(t1, &modt);
      if (IsAtomTerm(modt)) {
        Int rc;
        rc = cont_prolog_flag_property(PASS_REGS1);

        return rc;
      }
    } else if (IsAtomTerm(t1)) {
      do_cut(0);
      return do_prolog_flag_property(t1, Deref(ARG2) PASS_REGS);
    } else {
      Yap_ThrowError(TYPE_ERROR_ATOM, t1, "prolog_flag_property/2");
    }
  }
  return false;
}

static void newFlag(Term fl, Term val) {
  flag_info f;
  int i = GLOBAL_flagCount;

  GLOBAL_flagCount++;
  f.name = (char *)RepAtom(AtomOfTerm(fl))->StrOfAE;
  f.writable = true;
  f.helper = NULL;
  f.def = ok;
  initFlag(&f, i, true);
  if (IsAtomOrIntTerm(val)) {
    GLOBAL_Flags[i].at = val;
  } else {
    GLOBAL_Flags[i].DBT = Yap_StoreTermInDB(val);
  }
}

static Int do_create_prolog_flag(USES_REGS1) {
  FlagEntry *fv;
  xarg *args;
  prolog_flag_property_choices_t i;
  Term tflag = Deref(ARG1), tval = Deref(ARG2), opts = Deref(ARG3);

  args =
    Yap_ArgList2ToVector(opts, prolog_flag_property_defs,
			 PROLOG_FLAG_PROPERTY_END, DOMAIN_ERROR_FLAG_VALUE);
  if (args == NULL) {
    Yap_ThrowError(LOCAL_Error_TYPE, opts, NULL);
    return false;
  }
  fv = GetFlagProp(AtomOfTerm(tflag));
  if (fv) {
    if (args[PROLOG_FLAG_PROPERTY_KEEP].used &&
        args[PROLOG_FLAG_PROPERTY_KEEP].tvalue == TermTrue) {
      free(args);
      return true;
    }
  } else {
    newFlag(tflag, tval);
    fv = GetFlagProp(AtomOfTerm(tflag));
  }
  for (i = 0; i < PROLOG_FLAG_PROPERTY_END; i++) {
    if (args[i].used) {
      switch (i) {
      case PROLOG_FLAG_PROPERTY_KEEP:
        break;
      case PROLOG_FLAG_PROPERTY_ACCESS:
        if (args[PROLOG_FLAG_PROPERTY_ACCESS].tvalue == TermReadWrite)
          fv->rw = true;
        else
          fv->rw = false;
        break;
      case PROLOG_FLAG_PROPERTY_TYPE: {
        Term ttype = args[PROLOG_FLAG_PROPERTY_TYPE].tvalue;
        if (ttype == TermBoolean)
          fv->type = booleanFlag;
        else if (ttype == TermInteger)
          fv->type = isatom;
        else if (ttype == TermFloat)
          fv->type = isfloat;
        else
          fv->type = isground;
      } break;
      case PROLOG_FLAG_PROPERTY_SCOPE:
        free(args);
        return false;
      case PROLOG_FLAG_PROPERTY_END:
        break;
      }
    }
  }
  // UNLOCK(GLOBAL_Prolog_Flag[sno].prolog_flaglock);
  free(args);
  return true;
}

/**
 * Create a new global prolog flag.
 *
 * @arg name
 * @arg whether read-only or writable
 * @arg type: boolean, integer, atom, any as a pprolog term
 *
 */
X_API bool Yap_create_prolog_flag(const char *name, bool writable,  Term ttype, Term v) {

  Atom aname = Yap_LookupAtom (name);
  FlagEntry *fv;
  fv = GetFlagProp(aname);
  if (fv) {
    return false;
  } else {
    newFlag(MkAtomTerm(aname), v);
    fv = GetFlagProp(aname);
  }
  fv->rw = writable;
  if (ttype == TermBoolean)
    fv->type = booleanFlag;
  else if (ttype == TermInteger)
    fv->type = isatom;
  else if (ttype == TermFloat)
    fv->type = isfloat;
  else
    fv->type = isground;
  return true;
}

/**
 * Init System Prolog flags. This is done in two phases:
 *   early on, it takes care of the atomic flags that are required by other
 * modules;
 * later, it looks at flags that are structured terms
 *
 * @param bootstrap: wether this is done before stack initialization, or
 *afterwards.
 * Complex terms can only be built in the second step.
 */

void Yap_InitFlags(bool bootstrap) {
  CACHE_REGS

    tr_fr_ptr tr0 = TR;
  flag_info *f = global_flags_setup;
  int lvl = push_text_stack();
  char *buf = Malloc(4098);
  GLOBAL_flagCount = 0;
  if (bootstrap) {
    GLOBAL_Flags = (union flagTerm *)Yap_AllocCodeSpace(
							sizeof(union flagTerm) *
							(2 * sizeof(global_flags_setup) / sizeof(flag_info)));
  }
  int nflags = sizeof(local_flags_setup) / sizeof(flag_info);
  if (bootstrap)
    LOCAL_Flags =
      (union flagTerm *)Yap_AllocCodeSpace(sizeof(union flagTerm) * nflags);
  while (f->name != NULL) {
    bool itf = setInitialValue(bootstrap, f->def, f->init,
                               GLOBAL_Flags + GLOBAL_flagCount);
    if (itf) {
      initFlag(f, GLOBAL_flagCount, true);
    }
    GLOBAL_flagCount++;
    f++;
  }
  LOCAL_flagCount = 0;
  f = local_flags_setup;
  while (f->name != NULL) {
    char *s;
    if (f->init == NULL || f->init[0] == '\0') s = NULL;
    else if (strlen(f->init) < 4096) {
      s = buf;
      strcpy(buf, f->init);
    } else {
      s = Malloc(strlen(f->init)+1);
      strcpy(s, f->init);
    }
    bool itf = setInitialValue(bootstrap, f->def, s,
                               LOCAL_Flags + LOCAL_flagCount);
    //    Term itf = Yap_BufferToTermWithPrioBindings(f->init,
    //    strlen(f->init)+1,
    //    LOBAL_MaxPriority, &tp);
    if (itf) {
      initFlag(f, LOCAL_flagCount, false);
    }
    LOCAL_flagCount++;
    f++;
  }
  // fix readline gettong set so early
  if (GLOBAL_Stream[StdInStream].status & Readline_Stream_f) {
    setBooleanGlobalPrologFlag(READLINE_FLAG, true);
  }
  pop_text_stack(lvl);
  if (!bootstrap) {
    Yap_InitCPredBack("current_prolog_flag", 2, 1, current_prolog_flag,
                      cont_current_prolog_flag, 0);
    TR = tr0;
    Yap_InitCPred("set_prolog_flag", 2, set_prolog_flag, SyncPredFlag);
    Yap_InitCPred("$create_prolog_flag", 3, do_create_prolog_flag,
                  SyncPredFlag);
    Yap_InitCPredBack("prolog_flag_property", 2, 1, prolog_flag_property,
                      cont_prolog_flag_property, 0);
    Yap_InitCPred("source", 0, source, SyncPredFlag);
    Yap_InitCPred("no_source", 0, no_source, SyncPredFlag);
    Yap_InitCPred("source_mode", 2, source_mode, SyncPredFlag);
  }
}


// Yap_set_flag(Term tflag, Term t2);

/// @}


