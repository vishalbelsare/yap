/*************************************************************************
 *									 *
 *	 YAP Prolog 							 *
 *									 *
 *	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
 *									 *
 * Copyright L.Damas, V. Santos Costa and Universidade do Porto 1985--	 *
 *									 *
 **************************************************************************
 *									 *
 * File:		strings.c *
 * comments:	General-conversion of character sequences.		 *
 *									 *
 * Last rev:     $Date: 2008-07-24 16:02:00 $,$Author: vsc $	     	 *
 *									 *
 *************************************************************************/

/**
   @file text.c
   @brief Support routines for text processing

@defgroup TextSup Text  Processing Support Routines
@ingroup Text_Processing
@brief generic text processing engine.

@{
  Support for text processing:
  - converting to UTF-8
  - converting from UTF-8
  - striping
  - splitting
 -- concatenating
*/
#include "Yap.h"
#include "YapEval.h"
#include "YapHeap.h"
#include "YapStreams.h"
#include "YapText.h"
#include "Yatom.h"
#include "alloc.h"
#include "yapio.h"

#include <YapText.h>
#include <string.h>
#include <wchar.h>

#ifndef HAVE_WCSNLEN
inline static ssize_t min_size(ssize_t i, ssize_t j) { return (i < j ? i : j); }
#define wcsnlen(S, N) min_size(N, wcslen(S))
#endif

#if !defined(HAVE_STPCPY) && !defined(__APPLE__)
inline static void *__stpcpy(void *i, const void *j) {
  return strcpy(i, j) + strlen(j);
}
#define stpcpy __stpcpy
#endif

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

inline char_kind_t Yap_wide_chtype(int ch) {
  switch (utf8proc_category(ch)) {
  case UTF8PROC_CATEGORY_CN: /**< Other, not assigned */
    return BG;
  case UTF8PROC_CATEGORY_LU: /**< Letter, uppercase */
    return UC;
  case UTF8PROC_CATEGORY_LL: /**< Letter, lowercase */
    return LC;
  case UTF8PROC_CATEGORY_LT: /**< Letter, titlecase */
    return UC;
  case UTF8PROC_CATEGORY_LM: /**< Letter, modifier */
    return LC;
  case UTF8PROC_CATEGORY_LO: /**< Letter, other */
    return LC;
  case UTF8PROC_CATEGORY_MN: /**< Mark, nonspacing */
    return BG;
  case UTF8PROC_CATEGORY_MC: /**< Mark, spacing combining */
    return BK;
  case UTF8PROC_CATEGORY_ME: /**< Mark, enclosing */
    return BK;
  case UTF8PROC_CATEGORY_ND: /**< Number, decimal digit */
    return NU;
  case UTF8PROC_CATEGORY_NL: /**< Number, letter */
    return NU;
  case UTF8PROC_CATEGORY_NO: /**< Number, other */
    return NU;
  case UTF8PROC_CATEGORY_PC: /**< Punctuation, connector */
    return SL;
  case UTF8PROC_CATEGORY_PD: /**< Punctuation, dash */
    return SY;
  case UTF8PROC_CATEGORY_PS: /**< Punctuation, open */
    return BK;
  case UTF8PROC_CATEGORY_PE: /**< Punctuation, close */
    return BK;
  case UTF8PROC_CATEGORY_PI: /**< Punctuation, initial quote */
    return QT;
  case UTF8PROC_CATEGORY_PF: /**< Punctuation, final quote */
    return QT;
  case UTF8PROC_CATEGORY_PO: /**< Punctuation, other */
    return SL;
  case UTF8PROC_CATEGORY_SM: /**< Symbol, math */
    return SY;
  case UTF8PROC_CATEGORY_SC: /**< Symbol, currency */
    return SY;
  case UTF8PROC_CATEGORY_SK: /**< Symbol, modifier

   unsure in YAP, let's assume a,c us treated as aç
 */
    return LC;
  case UTF8PROC_CATEGORY_SO: /**< Symbol, other */
    return SL;
  case UTF8PROC_CATEGORY_ZS: /**< Separator, space */
    return BS;
  case UTF8PROC_CATEGORY_ZL: /**< Separator, line */
    return BS;
  case UTF8PROC_CATEGORY_ZP: /**< Separator, paragraph */
    return BS;
  case UTF8PROC_CATEGORY_CC: /**< Other, control */
    return BG;
  case UTF8PROC_CATEGORY_CF: /**< Other, format */
    return BG;
  case UTF8PROC_CATEGORY_CS: /**< Other, surrogate */
    return BG;
  case UTF8PROC_CATEGORY_CO: /**< Other, private use */
    return BG;
  }
  return BS;
}

static Term Globalize(Term v USES_REGS) {
  if (!IsVarTerm(v = Deref(v))) {
    return v;
  }
  if (VarOfTerm(v) > HR && VarOfTerm(v) < LCL0) {
    Bind_Local(VarOfTerm(v), MkVarTerm());
    v = Deref(v);
  }
  return v;
}

