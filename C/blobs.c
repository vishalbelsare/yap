///
//  @file blobs.c
//  @bried Support of SWI-like atom-extension blobs.
//
//  blobs.c
//  yap
//
//  Created by VITOR SANTOS COSTA on 09/05/15.
//  Copyright (c) 2015 VITOR SANTOS COSTA. All rights reserved.
//

/// @defgroup BlobsImplementation Implementation of Blobs 
/// @ingroup SWIFLI-Implementation
/// @{

#include <stdio.h>
#include <string.h>

#include "Yap.h"
#include "YapHeap.h"
#include "iopreds.h"
#include "yapio.h"

/* for freeBSD9.1 */
#define _WITH_DPRINTF

#include "YapBlobs.h"

static blob_type_t unregistered_blob_atom = {
    YAP_BLOB_MAGIC_B, PL_BLOB_NOCOPY | PL_BLOB_TEXT, "unregistered"};

char *Yap_blob_to_string(AtomEntry *ref, const char *s0, size_t sz) {
  // int rc;
  char *s = (char *)s0;

#if HAVE_FMEMOPEN
  blob_type_t *type = RepBlobProp(ref->PropsOfAE)->blob_type;
  if (type->write) {
    FILE *f = fmemopen(s, sz, "w");
    if (f == NULL) {
      // could not find stream;
      return NULL;
    }
    Atom at = AbsAtom(ref);
    int rc = type->write(f, at, 0);
    if (rc < 0) {
      Yap_Error(EVALUATION_ERROR_UNDEFINED, MkAtomTerm(at),
                "failure in user-defined blob to string code");
    }
    fclose(f); // return the final result.
    return s;
  } else {
#endif
#if __APPLE__
    size_t sz0 = strlcpy(s, (char *)RepAtom(AtomSWIStream)->StrOfAE, sz);
#else
  size_t sz0;
  char *f = (char *)memcpy(s, (char *)RepAtom(AtomSWIStream)->StrOfAE, sz);
  f[0] = '\0';
  sz0 = f - s;
#endif
    s = s + sz0;
    sz -= sz0;
#if defined(__linux__) || defined(__APPLE__)
    snprintf(s + strlen(s), sz0, "(%p)", ref);
#else
  snprintf(s + strlen(s), sz0, "(0x%p)", ref);
#endif
    return s;
#if HAVE_FMEMOPEN
  }
  return NULL;
#endif
}

int Yap_write_blob(AtomEntry *ref, FILE *stream) {
  blob_type_t *type = RepBlobProp(ref->PropsOfAE)->blob_type;

  if (type->write) {

    Atom at = AbsAtom(ref);
    return type->write(stream, at, 0);
  } else {
    const char *s = RepAtom(AtomSWIStream)->StrOfAE;
#if defined(__linux__) || defined(__APPLE__)
    if (s && s[0]) {
      return fprintf(stream, "\'%s\'(%p)",s , ref);
    }
    return fprintf(stream, "blob(%p)",ref);
#else
    if (s && s[0]) {
      return fprintf(stream, "\'%s\'(0x%p)",s , ref);
    }
    return fprintf(stream, "blob(0x%p)", ref);
#endif
  }
  return 0;
}

bool YAP_is_blob(Term t, blob_type_t **type) {
  CACHE_REGS
  Term yt = Yap_GetFromSlot(t);
  Atom a;
  YAP_BlobPropEntry *b;

  if (IsVarTerm(yt))
    return FALSE;
  if (!IsAtomTerm(yt))
    return FALSE;
  a = AtomOfTerm(yt);
  if (!IsBlob(a))
    return FALSE;
  b = RepBlobProp(a->PropsOfAE);
  *type = b->blob_type;
  return TRUE;
}

/* void check_chain(void); */

/* void check_chain(void) { */
/*   AtomEntry *ae, *old; */
/*     ae = Blobs; */
/*     old = NULL; */
/*     while (ae) { */
/*       old = ae; */
/*       ae = RepAtom(ae->NextOfAE); */
/*     } */
/* } */

