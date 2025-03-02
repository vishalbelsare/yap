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
* File:		adtdefs.c						 *
* Last rev:								 *
* mods:									 *
* comments:	abstract machine definitions				 *
*									 *
*************************************************************************/
#ifdef SCCS
static char SccsId[] = "%W% %G%";
#endif

#define ADTDEFS_C

#ifdef __SUNPRO_CC
#define inline
#endif

#include "Yap.h"
#include "Yatom.h"
#include "clause.h"
#include "alloc.h"
#include <stdio.h>
#include <wchar.h>
#if HAVE_STRING_Hq
#include <string.h>
#endif

uint64_t HashFunction(const unsigned char *CHP) {
  /* djb2 */
  uint64_t hash = 5381;
  uint64_t c;

  while ((c = *CHP++) != '\0') {
    /* hash = ((hash << 5) + hash) + c; hash * 33 + c */
    hash = hash * (uint64_t)33 + c;
  }
  return hash;
  /*
    UInt OUT=0, i = 1;
    while(*CHP != '\0') { OUT += (UInt)(*CHP++); }
    return OUT;
  */
}

/* this routine must be run at least having a read lock on ae */
static Prop
GetFunctorProp(AtomEntry *ae,
               arity_t arity) { /* look property list of atom a for kind */

  PropEntry *p = ae->PropsOfAE;
  while (p != NIL) {
    if (p->KindOfPE == FunctorProperty &&
        RepFunctorProp(p)->ArityOfFE == arity) {
      return p;
    }
    p = p->NextOfPE;
  }
  return NIL;
}

/* vsc: We must guarantee that IsVarTerm(functor) returns true! */
static inline Functor InlinedUnlockedMkFunctor(AtomEntry *ae, arity_t arity) {
  FunctorEntry *p;
  Prop p0;

  p0 = GetFunctorProp(ae, arity);
  if (p0 != NIL) {
    return ((Functor)RepProp(p0));
  }
  p = (FunctorEntry *)Yap_AllocAtomSpace(sizeof(*p));
  if (!p)
    return NULL;
  p->KindOfPE = FunctorProperty;
  p->NameOfFE = AbsAtom(ae);
  p->ArityOfFE = arity;
  p->PropsOfFE = NIL;
  INIT_RWLOCK(p->FRWLock);
  /* respect the first property, in case this is a wide atom */
  AddPropToAtom(ae, (PropEntry *)p);
  return ((Functor)p);
}

Functor Yap_UnlockedMkFunctor(AtomEntry *ae, arity_t arity) {
  return (InlinedUnlockedMkFunctor(ae, arity));
}

/* vsc: We must guarantee that IsVarTerm(functor) returns true! */
Functor Yap_MkFunctor(Atom ap, arity_t arity) {
  AtomEntry *ae = RepAtom(ap);
  Functor f;

  WRITE_LOCK(ae->ARWLock);
  f = InlinedUnlockedMkFunctor(ae, arity);
  WRITE_UNLOCK(ae->ARWLock);
  return f;
}

/* vsc: We must guarantee that IsVarTerm(functor) returns true! */
void Yap_MkFunctorWithAddress(Atom ap, unsigned int arity, FunctorEntry *p) {
  AtomEntry *ae = RepAtom(ap);

  WRITE_LOCK(ae->ARWLock);
  p->KindOfPE = FunctorProperty;
  p->NameOfFE = ap;
  p->ArityOfFE = arity;
  AddPropToAtom(ae, (PropEntry *)p);
  WRITE_UNLOCK(ae->ARWLock);
}

inline static Atom SearchInInvisible(const unsigned char *atom) {
  AtomEntry *chain;

  READ_LOCK(INVISIBLECHAIN.AERWLock);
  chain = RepAtom(INVISIBLECHAIN.Entry);
  while (!EndOfPAEntr(chain) && strcmp((char *)chain->StrOfAE, (char *)atom)) {
    chain = RepAtom(chain->NextOfAE);
  }
  READ_UNLOCK(INVISIBLECHAIN.AERWLock);
  if (EndOfPAEntr(chain))
    return (NIL);
  else
    return (AbsAtom(chain));
}



static inline Atom SearchAtom(const unsigned char *p, Atom a) {
  AtomEntry *ae;
  const char *ps = (const char *)p;
  /* search atom in chain */
  while (a != NIL) {
    ae = RepAtom(a);
    //    printf("%d: %s %d %s %d\n",iv++,ps, strlen(ps), ae->StrOfAE, strlen(ae->StrOfAE));
    if (strcmp(ae->StrOfAE, ps) == 0) {
      return (a);
    }
    a = ae->NextOfAE;
  }
  return (NIL);
}