static void *codes2buf(Term t0, void *b0, bool get_codes,
                       bool fixed USES_REGS) {
  unsigned char *st0, *st, ar[16];
  Term t = t0;
  ssize_t length = 0;

  if (t == TermNil) {
    st0 = Malloc(4);
    st0[0] = 0;
    return st0;
  }
  if (!IsPairTerm(t)) {
    Yap_ThrowError(TYPE_ERROR_LIST, t0, "scanning list of codes");
    return NULL;
  }
  bool codes = IsIntegerTerm(HeadOfTerm(t));
  if (get_codes != codes && fixed) {
    if (codes) {
      Yap_ThrowError(TYPE_ERROR_INTEGER, t0,
                     "scanning list of codes");
    } else {
      Yap_ThrowError(TYPE_ERROR_ATOM, t0, "scanning list of atoms");
    }
  }
  if (codes) {
    while (IsPairTerm(t)) {
      Term hd = HeadOfTerm(t);
      if (IsVarTerm(hd)) {
        Yap_ThrowError(INSTANTIATION_ERROR, t0, "scanning list of codes");
        return NULL;
      }
      if (!IsIntegerTerm(hd)) {
        Yap_ThrowError(TYPE_ERROR_CHARACTER_CODE, hd, "scanning list of codes");
        return NULL;
      }
      Int code = IntegerOfTerm(hd);
      if (code < 0) {
        Yap_ThrowError(REPRESENTATION_ERROR_CHARACTER_CODE, t0,
                       "scanning list of character codes, found %d", code);
        return NULL;
      }else if (code == 0) {
        length += 2;
      } else {
      length += put_utf8(ar, code);
    }
      t = TailOfTerm(t);
      if (IsVarTerm(t)) {
        Yap_ThrowError(INSTANTIATION_ERROR, t0, "scanning list of codes");
        return NULL;
      }
      if (!IsPairTerm(t) && t != TermNil) {
        Yap_ThrowError(TYPE_ERROR_LIST, t0, "scanning list of codes");
        return NULL;
      }
    }
  } else {
    while (IsPairTerm(t)) {
      Term hd = HeadOfTerm(t);
      if (IsVarTerm(hd)) {
        Yap_ThrowError(INSTANTIATION_ERROR, t0, "scanning list of codes");
        return NULL;
      }
      if (!IsAtomTerm(hd)) {
        Yap_ThrowError(TYPE_ERROR_CHARACTER, hd, "scanning list of texts");
        return NULL;
      }
      const char *code = RepAtom(AtomOfTerm(hd))->StrOfAE;
      const unsigned char*ucode =  RepAtom(AtomOfTerm(hd))->UStrOfAE;
      int chr;
      int n = get_utf8(ucode, -1, &chr);
      if (n!=strlen(code)) {
        Yap_ThrowError(TYPE_ERROR_CHARACTER, hd, "scanning list of atoms");
        return NULL;
      } else if (code == 0) {
          length += 2;
      } else {
	length += strlen(code);
      }
      t = TailOfTerm(t);
      if (IsVarTerm(t)) {
        Yap_ThrowError(INSTANTIATION_ERROR, t0, "scanning list of codes");
        return NULL;
      }
      if (!IsPairTerm(t) && t != TermNil) {
        Yap_ThrowError(TYPE_ERROR_LIST, t0, "scanning list of codes");
        return NULL;
      }
    }
  }

  if (!IsVarTerm(t)) {
    if (t != TermNil) {
      Yap_ThrowError(TYPE_ERROR_LIST, t0, "scanning list of codes");
      return NULL;
    }
  }

  st0 = st = Malloc(length + 1);
  t = t0;
  if (codes) {
    while (IsPairTerm(t)) {
      Term hd = HeadOfTerm(t);
      Int code = IntegerOfTerm(hd);
 
      if (code == 0) {
       st[0] = 0xC0;
        st[1] = 0x80;
st +=2;
      } else
      st = st + put_utf8(st, code);
      t = TailOfTerm(t);
    }
  } else {
    while (IsPairTerm(t)) {
      Term hd = HeadOfTerm(t);
      const char *code = RepAtom(AtomOfTerm(hd))->StrOfAE;
      st = (unsigned char *)stpcpy((char *)st, code);
      t = TailOfTerm(t);
    }
  }
  st[0] = '\0';

  return st0;
}

static unsigned char *latin2utf8(seq_tv_t *inp) {
   CACHE_REGS
  unsigned char *b0 = inp->val.uc;
  ssize_t sz = strlen(inp->val.c);
  sz *= 2;
  int ch;
  unsigned char *buf = Malloc(sz + 1), *pt = buf;
  if (!buf)
    return NULL;
  while ((ch = *b0++)) {
    int off = put_utf8(pt, ch);
    if (off < 0) {
      continue;
    }
    pt += off;
  }
  *pt++ = '\0';
  return buf;
}

static unsigned char *wchar2utf8(seq_tv_t *inp) {
   CACHE_REGS
  ssize_t sz = wcslen(inp->val.w) * 4;
  wchar_t *b0 = inp->val.w;
  unsigned char *buf = Malloc(sz + 1), *pt = buf;
  int ch;
  if (!buf)
    return NULL;
  while ((ch = *b0++))
    pt += put_utf8(pt, ch);
  *pt++ = '\0';
  return buf;
}


