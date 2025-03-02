/*************************************************************************
*									 *
*	 YAP Prolog 	%W% %G% 					 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		TermExt.h						 *
* mods:									 *
* comments:	Extensions to standard terms for YAP			 *
* version:      $Id: TermExt.h,v 1.15 2008-03-25 22:03:13 vsc Exp $	 *
*************************************************************************/
/**

@file TermExt.h

 */
#ifndef DOXYGEN

#ifndef TERMEXT_H_INCLUDED
#define TERMEXT_H_INCLUDED

#pragma once

#ifdef USE_SYSTEM_MALLOC
#define SF_STORE (&(Yap_heap_regs->funcs))
#else
#define SF_STORE ((special_functors *)HEAP_INIT_BASE)
#endif


#if 1
extern Atom AtomFoundVar, AtomFreeTerm, AtomNil, AtomDot;
#elif defined(USE_OFFSETS)
#define AtomFoundVar ((Atom)(&(((special_functors *)(NULL))->AtFoundVar)))
#define AtomFreeTerm ((Atom)(&(((special_functors *)(NULL))->AtFreeTerm)))
#define AtomNil ((Atom)(&(((special_functors *)(NULL))->AtNil)))
#define AtomDot ((Atom)(&(((special_functors *)(NULL))->AtDot)))
#elif OLD_STYLE_INITIAL_ATOMS
#define AtomFoundVar AbsAtom((AtomEntry *)&(SF_STORE->AtFoundVar))
#define AtomFreeTerm AbsAtom((AtomEntry *)&(SF_STORE->AtFreeTerm))
#define AtomNil AbsAtom((AtomEntry *)&(SF_STORE->AtNil))
#define AtomDot AbsAtom((AtomEntry *)&(SF_STORE->AtDot))
#else
#define AtomFoundVar AbsAtom(SF_STORE->AtFoundVar)
#define AtomFreeTerm AbsAtom(SF_STORE->AtFreeTerm)
#define AtomNil AbsAtom(SF_STORE->AtNil)
#define AtomDot AbsAtom(SF_STORE->AtDot)
#endif

#define TermFoundVar MkAtomTerm(AtomFoundVar)
#define TermFreeTerm MkAtomTerm(AtomFreeTerm)
#define TermNil MkAtomTerm(AtomNil)
#define TermDot MkAtomTerm(AtomDot)

typedef enum {
  db_ref_e = sizeof(Functor *),
  blob_e = 2 * sizeof(Functor *),
  double_e = 3 * sizeof(Functor *),
  long_int_e = 4 * sizeof(Functor *),
  big_int_e = 5 * sizeof(Functor *),
    string_e = 6 * sizeof(Functor *)
} blob_type;
#define end_e (8  * sizeof(Functor *))
                                                                                            
#define FunctorDBRef ((Functor)(db_ref_e))
#define FunctorDouble ((Functor)(double_e))
#define FunctorLongInt ((Functor)(long_int_e))
#define FunctorBigInt ((Functor)(big_int_e))
#define FunctorString ((Functor)(string_e))
#define FunctorBlob   ((Functor)(blob_e))

#include "inline-only.h"

typedef enum {
  BIG_INT = 0x01,
  BIG_RATIONAL = 0x02,
  BIG_FLOAT = 0x04,
 EMPTY_ARENA = 0x10,
  MATRIX_INT = 0x21,
  MATRIX_FLOAT = 0x22,
  CLAUSE_LIST = 0x40,
  EXTERNAL_BLOB = 0x0A0,    /* generic data */
  GOAL_CUT_POINT = 0x0A1,
  USER_BLOB_START = 0x0100, /* user defined blob */
  USER_BLOB_END = 0x0200    /* end of user defined blob */
} big_blob_type;

INLINE_ONLY blob_type BlobOfFunctor(Functor f);

INLINE_ONLY blob_type BlobOfFunctor(Functor f) {
  return (blob_type)((CELL)f);
}

#ifdef COROUTINING

typedef struct {
  /* what to do when someone tries to bind our term to someone else
hv     in some  predefined context */
  void (*bind_op)(Term *, Term CACHE_TYPE);
  /* what to do if someone wants to copy our constraint */
  int (*copy_term_op)(CELL *, void*, CELL *CACHE_TYPE);
  /* copy the constraint into a term and back */
  Term (*to_term_op)(CELL *);
  int (*term_to_op)(Term, Term CACHE_TYPE);
  /* op called to do marking in GC */
  void (*mark_op)(CELL *);
} ext_op;

/* known delays */
typedef enum {
  empty_ext = 0 * sizeof(ext_op),  /* default op, this should never be called */
  attvars_ext = 1 * sizeof(ext_op) /* support for attributed variables */
                                   /* add your own extensions here */
                                   /* keep this one */
} exts;