static Atom
LookupAtom(const unsigned char *atom) { /* lookup atom in atom table */
  uint64_t hash;
  const unsigned char *p;
  Atom a, na = NIL;
  AtomEntry *ae;
  size_t sz = AtomHashTableSize;
  /* compute hash */
  p = atom;
  //printf("(%d)--> %s\n",iv++,atom);
  if (atom==NULL) return NULL;
  if (atom[0]==0) return AtomEmpty;
    hash = HashFunction(p);
    hash = hash % sz;
  /* we'll start by holding a read lock in order to avoid contention */
  READ_LOCK(HashChain[hash].AERWLock);
  a = HashChain[hash].Entry;
  /* search atom in chain */
  na = SearchAtom(atom,a);
  if (na != NIL) {
    READ_UNLOCK(HashChain[hash].AERWLock);
    return (na);
  }
  READ_UNLOCK(HashChain[hash].AERWLock);
  /* we need a write lock */
  WRITE_LOCK(HashChain[hash].AERWLock);
/* concurrent version of Yap, need to take care */
#if defined(YAPOR) || defined(THREADS)
  if (a != HashChain[hash].Entry) {
    a = HashChain[hash].Entry;
    na = SearchAtom(atom, a);
    
    if (na != NIL) {
      WRITE_UNLOCK(HashChain[hash].AERWLock);
      return na;
    }
  }
#endif
  /* add new atom to start of chain */
  sz = strlen((const char *)atom);
  size_t asz = (sizeof *ae)+ALIGN_SIZE( sz+2,sizeof(CELL));
  ae = calloc(asz, 1);
  if (ae == NULL) {
    WRITE_UNLOCK(HashChain[hash].AERWLock);
    return NIL;
  }
  NOfAtoms++;
  na = AbsAtom(ae);
  ae->PropsOfAE = NIL;
  strcpy(ae->StrOfAE, (const char *)atom);

  ae->NextOfAE = a;
  HashChain[hash].Entry = na;
  INIT_RWLOCK(ae->ARWLock);
  WRITE_UNLOCK(HashChain[hash].AERWLock);
  if (NOfAtoms > 2 * AtomHashTableSize) {
    Yap_signal(YAP_CDOVF_SIGNAL);
  }

  return na;
}

Atom Yap_LookupAtomWithLength(const char *atom,
			      size_t len0) { /* 
lookup atom in atom table */
    Atom at;
    unsigned char *ptr;

    /* not really a wide atom */
  if (atom==NULL) return NULL;
  if (atom[0]=='\0')
    return AtomEmpty;
  ptr = Yap_AllocCodeSpace(len0 + 1);
    if (!ptr)
      return NIL;
    memcpy(ptr, atom, len0);
    ptr[len0] = '\0';
    at = LookupAtom(ptr);
    return at;
  }

  Atom Yap_LookupAtom(const char *atom) { /* lookup atom in atom table */
    return LookupAtom((const unsigned char *)atom);
  }

  Atom Yap_ULookupAtom(
		       const unsigned char *atom) { /* lookup atom in atom table            */
    return LookupAtom(atom);
  }


  Atom Yap_FullLookupAtom(const char *atom) { /* lookup atom in atom table */
    Atom t;

    if ((t = SearchInInvisible((const unsigned char *)atom)) != NIL) {
      return (t);
    }
    return LookupAtom((const unsigned char *)atom);
  }

  void Yap_LookupAtomWithAddress(const char *atom,
				 AtomEntry *ae) { /* lookup atom in atom table */
    register CELL hash;
    register const unsigned char *p;
    Atom a;

    if (atom == NULL) return;

    /* compute hash */
    p = (const unsigned char *)atom;
    hash = HashFunction(p) % AtomHashTableSize;
    /* ask for a WRITE lock because it is highly unlikely we shall find anything
     */
    WRITE_LOCK(HashChain[hash].AERWLock);
    a = HashChain[hash].Entry;
    /* search atom in chain */
    if (SearchAtom(p, a) != NIL) {
      Yap_Error(SYSTEM_ERROR_INTERNAL, TermNil,
		"repeated initialization for atom %s", ae);
      WRITE_UNLOCK(HashChain[hash].AERWLock);
      return;
    }
    /* add new atom to start of chain */
    NOfAtoms++;
    ae->NextOfAE = a;
    HashChain[hash].Entry = AbsAtom(ae);
    ae->PropsOfAE = NIL;
    strcpy((char *)ae->StrOfAE, (char *)atom);
    INIT_RWLOCK(ae->ARWLock);
    WRITE_UNLOCK(HashChain[hash].AERWLock);
  }

  void Yap_ReleaseAtom(Atom atom) { /* Releases an atom from the hash chain */
    register Int hash;
    register const unsigned char *p;
    AtomEntry *inChain;
    AtomEntry *ap = RepAtom(atom);
    char unsigned *name = ap->UStrOfAE;

    /* compute hash */
    p = name;
    hash = HashFunction(p) % AtomHashTableSize;
    WRITE_LOCK(HashChain[hash].AERWLock);
    if (HashChain[hash].Entry == atom) {
      NOfAtoms--;
      HashChain[hash].Entry = ap->NextOfAE;
      WRITE_UNLOCK(HashChain[hash].AERWLock);
      return;
    }
    /* else */
    inChain = RepAtom(HashChain[hash].Entry);
    while (inChain && inChain->NextOfAE != atom)
      inChain = RepAtom(inChain->NextOfAE);
    if (!inChain)
      return;
    WRITE_LOCK(inChain->ARWLock);
    inChain->NextOfAE = ap->NextOfAE;
    WRITE_UNLOCK(inChain->ARWLock);
    WRITE_UNLOCK(HashChain[hash].AERWLock);
    ap->NextOfAE = NULL;
  }

  static Prop
    GetAPropHavingLock(AtomEntry *ae,
		       PropFlags kind) { /* look property list of atom a for kind */
    PropEntry *pp;

    pp = RepProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp) && pp->KindOfPE != kind)
      pp = RepProp(pp->NextOfPE);
    return (AbsProp(pp));
  }

  Prop Yap_GetAPropHavingLock(
			      AtomEntry *ae, PropFlags kind) { /* look property list of atom a for kind */
    return GetAPropHavingLock(ae, kind);
  }

  static Prop
    GetAProp(Atom a, PropFlags kind) { /* look property list of atom a for kind  */
    AtomEntry *ae = RepAtom(a);
    Prop out;

    READ_LOCK(ae->ARWLock);
    out = GetAPropHavingLock(ae, kind);
    READ_UNLOCK(ae->ARWLock);
    return (out);
  }

  Prop Yap_GetAProp(Atom a,
		    PropFlags kind) { /* look property list of atom a for kind  */
    return GetAProp(a, kind);
  }

  OpEntry *Yap_GetOpPropForAModuleHavingALock(
					      Atom a, Term mod) { /* look property list of atom a for kind  */
    AtomEntry *ae = RepAtom(a);
    PropEntry *pp;

    pp = RepProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp) &&
	   (pp->KindOfPE != OpProperty || ((OpEntry *)pp)->OpModule != mod))
      pp = RepProp(pp->NextOfPE);
    if (EndOfPAEntr(pp)) {
      return NULL;
    }
    return (OpEntry *)pp;
  }

  int Yap_HasOp(Atom a) { /* look property list of atom a for kind  */
    AtomEntry *ae = RepAtom(a);
    PropEntry *pp;

    READ_LOCK(ae->ARWLock);
    pp = RepProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp) && (pp->KindOfPE != OpProperty))
      pp = RepProp(pp->NextOfPE);
    READ_UNLOCK(ae->ARWLock);
    if (EndOfPAEntr(pp)) {
      return FALSE;
    } else {
      return TRUE;
    }
  }

  OpEntry *
    Yap_OpPropForModule(Atom a,
			Term mod) { /* look property list of atom a for kind  */
    AtomEntry *ae = RepAtom(a);
    PropEntry *pp;
    OpEntry *info = NULL;

    if (mod == TermProlog)
      mod = PROLOG_MODULE;
    WRITE_LOCK(ae->ARWLock);
    pp = RepProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp)) {
      if (pp->KindOfPE == OpProperty) {
	info = (OpEntry *)pp;
	if (info->OpModule == mod) {
	  WRITE_LOCK(info->OpRWLock);
	  WRITE_UNLOCK(ae->ARWLock);
	  return info;
	}
      }
      pp = pp->NextOfPE;
    }
    info = (OpEntry *)Yap_AllocAtomSpace(sizeof(OpEntry));
    info->KindOfPE = Ord(OpProperty);
    info->NextOfPE = NULL;
    info->OpModule = mod;
    info->OpName = a;
    LOCK(OpListLock);
    info->OpNext = OpList;
    OpList = info;
    UNLOCK(OpListLock);
    AddPropToAtom(ae, (PropEntry *)info);
    INIT_RWLOCK(info->OpRWLock);
    WRITE_LOCK(info->OpRWLock);
    WRITE_UNLOCK(ae->ARWLock);
    info->Prefix = info->Infix = info->Posfix = 0;
    return info;
  }