static void *slice(ssize_t min, ssize_t max, const unsigned char *buf USES_REGS) {
size_t sz =  strlen((char*)buf)+1;
if (max >sz) max = sz;
 if (max==min) {
   unsigned char *nbuf = Malloc(1);
   nbuf[0] = '\0';
   return nbuf;
 }

 const unsigned char *ptr = skip_utf8(buf, min), *end = skip_utf8(ptr,max-min);
 unsigned char *nbuf = Malloc((end-ptr) + 1);
  memmove(nbuf,ptr,(end-ptr));
  nbuf[end-ptr] = '\0';
  return nbuf;
}


unsigned char *Yap_ListOfCodesToBuffer(unsigned char *buf, Term t,
                                              seq_tv_t *inp USES_REGS) {
  bool codes = true, fixed = true;
  unsigned char *nbuf = codes2buf(t, buf, codes, fixed PASS_REGS);
  return nbuf;
}

 unsigned char *Yap_ListOfCharsToBuffer(unsigned char *buf, Term t,
                                              seq_tv_t *inp USES_REGS) {
  bool codes = false;
  unsigned char *nbuf = codes2buf(t, buf, codes, true PASS_REGS);
  return nbuf;
}

static unsigned char *Yap_ListToBuffer(unsigned char *buf, Term t,
                                       seq_tv_t *inp USES_REGS) {
  return codes2buf(t, buf, NULL, false PASS_REGS);
}

static yap_error_number gen_type_error(int flags) {
  if ((flags & (YAP_STRING_STRING | YAP_STRING_ATOM | YAP_STRING_INT |
                YAP_STRING_FLOAT | YAP_STRING_ATOMS_CODES | YAP_STRING_BIG)) ==
      (YAP_STRING_STRING | YAP_STRING_ATOM | YAP_STRING_INT | YAP_STRING_FLOAT |
       YAP_STRING_ATOMS_CODES | YAP_STRING_BIG))
    return TYPE_ERROR_TEXT;
  if ((flags & (YAP_STRING_STRING | YAP_STRING_ATOM | YAP_STRING_INT |
                YAP_STRING_FLOAT | YAP_STRING_BIG)) ==
      (YAP_STRING_STRING | YAP_STRING_ATOM | YAP_STRING_INT | YAP_STRING_FLOAT |
       YAP_STRING_BIG))
    return TYPE_ERROR_ATOMIC;
  if ((flags & (YAP_STRING_INT | YAP_STRING_FLOAT | YAP_STRING_BIG)) ==
      (YAP_STRING_INT | YAP_STRING_FLOAT | YAP_STRING_BIG))
    return TYPE_ERROR_NUMBER;
  if (flags & YAP_STRING_ATOM)
    return TYPE_ERROR_ATOM;
  if (flags & YAP_STRING_STRING)
    return TYPE_ERROR_STRING;
  if (flags & (YAP_STRING_CODES | YAP_STRING_ATOMS))
    return TYPE_ERROR_LIST;
  return TYPE_ERROR_NUMBER;
}

//  static int cnt;