#endif

#define CloseExtension(x) MkAtomTerm((Atom)(x))

#define GetStartOfExtension(x) ((CELL*)AtomOfTerm(*x))

inline static     bool IsEndExtension__(CELL *x USES_REGS) {
  CELL c = *x;
 if (!IsAtomTerm(c)) return false;
 Atom a = AtomOfTerm(c);
 CELL *ca =  (CELL*)a;
 if (ca < H0 || ca >= HR)
   return false;
 // if (!IsExtensionFunctor((Functor)ca[0]))
 //  return false;
 return true;
}

#define  IsEndExtension(x )  IsEndExtension__(x PASS_REGS)

#if defined(YAP_H)
/* make sure that these data structures are the first thing to be allocated
   in the heap when we start the system */
typedef struct special_functors_struct {

#if 0
  struct ExtraAtomEntryStruct AtFoundVar;
  struct ExtraAtomEntryStruct AtFreeTerm;
  struct ExtraAtomEntryStruct AtNil;
  struct ExtraAtomEntryStruct AtDot;
#else
  struct AtomEntryStruct *AtFoundVar;
  struct AtomEntryStruct *AtFreeTerm;
  struct AtomEntryStruct *AtNil;
  struct AtomEntryStruct *AtDot;
#endif
} special_functors;
#endif /* YAP_H */


extern size_t
SizeOfOpaqueTerm(Term *next, CELL cnext);

INLINE_ONLY Float CpFloatUnaligned(CELL *ptr);


#define MkFloatTerm(fl) __MkFloatTerm((fl)PASS_REGS)

#define MASK32B (((CELL)1<<32)-1)
 

INLINE_ONLY Term __MkFloatTerm(Float dbl USES_REGS) {
    union float_words {
       double val;
      uint64_t s;
    } u;
    u.val = dbl;
    HR[0] = (CELL)FunctorDouble;
    HR[1] = ( (u.s & MASK32B)<<8) |NumberTag;
    HR[2] = ((u.s & (MASK32B<<32))>>8) |NumberTag;
    HR+=3;
    return AbsAppl(HR-3);
}


INLINE_ONLY Float FloatOfTerm(Term t) {
    union {
       double val;
      uint64_t s;
    } u;
    CELL* pt = RepAppl(t);
    u.s = ((pt[1]>>8)&MASK32B)|((pt[2]<<8)&(MASK32B<<32));
    return u.val;
}




#endif

#ifndef YAP_H
#include <stddef.h>
#endif

INLINE_ONLY bool IsLongIntTerm(Term);

INLINE_ONLY bool IsFloatTerm(Term t) {
  return (int)(IsApplTerm(t) && FunctorOfTerm(t) == FunctorDouble);
}

/* extern Functor FunctorLongInt; */


#define MkLongIntTerm(fl) __MkLongIntTerm((fl)PASS_REGS)

INLINE_ONLY Term __MkLongIntTerm(Int x  USES_REGS) {
    union float_words {
       double val;
      uint64_t s;
    } u;
    u.val = x;
    HR[0] = (CELL)FunctorLongInt;
    HR[1] = ( (u.s & MASK32B)<<8) |NumberTag;
    HR[2] = ((u.s & (MASK32B<<32))>>8) |NumberTag;
    HR+=3;
    return AbsAppl(HR-3);
}

INLINE_ONLY Int LongIntOfTerm(Term t) {
    union {
       double val;
      Int s;
    } u;
    CELL* pt = RepAppl(t);
    u.s = ((pt[1]>>8)&MASK32B)|((pt[2]<<8)&(MASK32B<<32));
    return u.val;
}

INLINE_ONLY bool IsLongIntTerm(Term t) {
  return IsApplTerm(t) &&
          FunctorOfTerm(t) == FunctorLongInt;
}

/****************************************************/

/*********** strings, coded as UTF-8 ****************/

#include <string.h>

/* extern Functor FunctorString; */

#define MkStringTerm(i) __MkStringTerm((i)PASS_REGS)
// < functor, request (size in cells ), cells, eot >
INLINE_ONLY Term
__MkStringTerm(const  char *s USES_REGS);

INLINE_ONLY Term __MkStringTerm(const char *s USES_REGS) {
    Term t = AbsAppl(HR);
    size_t sz = (s[0] == '\0' ? 1 : strlen((const char *) s) + 1);
    size_t request = (sz + CELLSIZE - 1) / CELLSIZE; // request is in cells >= 1
    HR[0] = (CELL) FunctorString;
    HR[1] = request;
    HR[1 + request] = 0;
    memcpy((HR + 2), s, sz);
    HR[2 + request] = CloseExtension(HR);
    HR += 3 + request;
    return t;
}