// called with Atm Lock
  OpEntry *Yap_GetOpProp(Atom a, op_type type,
		  Term cmod USES_REGS) { /* look property list of atom a for kind */
    AtomEntry *ae = RepAtom(a);
    PropEntry *pp;
    OpEntry *oinfo = NULL;

    pp = RepProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp)) {
      OpEntry *info = NULL;
      if (pp->KindOfPE != OpProperty) {
	pp = RepProp(pp->NextOfPE);
	continue;
      }
      info = (OpEntry *)pp;
      if (info->OpModule != cmod && info->OpModule != PROLOG_MODULE) {
	pp = RepProp(pp->NextOfPE);
	continue;
      }
      if (type == INFIX_OP) {
	if (!info->Infix) {
	  pp = RepProp(pp->NextOfPE);
	  continue;
	}
      } else if (type == POSFIX_OP) {
	if (!info->Posfix) {
	  pp = RepProp(pp->NextOfPE);
	  continue;
	}
      } else {
	if (!info->Prefix) {
	  pp = RepProp(pp->NextOfPE);
	  continue;
	}
      }
      /* if it is not the latest module */
      if (info->OpModule == PROLOG_MODULE) {
	/* cannot commit now */
	oinfo = info;
	pp = RepProp(pp->NextOfPE);
      } else {
	return info;
      }
    }
    if (oinfo) {
      return oinfo;
    }
    return NULL;
  }

  inline static Prop GetPredPropByAtomHavingLock(AtomEntry *ae, Term cur_mod)
  /* get predicate entry for ap/arity; create it if neccessary.              */
  {
    Prop p0;

    p0 = ae->PropsOfAE;
    while (p0) {
      PredEntry *pe = RepPredProp(p0);
      if (pe->KindOfPE == PEProp &&
	  (pe->ModuleOfPred == cur_mod || !pe->ModuleOfPred)) {
	return (p0);
#if THREADS
	/* Thread Local Predicates */
	if (pe->PredFlags & ThreadLocalPredFlag) {
	  return AbsPredProp(Yap_GetThreadPred(pe INIT_REGS));
	}
#endif
      }
      p0 = pe->NextOfPE;
    }
    return (NIL);
  }

  Prop Yap_GetPredPropByAtom(Atom at, Term cur_mod)
  /* get predicate entry for ap/arity; create it if neccessary.              */
  {
    Prop p0;
    AtomEntry *ae = RepAtom(at);

    READ_LOCK(ae->ARWLock);
    p0 = GetPredPropByAtomHavingLock(ae, cur_mod);
    READ_UNLOCK(ae->ARWLock);
    return (p0);
  }

  inline static Prop GetPredPropByAtomHavingLockInThisModule(AtomEntry *ae, Term cur_mod)
  /* get predicate entry for ap/arity; create it if neccessary.              */
  {
    Prop p0;

    p0 = ae->PropsOfAE;
    while (p0) {
      PredEntry *pe = RepPredProp(p0);
      if (pe->KindOfPE == PEProp && pe->ModuleOfPred == cur_mod) {
#if THREADS
	/* Thread Local Predicates */
	if (pe->PredFlags & ThreadLocalPredFlag) {
	  return AbsPredProp(Yap_GetThreadPred(pe INIT_REGS));
	}
#endif
	return (p0);
      }
      p0 = pe->NextOfPE;
    }
    return (NIL);
  }

  Prop Yap_GetPredPropByAtomInThisModule(Atom at, Term cur_mod)
  /* get predicate entry for ap/arity; create it if neccessary.              */
  {
    Prop p0;
    AtomEntry *ae = RepAtom(at);

    READ_LOCK(ae->ARWLock);
    p0 = GetPredPropByAtomHavingLockInThisModule(ae, cur_mod);
    READ_UNLOCK(ae->ARWLock);
    return (p0);
  }


  Prop Yap_GetPredPropByFunc(Functor f, Term cur_mod)
  /* get predicate entry for ap/arity;               */
  {
    Prop p0;
    FUNC_READ_LOCK(f);

    p0 = GetPredPropByFuncHavingLock(f, cur_mod);

    FUNC_READ_UNLOCK(f);
    return (p0);
  }

  Prop Yap_GetPredPropByFuncInThisModule(Functor f, Term cur_mod)
  /* get predicate entry for ap/arity;               */
  {
    Prop p0;

    FUNC_READ_LOCK(f);
    p0 = GetPredPropByFuncHavingLock(f, cur_mod);
    FUNC_READ_UNLOCK(f);
    return (p0);
  }

  Prop Yap_GetPredPropHavingLock(Atom ap, unsigned int arity, Term mod)
  /* get predicate entry for ap/arity;               */
  {
    Prop p0;
    AtomEntry *ae = RepAtom(ap);
    Functor f;

    if (arity == 0) {
      GetPredPropByAtomHavingLock(ae, mod);
    }
    f = InlinedUnlockedMkFunctor(ae, arity);
    FUNC_READ_LOCK(f);
    p0 = GetPredPropByFuncHavingLock(f, mod);
    FUNC_READ_UNLOCK(f);
    return (p0);
  }

  /* get expression entry for at/arity;               */
  Prop Yap_GetExpProp(Atom at, unsigned int arity) {
    Prop p0;
    AtomEntry *ae = RepAtom(at);
    ExpEntry *p;

    READ_LOCK(ae->ARWLock);
    p = RepExpProp(p0 = ae->PropsOfAE);
    while (p0 && (p->KindOfPE != ExpProperty || p->ArityOfEE != arity))
      p = RepExpProp(p0 = p->NextOfPE);
    READ_UNLOCK(ae->ARWLock);
    return (p0);
  }

  /* get expression entry for at/arity, at is already locked;         */
  Prop Yap_GetExpPropHavingLock(AtomEntry *ae, unsigned int arity) {
    Prop p0;
    ExpEntry *p;

    p = RepExpProp(p0 = ae->PropsOfAE);
    while (p0 && (p->KindOfPE != ExpProperty || p->ArityOfEE != arity))
      p = RepExpProp(p0 = p->NextOfPE);

    return (p0);
  }

  static int ExpandPredHash(void) {
    UInt new_size = PredHashTableSize + PredHashIncrement;
    PredEntry **oldp = PredHash;
    PredEntry **np =
      calloc(sizeof(PredEntry **),new_size);
    UInt i;

    if (!np) {
      return FALSE;
    }
    for (i = 0; i < PredHashTableSize; i++) {
      PredEntry *p = PredHash[i];

      while (p) {
	PredEntry *nextp = p->NextPredOfHash;
	UInt hsh = PRED_HASH(p->FunctorOfPred, p->ModuleOfPred, new_size);
	p->NextPredOfHash = np[hsh];
	np[hsh] = p;
	p = nextp;
      }
    }
    PredHashTableSize = new_size;
    PredHash = np;
    Yap_FreeAtomSpace((ADDR)oldp);
    return TRUE;
  }

  /* fe is supposed to be locked */
  Prop Yap_NewPredPropByFunctor(FunctorEntry *fe, Term cur_mod) {
    PredEntry *p = (PredEntry *)Yap_AllocAtomSpace(sizeof(*p));

    if (p == NULL) {
      return NULL;
    }
    if (cur_mod == TermProlog) {
      p->ModuleOfPred = 0L;
    } else
      p->ModuleOfPred = cur_mod;
    // TRUE_FUNC_WRITE_LOCK(fe);
    INIT_LOCK(p->PELock);
    p->KindOfPE = PEProp;
    p->ArityOfPE = fe->ArityOfFE;
    /* if (!strcmp(RepAtom(NameOfFunctor(fe))->StrOfAE,"skip_list")) { */
    /* void jmp_deb(int), jmp_deb2(void); */
    /* Yap_DebugPlWriteln((cur_mod==0?TermProlog:cur_mod)); */
    /*   jmp_deb(1); */

    /* } */
      p->cs.p_code.FirstClause = p->cs.p_code.LastClause = NULL;
    p->cs.p_code.NOfClauses = 0;
    p->PredFlags = UndefPredFlag;
    p->src.OwnerLine = Yap_source_line_no();
    p->src.OwnerFile = Yap_source_file_name();
    p->OpcodeOfPred = UNDEF_OPCODE;
    p->CodeOfPred = p->cs.p_code.TrueCodeOfPred = (yamop *)(&(p->OpcodeOfPred));
    p->cs.p_code.ExpandCode = EXPAND_OP_CODE;
    p->CallLineForUndefinedPred = Yap_source_line_no();
    p->LastCallOfPred = LUCALL_ASSERT;
    p->MetaEntryOfPred = NULL;
    if (cur_mod == TermProlog)
      p->ModuleOfPred = 0L;
    else
      p->ModuleOfPred = cur_mod;
    p->StatisticsForPred = NULL;
    Yap_NewModulePred( p);

#ifdef TABLING
    p->TableOfPred = NULL;
#endif /* TABLING */
#ifdef BEAM
    p->beamTable = NULL;
#endif /* BEAM */
       /* careful that they don't cross MkFunctor */
    if (!trueGlobalPrologFlag(DEBUG_INFO_FLAG)) {
      p->PredFlags |= NoTracePredFlag;
    }
    p->FunctorOfPred = fe;
    if (fe->PropsOfFE) {
      UInt hsh = PRED_HASH(fe, cur_mod, PredHashTableSize);

      WRITE_LOCK(PredHashRWLock);
      if (10 * (PredsInHashTable + 1) > 6 * PredHashTableSize) {
	if (!ExpandPredHash()) {
	  Yap_FreeCodeSpace((ADDR)p);
	  WRITE_UNLOCK(PredHashRWLock);
	  return NULL;
	}
	/* retry hashing */
	hsh = PRED_HASH(fe, cur_mod, PredHashTableSize);
      }
      PredsInHashTable++;
      if (p->ModuleOfPred == 0L) {
	PredEntry *pe = RepPredProp(fe->PropsOfFE);

	hsh = PRED_HASH(fe, pe->ModuleOfPred, PredHashTableSize);
	/* should be the first one */
	pe->NextPredOfHash = PredHash[hsh];
	PredHash[hsh] = pe;
	fe->PropsOfFE = AbsPredProp(p);
	p->NextOfPE = AbsPredProp(pe);
      } else {
	p->NextPredOfHash = PredHash[hsh];
	PredHash[hsh] = p;
	p->NextOfPE = fe->PropsOfFE->NextOfPE;
	fe->PropsOfFE->NextOfPE = AbsPredProp(p);
      }
      WRITE_UNLOCK(PredHashRWLock);
    } else {
      fe->PropsOfFE = AbsPredProp(p);
      p->NextOfPE = NIL;
    }
    {
      Yap_inform_profiler_of_clause(&(p->OpcodeOfPred), &(p->OpcodeOfPred) + 1, p,
				    GPROF_NEW_PRED_FUNC);
      if (!(p->PredFlags & (CPredFlag | AsmPredFlag))) {
	Yap_inform_profiler_of_clause(&(p->cs.p_code.ExpandCode),
				      &(p->cs.p_code.ExpandCode) + 1, p,
				      GPROF_NEW_PRED_FUNC);
      }
    }
    return AbsPredProp(p);
  }

