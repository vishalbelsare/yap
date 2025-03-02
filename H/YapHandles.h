/*************************************************************************
*									 *
*	 YAP Prolog 	%W% %G% 					 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		YapHandles.h   						 *
* mods:									 *
* comments:	term handles for YAP: basic ops				 *
* version:      $Id: Yap.h,v 1.38 2008-06-18 10:02:27 vsc Exp $	 *
*************************************************************************/

#ifndef YAP_HANDLES_H
#define YAP_HANDLES_H 1

//#define DEBUG_HANDLES 1

#include "Regs.h"
#include "Yatom.h"

#define LOCAL_CurHandle LOCAL_CurSlot
#define REMOTE_CurHandle REMOTE_CurSlot
#define LOCAL_ NHandles LOCAL_NSlots
#define REMOTE_NHandles REMOTE_NSlots
#define LOCAL_HandleBase LOCAL_SlotBase
#define REMOTE_HanvdleBase SlotBase

/**
   @defgroup  term_t_slots Term Handles
@ingroup ChYInterface

Also known as term handles, slots are offsets to entries in the local stack. YAP
never compresses the local stack, so slots are respected by the garbage
collector,
hence providing a way to access terms without being exposed to stack shifts or
garbage-collection.

 Space is released when the function terminates. Thus, slots will be
automatically released at the end
of a function. Hence, slots should always be used as local variables.

Handles are organized as follows:
---- Offset of next pointer in chain (tagged as an handle_t)
---- Number of entries (tagged as handle_t), in the example TAG(INT,4)
Entry
Entry
Entry
Entry
---- Number of entries (tagged as handle_t), in the example TAG(INT,4)

Handles are not known to the yaam. Instead, A new set of slots is created when
the
emulator calls user C-code.
(see YAP_Execute* functions). They are also created:

   - by SWI's PL_foreign_frame() function,

   - by the YAP_RunGoal() routines and friends, when they exit successfully.
Notice that all handles created by c-goals  within
     a `Goal` execution should not be used afterwards.

     This section lists the main internal functions for slot management. These
functions are then exported through corresponding FLI C-functions

*************************************************************************************************/

#include <stdio.h>

/// @brief reboot the slot system.
/// Used when wwe start from scratch (Reset).
#define Yap_RebootHandles(wid) Yap_RebootHandles__(wid PASS_REGS)
#define Yap_RebootSlots(wid) Yap_RebootHandles__(wid PASS_REGS)

static inline void Yap_RebootHandles__(int wid USES_REGS) {
  REMOTE_CurSlot(wid) = REMOTE_HandleBorder(wid);
}
 
/// @brief declares a new set of slots.
/// Used to tell how many slots we have so d=dara when we entered a segment of
/// code.
#if DEBUG_HANDLES
#define Yap_StartHandles() ( fprintf(stderr,"%ld [<<<%s,%s,%d\n",LOCAL_CurHandle,__FILE__,__FUNCTION__,__LINE__)?Yap_StartHandles__(PASS_REGS1): -1)
#else
#define Yap_StartHandles() Yap_StartHandles__(PASS_REGS1)
#endif
#define Yap_StartSlots() Yap_StartHandles()

INLINE_ONLY yhandle_t Yap_StartHandles__(USES_REGS1) {
  if (LOCAL_CurHandle < 0) {
    Yap_Error(SYSTEM_ERROR_INTERNAL, 0L, " StartHandles = %ld",
              LOCAL_CurHandle);
  }
  return LOCAL_CurHandle;
}

/// @brief reset the nmber of slots _slot_ to the number existing before the
/// call that produce _slot_
///(eg, Yap_StartHandles(), YAP_NewHandles(), or YAP_PushHandle)

#if DEBUG_HANDLES
#define Yap_CloseHandles(slot) ( fprintf(stderr,"%ld - %s,%s,%d>>>]\n",slot, __FILE__,__FUNCTION__,__LINE__),Yap_CloseHandles__(slot PASS_REGS))
#else
#define Yap_CloseHandles(slot) Yap_CloseHandles__(slot PASS_REGS)
#endif
#define Yap_CloseSlots(slot) Yap_CloseHandles(slot)

INLINE_ONLY void Yap_CloseHandles__(yhandle_t slot USES_REGS) {
  LOCAL_CurHandle = slot;
}

#define Yap_CurrentHandle() Yap_CurrentHandle__(PASS_REGS1)
#define Yap_CurrentSlot() Yap_CurrentHandle__(PASS_REGS1)

/// @brief report the current position of the slots, assuming that they occupy
/// the top of the stack.
INLINE_ONLY yhandle_t Yap_CurrentHandle__(USES_REGS1);
INLINE_ONLY yhandle_t Yap_CurrentHandle__(USES_REGS1) {
  return LOCAL_CurHandle;
}