unsigned char *Yap_readText(seq_tv_t *inp USES_REGS) {
#define POPRET(x) return pop_output_text_stack(lvl, x)
  int lvl = push_text_stack();
  yap_error_number err = YAP_NO_ERROR;
  /* we know what the term is */
  if (!(inp->type & (YAP_STRING_CHARS | YAP_STRING_WCHARS))) {
    seq_type_t inpt = inp->type & (YAP_STRING_TERM|YAP_STRING_ATOM|YAP_STRING_ATOMS_CODES);
    if (!(inpt & YAP_STRING_TERM)) {
      if (IsVarTerm(inp->val.t)) {
       err = INSTANTIATION_ERROR;
      } else if (!IsAtomTerm(inp->val.t) && inpt == YAP_STRING_ATOM) {
        err = TYPE_ERROR_ATOM;
      } else if (!IsStringTerm(inp->val.t) && inpt == YAP_STRING_STRING) {
        err = TYPE_ERROR_STRING;
      } else if (!IsPairOrNilTerm(inp->val.t) && !IsStringTerm(inp->val.t) 
                 && inpt == (YAP_STRING_ATOMS_CODES | YAP_STRING_STRING)) {
        err = TYPE_ERROR_LIST;
      } else if (!IsPairOrNilTerm(inp->val.t) && !IsStringTerm(inp->val.t) &&
                 !IsAtomTerm(inp->val.t) && !(inp->type & YAP_STRING_DATUM)) {
       err = TYPE_ERROR_TEXT;
      }
      if (err) {
	pop_text_stack(lvl);
	Yap_ThrowError(err,
		       inp->val.t, "while converting term %s", Yap_TermToBuffer(
										inp->val.t, YAP_WRITE_HANDLE_CYCLES|YAP_WRITE_QUOTED | Number_vars_f));
      }
    }
    if ((inp->val.t == TermNil) && (inp->type & YAP_STRING_PREFER_LIST ))
  {
    char *out = Malloc(4);
      memset(out, 0, 4);
      POPRET( out );
    }
  if (IsAtomTerm(inp->val.t) && inp->type & YAP_STRING_ATOM) {
    // this is a term, extract to a buffer, and representation is wide
    // Yap_DebugPlWriteln(inp->val.t);
    Atom at = AtomOfTerm(inp->val.t);
    if (RepAtom(at)->UStrOfAE[0] == 0) {
      char *out = Malloc(4);
      memset(out, 0, 4);
      POPRET( out );
    }
    if (inp->type & YAP_STRING_WITH_BUFFER) {
      pop_text_stack(lvl);
      return at->UStrOfAE;
    }
    {
      ssize_t sz = strlen(at->StrOfAE);
      char *out = Malloc(sz + 1);
      strcpy(out, at->StrOfAE);
      POPRET( out );
    }
  }
  if (IsStringTerm(inp->val.t) && inp->type & YAP_STRING_STRING) {
    // this is a term, extract to a buffer, and representation is wide
    // Yap_DebugPlWriteln(inp->val.t);
    const char *s = StringOfTerm(inp->val.t);
    if (s[0] == 0) {
      char *out = Malloc(4);
      memset(out, 0, 4);
      POPRET( out );

    }
    if (inp->type & YAP_STRING_WITH_BUFFER) {
      pop_text_stack(lvl);
      return (unsigned char *)UStringOfTerm(inp->val.t);
    }
    {
      inp->type |= YAP_STRING_IN_TMP;
      ssize_t sz = strlen(s);
      char *out = Malloc(sz + 1);
      strcpy(out, s);
      POPRET( out );
    }
  } else if (IsPairOrNilTerm(inp->val.t)) {
    if (((inp->type &  YAP_STRING_CODES ) ||
	 inp->type &YAP_STRING_ATOMS)) {
      // Yap_DebugPlWriteln(inp->val.t);
     char * out = (char *)Yap_ListToBuffer(NULL, inp->val.t, inp PASS_REGS);
      POPRET( out );
      // this is a term, extract to a sfer, and representation is wide
    }
    if (inp->type & YAP_STRING_CODES) {
      // Yap_DebugPlWriteln(inp->val.t);
      char *out = (char *)Yap_ListOfCodesToBuffer(NULL, inp->val.t, inp PASS_REGS);
      // this is a term, extract to a sfer, and representation is wide
      POPRET( out );
    }
    if (inp->type & YAP_STRING_ATOMS) {
      // Yap_DebugPlWriteln(inp->val.t);
      char *out = (char *)Yap_ListOfCharsToBuffer(NULL, inp->val.t, inp PASS_REGS);
      // this is a term, extract to a buffer, and representation is wide
      POPRET( out );
    }
  }
  if (inp->type & YAP_STRING_INT && IsIntegerTerm(inp->val.t)) {
    // ASCII, so both LATIN1 and UTF-8
    // Yap_DebugPlWriteln(inp->val.t);
    char * out = Malloc(2 * MaxTmp(PASS_REGS1));
    if (snprintf(out, MaxTmp(PASS_REGS1) - 1, Int_FORMAT,
                 IntegerOfTerm(inp->val.t)) < 0) {
      AUX_ERROR(inp->val.t, 2 * MaxTmp(PASS_REGS1), out, char);
    }
    POPRET( out );
  }
  if (inp->type & YAP_STRING_FLOAT && IsFloatTerm(inp->val.t)) {
    char *out = Malloc(2 * MaxTmp(PASS_REGS1));
    if (!Yap_FormatFloat(FloatOfTerm(inp->val.t), &out, 1024)) {
      pop_text_stack(lvl);
      return NULL;
    }
    POPRET(out);
  } 
  if (inp->type & YAP_STRING_BIG && IsBigIntTerm(inp->val.t)) {
    // Yap_DebugPlWriteln(inp->val.t);
    char *out = Malloc(MaxTmp());
    if (!Yap_mpz_to_string(Yap_BigIntOfTerm(inp->val.t), out, MaxTmp() - 1,
                           10)) {
      AUX_ERROR(inp->val.t, MaxTmp(PASS_REGS1), out, char);
    }
    POPRET(out);
  }
  if (inp->type & YAP_STRING_TERM) {
    pop_text_stack(lvl);
    return (unsigned char *)Yap_TermToBuffer(inp->val.t, 0);
  }
  pop_text_stack(lvl);
    Yap_ThrowError(TYPE_ERROR_TEXT,
       inp->val.t, "while converting term %s", Yap_TermToBuffer(
         inp->val.t, YAP_WRITE_HANDLE_CYCLES|YAP_WRITE_QUOTED | Number_vars_f));

    return NULL;
  }

  if (inp->type & YAP_STRING_CHARS) { 
    if (inp->enc == ENC_ISO_ASCII) {
      pop_text_stack(lvl);
      return inp->val.uc;
    }

    if (inp->enc == ENC_ISO_LATIN1) {
      POPRET( (char*)latin2utf8(inp));
    }

      pop_text_stack(lvl);
      
      return inp->val.uc;
  }
  if (inp->type & YAP_STRING_WCHARS) {
    // printf("%S\n",inp->val.w);
    POPRET( (char *)wchar2utf8(inp) );
  }
  pop_text_stack(lvl);
    Yap_ThrowError(TYPE_ERROR_TEXT,
		   TermNil, "Bad text input ");
  return NULL;
}

static Term write_strings(unsigned char *s0, seq_tv_t *out USES_REGS) {
  ssize_t max;

  if (s0 && s0[0]) max = strlen((char *)s0);
  else max = 0;

  if (out->type & (YAP_STRING_NCHARS | YAP_STRING_TRUNC)) {
    if (out->type & YAP_STRING_NCHARS)
    if (out->type & YAP_STRING_TRUNC && out->max < max) {
      max = out->max;
      s0[max] = '\0';
    }
  }

  if (HR+strlen((char*)s0)/sizeof(CELL) > ASP-64*1024)
    out->val.t = 0;
  else
    out->val.t = MkUStringTerm(s0);

  return out->val.t;
}