#if THREADS
  Prop Yap_NewThreadPred(PredEntry *ap USES_REGS) {
    PredEntry *p = (PredEntry *)Yap_AllocAtomSpace(sizeof(*p));

    if (p == NULL) {
      return NIL;
    }
    INIT_LOCK(p->PELock);
    p->StatisticsForPred = NULL ; p->KindOfPE = PEProp;
    p->ArityOfPE = ap->ArityOfPE;
    p->cs.p_code.FirstClause = p->cs.p_code.LastClause = NULL;
    p->cs.p_code.NOfClauses = 0;
    p->PredFlags = ap->PredFlags & ~(IndexedPredFlag | SpiedPredFlag);
#if SIZEOF_INT_P == 4
    p->ExtraPredFlags = 0L;
#endif
    p->MetaEntryOfPred = NULL;
    p->src.OwnerLine = Yap_source_line_no();
    p->src.OwnerFile = Yap_source_file_name();
    p->OpcodeOfPred = FAIL_OPCODE;
    p->CodeOfPred = p->cs.p_code.TrueCodeOfPred = (yamop *)(&(p->OpcodeOfPred));
    p->cs.p_code.ExpandCode = EXPAND_OP_CODE;
    p->ModuleOfPred = ap->ModuleOfPred;
    p->NextPredOfModule = NULL;
    p->TimeStampOfPred = 0L;
    p->LastCallOfPred = LUCALL_ASSERT;
#ifdef TABLING
    p->TableOfPred = NULL;
#endif /* TABLING */
#ifdef BEAM
    p->beamTable = NULL;
#endif
    /* careful that they don't cross MkFunctor */
    p->NextOfPE = AbsPredProp(LOCAL_ThreadHandle.local_preds);
    LOCAL_ThreadHandle.local_preds = p;
    p->FunctorOfPred = ap->FunctorOfPred;
    Yap_inform_profiler_of_clause(&(p->OpcodeOfPred), &(p->OpcodeOfPred) + 1, p,
				  GPROF_NEW_PRED_THREAD);
    if (falseGlobalPrologFlag(DEBUG_INFO_FLAG)) {
      p->PredFlags |= (NoSpyPredFlag | NoTracePredFlag);
    }
    if (!(p->PredFlags & (CPredFlag | AsmPredFlag))) {
      Yap_inform_profiler_of_clause(&(p->cs.p_code.ExpandCode),
				    &(p->cs.p_code.ExpandCode) + 1, p,
				    GPROF_NEW_PRED_THREAD);
    }
    return AbsPredProp(p);
  }