#define Yap_GetFromHandle(slot) Yap_GetFromHandle__(slot PASS_REGS)
#define Yap_GetFromSlot(slot) Yap_GetFromHandle__(slot PASS_REGS)

/// @brief read from a slot.
INLINE_ONLY Term Yap_GetFromHandle__(yhandle_t slot USES_REGS);
INLINE_ONLY Term Yap_GetFromHandle__(yhandle_t slot USES_REGS) {
  return Deref(LOCAL_HandleBase[slot]);
}

#define Yap_GetDerefedFromHandle(slot)                                         \
  Yap_GetDerefedFromHandle__(slot PASS_REGS)
#define Yap_GetDerefedFromSlot(slot) Yap_GetDerefedFromHandle__(slot PASS_REGS)

/// @brief read from a slot. but does not try to dereference the slot.
INLINE_ONLY Term
Yap_GetDerefedFromHandle__(yhandle_t slot USES_REGS);
INLINE_ONLY Term
Yap_GetDerefedFromHandle__(yhandle_t slot USES_REGS) {
  return LOCAL_HandleBase[slot];
}

#define Yap_GetPtrFromHandle(slot) Yap_GetPtrFromHandle__(slot PASS_REGS)
#define Yap_GetPtrFromSlot(slot) Yap_GetPtrFromHandle__(slot PASS_REGS)

/// @brief read the object in a slot. but do not try to dereference the slot.
INLINE_ONLY Term *
Yap_GetPtrFromHandle__(yhandle_t slot USES_REGS);
INLINE_ONLY Term *
Yap_GetPtrFromHandle__(yhandle_t slot USES_REGS) {
  return (Term *)LOCAL_HandleBase[slot];
}

#define Yap_AddressFromHandle(slot) Yap_AddressFromHandle__(slot PASS_REGS)
#define Yap_AddressFromSlot(slot) Yap_AddressFromHandle__(slot PASS_REGS)

INLINE_ONLY CELL *
Yap_AddressFromHandle__(yhandle_t slot USES_REGS);
INLINE_ONLY CELL *
Yap_AddressFromHandle__(yhandle_t slot USES_REGS) {
  /// @brief get the memory address of a slot

  return (CELL *)LOCAL_HandleBase + slot;
}

#define Yap_PutInSlot(slot, t) Yap_PutInHandle__(slot, t PASS_REGS)
#define Yap_PutInHandle(slot, t) Yap_PutInHandle__(slot, t PASS_REGS)
/// @brief store  term in a slot
INLINE_ONLY void Yap_PutInHandle__(yhandle_t slot,
                                                 Term t USES_REGS);
INLINE_ONLY void Yap_PutInHandle__(yhandle_t slot,
                                                 Term t USES_REGS) {
  LOCAL_HandleBase[slot] = t;
}

#ifndef Yap_Max
#define Yap_Max(X, Y) (X > Y ? X : Y)
#endif

#define ensure_handles ensure_slots
INLINE_ONLY void ensure_slots(ssize_t extra_slots USES_REGS) {
  
  ssize_t max_slots = LOCAL_NSlots;
  max_slots += Yap_Max(extra_slots,1024);
  if (LOCAL_HandleBase == NULL)
    LOCAL_HandleBase = (CELL*)calloc(max_slots,sizeof(CELL));
  else if (max_slots < LOCAL_NSlots+1024) {
    LOCAL_HandleBase = (CELL*)realloc(LOCAL_HandleBase,
                                      max_slots * sizeof(CELL));
  }
}

/// @brief create a new slot with term t
// #define Yap_InitHandle(t)


#if DEBUG_HANDLES
#define Yap_InitHandle(t)  (fprintf(stderr,"%ld + %s,%s,%d>>>]\n", LOCAL_CurHandle,__FILE__, __FUNCTION__,  __LINE__)    ? Yap_InitHandle__(t PASS_REGS)    : -1)
#else
#define Yap_InitHandle(t) Yap_InitHandle__(t PASS_REGS)
#endif
#define Yap_PushHandle(t) Yap_InitHandle__(t PASS_REGS)
#define Yap_InitSlot(t) Yap_InitHandle__(t PASS_REGS)

INLINE_ONLY yhandle_t Yap_InitHandle__(Term t USES_REGS) {
  yhandle_t old_slots = LOCAL_CurHandle;

  ensure_slots(1 PASS_REGS);
  if (t==0) {
    t = MkVarTerm();
  } else if (IsVarTerm(t) && (H0 > (CELL*)t || (CELL*)t > HR)) {
    RESET_VARIABLE(HR); 
    Yap_unify(t,(CELL)HR); t = (CELL)HR++;
  }
  LOCAL_HandleBase[old_slots] = t;
  LOCAL_CurHandle++;
  return old_slots;
}