#define MkUStringTerm(i) __MkStringTerm((const char *)(i)PASS_REGS)


INLINE_ONLY const unsigned char *UStringOfTerm(Term t);

INLINE_ONLY const unsigned char *UStringOfTerm(Term t) {
  return (const unsigned char *)(RepAppl(t) + 2);
}

INLINE_ONLY const char *StringOfTerm(Term t);

INLINE_ONLY const char *StringOfTerm(Term t) {
  return (const char *)(RepAppl(t) + 2);
}

INLINE_ONLY bool IsStringTerm(Term);

INLINE_ONLY bool IsStringTerm(Term t) {
  return IsApplTerm(t) &&
          FunctorOfTerm(t) == FunctorString;
}


/****************************************************/

#ifdef USE_GMP

#include <stdio.h>

#else

typedef UInt mp_limb_t;

typedef struct {
  Int _mp_size, _mp_alloc;
  mp_limb_t *_mp_d;
} MP_INT;

typedef struct {
  MP_INT _mp_num;
  MP_INT _mp_den;
} MP_RAT;

#endif

INLINE_ONLY bool IsBigIntTerm(Term t) {
  return IsApplTerm(t) &&
          FunctorOfTerm(t) == FunctorBigInt;
}

INLINE_ONLY bool IsBlobTerm(Term t) {
  return IsApplTerm(t) &&
          FunctorOfTerm(t) == FunctorBlob;
}

#ifdef USE_GMP

Term Yap_MkBigIntTerm(MP_INT *);
MP_INT *Yap_BigIntOfTerm(Term);

Term Yap_MkBigRatTerm(MP_RAT *);
MP_RAT *Yap_BigRatOfTerm(Term);

INLINE_ONLY void MPZ_SET(mpz_t, MP_INT *);

INLINE_ONLY void MPZ_SET(mpz_t dest, MP_INT *src) {
  dest->_mp_size = src->_mp_size;
  dest->_mp_alloc = src->_mp_alloc;
  dest->_mp_d = src->_mp_d;
}

INLINE_ONLY bool IsLargeIntTerm(Term);

INLINE_ONLY bool IsLargeIntTerm(Term t) {
  return IsApplTerm(t) &&
          ((FunctorOfTerm(t) <= FunctorBigInt) &&
           (FunctorOfTerm(t) >= FunctorLongInt));
}



/**
 *
 * @param t input extension term: long ints, bignums, rationals, matrices and
 *  opaque terms.
 * @return number of cells taken by the representation of the term.
 */
INLINE_ONLY UInt Yap_SizeOfBigInt(Term t)  {

  CELL *pt = RepAppl(t) + 1;
  if (pt[0 ]  == BIG_RATIONAL) {
  return 2 +
    (sizeof(MP_INT) + (((MP_INT *)(pt+1))->_mp_alloc * sizeof(mp_limb_t))) /
         sizeof(CELL)+
         (sizeof(MP_INT) + ((((MP_INT *)(pt+1))+1)->_mp_alloc * sizeof(mp_limb_t))) /
         sizeof(CELL);

  }
  return 2 +
         (sizeof(MP_INT) + (((MP_INT *)(pt+1))->_mp_alloc * sizeof(mp_limb_t))) /
             sizeof(CELL);
}

#else

INLINE_ONLY int IsLargeIntTerm(Term);

INLINE_ONLY int IsLargeIntTerm(Term t) {
  return (int)(IsApplTerm(t) && FunctorOfTerm(t) == FunctorLongInt);
}

#endif

/* extern Functor FunctorLongInt; */

INLINE_ONLY bool IsLargeNumTerm(Term);

INLINE_ONLY bool IsLargeNumTerm(Term t) {
  return IsApplTerm(t) &&
          ((FunctorOfTerm(t) <= FunctorBigInt) &&
           (FunctorOfTerm(t) >= FunctorDouble));
}

INLINE_ONLY bool IsExternalBlobTerm(Term, CELL);

INLINE_ONLY bool IsExternalBlobTerm(Term t, CELL tag) {
  return IsApplTerm(t) &&
          FunctorOfTerm(t) == FunctorBlob &&
          RepAppl(t)[1] == tag;
}

INLINE_ONLY void *ExternalBlobFromTerm(Term);

INLINE_ONLY void *ExternalBlobFromTerm(Term t) {
  return RepAppl(t)+3;
}

INLINE_ONLY bool IsNumTerm(Term);

