/************************************************************************\
 *    Call C predicates instructions                                   *
\************************************************************************/

#include "Yatom.h"
#ifdef INDENT_CODE
{
  {
    {
#endif /* INDENT_CODE */

      BOp(call_cpred, Osbpp);
#if __ANDROID__ && STRONG_DEBUG
      char *s;
      Atom name;
      if (PREG->y_u.Osbpp.p->ArityOfPE) {
        Functor f = PREG->y_u.Osbpp.p->FunctorOfPred;
        name = f->NameOfFE;
      } else {
        name = (Atom)(PREG->y_u.Osbpp.p->FunctorOfPred);
      }
      s = name->StrOfAE;

      LOG(" %s ", s);
#endif
      check_trail(TR);
      	PredEntry *pt0 = PREG->y_u.Osbpp.p;
        CACHE_Y_AS_ENV(YREG);
	if (!(pt0->PredFlags & SafePredFlag)) {
	  check_stack(NoStackCCall, HR);
	}
        ENDCACHE_Y_AS_ENV();
    do_ccall :
      SET_ASP(YREG, AS_CELLS(PREG->y_u.Osbpp.s) );
      /* for slots to work */
#ifdef LOW_LEVEL_TRACER
      if (Yap_do_low_level_trace)
        low_level_trace(enter_pred, pt0, XREGS + 1);
#endif /* LOW_LEVEL_TRACE */
      BEGD(d0);
      CPredicate f = pt0->cs.f_code;
      SREG=(CELL *)PREG;
      yamop *op = PREG;
      saveregs();
      d0 = f(PASS_REGS1);
      setregs();
#ifdef SHADOW_S
      SREG = Yap_REGS.S_;
#endif
      if (PREG==op) {
     PREG = NEXTOP(PREG, Osbpp);
     }
      if (!d0) {
        FAIL();
      }
      ENDD(d0);
      CACHE_A1();
      JMPNext();

    NoStackCCall:
	EXPORT_INT(interrupt_c_call, pt0);

	goto do_ccall;
      ENDCACHE_Y_AS_ENV();
      JMPNext();
      ENDBOp();
      
      /* execute     Label               */
      BOp(execute_cpred, Osbpp);
        CACHE_Y_AS_ENV(YREG);
#ifdef LOW_LEVEL_TRACER
        if (Yap_do_low_level_trace) {
          low_level_trace(enter_pred, PREG->y_u.Osbpp.p, XREGS + 1);
        }
#endif /* LOW_LEVEL_TRACE */
      check_trail(TR);
#ifndef NO_CHECKING
        check_stack(NoStackExecuteC, HR);
#endif
        PredEntry *        pt0 = PREG->y_u.Osbpp.p;
    do_executec:

        SET_ASP(YREG, EnvSizeInCells );
/* for slots to work */
        CACHE_A1();
        BEGD(d0);
        d0 = (CELL)B;
        /* for profiler */
        save_pc();
        YREG[E_CB] = d0;
        ENDD(d0);
#ifdef DEPTH_LIMIT
        if (DEPTH <= MkIntTerm(1)) { /* I assume Module==0 is prolog */
          if (pt0->ModuleOfPred) {
            if (DEPTH == MkIntTerm(0)) {
              FAIL();
            } else {
              DEPTH = RESET_DEPTH();
            }
          }
        } else if (pt0->ModuleOfPred) {
          DEPTH -= MkIntConstant(2);
        }
#endif /* DEPTH_LIMIT */
        /* now call C-Code */
          CPredicate f = pt0->cs.f_code;
	  BEGD(d0);

          yamop *oldPREG = PREG;
          saveregs();
          d0 = f(PASS_REGS1);
          setregs();
#ifdef SHADOW_S
          SREG = Yap_REGS.S_;
#endif
          if (!d0) {
            FAIL();
          }
          if (oldPREG == PREG) {
            /* we did not update PREG */
            /* we can proceed */
            PREG = P = CP;
            ENV_YREG = ENV;
#ifdef DEPTH_LIMIT
            DEPTH = ENV_YREG[E_DEPTH];
#endif
            WRITEBACK_Y_AS_ENV();
          } else {
            /* call the new code  */
            CACHE_A1();
          }
        JMPNext();
        ENDD(d0);
      

    NoStackExecuteC:
      EXPORT_INT(interrupt_executec, pt0);


       goto do_executec;
    }
       ENDCACHE_Y_AS_ENV();
      JMPNext();
      ENDBOp();


      /* Like previous, the only difference is that we do not */
      /* trust the C-function we are calling and hence we must */
      /* guarantee that *all* machine registers are saved and */
      /* restored */
      BOp(call_usercpred, Osbpp);
      CACHE_Y_AS_ENV(YREG);
      check_stack(NoStackUserCall, HR);
      ENDCACHE_Y_AS_ENV();
      //do_user_call:
#ifdef LOW_LEVEL_TRACER
      if (Yap_do_low_level_trace) {
        low_level_trace(enter_pred, PREG->y_u.Osbpp.p, XREGS + 1);
      }
#endif /* LOW_LEVEL_TRACE */
#ifdef FROZEN_STACKS
      {
        choiceptr top_b = PROTECT_FROZEN_B(B);
#ifdef YAPOR_SBA
        if (YREG > (CELL *)top_b || YREG < HR)
          ASP = (CELL *)top_b;
#else
        if (YREG > (CELL *)top_b)
          ASP = (CELL *)top_b;
#endif /* YAPOR_SBA */
        else
          ASP = (CELL *)(((char *)YREG) + PREG->y_u.Osbpp.s);
      }
#else
      SET_ASP(YREG, AS_CELLS(PREG->y_u.Osbpp.s) );
/* for slots to work */
#endif /* FROZEN_STACKS */
      {
        /* make sure that we can still have access to our old PREG after calling
         * user defined goals and backtracking or failing */
        yamop *savedP;

        LOCAL_PrologMode |= UserCCallMode;
        {
          PredEntry *p = PREG->y_u.Osbpp.p;

          savedP = PREG;
          saveregs();
          save_machine_regs();

          SREG = (CELL *)YAP_Execute(p, p->cs.f_code);
        setregs();
        LOCAL_PrologMode &= ~UserCCallMode;
        restore_machine_regs();
      if (PREG==savedP)
	PREG=NEXTOP(PREG, Osbpp);
        }

      }
      if (Yap_HasException(PASS_REGS1)) {
Yap_RaiseException();
        SREG = NULL;
      }
      if (!SREG) {
        FAIL();
      }
      /* in case we call Execute */
      YENV = ENV;
      YREG = ENV;
      JMPNext();

    NoStackUserCall:
      PROCESS_INT(interrupt_user_call, do_user_call);
      JMPNext();
      ENDBOp();

      BOp(call_c_wfail, slpp);
#ifdef LOW_LEVEL_TRACER
      if (Yap_do_low_level_trace) {
        low_level_trace(enter_pred, PREG->y_u.slpp.p, XREGS + 1);
      }
#endif /* LOW_LEVEL_TRACE */
#ifdef FROZEN_STACKS
      {
        choiceptr top_b = PROTECT_FROZEN_B(B);
#ifdef YAPOR_SBA
        if (YREG > (CELL *)top_b || YREG < HR)
          ASP = (CELL *)top_b;
#else
        if (YREG > (CELL *)top_b)
          ASP = (CELL *)top_b;
#endif /* YAPOR_SBA */
        else {
          BEGD(d0);
          d0 = PREG->y_u.slpp.s;
          ASP = ((CELL *)YREG) + d0;
          ENDD(d0);
        }
      }
#else
      if (YREG > (CELL *)B)
        ASP = (CELL *)B;
      else {
        BEGD(d0);
        d0 = PREG->y_u.slpp.s;
        ASP = ((CELL *)YREG) + d0;
        ENDD(d0);
      }
#endif /* FROZEN_STACKS */
      {
        CPredicate f = PREG->y_u.slpp.p->cs.f_code;
        saveregs();
        SREG = (CELL *)((f)(PASS_REGS1));
        setregs();
      }
      if (!SREG) {
        /* be careful about error handling */
        if (PREG != FAILCODE)
          PREG = PREG->y_u.slpp.l;
      } else {
        PREG = NEXTOP(PREG, slpp);
      }
      CACHE_A1();
      JMPNext();
      ENDBOp();

      BOp(try_c, OtapFs);
#ifdef YAPOR
      CUT_wait_leftmost();
#endif /* YAPOR */
      CACHE_Y(YREG);
      /* Alocate space for the cut_c structure*/
      CUT_C_PUSH(NEXTOP(NEXTOP(PREG, OtapFs), OtapFs), S_YREG);
      S_YREG = S_YREG - PREG->y_u.OtapFs.extra;
      store_args(PREG->y_u.OtapFs.s);
      store_yaam_regs(NEXTOP(P, OtapFs), 0);
      B = B_YREG;
#ifdef YAPOR
      SCH_set_load(B_YREG);
#endif /* YAPOR */
      SET_BB(B_YREG);
      ENDCACHE_Y();

    TRYCC:
      ASP = (CELL *)B;
      {
        CPredicate f = (CPredicate)(PREG->y_u.OtapFs.f);
        saveregs();
        SREG = (CELL *)((f)(PASS_REGS1));
        /* This last instruction changes B B*/
        while (POP_CHOICE_POINT(B)) {
          cut_c_pop();
        }
        setregs();
      }
      if (!SREG) {
        /* Removes the cut functions from the stack
           without executing them because we have fail
           and not cuted the predicate*/
        while (POP_CHOICE_POINT(B))
          cut_c_pop();
        FAIL();
      }
      if ((CELL *)B == YREG && ASP != (CELL *)B) {
        /* as Luis says, the predicate that did the try C might
         * have left some data on the stack. We should preserve
         * it, unless the builtin also did cut */
        YREG = ASP;
        HBREG = PROTECT_FROZEN_H(B);
        SET_BB(B);
      }
      PREG = CPREG;
      YREG = ENV;
      JMPNext();
      ENDBOp();

      BOp(retry_c, OtapFs);
#ifdef YAPOR
      CUT_wait_leftmost();
#endif /* YAPOR */
      CACHE_Y(B);
      CPREG = B_YREG->cp_cp;
      ENV = B_YREG->cp_env;
      HR = PROTECT_FROZEN_H(B);
#ifdef DEPTH_LIMIT
      DEPTH = B->cp_depth;
#endif
      HBREG = HR;
      restore_args(PREG->y_u.OtapFs.s);
      ENDCACHE_Y();
      goto TRYCC;
      ENDBOp();

      BOp(cut_c, OtapFs);
/*This is a phantom instruction. This is not executed by the WAM*/
#ifdef DEBUG
      /*If WAM executes this instruction, probably there's an error
        when we put this instruction, cut_c, after retry_c*/
      printf("ERROR: Should not print this message FILE: absmi.c %d\n",
             __LINE__);
#endif /*DEBUG*/
      ENDBOp();

      BOp(try_userc, OtapFs);
#ifdef YAPOR
      CUT_wait_leftmost();
#endif /* YAPOR */
      CACHE_Y(YREG);
      /* Alocate space for the cut_c structure*/
      CUT_C_PUSH(NEXTOP(NEXTOP(PREG, OtapFs), OtapFs), S_YREG);
      S_YREG = S_YREG - PREG->y_u.OtapFs.extra;
      store_args(PREG->y_u.OtapFs.s);
      store_yaam_regs(NEXTOP(PREG, OtapFs), 0);
      B = B_YREG;
#ifdef YAPOR
      SCH_set_load(B_YREG);
#endif
      SET_BB(B_YREG);
      ENDCACHE_Y();
      LOCAL_PrologMode = UserCCallMode;
      ASP = YREG;
      saveregs();
      save_machine_regs();
      SREG = (CELL *)YAP_ExecuteFirst(PREG->y_u.OtapFs.p,
                                      (CPredicate)(PREG->y_u.OtapFs.f));
      restore_machine_regs();
      setregs();
      LOCAL_PrologMode &= UserMode;
      if (!SREG) {
        FAIL();
      }
      if ((CELL *)B == YREG && ASP != (CELL *)B) {
        /* as Luis says, the predicate that did the try C might
         * have left some data on the stack. We should preserve
         * it, unless the builtin also did cut */
        YREG = ASP;
        HBREG = PROTECT_FROZEN_H(B);
      }
      PREG = CPREG;
      YREG = ENV;
      CACHE_A1();
      JMPNext();
      ENDBOp();

      BOp(retry_userc, OtapFs);
#ifdef YAPOR
      CUT_wait_leftmost();
#endif /* YAPOR */
      CACHE_Y(B);
      CPREG = B_YREG->cp_cp;
      ENV = B_YREG->cp_env;
      HR = PROTECT_FROZEN_H(B);
#ifdef DEPTH_LIMIT
      DEPTH = B->cp_depth;
#endif
      HBREG = HR;
      restore_args(PREG->y_u.OtapFs.s);
      ENDCACHE_Y();

      LOCAL_PrologMode |= UserCCallMode;
      SET_ASP(YREG, EnvSizeInCells);
      saveregs();
      save_machine_regs();
      SREG = (CELL *)YAP_ExecuteNext(PREG->y_u.OtapFs.p,
                                     (CPredicate)(PREG->y_u.OtapFs.f));
      restore_machine_regs();
      setregs();
      LOCAL_PrologMode &= ~UserCCallMode;
      if (!SREG) {
        /* Removes the cut functions from the stack
           without executing them because we have fail
           and not cuted the predicate*/
        while (POP_CHOICE_POINT(B))
          cut_c_pop();
        FAIL();
      }
      if ((CELL *)B == YREG && ASP != (CELL *)B) {
        /* as Luis says, the predicate that did the try C might
         * have left some data on the stack. We should preserve
         * it, unless the builtin also did cut */
        YREG = ASP;
        HBREG = PROTECT_FROZEN_H(B);
      }
      PREG = CPREG;
      YREG = ENV;
      CACHE_A1();
      JMPNext();
      ENDBOp();

      BOp(cut_userc, OtapFs);
/*This is a phantom instruction. This is not executed by the WAM*/
#ifdef DEBUG
      /*If WAM executes this instruction, probably there's an error
        when we put this instruction, cut_userc, after retry_userc*/
      printf("ERROR: Should not print this message FILE: absmi.c %d\n",
             __LINE__);
#endif /*DEBUG*/
      CACHE_A1();
      JMPNext();
      ENDBOp();

      /************************************************************************\
       *    support instructions                                             *
\************************************************************************/

      BOp(lock_pred, e);
      {
        PredEntry *ap = PredFromDefCode(PREG);
        PELOCK(10, ap);
        PP = ap;
        if (!ap->cs.p_code.NOfClauses) {
          UNLOCKPE(11, ap);
          FAIL();
        }
        /*
          we do not lock access to the predicate,
          we must take extra care here
        */
        if (ap->cs.p_code.NOfClauses > 1 &&
            !(ap->PredFlags & IndexedPredFlag)) {
          /* update ASP before calling IPred */
          SET_ASP(YREG, EnvSizeInCells );
          saveregs();

          Yap_IPred(ap, 0, CP);
          /* IPred can generate errors, it thus must get rid of the lock itself
           */
          setregs();
          CACHE_A1();
          /* for profiler */
          save_pc();
        }
        PREG = ap->cs.p_code.TrueCodeOfPred;
      }
      JMPNext();
      ENDBOp();

      BOp(index_pred, e);
      {
        PredEntry *ap = PredFromDefCode(PREG);
#if defined(YAPOR) || defined(THREADS)
        /*
          we do
	  not lock access to the predicate,
          we must take extra care here
        */
        if (!PP) {
          PELOCK(11, ap);
        }
        if (ap->OpcodeOfPred != INDEX_OPCODE) {
          /* someone was here before we were */
          if (!PP) {
            UNLOCKPE(11, ap);
          }
          PREG = ap->CodeOfPred;
          /* for profiler */
          save_pc();
          JMPNext();
        }
#endif
        /* update ASP before calling IPred */
        SET_ASP(YREG, EnvSizeInCells);
        saveregs();
        Yap_IPred(ap, 0, CP);
        /* IPred can generate errors, it thus must get rid of the lock itself */
        setregs();
        CACHE_A1();
        PREG = ap->CodeOfPred;
        /* for profiler */
        save_pc();
#if defined(YAPOR) || defined(THREADS)
        if (!PP)
#endif
          UNLOCKPE(14, ap);
      }
      JMPNext();
      ENDBOp();

#if THREADS
      BOp(thread_local, e);
      {
        PredEntry *ap = PredFromDefCode(PREG);
        ap = Yap_GetThreadPred(ap PASS_REGS);
        PREG = ap->CodeOfPred;
        /* for profiler */
        save_pc();
      }
      JMPNext();
      ENDBOp();
#endif

      BOp(expand_index, e);
      {
        PredEntry *pe = PredFromExpandCode(PREG);
        yamop *pt0;

        /* update ASP before calling IPred */
        SET_ASP(YREG, EnvSizeInCells);
#if defined(YAPOR) || defined(THREADS)
        if (!PP) {
          PELOCK(12, pe);
        }
        if (!same_lu_block(PREG_ADDR, PREG)) {
          PREG = *PREG_ADDR;
          if (!PP) {
            UNLOCKPE(15, pe);
          }
          JMPNext();
        }
#endif
#ifdef SHADOW_S
        S = SREG;
#endif /* SHADOW_S */
        saveregs();
        pt0 = Yap_ExpandIndex(pe, 0);
        /* restart index */
        setregs();
#ifdef SHADOW_S
        SREG = S;
#endif /* SHADOW_S */
        PREG = pt0;
#if defined(YAPOR) || defined(THREADS)
        if (!PP) {
          UNLOCKPE(12, pe);
        }
#endif
        JMPNext();
      }
      ENDBOp();

      BOp(expand_clauses, sssllp);
      {
        PredEntry *pe = PREG->y_u.sssllp.p;
        yamop *pt0;

        /* update ASP before calling IPred */
        SET_ASP(YREG, EnvSizeInCells);
#if defined(YAPOR) || defined(THREADS)
        if (PP == NULL) {
          PELOCK(13, pe);
        }
        if (!same_lu_block(PREG_ADDR, PREG)) {
          PREG = *PREG_ADDR;
          if (!PP) {
            UNLOCKPE(16, pe);
          }
          JMPNext();
        }
#endif
        saveregs();
        pt0 = Yap_ExpandIndex(pe, 0);
        /* restart index */
        setregs();
        PREG = pt0;
#if defined(YAPOR) || defined(THREADS)
        if (!PP) {
          UNLOCKPE(18, pe);
        }
#endif
        JMPNext();
      }
      ENDBOp();

      BOp(undef_p, e);
      /* save S for module name */
      {
        PredEntry *pe = PredFromDefCode(PREG);
	saveregs();
	undef_goal(pe PASS_REGS);
	setregs();
	/* for profiler */
      }
	CACHE_A1();
      JMPNext();
      ENDBOp();

      BOp(spy_pred, e);
      saveregs();
      spy_goal(PASS_REGS1);
      setregs();
      CACHE_A1();
      JMPNext();
      ENDBOp();

#ifdef INDENT_CODE
    }
 }
  }
#endif /* INDENT_CODE */