#endif

  Prop Yap_NewPredPropByAtom(AtomEntry *ae, Term cur_mod) {
    Prop p0;
    PredEntry *p = (PredEntry *)Yap_AllocAtomSpace(sizeof(*p));
    CACHE_REGS
      /* Printf("entering %s:%s/0\n", RepAtom(AtomOfTerm(cur_mod))->StrOfAE,
       * ae->StrOfAE); */

      if (p == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return NIL;
      }
    INIT_LOCK(p->PELock);
    //printf("------------->atom %s\n", ae->StrOfAE);
    p->FunctorOfPred = (Functor)AbsAtom(ae);
    if (cur_mod == TermProlog)
      p->ModuleOfPred = 0;
    else
      p->ModuleOfPred = cur_mod;
    Yap_NewModulePred_HoldingLock( p);

    
    p->KindOfPE = PEProp;
    p->ArityOfPE = 0;
    p->StatisticsForPred = NULL;
    p->cs.p_code.FirstClause = p->cs.p_code.LastClause = NULL;
    p->cs.p_code.NOfClauses = 0;
    p->PredFlags = UndefPredFlag;
    p->src.OwnerFile = Yap_source_file_name();
    p->OpcodeOfPred = UNDEF_OPCODE;
    p->cs.p_code.ExpandCode = EXPAND_OP_CODE;
    p->CodeOfPred = p->cs.p_code.TrueCodeOfPred = (yamop *)(&(p->OpcodeOfPred));
    p->MetaEntryOfPred = NULL;
    p->TimeStampOfPred = 0L;
    p->LastCallOfPred = LUCALL_ASSERT;
#ifdef TABLING
    p->TableOfPred = NULL;
#endif /* TABLING */
#ifdef BEAM
    p->beamTable = NULL;
#endif
    /* careful that they don't cross MkFunctor */
    AddPropToAtom(ae, (PropEntry *)p);
    p0 = AbsPredProp(p);
    if (!trueGlobalPrologFlag(DEBUG_INFO_FLAG)) {
      p->PredFlags |= (NoTracePredFlag | NoSpyPredFlag);
    }
    if (Yap_isSystemModule_HoldingLock(CurrentModule,  ae))
      p->PredFlags |= StandardPredFlag;
    WRITE_UNLOCK(ae->ARWLock);
    {
      Yap_inform_profiler_of_clause(&(p->OpcodeOfPred), &(p->OpcodeOfPred) + 1, p,
				    GPROF_NEW_PRED_ATOM);
      if (!(p->PredFlags & (CPredFlag | AsmPredFlag))) {
	Yap_inform_profiler_of_clause(&(p->cs.p_code.ExpandCode),
				      &(p->cs.p_code.ExpandCode) + 1, p,
				      GPROF_NEW_PRED_ATOM);
      }
    }
    return p0;
  }

  Prop Yap_PredPropByFunctorNonThreadLocal(Functor f, Term cur_mod)
  /* get predicate entry for ap/arity; create it if neccessary.              */
  {
    PredEntry *p;

    FUNC_WRITE_LOCK(f);
    if (!(p = RepPredProp(f->PropsOfFE))) {
      Prop pn = Yap_NewPredPropByFunctor(f, cur_mod);
      FUNC_WRITE_UNLOCK(f);
      return pn;
    }
    if ((p->ModuleOfPred == cur_mod || !(p->ModuleOfPred))) {
      /* don't match multi-files */
      if (/*!(p->PredFlags & MultiFileFlag) ||*/ true || p->ModuleOfPred || !cur_mod ||
	  cur_mod == TermProlog) {
	FUNC_WRITE_UNLOCK(f);
	return AbsPredProp(p);
      }
    }
    if (p->NextOfPE) {
      UInt hash = PRED_HASH(f, cur_mod, PredHashTableSize);
      READ_LOCK(PredHashRWLock);
      p = PredHash[hash];

      while (p) {
	if (p->FunctorOfPred == f && p->ModuleOfPred == cur_mod) {
	  READ_UNLOCK(PredHashRWLock);
	  FUNC_WRITE_UNLOCK(f);
	  return AbsPredProp(p);
	}
	p = p->NextPredOfHash;
      }
      READ_UNLOCK(PredHashRWLock);
    }
    Prop pf =  Yap_NewPredPropByFunctor(f, cur_mod);
    FUNC_WRITE_UNLOCK(f);
    return pf;
  }

  Prop Yap_PredPropByAtomNonThreadLocal(Atom at, Term cur_mod)
  /* get predicate entry for ap/arity; create it if neccessary.              */
  {
    Prop p0;
    AtomEntry *ae = RepAtom(at);

    WRITE_LOCK(ae->ARWLock);
    p0 = ae->PropsOfAE;
    while (p0) {
      PredEntry *pe = RepPredProp(p0);
      if (pe->KindOfPE == PEProp &&
	  (pe->ModuleOfPred == cur_mod || !pe->ModuleOfPred)) {
	/* don't match multi-files */
	if (/*!(pe->PredFlags & MultiFileFlag) ||*/ true || pe->ModuleOfPred || !cur_mod ||
	    cur_mod == TermProlog) {
	  WRITE_UNLOCK(ae->ARWLock);
	  return (p0);
	}
      }
      p0 = pe->NextOfPE;
    }
    return Yap_NewPredPropByAtom(ae, cur_mod);
  }

  Term Yap_GetValue(Atom a) {
    Prop p0 = GetAProp(a, ValProperty);
    Term out;

    if (p0 == NIL)
      return (TermNil);
    READ_LOCK(RepValProp(p0)->VRWLock);
    out = RepValProp(p0)->ValueOfVE;
    if (IsApplTerm(out)) {
      Functor f = FunctorOfTerm(out);
      if (f == FunctorDouble) {
	CACHE_REGS
	  out = MkFloatTerm(FloatOfTerm(out));
      } else if (f == FunctorLongInt) {
	CACHE_REGS
	  out = MkLongIntTerm(LongIntOfTerm(out));
      } else if (f == FunctorString) {
	CACHE_REGS
	  out = MkStringTerm(StringOfTerm(out));
      }
#ifdef USE_GMP
      else {
	out = Yap_MkBigIntTerm(Yap_BigIntOfTerm(out));
      }
#endif
    }
    READ_UNLOCK(RepValProp(p0)->VRWLock);
    return (out);
  }

  void Yap_PutValue(Atom a, Term v) {
    AtomEntry *ae = RepAtom(a);
    Prop p0;
    ValEntry *p;
    Term t0;

    WRITE_LOCK(ae->ARWLock);
    p0 = GetAPropHavingLock(ae, ValProperty);
    if (p0 != NIL) {
      p = RepValProp(p0);
      WRITE_LOCK(p->VRWLock);
      WRITE_UNLOCK(ae->ARWLock);
    } else {
      p = (ValEntry *)Yap_AllocAtomSpace(sizeof(ValEntry));
      if (p == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return;
      }
      p->KindOfPE = ValProperty;
      p->ValueOfVE = TermNil;
      AddPropToAtom(RepAtom(a), (PropEntry *)p);
      /* take care that the lock for the property will be inited even
	 if someone else searches for the property */
      INIT_RWLOCK(p->VRWLock);
      WRITE_LOCK(p->VRWLock);
      WRITE_UNLOCK(ae->ARWLock);
    }
    t0 = p->ValueOfVE;
    if (IsFloatTerm(v)) {
      /* store a float in code space, so that we can access the property */
      union {
	Float f;
	CELL ar[sizeof(Float) / sizeof(CELL)];
      } un;
      CELL *pt, *iptr;
      unsigned int i;

      un.f = FloatOfTerm(v);
      if (IsFloatTerm(t0)) {
	pt = RepAppl(t0);
      } else {
	if (IsApplTerm(t0)) {
	  Yap_FreeCodeSpace((char *)(RepAppl(t0)));
	}
	pt = (CELL *)Yap_AllocAtomSpace(sizeof(CELL) *
					(1 + 2 * sizeof(Float) / sizeof(CELL)));
	if (pt == NULL) {
	  WRITE_UNLOCK(ae->ARWLock);
	  return;
	}
	p->ValueOfVE = AbsAppl(pt);
	pt[0] = (CELL)FunctorDouble;
      }

      iptr = pt + 1;
      for (i = 0; i < sizeof(Float) / sizeof(CELL); i++) {
	*iptr++ = (CELL)un.ar[i];
      }
    } else if (IsLongIntTerm(v)) {
      CELL *pt;
      Int val = LongIntOfTerm(v);

      if (IsLongIntTerm(t0)) {
	pt = RepAppl(t0);
      } else {
	if (IsApplTerm(t0)) {
	  Yap_FreeCodeSpace((char *)(RepAppl(t0)));
	}
	pt = (CELL *)Yap_AllocAtomSpace(2 * sizeof(CELL));
	if (pt == NULL) {
	  WRITE_UNLOCK(ae->ARWLock);
	  return;
	}
	p->ValueOfVE = AbsAppl(pt);
	pt[0] = (CELL)FunctorLongInt;
      }
      pt[1] = (CELL)val;
#ifdef USE_GMP
    } else if (IsBigIntTerm(v)) {
      CELL *ap = RepAppl(v);
      Int sz = sizeof(MP_INT) + sizeof(CELL) +
	(((MP_INT *)(ap + 1))->_mp_alloc * sizeof(mp_limb_t));
      CELL *pt = (CELL *)Yap_AllocAtomSpace(sz);

      if (pt == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return;
      }
      if (IsApplTerm(t0)) {
	Yap_FreeCodeSpace((char *)RepAppl(t0));
      }
      memcpy((void *)pt, (void *)ap, sz);
      p->ValueOfVE = AbsAppl(pt);
#endif
    } else if (IsStringTerm(v)) {
      CELL *ap = RepAppl(v);
      Int sz = sizeof(CELL) * (3 + ap[1]);
      CELL *pt = (CELL *)Yap_AllocAtomSpace(sz);

      if (pt == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return;
      }
      if (IsApplTerm(t0)) {
	Yap_FreeCodeSpace((char *)RepAppl(t0));
      }
      memcpy((void *)pt, (void *)ap, sz);
      p->ValueOfVE = AbsAppl(pt);
    } else {
      if (IsApplTerm(t0)) {
	/* recover space */
	Yap_FreeCodeSpace((char *)(RepAppl(p->ValueOfVE)));
      }
      p->ValueOfVE = v;
    }
    WRITE_UNLOCK(p->VRWLock);
  }

  bool Yap_PutAtomTranslation(Atom a, arity_t arity, Int i) {
    AtomEntry *ae = RepAtom(a);
    Prop p0;
    TranslationEntry *p;

    WRITE_LOCK(ae->ARWLock);
    p0 = GetAPropHavingLock(ae, TranslationProperty);
    if (p0 == NIL) {
      p = (TranslationEntry *)Yap_AllocAtomSpace(sizeof(TranslationEntry));
      if (p == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return false;
      }
      p->KindOfPE = TranslationProperty;
      p->Translation = i;
      p->arity = arity;
      AddPropToAtom(RepAtom(a), (PropEntry *)p);
    }
    /* take care that the lock for the property will be inited even
       if someone else searches for the property */
    WRITE_UNLOCK(ae->ARWLock);
    return true;
  }

  bool Yap_PutFunctorTranslation(Atom a, arity_t arity, Int i) {
    AtomEntry *ae = RepAtom(a);
    Prop p0;
    TranslationEntry *p;

    WRITE_LOCK(ae->ARWLock);
    p0 = GetAPropHavingLock(ae, TranslationProperty);
    if (p0 == NIL) {
      p = (TranslationEntry *)Yap_AllocAtomSpace(sizeof(TranslationEntry));
      if (p == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return false;
      }
      p->KindOfPE = TranslationProperty;
      p->Translation = i;
      p->arity = arity;
      AddPropToAtom(RepAtom(a), (PropEntry *)p);
    }
    /* take care that the lock for the property will be inited even
       if someone else searches for the property */
    WRITE_UNLOCK(ae->ARWLock);
    return true;
  }

  bool Yap_PutAtomMutex(Atom a, void *i) {
    AtomEntry *ae = RepAtom(a);
    Prop p0;
    MutexEntry *p;

    WRITE_LOCK(ae->ARWLock);
    p0 = GetAPropHavingLock(ae, MutexProperty);
    if (p0 == NIL) {
      p = (MutexEntry *)Yap_AllocAtomSpace(sizeof(MutexEntry));
      if (p == NULL) {
	WRITE_UNLOCK(ae->ARWLock);
	return false;
      }
      p->KindOfPE = MutexProperty;
      p->Mutex = i;
      AddPropToAtom(RepAtom(a), (PropEntry *)p);
    }
    /* take care that the lock for the property will be inited even
       if someone else searches for the property */
    WRITE_UNLOCK(ae->ARWLock);
    return true;
  }

  Term Yap_ArrayToList(register Term *tp, size_t nof) {
    CACHE_REGS
      register Term *pt = tp + nof;
    register Term t;

    t = MkAtomTerm(AtomNil);
    while (pt > tp) {
      Term tm = *--pt;
#if YAPOR_SBA
      if (tm == 0)
	t = MkPairTerm((CELL)pt, t);
      else
#endif
	t = MkPairTerm(tm, t);
    }
    return (t);
  }

  int Yap_GetName(char *s, UInt max, Term t) {
    register Term Head;
    register Int i;

    if (IsVarTerm(t) || !IsPairTerm(t))
      return FALSE;
    while (IsPairTerm(t)) {
      Head = HeadOfTerm(t);
      if (!IsNumTerm(Head))
	return (FALSE);
      i = IntOfTerm(Head);
      if (i < 0)
	return FALSE;
      *s++ = i;
      t = TailOfTerm(t);
      if (--max == 0) {
	Yap_Error(SYSTEM_ERROR_FATAL, t, "not enough space for GetName");
      }
    }
    *s = '\0';
    return TRUE;
  }

