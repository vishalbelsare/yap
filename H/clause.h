/*************************************************************************
*									 *
*	 YAP Prolog 	%W% %G%		*
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		clause.h						 *
* Last rev:								 *
* mods:									 *
* comments:	clause info						 *
*									 *
*************************************************************************/

#ifndef CLAUSE_H
#define CLAUSE_H 1

#include "YapHeap.h"

#include "Yatom.h"


/* Either we are assembling clauses or indexing code */

#define ASSEMBLING_CLAUSE 0
#define ASSEMBLING_INDEX 1
#define ASSEMBLING_EINDEX 2

#define NextDynamicClause(X) (((yamop *)X)->y_u.Otapl.d)

#define PredFirstClause 0
#define PredMiddleClause 1
#define PredLastClause 2

typedef struct logic_upd_index {
  dbentry_flags ClFlags;
  UInt ClRefCount;
#if defined(YAPOR) || defined(THREADS)
/* A lock for manipulating the clause */
//  lockvar          ClLock;
#endif
  UInt ClSize;
  struct logic_upd_index *ParentIndex;
  struct logic_upd_index *SiblingIndex;
  struct logic_upd_index *PrevSiblingIndex;
  struct logic_upd_index *ChildIndex;
  /* The instructions, at least one of the form sl */
  PredEntry *ClPred;
  yamop ClCode[MIN_ARRAY];
} LogUpdIndex;

/* The ordering of the first 3 fields should be compatible with dbrefs */
typedef struct logic_upd_clause {
  Functor Id; /* allow pointers to this struct to id  */
              /*   as dbref                           */
  /* A set of flags describing info on the clause */
  /* A set of flags describing info on the clause */
  dbentry_flags   ClFlags;
#if defined(YAPOR) || defined(THREADS)
/* A lock for manipulating the clause */
//  lockvar          ClLock;
#endif
  UInt ClSize;
  /* extra clause information for logical update indices and facts */
  /* indices that may still backtrack to this clause */
  UInt ClRefCount;
  /* data for clauses  with environments */
  yamop *ClExt;
  union {
    DBTerm *ClSource;
    Int ClLine;
  } lusl;
  Atom ClOwner;
  /* doubly linked list of clauses */
  struct logic_upd_clause *ClPrev, *ClNext;
  /* parent pointer */
  PredEntry *ClPred;
  UInt ClTimeStart, ClTimeEnd;
  /* The instructions, at least one of the form sl */
  yamop ClCode[MIN_ARRAY];
} LogUpdClause;

#include "inline-only.h"
INLINE_ONLY int VALID_TIMESTAMP(UInt, struct logic_upd_clause *);

INLINE_ONLY int VALID_TIMESTAMP(UInt timestamp,
                                              struct logic_upd_clause *cl) {
  return IN_BETWEEN(cl->ClTimeStart, timestamp, cl->ClTimeEnd);
}

typedef struct dynamic_clause {
  /* A set of flags describing info on the clause */
  dbentry_flags ClFlags;
#if defined(YAPOR) || defined(THREADS)
  /* A lock for manipulating the clause */
  lockvar ClLock;
#endif
  UInt ClSize;
  Int ClLine;
  UInt ClRefCount;
  yamop *ClPrevious; /* immediate update clause */
  /* The instructions, at least one of the form sl */
  yamop ClCode[MIN_ARRAY];
} DynamicClause;

typedef struct static_index {
  /* A set of flags describing info on the clause */
  dbentry_flags   ClFlags;
  UInt ClSize;
  struct static_index *SiblingIndex;
  struct static_index *ChildIndex;
  /* The instructions, at least one of the form sl */
  PredEntry *ClPred;
  yamop ClCode[MIN_ARRAY];
} StaticIndex;

typedef struct static_clause {
  /* A set of flags describing info on the clause */
  dbentry_flags ClFlags;
  UInt ClSize;
  struct  {
    DBTerm *ClSource;
    Int ClLine;
  } usc;
  Atom ClOwner;
  PredEntry *ClPred;
  struct static_clause *ClNext;
  /* The instructions, at least one of the form sl */
  yamop ClCode[MIN_ARRAY];
} StaticClause;

typedef struct static_mega_clause {
  /* A set of flags describing info on the clause */
  dbentry_flags ClFlags;
  UInt ClSize;
  PredEntry *ClPred;
  UInt ClItemSize;
  Atom ClOwner;
  Int ClLine;
  struct static_mega_clause *ClNext;
  /* The instructions, at least one of the form sl */
  yamop ClCode[MIN_ARRAY];
} MegaClause;

typedef union clause_obj {
  struct logic_upd_clause luc;
  struct logic_upd_index lui;
  struct dynamic_clause ic;
  struct static_clause sc;
  struct static_mega_clause mc;
  struct static_index si;
  PredEntry pe;
} ClauseUnion;