static Term write_atoms(void *s0, seq_tv_t *out USES_REGS) {
  Term t = AbsPair(HR);
  char *s1 = (char *)s0;
  ssize_t sz = 0;
  size_t max = strlen(s1);
  if (s1[0] == '\0') {
    out->val.t = TermNil;
    return TermNil;
  }
  if (out->type & (YAP_STRING_NCHARS | YAP_STRING_TRUNC)) {
    if (out->type & YAP_STRING_TRUNC && out->max < max)
      max = out->max;
  }

  unsigned char *s = s0, *lim = s + strnlen((char *)s, max);
  unsigned char *cp = s;
  unsigned char w[10];
  int wp = 0;
  LOCAL_TERM_ERROR(t, 2 * (lim - s));
  while (cp < lim && *cp) {
    utf8proc_int32_t chr;
    CELL *cl;
    s += get_utf8(s, -1, &chr);
    if (chr == '\0') {
      w[0] = '\0';
      break;
    }
    wp = put_utf8(w, chr);
    w[wp] = '\0';
    cl = HR;
    HR += 2;
    cl[0] = MkAtomTerm(Yap_ULookupAtom(w));
    cl[1] = AbsPair(HR);
    sz++;
    if (sz == max)
      break;
  }
  if (out->type & YAP_STRING_DIFF) {
    if (sz == 0)
      t = out->dif;
    else
      HR[-1] = Globalize(out->dif PASS_REGS);
  } else {
    if (sz == 0)
      t = TermNil;
    else
      HR[-1] = TermNil;
  }
  out->val.t = t;
  return (t);
}

static Term write_codes(void *s0, seq_tv_t *out USES_REGS) {
  Term t;
  size_t sz = strlen(s0);
  if (sz == 0) {
    if (out->type & YAP_STRING_DIFF) {
      out->val.t = Globalize(out->dif PASS_REGS);
    } else {
      out->val.t = TermNil;
    }
    return out->val.t;
  }
  unsigned char *s = s0, *lim = s + sz;
  unsigned char *cp = s;

  t = AbsPair(HR);
  LOCAL_TERM_ERROR(t, 2 * (lim - s));
  t = AbsPair(HR);
  while (*cp) {
    utf8proc_int32_t chr;
    CELL *cl;
    cp += get_utf8(cp, -1, &chr);
    if (chr == '\0')
      break;
    cl = HR;
    HR += 2;
    cl[0] = MkIntegerTerm(chr);
    cl[1] = AbsPair(HR);
  }
  if (sz == 0) {
    HR[-1] = Globalize(out->dif PASS_REGS);
  } else {
    HR[-1] = TermNil;
  }
  out->val.t = t;
  return (t);
}

static Atom write_atom(void *s0, seq_tv_t *out USES_REGS) {
  unsigned char *s = s0;
  int32_t ch;
  if (s[0] == '\0') {
    return Yap_LookupAtom("");
  }
  size_t leng = strlen(s0);
  if (strlen_utf8(s0) <= leng) {
    return Yap_LookupAtom(s0);
  } else {
    size_t n = get_utf8(s, -1, &ch);
    unsigned char *buf = Malloc(n + 1);
    memmove(buf, s0, n + 1);
    return Yap_ULookupAtom(buf);
  }
}

void *write_buffer(unsigned char *s0, seq_tv_t *out USES_REGS) {
  int l = push_text_stack();
  size_t leng = strlen((char *)s0);
  size_t min = 0, max = leng;
  if (out->enc == ENC_ISO_UTF8) {
    if (out->val.uc == NULL) { // this should always be the case
      out->val.uc = Malloc(leng + 1);
      strcpy(out->val.c, (char *)s0);
    } else if (out->val.uc != s0) {
      out->val.c = Malloc(leng + 1);
      strcpy(out->val.c, (char *)s0);
    }
  } else if (out->enc == ENC_ISO_LATIN1) {

    unsigned char *s = s0;
    unsigned char *cp = s;
    unsigned char *buf = out->val.uc;
    if (!buf) {
      pop_text_stack(l);
      return NULL;
    }
    while (*cp) {
      utf8proc_int32_t chr;
      int off = get_utf8(cp, -1, &chr);
      if (off <= 0 || chr > 255) {
        pop_text_stack(l);
        return NULL;
      }
      if (off == max)
        break;
      cp += off;
      *buf++ = chr;
    }
    if (max >= min)
      *buf++ = '\0';
    else
      while (max < min) {
        utf8proc_int32_t chr;
        max++;
        cp += get_utf8(cp, -1, &chr);
        *buf++ = chr;
      }
  } else if (out->enc == ENC_WCHAR) {
    unsigned char *s = s0, *lim = s + (max = strnlen((char *)s0, max));
    unsigned char *cp = s;
    wchar_t *buf0, *buf;

    buf = buf0 = out->val.w;
    if (!buf) {
      pop_text_stack(l);
      return NULL;
    }
    while (*cp && cp < lim) {
      utf8proc_int32_t chr;
      cp += get_utf8(cp, -1, &chr);
      *buf++ = chr;
    }
    if (max >= min)
      *buf++ = '\0';
    else
      while (max < min) {
        utf8proc_int32_t chr;
        max++;
        cp += get_utf8(cp, -1, &chr);
        *buf++ = chr;
      }
    *buf = '\0';
  } else {
    // no other encodings are supported.
    pop_text_stack(l);
    return NULL;
  }
  out->val.c = pop_output_text_stack(l, out->val.c);
  return out->val.c;
}