#ifdef SFUNC

  Term MkSFTerm(Functor f, int n, Term *a, empty_value) {
    Term t, p = AbsAppl(H);
    int i;

    *H++ = f;
    RESET_VARIABLE(H);
    ++H;
    for (i = 1; i <= n; ++i) {
      t = Derefa(a++);
      if (t != empty_value) {
	*H++ = i;
	*H++ = t;
      }
    }
    *H++ = 0;
    return (p);
  }

  CELL *ArgsOfSFTerm(Term t) {
    CELL *p = RepAppl(t) + 1;

    while (*p != (CELL)p)
      p = CellPtr(*p) + 1;
    return (p + 1);
  }

#endif

  static HoldEntry *InitAtomHold(void) {
    HoldEntry *x = (HoldEntry *)Yap_AllocAtomSpace(sizeof(struct hold_entry));
    if (x == NULL) {
      return NULL;
    }
    x->KindOfPE = HoldProperty;
    x->NextOfPE = NIL;
    x->RefsOfPE = 1;
    return x;
  }

  int Yap_AtomIncreaseHold(Atom at) {
    AtomEntry *ae = RepAtom(at);
    HoldEntry *pp;
    Prop *opp = &(ae->PropsOfAE);

    WRITE_LOCK(ae->ARWLock);
    pp = RepHoldProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp) && pp->KindOfPE != HoldProperty) {
      opp = &(pp->NextOfPE);
      pp = RepHoldProp(pp->NextOfPE);
    }
    if (!pp) {
      HoldEntry *new = InitAtomHold();
      if (!new) {
	WRITE_UNLOCK(ae->ARWLock);
	return FALSE;
      }
      *opp = AbsHoldProp(new);
    } else {
      pp->RefsOfPE++;
    }
    WRITE_UNLOCK(ae->ARWLock);
    return TRUE;
  }

  int Yap_AtomDecreaseHold(Atom at) {
    AtomEntry *ae = RepAtom(at);
    HoldEntry *pp;
    Prop *opp = &(ae->PropsOfAE);

    WRITE_LOCK(ae->ARWLock);
    pp = RepHoldProp(ae->PropsOfAE);
    while (!EndOfPAEntr(pp) && pp->KindOfPE != HoldProperty) {
      opp = &(pp->NextOfPE);
      pp = RepHoldProp(pp->NextOfPE);
    }
    if (!pp) {
      WRITE_UNLOCK(ae->ARWLock);
      return FALSE;
    }
    pp->RefsOfPE--;
    if (!pp->RefsOfPE) {
      *opp = pp->NextOfPE;
      Yap_FreeCodeSpace((ADDR)pp);
    }
    WRITE_UNLOCK(ae->ARWLock);
    return TRUE;
  }

  const char *IndicatorOfPred(PredEntry *pe) {
    const char *mods;
    Atom at;
    arity_t arity;
    if (pe->ModuleOfPred == IDB_MODULE) {
      mods = "idb";
      if (pe->PredFlags & NumberDBPredFlag) {
    char *   buf = malloc(MAX_PATH+1);
	snprintf(buf, MAX_PATH, "idb:" UInt_FORMAT,
		 (Int)(pe->FunctorOfPred));
	return buf;
      } else if (pe->PredFlags & AtomDBPredFlag) {
	at = (Atom)pe->FunctorOfPred;
	arity = 0;
      } else {
	at = NameOfFunctor(pe->FunctorOfPred);
	arity = ArityOfFunctor(pe->FunctorOfPred);
      }
    } else {
      if (pe->ModuleOfPred == 0)
	mods = "prolog";
      else
	mods = RepAtom(AtomOfTerm(pe->ModuleOfPred))->StrOfAE;
      arity = pe->ArityOfPE;
      if (arity == 0) {
	at = (Atom)pe->FunctorOfPred;
      } else {
	at = NameOfFunctor(pe->FunctorOfPred);
      }
    }
    char *   buf = malloc(MAX_PATH+1);
    snprintf(buf, MAX_PATH, "%s:%s/" UInt_FORMAT, mods,
	     RepAtom(at)->StrOfAE, arity);
    return buf;
  }