typedef union clause_ptr {
  struct logic_upd_clause *luc;
  struct logic_upd_index *lui;
  struct dynamic_clause *ic;
  struct static_clause *sc;
  struct static_mega_clause *mc;
  struct static_index *si;
  PredEntry *pe;
  } ClausePointer;

typedef struct index_t {
  struct index_t *next, *prev;
  UInt nels;
  UInt arity;
  PredEntry *ap;
  CELL bmap;
  int is_key;
  int is_udi;
  UInt ncollisions;
  UInt max_col_count;
  UInt ntrys;
  UInt nentries;
  UInt hsize;
  BITS32 *key;
  CELL *cls, *bcls;
  BITS32 *links;
  size_t size;
  yamop *code;
  BITS32 *udi_data;
  void *udi_first, *udi_next;
  UInt udi_free_args;
  UInt udi_arg;
} Index_t;

INLINE_ONLY  BITS32 EXO_ADDRESS_TO_OFFSET(struct index_t *it,
                                                       CELL *ptr);

INLINE_ONLY  BITS32 EXO_ADDRESS_TO_OFFSET(struct index_t *it,
                                                       CELL *ptr) {
  return (ptr - it->cls) / it->arity + 1;
}

INLINE_ONLY  CELL *EXO_OFFSET_TO_ADDRESS(struct index_t *it,
                                                      BITS32 off);

INLINE_ONLY  CELL *EXO_OFFSET_TO_ADDRESS(struct index_t *it,
                                                      BITS32 off) {
  if (off == 0L)
    return (CELL *)NULL;
  return (it->cls) + (off - 1) * it->arity;
}


INLINE_ONLY  BITS32 ADDRESS_TO_LINK(struct index_t *it,
                                                 BITS32 *ptr) {
  return ptr - it->links;
}


INLINE_ONLY  BITS32 *LINK_TO_ADDRESS(struct index_t *it,
                                                  BITS32 off) {
  return it->links + off;
}

typedef void (*CRefitExoIndex)(struct index_t **ip, UInt b[] USES_REGS);
typedef yamop *(*CEnterExoIndex)(struct index_t *it USES_REGS);
typedef int (*CRetryExoIndex)(struct index_t *it USES_REGS);

typedef struct dbterm_list {
  /* a list of dbterms associated with a clause */
  DBTerm *dbterms;
  yamop *clause_code;
  PredEntry *p;
  struct dbterm_list *next_dbl;
} DBTermList;

#define ClauseCodeToDynamicClause(p)                                           \
  ((DynamicClause *)((CODEADDR)(p) - (CELL)(((DynamicClause *)NULL)->ClCode)))
#define ClauseCodeToStaticClause(p)                                            \
  ((StaticClause *)((CODEADDR)(p) - (CELL)(((StaticClause *)NULL)->ClCode)))
#define ClauseCodeToLogUpdClause(p)                                            \
  ((LogUpdClause *)((CODEADDR)(p) - (CELL)(((LogUpdClause *)NULL)->ClCode)))
#define ClauseCodeToMegaClause(p)                                              \
  ((MegaClause *)((CODEADDR)(p) - (CELL)(((MegaClause *)NULL)->ClCode)))
#define ClauseCodeToLogUpdIndex(p)                                             \
  ((LogUpdIndex *)((CODEADDR)(p) - (CELL)(((LogUpdIndex *)NULL)->ClCode)))
#define ClauseCodeToStaticIndex(p)                                             \
  ((StaticIndex *)((CODEADDR)(p) - (CELL)(((StaticIndex *)NULL)->ClCode)))

#define ClauseFlagsToDynamicClause(p) ((DynamicClause *)(p))
#define ClauseFlagsToLogUpdClause(p)                                           \
  ((LogUpdClause *)((CODEADDR)(p) - (CELL)(&(((LogUpdClause *)NULL)->ClFlags))))
#define ClauseFlagsToLogUpdIndex(p)                                            \
  ((LogUpdIndex *)((CODEADDR)(p) - (CELL)(&(((LogUpdIndex *)NULL)->ClFlags))))
#define ClauseFlagsToStaticClause(p) ((StaticClause *)(p))

#define DynamicFlags(X) (ClauseCodeToDynamicClause(X)->ClFlags)

#define DynamicLock(X) (ClauseCodeToDynamicClause(X)->ClLock)

#if MULTIPLE_STACKS
#define INIT_CLREF_COUNT(X) (X)->ClRefCount = 0
#define INC_CLREF_COUNT(X) (X)->ClRefCount++
#define DEC_CLREF_COUNT(X) (X)->ClRefCount--