static size_t write_length(const unsigned char *s0, seq_tv_t *out USES_REGS) {
  return strlen_utf8(s0);
}

static Term write_number(unsigned char *s, seq_tv_t *out                       USES_REGS) {
  Term t;

  t = Yap_StringToNumberTerm((char *)s, &out->enc,false);
  LOCAL_delay = false;
  return t;
}

static Term string_to_term(void *s, seq_tv_t *out USES_REGS) {
  Term o;
  int lvl = push_text_stack();
  yap_error_descriptor_t *new_error = Malloc(sizeof(yap_error_descriptor_t));
  yap_error_descriptor_t *old = Yap_pushErrorContext(true, new_error, LOCAL_ActiveError);
  o = out->val.t = Yap_BufferToTerm(s, TermNil);
  LOCAL_ActiveError = Yap_popErrorContext(true, false, old);
  pop_text_stack(lvl);
  return o;
}

Term Yap_StringToNumberTerm(const char *s, encoding_t *encp, bool error_on) {
  CACHE_REGS
  int sno;
  Atom nat = AtomEmptyBrackets;
  sno = Yap_open_buf_read_stream(NULL, s, strlen(s), encp, MEM_BUF_USER, nat,
                                 TermEvaluable);
  if (sno < 0)
    return TermNil;
  if (encp)
    GLOBAL_Stream[sno].encoding = *encp;
  else
    GLOBAL_Stream[sno].encoding = LOCAL_encoding;
#ifdef __ANDROID__

  while (*s && isblank(*s) && Yap_wide_chtype(*s) == BS)
    s++;
#endif
  GLOBAL_Stream[sno].status |= CloseOnException_Stream_f;
  if (error_on) {
    GLOBAL_Stream[sno].status |= RepError_Prolog_f;

    return TermNil;
  }
  int i = push_text_stack();
  Term t = Yap_scan_num(GLOBAL_Stream + sno, error_on);
  Yap_CloseStream(sno);
  pop_text_stack(i);
  return t;
}


bool write_Text(unsigned char *inp, seq_tv_t *out USES_REGS) {
  /* we know what the term is */
  out->val.t = 0;
  if (out->type == 0) {
    return true;
  }

  if ((out->type &(YAP_STRING_INT |
			 YAP_STRING_FLOAT |
		   YAP_STRING_BIG))!=0) {
    if ((out->val.t = write_number(
				   inp, out PASS_REGS)) !=0&&
	(out->val.t != TermNil)) {
      // Yap_DebugPlWriteln(out->val.t);

      return true;
    } else {

      if (!(out->type & (YAP_STRING_ATOM |
			 YAP_STRING_STRING |
			YAP_STRING_CODES))) {
	return false;
      }
      Yap_ResetException(NULL);

      
  }
  }
  if (out->type & (YAP_STRING_ATOM)) {


    if ((out->val.a = write_atom(inp, out PASS_REGS)) != NIL) {
      Atom at = out->val.a;
      if (at && (out->type & YAP_STRING_OUTPUT_TERM))
        out->val.t = MkAtomTerm(at);
      // Yap_DebugPlWriteln(out->val.t);
      return at != NIL;
    }
  }
  if (out->type & YAP_STRING_DATUM) {
    if ((out->val.t = string_to_term(inp, out PASS_REGS)) != 0L)
      return out->val.t != 0;
  }

  switch (out->type & YAP_TYPE_MASK) {
  case YAP_STRING_CHARS: {
    void *room = write_buffer(inp, out PASS_REGS);
    // printf("%s\n", out->val.c);
    return room != NULL;
  }
  case YAP_STRING_WCHARS: {
    void *room = write_buffer(inp, out PASS_REGS);
    // printf("%S\n", out->val.w);
    return room != NULL;
  }
  case YAP_STRING_STRING:
    out->val.t = write_strings(inp, out PASS_REGS);
    // Yap_DebugPlWriteln(out->val.t);
    return out->val.t != 0;
  case YAP_STRING_ATOMS:
    out->val.t = write_atoms(inp, out PASS_REGS);
    // Yap_DebugPlWriteln(out->val.t);
    return out->val.t != 0;
  case YAP_STRING_CODES:
    out->val.t = write_codes(inp, out PASS_REGS);
    // Yap_DebugPlWriteln(out->val.t);
    return out->val.t != 0;
  case YAP_STRING_LENGTH:
    out->val.l = write_length(inp, out PASS_REGS);
    // printf("s\n",out->val.l);
    return out->val.l != (size_t)(-1);
  case YAP_STRING_ATOM:
    out->val.a = write_atom(inp, out PASS_REGS);
    // Yap_DebugPlWriteln(out->val.t);
    return out->val.a != NULL;
  case YAP_STRING_INT | YAP_STRING_FLOAT | YAP_STRING_BIG:
    out->val.t = write_number(inp, out PASS_REGS);
    // Yap_DebugPlWriteln(out->val.t);
    if (out->val.t==TermNil &&(out->type & (YAP_STRING_INT | YAP_STRING_FLOAT | YAP_STRING_BIG |YAP_STRING_ATOM | YAP_STRING_STRING| YAP_STRING_TERM)) ==
	(YAP_STRING_INT | YAP_STRING_FLOAT | YAP_STRING_BIG))
      Yap_ThrowError(TYPE_ERROR_NUMBER, MkUStringTerm(inp), NULL);
    return out->val.t != 0;
  default: { return true; }
  }
  return false;
}