INLINE_ONLY bool IsNumTerm(Term t) {
  return (IsIntTerm(t) || IsLargeNumTerm(t));
}

INLINE_ONLY bool IsAtomicTerm(Term);

INLINE_ONLY bool IsAtomicTerm(Term t) {
  return IsAtomOrIntTerm(t) ||
          IsLargeNumTerm(t) ||
          IsStringTerm(t);
}

INLINE_ONLY bool IsExtensionFunctor(Functor);

INLINE_ONLY bool IsExtensionFunctor(Functor f) {
  return f <= (Functor)end_e;
}

INLINE_ONLY bool IsBlobFunctor(Functor);

INLINE_ONLY bool IsBlobFunctor(Functor f) {
  return (f <= FunctorString &&
          f >= FunctorDBRef);
}

INLINE_ONLY bool IsPrimitiveTerm(Term);

INLINE_ONLY bool IsPrimitiveTerm(Term t) {
  return (IsAtomOrIntTerm(t) ||
          (IsApplTerm(t) &&
                  IsBlobFunctor(FunctorOfTerm(t))));
}

INLINE_ONLY exts ExtFromCell(CELL *);

INLINE_ONLY exts ExtFromCell(CELL *pt) { return attvars_ext; }

INLINE_ONLY Int Yap_BlobTag(Term t);

INLINE_ONLY Int Yap_BlobTag(Term t) {
  CELL *pt = RepAppl(t);

  return pt[1];
}

INLINE_ONLY void *Yap_BlobInfo(Term t);

INLINE_ONLY void *Yap_BlobInfo(Term t) {
  CELL *pt = RepAppl(t);

  return pt+3;
}

#ifdef YAP_H

INLINE_ONLY bool unify_extension(Functor, CELL, CELL *, CELL);

EXTERN bool unify_extension(Functor, CELL, CELL *, CELL);

int Yap_gmp_tcmp_big_big(Term, Term);

INLINE_ONLY bool unify_extension(Functor f, CELL d0, CELL *pt0, CELL d1)
{
  switch (BlobOfFunctor(f)) {
  case db_ref_e:
    return (d0 == d1);
  case long_int_e:
    return (pt0[1] == RepAppl(d1)[1] && pt0[2] == RepAppl(d1)[2] );
  case blob_e:
    return (pt0[2] == RepAppl(d1)[2] && !memcmp(pt0+3, RepAppl(d1)+3, pt0[2]*sizeof(CELL) ) );
  case string_e:
    return strcmp((char *)(pt0 + 2), (char *)(RepAppl(d1) + 2)) == 0;
  case big_int_e:
#ifdef USE_GMP
    return (Yap_gmp_tcmp_big_big(d0, d1) == 0);
#else
    return d0 == d1;
#endif /* USE_GMP */
  case double_e: {
    CELL *pt1 = RepAppl(d1);
    return pt0[1] == pt1[1] &&
	    pt0[2] == pt1[2]
#if SIZEOF_DOUBLE == 2 * SIZEOF_INT_P
      &&
	    pt0[3] == pt1[3] &&
            pt0[4] == pt1[4]
#endif
            ;
  }
  }
	    return false;
}

static inline CELL Yap_IntP_key(CELL *pt) {
#ifdef USE_GMP
  if (((Functor)pt[-1] == FunctorBigInt)) {
    MP_INT *b1 = Yap_BigIntOfTerm(AbsAppl(pt - 1));
    /* first cell in program */
    CELL val = ((CELL *)(b1 + 1))[0];
    return MkIntTerm(val & (MAX_ABS_INT - 1));
  }
#endif
  return MkIntTerm(pt[0] & (MAX_ABS_INT - 1));
}

static inline CELL Yap_Int_key(Term t) { return Yap_IntP_key(RepAppl(t) + 1); }

static inline CELL Yap_DoubleP_key(CELL *pt) {
#if SIZEOF_DOUBLE1 == 2 * SIZEOF_INT_P
  CELL val = pt[0] ^ pt[1];
#else
  CELL val = pt[0];
#endif
  return MkIntTerm(val & (MAX_ABS_INT - 1));
}

static inline CELL Yap_Double_key(Term t) {
  return Yap_DoubleP_key(RepAppl(t) + 1);
}

static inline CELL Yap_StringP_key(CELL *pt) {
  UInt n = pt[1], i;
  CELL val = pt[2];
  for (i = 1; i < n; i++) {
    val ^= pt[i + 1];
  }
  return MkIntTerm(val & (MAX_ABS_INT - 1));
}

static inline CELL Yap_String_key(Term t) {
  return Yap_StringP_key(RepAppl(t) + 1);
}

#endif
 

#endif // TERMEXT_H_INCLUDED