#define CL_IN_USE(X) ((X)->ClRefCount)
#else
#define INIT_CLREF_COUNT(X)
#define INC_CLREF_COUNT(X)
#define DEC_CLREF_COUNT(X)
#define CL_IN_USE(X) ((X)->ClFlags & InUseMask || (X)->ClRefCount)
#endif

/* amasm.c */
wamreg Yap_emit_x(CELL);
COUNT Yap_compile_cmp_flags(PredEntry *);

/* cdmgr.c */
extern void Yap_IPred(PredEntry *, UInt, yamop *);
extern bool Yap_addclause(PredEntry *p, Term, yamop *, Term, Term, Term *);
extern void Yap_add_logupd_clause(PredEntry *, LogUpdClause *, int);
extern void Yap_kill_iblock(ClauseUnion *, ClauseUnion *, PredEntry *);
extern void Yap_EraseStaticClause(StaticClause *, PredEntry *, Term);
extern ClausePointer Yap_find_owner_index(yamop *, PredEntry *);

/* dbase.c */
extern void Yap_ErCl(DynamicClause *);
extern void Yap_ErLogUpdCl(LogUpdClause *);
extern void Yap_ErLogUpdIndex(LogUpdIndex *);
extern Int Yap_Recordz(Atom, Term);
extern Int Yap_db_nth_recorded(PredEntry *, Int USES_REGS);
extern Int Yap_unify_immediate_ref(DBRef ref USES_REGS);

/* exec.c */
extern Term Yap_cp_as_integer(choiceptr);

/* index.c */
extern yamop *Yap_PredIsIndexable(PredEntry *, UInt, yamop *);
extern yamop *Yap_ExpandIndex(PredEntry *, UInt);
extern void Yap_CleanUpIndex(struct logic_upd_index *);
extern void Yap_CleanKids(struct logic_upd_index *);
extern void Yap_AddClauseToIndex(PredEntry *, yamop *, int);
extern void Yap_RemoveClauseFromIndex(PredEntry *, yamop *);
extern LogUpdClause *Yap_NthClause(PredEntry *, Int);
extern LogUpdClause *Yap_FollowIndexingCode(PredEntry *, yamop *, yhandle_t, yamop *,
                                     yamop *);
extern PredEntry *Yap_pc_to_pred(yamop *ip);

/* exo.c */
extern yamop *Yap_ExoLookup(PredEntry *ap USES_REGS);
extern CELL Yap_NextExo(choiceptr cpt, struct index_t *it);

#
#if USE_THREADED_CODE

#define OP_HASH_SIZE 2048



/*************************************************************************************************
                                   reverse lookup of instructions
*************************************************************************************************/
typedef struct opcode_optab_entry {
    OPCODE opc;
    op_numbers opnum;
} op_entry;


INLINE_ONLY int rtable_hash_op(OPCODE opc, int hash_mask) {
  return ((((CELL)opc) >> 3) & hash_mask);
}


/* given an opcode find the corresponding opnumber. This should make
   switches on ops a much easier operation */
INLINE_ONLY  op_numbers Yap_op_from_opcode(OPCODE opc) {
  int j = rtable_hash_op(opc, OP_HASH_SIZE - 1);

  while (OP_RTABLE[j].opc != opc) {
    if (!OP_RTABLE[j].opc)
      return _Nstop;
    if (j == OP_HASH_SIZE - 1) {
      j = 0;
    } else {
      j++;
    }
  }
  return OP_RTABLE[j].opnum;
}
#else
INLINE_ONLY OPCODE op_numbers Yap_op_from_opcode(OPCODE opc) {
  return ((op_numbers)opc);
}
#endif /* USE_THREADED_CODE */

#if defined(YAPOR) || defined(THREADS)

static inline int same_lu_block(yamop **paddr, yamop *p) {
  yamop *np = *paddr;
  if (np != p) {
    OPCODE jmp_op = Yap_opcode(_jump_if_nonvar);

    while (np->opc == jmp_op) {
      np = NEXTOP(np, xll);
      if (np == p)
        return TRUE;
    }
    return FALSE;
  } else {
    return TRUE;
  }
}
#endif

#define Yap_MkStaticRefTerm(cp, ap) __Yap_MkStaticRefTerm((cp), (ap)PASS_REGS)

static inline Term __Yap_MkStaticRefTerm(StaticClause *cp,
                                         PredEntry *ap USES_REGS) {
  Term t[2];
  t[0] = MkIntegerTerm((Int)cp);
  t[1] = MkIntegerTerm((Int)ap);
  return Yap_MkApplTerm(FunctorStaticClause, 2, t);
}

INLINE_ONLY StaticClause *Yap_ClauseFromTerm(Term t) {
  return (StaticClause *)IntegerOfTerm(ArgOfTerm(1, t));
}

