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
 * File:		YapMirror.c *
 * Last rev:	5/2/88							 *
 * mods: *
 * comments:	Term conversion C implemented support			 *
 *									 *
 *************************************************************************/
#ifndef YAP_UTF8_H
#define YAP_UTF8_H

/**
 * @file YapUTF8.h
 *
 * @brief uTF-8 codepoint translation and processing.
 *
 */

#if SIZEOF_WCHAR_T == 2
#define CHARCODE_MAX 0xffff
#else
#define CHARCODE_MAX 0x10ffff
#endif

#include "utf8proc.h"
  
#ifndef INLINE_ONLY
#define  INLINE_ONLY
#endif

 inline static utf8proc_ssize_t __get_utf8(const utf8proc_uint8_t *ptr,
                                                    size_t n,
                                                    utf8proc_int32_t *valp USES_REGS) {
     utf8proc_ssize_t rc = utf8proc_iterate(ptr, n, valp);
  if (rc <= 0) {
      if (ptr[0] == 0xC0 && ptr[1] == 0x80) {
          *valp = 0;
          return 2;
      }
      Yap_encoding_error(ptr[0],0,NULL);
      *valp = -1;
      return  1;
  }
  return rc < 1 ? 1 : rc;
}

inline static utf8proc_ssize_t __put_xutf8(utf8proc_uint8_t *ptr,
                                                    utf8proc_int32_t val USES_REGS) {
    if (val == 0) {
        ptr[0] = 0xC0;
        ptr[1] = 0x80;
        return 2;
    }
    utf8proc_ssize_t rc = utf8proc_encode_char(val, ptr);
  if (rc <= 0) {

      Yap_encoding_error(ptr[0],0,NULL);
      return  1;
  }
  return rc < 1 ? 1 : rc;
}


inline static utf8proc_ssize_t __put_utf8(utf8proc_uint8_t *ptr,
                                       utf8proc_int32_t val USES_REGS) {
    utf8proc_ssize_t rc = utf8proc_encode_char(val, ptr);
    if (rc <= 0) {

      Yap_encoding_error(ptr[0],0,NULL);

    }
    return rc < 1 ? 1 : rc;
}

inline static const utf8proc_uint8_t *skip_utf8(const utf8proc_uint8_t *pt,
						utf8proc_ssize_t n) {
  utf8proc_ssize_t i;
  utf8proc_int32_t b;
  for (i = 0; i < n; i++) {
    utf8proc_ssize_t l = utf8proc_iterate(pt, -1, &b);
    if (b == 0)
      return pt;
    if (l < 0) {
      // LOCAL_ActiveError->errorNo = REPRESENTATION_ERROR_CHARACTER_CODE;
    } else {
      pt += l;
    }
  }
  return pt;
}

inline static utf8proc_ssize_t utf8_nof(utf8proc_int32_t val) {
  return utf8proc_charwidth(val);
}

inline static utf8proc_ssize_t strlen_utf8(const utf8proc_uint8_t *pt) {
  utf8proc_ssize_t rc = 0;
  utf8proc_int32_t b;
  while (true) {
    utf8proc_ssize_t l = utf8proc_iterate(pt, -1, &b);
    if (b == 0)
      return rc;
    else if (l > 0) {
      pt += l;
      rc++;
    } else {
      // LOCAL_ActiveError->errorNo = REPRESENTATION_ERROR_CHARACTER_CODE;
      pt++;
    }
  }
  return rc;
}

inline static utf8proc_ssize_t strlen_latin_utf8(const unsigned char *pt) {
  utf8proc_ssize_t rc = 0;
  utf8proc_uint8_t b;
  while (true) {
    utf8proc_ssize_t l = utf8proc_encode_char(*pt, &b);
    if (l<0) {
      pt++;
    }
    if (b == 0)
      return rc;
    pt++;
    rc += l;
  }
  return rc;
}

inline static utf8proc_ssize_t strnlen_latin_utf8(const unsigned char *pt,
						  size_t max) {
  utf8proc_ssize_t rc = 0;
  utf8proc_uint8_t b;
  while (true) {
    utf8proc_ssize_t l = utf8proc_encode_char(*pt, &b);
    if (b == 0)
      return rc;
    pt++;
    if (l > 0)
      rc += l;
    if (--max == 0)
      return rc;
  }
  return rc;
}

inline static utf8proc_ssize_t strlen_ucs2_utf8(const wchar_t *pt) {
  utf8proc_ssize_t rc = 0;
  utf8proc_uint8_t b;
  while (true) {
    utf8proc_ssize_t l = utf8proc_encode_char(*pt, &b);
    if (b == 0)
      return rc;
    if (l < 0)
      continue;
    pt++;
    rc += l;
  }
  return rc;
}

inline static utf8proc_ssize_t strnlen_ucs2_utf8(const wchar_t *pt,
						 size_t max) {
  utf8proc_ssize_t rc = 0;
  utf8proc_uint8_t b;
  while (true) {
    utf8proc_ssize_t l = utf8proc_encode_char(*pt, &b);
    if (b == 0)
      return rc;
    pt++;
    rc += l;
    if (--max == 0)
      return rc;
  }
  return rc;
}

inline static int cmpn_utf8(const utf8proc_uint8_t *pt1,
			    const utf8proc_uint8_t *pt2, utf8proc_ssize_t n) {
  utf8proc_ssize_t i;
  utf8proc_int32_t b1, b2;
  for (i = 0; i < n; i++) {
    utf8proc_ssize_t l1 = utf8proc_iterate(pt1, -1, &b1);
    utf8proc_ssize_t l2 = utf8proc_iterate(pt2, -1, &b2);
    if (b1!=b2)
      return b1-b2;
    pt1 += l1;
    pt2 += l2;
  }
  return 0;
}

// UTF16

#define LEAD_OFFSET ((uint32_t)0xD800 - (uint32_t)(0x10000 >> 10))
#define SURROGATE_OFFSET						\
  ((uint32_t)0x10000 - (uint32_t)(0xD800 << 10) - (uint32_t)0xDC00)

#define put_utf8(ptr,val) __put_utf8(ptr,val PASS_REGS)
#define put_xutf8(ptr,val) __put_xutf8(ptr,val PASS_REGS)
#define get_utf8(ptr, n, valp) __get_utf8(ptr, n, valp PASS_REGS)

#endif