static size_t upcase(void *s0, seq_tv_t *out USES_REGS) {

  unsigned char *s = s0;
  while (*s) {
    // assumes the two code have always the same size;
    utf8proc_int32_t chr;
    get_utf8(s, -1, &chr);
    chr = utf8proc_toupper(chr);
    s += put_utf8(s, chr);
  }
  return true;
}

static size_t downcase(void *s0, seq_tv_t *out USES_REGS) {

  unsigned char *s = s0;
  while (*s) {
    // assumes the two code have always the same size;
    utf8proc_int32_t chr;
    get_utf8(s, -1, &chr);
    chr = utf8proc_tolower(chr);
    s += put_utf8(s, chr);
  }
  return true;
}

bool Yap_CVT_Text(seq_tv_t *inp, seq_tv_t *out USES_REGS) {
  unsigned char *buf;
  bool rc;

  /*
  //printf(stderr, "[ %d ", n++)    ;
  if (inp->type & (YAP_STRING_TERM|YAP_STRING_ATOM|YAP_STRING_ATOMS_CODES
  |YAP_STRING_STRING))
  //Yap_DebugPlWriteln(inp->val.t);
  else if (inp->type & YAP_STRING_WCHARS) fprintf(stderr,"S %S\n", inp->val
  .w);
  else  fprintf(stderr,"s %s\n", inp->val.c);
  */
  //  cnt++;
  int l = push_text_stack();
  buf = Yap_readText(inp PASS_REGS);
  if (!buf) {
    pop_text_stack(l);
    yap_error_number err;
    if (!(err=gen_type_error(inp->type)))
	  err = TYPE_ERROR_TEXT;
    return 0L;
  }
  if (buf[0]) {
    size_t leng = strlen_utf8(buf);
    if (out->type & (YAP_STRING_NCHARS | YAP_STRING_TRUNC)) {
      if (out->max < leng) {
        const unsigned char *ptr = skip_utf8(buf, out->max);
        size_t diff = (ptr - buf);
        char *nbuf = Malloc(diff + 1);
        memmove(nbuf, buf, diff);
        nbuf[diff] = '\0';
        leng = diff;
      }
      // else if (out->type & YAP_STRING_NCHARS &&
      // const unsigned char *ptr = skip_utf8(buf)
    }
    if (out->type & (YAP_STRING_UPCASE | YAP_STRING_DOWNCASE)) {
      if (out->type & YAP_STRING_UPCASE) {
        if (!upcase(buf, out PASS_REGS)) {
          pop_text_stack(l);
          return false;
        }
      }
      if (out->type & YAP_STRING_DOWNCASE) {
        if (!downcase(buf, out PASS_REGS)) {
          pop_text_stack(l);
          return false;
        }
      }
    }
  }
  rc = write_Text(buf, out PASS_REGS);
  /*    fprintf(stderr, " -> ");
        if (!rc) fprintf(stderr, "NULL");
        else if (out->type &
        (YAP_STRING_TERM|YAP_STRING_ATOMS_CODES
        |YAP_STRING_STRING)) //Yap_DebugPlWrite(out->val.t);
        else if (out->type &
        YAP_STRING_ATOM) //Yap_DebugPlWriteln(MkAtomTerm(out->val.a));
        else if (out->type & YAP_STRING_WCHARS) fprintf(stderr, "%S",
        out->val.w);
        else
        fprintf(stderr, "%s", out->val.c);
        fprintf(stderr, "\n]\n"); */
  out->val.uc = pop_output_text_stack(l,out->val.uc);
  return rc;
}


//
// Out must be an atom or a string

//
// Out must be an atom or a string
bool Yap_Concat_Text(int tot, seq_tv_t inp[], seq_tv_t *out USES_REGS) {
  unsigned char *buf;
  int i;
  size_t avai;
    unsigned char const *nbuf=NULL;
    size_t bsz = tot * 256;
  int lvl = push_text_stack();
  buf = calloc(1,bsz);
  buf[0] = '\0';
  if (!buf) {
     pop_text_stack(lvl);
    return NULL;
  }
  for (i = 0; i < tot; i++) {
    Term t = inp[i].val.t;
    if (IsAtomTerm(t)) {
      nbuf = RepAtom(AtomOfTerm(t))->UStrOfAE;
    } else    if (IsStringTerm(t)) {
      nbuf = UStringOfTerm(t);
    } else {
      nbuf = Yap_readText(inp + i PASS_REGS);
    }
    if (!nbuf) {
      //     pop_text_stack(lvl);
      //return NULL;
      continue;
    }
    //      if (!nbuf[0])
    //	continue;
    if (nbuf && nbuf[0]) {
      size_t sz = strlen((char*)nbuf);
      avai = (strlen((char *)buf)+sz +1);
      size_t x = 256*(tot-i-1);
      if (avai > bsz-x) {
	buf = realloc(buf, avai+x);
      }
      strcat((char*)buf,(char*)nbuf);
    }
  }
 
  bool rc = write_Text(buf, out PASS_REGS);
  free(buf);
  pop_text_stack( lvl );

  return rc;
}