#define Yap_MkMegaRefTerm(ap, ipc) __Yap_MkMegaRefTerm((ap), (ipc)PASS_REGS)

INLINE_ONLY  Term __Yap_MkMegaRefTerm(PredEntry *ap, yamop *ipc USES_REGS) {
  Term t[2];
  t[0] = MkIntegerTerm((Int)ap);
  t[1] = MkIntegerTerm((Int)ipc);
  return Yap_MkApplTerm(FunctorMegaClause, 2, t);
}

INLINE_ONLY  yamop *Yap_MegaClauseFromTerm(Term t) {
  return (yamop *)IntegerOfTerm(ArgOfTerm(2, t));
}

INLINE_ONLY  PredEntry *Yap_MegaClausePredicateFromTerm(Term t) {
  return (PredEntry *)IntegerOfTerm(ArgOfTerm(1, t));
}

#define Yap_MkExoRefTerm(ap, i) __Yap_MkExoRefTerm((ap), (i)PASS_REGS)

INLINE_ONLY  Term __Yap_MkExoRefTerm(PredEntry *ap, Int i USES_REGS) {
  Term t[2];
  t[0] = MkIntegerTerm((Int)ap);
  t[1] = MkIntegerTerm((Int)i);
  return Yap_MkApplTerm(FunctorExoClause, 2, t);
}

INLINE_ONLY  Int Yap_ExoClauseFromTerm(Term t) {
  return IntegerOfTerm(ArgOfTerm(2, t));
}

INLINE_ONLY  PredEntry *Yap_ExoClausePredicateFromTerm(Term t) {
  return (PredEntry *)IntegerOfTerm(ArgOfTerm(1, t));
}

/******************************************************************

                        EXECUTING PROLOG CLAUSES

******************************************************************/

static inline bool Yap_static_in_use(PredEntry *p, bool check_everything) {
#if defined(YAPOR) || defined(THREADS)
  return TRUE;
#else
  pred_flags_t pflags = p->PredFlags;
  if (pflags & (DynamicPredFlag | LogUpdatePredFlag)) {
    return FALSE;
  }
  if (STATIC_PREDICATES_MARKED) {
    return (p->PredFlags & InUsePredFlag);
  } else {
    /* This code does not work for YAPOR or THREADS!!!!!!!! */
    return Yap_search_for_static_predicate_in_use(p, check_everything);
  }
#endif

#define DEAD_REF(ref) FALSE
}

typedef enum {
  FIND_PRED_FROM_ANYWHERE,
  FIND_PRED_FROM_CP,
  FIND_PRED_FROM_ENV,
  FIND_PRED_FROM_CLAUSE
} find_pred_type;

extern  PredEntry * Yap_PredForCode(yamop *, find_pred_type,ClausePointer* );

extern PredEntry *Yap_PredEntryForCode(choiceptr, yamop *, find_pred_type);
extern LogUpdClause *Yap_new_ludbe(Term, PredEntry *, UInt);
extern Term Yap_LUInstance(LogUpdClause *, UInt);

/* udi.c */
extern int Yap_new_udi_clause(PredEntry *, yamop *, Term);
extern yamop *Yap_udi_search(PredEntry *);

extern Term Yap_bug_location(yamop *p, yamop *cp, choiceptr b_ptr, CELL *env);
extern Term Yap_pc_location(yamop *p, choiceptr b_ptr, CELL *env);
extern Term Yap_env_location(yamop *p, choiceptr b_ptr, CELL *env, Int ignore_first);

#if LOW_PROF
extern void Yap_InformOfRemoval(void *);
extern void Yap_dump_code_area_for_profiler(void);
#else
#define Yap_InformOfRemoval(X)
#endif

static inline void clean_tr(tr_fr_ptr TR0 USES_REGS) {
  tr_fr_ptr pt0 = TR;
  while (pt0 != TR0) {
    Term p = TrailTerm(--pt0);
    if (IsApplTerm(p)) {
      CELL *pt = RepAppl(p);
#ifdef FROZEN_STACKS
      pt[0] = TrailVal(pt0);
#else
      pt[0] = TrailTerm(pt0 - 1);
      pt0--;
#endif /* FROZEN_STACKS */
    } else {
      RESET_VARIABLE(p);
    }
  }
  TR = TR0;
}

#include "alloc.h"
#include "terms.h"

extern  PredEntry * Yap_track_cpred( op_numbers opcode, yamop *p, size_t min, void *i );
extern Term Yap_protect_goal(PredEntry **pe0, Term t,Term mod,  Term t0);
extern bool Yap_Compile(Term t, Term t1, Term tsrc, Term mod, Term pos, Term tref USES_REGS);

#endif