#if DEBUG_HANDLES
#define Yap_NewHandles(n) ( fprintf(stderr,"+%d %ld%s,%s,%d>>>]\n",n,LOCAL_CurHandle,__FILE__,__FUNCTION__,__LINE__)?Yap_NewHandles__(n PASS_REGS):-1)
#else
#define Yap_NewHandles(n) Yap_NewHandles__(n PASS_REGS)
#endif
#define Yap_NewSlots(n) Yap_NewHandles(n)

INLINE_ONLY yhandle_t Yap_NewHandles__(int n USES_REGS);
INLINE_ONLY yhandle_t Yap_NewHandles__(int n USES_REGS) {
  yhandle_t old_slots = LOCAL_CurHandle;
  int i;

  ensure_slots(n PASS_REGS);
  for (i = 0; i < n; i++) {
    LOCAL_HandleBase[old_slots + i] = MkVarTerm();
  }
  LOCAL_CurHandle += n;
  return old_slots;
}

#if DEBUG_HANDLESX
#define Yap_InitHandles(n, ts)
(fprintf(stderr,"+%d %d %s,%s,%d>>>]\n", n, LOCAL_CurHandle, __FILE__, __FUNCTION__,  __LINE__), Yap_InitHandles__(n, ts PASS_REGS))
#else
#define Yap_InitHandles(n, ts) Yap_InitHandles__(n, ts PASS_REGS)
#endif
#define Yap_InitSlots(n, ts) Yap_InitHandles(n, ts)

/// @brief create n new slots with terms ts[]
INLINE_ONLY yhandle_t Yap_InitHandles__(int n,
                                                      Term *ts USES_REGS);
INLINE_ONLY yhandle_t Yap_InitHandles__(int n,
                                                      Term *ts USES_REGS) {
  yhandle_t old_slots = LOCAL_CurHandle;
  int i;
  ensure_slots(n PASS_REGS);
  for (i = 0; i < n; i++)
    LOCAL_HandleBase[old_slots + i] = ts[i];
  LOCAL_CurHandle += n;
  return old_slots;
}


#define Yap_InitHandleWithAddress(pt) Yap_InitHandleWithAddress__(pt PASS_REGS)

INLINE_ONLY yhandle_t
Yap_InitHandleWithAddress__( CELL *pt USES_REGS) {
  /// @brief store a pointer in a slot

  yhandle_t old_slots = LOCAL_CurHandle;

  ensure_slots(1 PASS_REGS);
  LOCAL_HandleBase[old_slots] = (CELL)pt;
  LOCAL_CurHandle++;
  return old_slots;
}



#define Yap_RecoverHandles(n, ts) Yap_RecoverHandles__(n, ts PASS_REGS)
#define Yap_RecoverSlots(n, ts) Yap_RecoverHandles__(n, ts PASS_REGS)

/// @brief Succeeds if it is to recover the space allocated for $n$ contiguos
/// slots starting at topHandle.
static inline bool Yap_RecoverHandles__(int n, yhandle_t topHandle USES_REGS);
static inline bool Yap_RecoverHandles__(int n, yhandle_t topHandle USES_REGS) {
  if (topHandle + n < LOCAL_CurHandle)
    return false;
  LOCAL_CurHandle = topHandle;
  return true;
}

#define Yap_PopSlot(ts) Yap_PopHandle__(ts PASS_REGS)
#define Yap_PopHandle(ts) Yap_PopHandle__(ts PASS_REGS)

/// @brief recovers the element at position $n$ dropping any other elements p
static inline Term Yap_PopHandle__(yhandle_t topHandle USES_REGS) {
  if (LOCAL_CurHandle < topHandle)
    return 0;
  else {
    LOCAL_CurHandle = topHandle;
    return Deref(LOCAL_HandleBase[topHandle]);
  }
}

#define Yap_PopSlots(ts,s,ar) Yap_PopHandles__(ts, s, ar PASS_REGS)
#define Yap_PopHandles(ts,s,ar) Yap_PopHandles__(ts, s, ar PASS_REGS)

/// @brief recover a chunk of slots
static inline Term Yap_PopHandles__(yhandle_t topHandle, size_t n, Term *ar USES_REGS) {
  size_t i; 
  if (LOCAL_CurHandle < topHandle)
    return false;
  else {
    LOCAL_CurHandle = topHandle;
    for (i=0; i<n; i++)
      ar[i] = Deref(LOCAL_HandleBase[topHandle+i]);
  }
  return true;
}


#define Yap_PopSlotWithAddress(ts) Yap_PopHandleWithAddress__(ts PASS_REGS)
#define Yap_PopHandleWithAddress(ts) Yap_PopHandleWithAddress__(ts PASS_REGS)

/// @brief recovers the element at position $n$ dropping any other elements p
static inline CELL * Yap_PopHandleWithAddress__(yhandle_t topHandle USES_REGS) {
  if (LOCAL_CurHandle < topHandle)
    return NULL;
  else {
    LOCAL_CurHandle = topHandle;
    return (CELL*)(LOCAL_HandleBase[topHandle]);
  }
}

#endif