AtomEntry *Yap_lookupBlob(void *blob, size_t len, void *type0, int *new) {
  YAP_BlobPropEntry *b;
  AtomEntry *ae;
  blob_type_t *type = type0;
  if (new)
    *new = FALSE;

  LOCK(Blobs_Lock);
  if (type->flags & PL_BLOB_UNIQUE) {
    /* just keep a linked chain for now */
    ae = Blobs;
    while (ae) {
      if (ae->PropsOfAE && RepBlobProp(ae->PropsOfAE)->blob_type == type &&
          ae->rep.blob->length == len &&
          !memcmp(ae->rep.blob->data, blob, len)) {
        UNLOCK(Blobs_Lock);
        return ae;
      }
      ae = RepAtom(ae->NextOfAE);
    }
  }
  if (new)
    *new = TRUE;
  b = (YAP_BlobPropEntry *)Yap_AllocCodeSpace(sizeof(YAP_BlobPropEntry));
  if (!b) {
    UNLOCK(Blobs_Lock);
    return NULL;
  }
  b->NextOfPE = NIL;
  b->KindOfPE = BlobProperty;
  b->blob_type = type;
  ae =
      (AtomEntry *)Yap_AllocCodeSpace(sizeof(AtomEntry) + len + sizeof(size_t));
  if (!ae) {
    UNLOCK(Blobs_Lock);
    return NULL;
  }
  NOfBlobs++;
  INIT_RWLOCK(ae->ARWLock);
  ae->PropsOfAE = AbsBlobProp(b);
  ae->NextOfAE = AbsAtom(Blobs);
  ae->rep.blob->length = len;
  memcpy(ae->rep.blob->data, blob, len);
  Blobs = ae;
  if (NOfBlobs > NOfBlobsMax) {
    Yap_signal(YAP_CDOVF_SIGNAL);
  }
  UNLOCK(Blobs_Lock);
  return ae;
}

bool YAP_unify_blob(Term *t, void *blob, size_t len, blob_type_t *type) {
  AtomEntry *ae;

  if (!blob)
    return FALSE;
  ae = Yap_lookupBlob(blob, len, type, NULL);
  if (!ae) {
    return FALSE;
  }
  if (type->acquire) {
    type->acquire(AbsAtom(ae));
  }
  *t = MkAtomTerm(AbsAtom(ae));
  return true;
}

bool YAP_get_blob(Term t, void **blob, size_t *len, blob_type_t **type) {
  CACHE_REGS
  Atom a;
  Term tt;
  AtomEntry *ae;

  tt = Yap_GetFromSlot(t);
  if (IsVarTerm(tt))
    return FALSE;
  if (!IsAtomTerm(tt))
    return FALSE;
  a = AtomOfTerm(tt);
  if (!IsBlob(a))
    return FALSE;
  ae = RepAtom(a);
  if (type)
    *type = RepBlobProp(ae->PropsOfAE)->blob_type;
  if (len)
    *len = ae->rep.blob[0].length;
  if (blob)
    *blob = ae->rep.blob[0].data;
  return TRUE;
}

void *YAP_blob_data(YAP_Atom at, size_t *len, blob_type_t **type) {
Atom x = at;
  if (!IsBlob(x)) {

    if (len)
      *len = strlen_utf8(x->UStrOfAE);
    if (type)
      *type = &unregistered_blob_atom;
    return x->StrOfAE;
  }
  if (len)
    *len = x->rep.blob[0].length;
  if (type)
    *type = RepBlobProp(x->PropsOfAE)->blob_type;
  return x->rep.blob[0].data;
}

void YAP_register_blob_type(blob_type_t *type) {
  type->next = (void *)BlobTypes;
  BlobTypes = (void *)type;
}

blob_type_t *YAP_find_blob_type(const char *name) {
  AtomEntry *a = RepAtom(Yap_LookupAtom(name));
  if (!IsBlob(a)) {
    return &unregistered_blob_atom;
  }
  return RepBlobProp(a->PropsOfAE)->blob_type;
}

bool YAP_unregister_blob_type(blob_type_t *type) {
  fprintf(stderr, "YAP_unregister_blob_type not implemented yet\n");
  return FALSE;
}

void Yap_install_blobs(void) {}

/**
 * @}
 */