//
bool Yap_Splice_Text(int n, ssize_t cuts[], seq_tv_t *inp,
                     seq_tv_t outv[] USES_REGS) {
  int lvl = push_text_stack();
  const unsigned char *buf;
  ssize_t b_l, u_l, i;

  inp->type |= YAP_STRING_IN_TMP;
  buf = Yap_readText(inp PASS_REGS);
  if (!buf) {
    pop_text_stack(lvl);
    return false;
  }
  b_l = strlen((char *)buf);
  if (b_l == 0) {
    pop_text_stack(lvl);
    return false;
  }
  u_l = strlen_utf8(buf);
  if (!cuts) {
    if (n == 2) {
      size_t b_l0, b_l1, u_l0, u_l1;
      unsigned char *buf0, *buf1;

      if (outv[0].val.t) {
        buf0 = Yap_readText(outv PASS_REGS);
        if (!buf0) {
          return false;
        }
        b_l0 = strlen((const char *)buf0);
        if (memcmp(buf, buf0, b_l0) != 0) {
          pop_text_stack(lvl);
          return false;
        }
        u_l0 = strlen_utf8(buf0);
        u_l1 = u_l - u_l0;

        b_l1 = b_l - b_l0;
        buf1 = slice(u_l0, u_l, buf PASS_REGS);
        b_l1 = strlen((const char *)buf1);
        bool rc = write_Text(buf1, outv + 1 PASS_REGS);
        pop_text_stack(lvl);
        if (!rc) {
          return false;
        }
        return rc;
      } else /* if (outv[1].val.t) */ {
        buf1 = Yap_readText(outv + 1 PASS_REGS);
        if (!buf1) {
          pop_text_stack(lvl);
          return false;
        }
        b_l1 = strlen((char *)buf1);
        u_l1 = strlen_utf8(buf1);
        b_l0 = b_l - b_l1;
        u_l0 = u_l - u_l1;
        if (memcmp(skip_utf8((const unsigned char *)buf, b_l0), buf1, b_l1) !=
            0) {
          pop_text_stack(lvl);
          return false;
        }
        buf0 = slice(0, u_l0, buf PASS_REGS);
        buf0 = pop_output_text_stack(lvl, buf0);
        bool rc = write_Text(buf0, outv PASS_REGS);
        return rc;
      }
    }
  }
  pop_text_stack(lvl);
  for (i = 0; i < n; i++) {
  ssize_t from,next;
    if (i == 0)
      from = 0;
    else
      from = cuts[i - 1];
    next = cuts[i];
    lvl = push_text_stack();
    void *bufi = slice(from, next, buf PASS_REGS);
    bufi = pop_output_text_stack(lvl, bufi);
    if (!write_Text(bufi, outv + i PASS_REGS)) {
      return false;
    }
     }

  return true;
}

/**
 * Convert from a predicate structure to an UTF-8 string of the form
 *
 * module:name/arity.
 *
 * The result is in very volatile memory.
 *
 * @param s        the buffer
 *
 * @return the temporary string
 */
const char *Yap_PredIndicatorToUTF8String(PredEntry *ap, char *s0, size_t sz) {
  CACHE_REGS
  Atom at;
  arity_t arity = 0;
  Functor f;
  char *s, *smax;
  s = s0;
  smax = s + sz;
  Term tmod = ap->ModuleOfPred;
  if (tmod) {
    char *sn = Yap_AtomToUTF8Text(AtomOfTerm(tmod) PASS_REGS);
    stpcpy(s, sn);
    if (smax - s > 1) {
      strcat(s, ":");
    } else {
      return NULL;
    }
    s++;
  } else {
    if (smax - s > strlen("prolog:")) {
      s = strcpy(s, "prolog:");
    } else {
      return NULL;
    }
  }
  // follows the actual functor
  if (ap->ModuleOfPred == IDB_MODULE) {
    if (ap->PredFlags & NumberDBPredFlag) {
      Int key = ap->src.IndxId;
      snprintf(s, smax - s, "%" PRIdPTR, key);
      return s0;
    } else if (ap->PredFlags & AtomDBPredFlag) {
      at = (Atom)(ap->FunctorOfPred);
      if (!stpcpy(s, Yap_AtomToUTF8Text(at PASS_REGS)))
        return NULL;
    } else {
      f = ap->FunctorOfPred;
      at = NameOfFunctor(f);
      arity = ArityOfFunctor(f);
    }
  } else {
    arity = ap->ArityOfPE;
    if (arity) {
      at = NameOfFunctor(ap->FunctorOfPred);
    } else {
      at = (Atom)(ap->FunctorOfPred);
    }
  }
  if (!stpcpy(s, Yap_AtomToUTF8Text(at PASS_REGS))) {
    return NULL;
  }
  s += strlen(s);
  snprintf(s, smax - s, "/%" PRIdPTR, arity);
  return s0;
}


/// @}




