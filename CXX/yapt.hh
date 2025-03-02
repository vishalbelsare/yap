/**
 * @file yapt.hh
 */

#ifndef X_API
#define X_API
#endif

/**
 *   @defgroup FLI_YAP-cplus-term-handling Term Handling in the YAP interface.
 *
 *   @{
 *
 *   @ingroup FLI_YAP-cplus-interface
 *   @tableofcontents
 *
 *
 * These classes offer term construction and access. Terms are seens
 * as objects that inherit from a virtual class, Currently, all
 * terms must reside in the stack and may be moved around during
 * garbage collection. Term objects use an handle, in the SWI-Prolog style.
 *
 * Notice that terms  are forcefully destroyed during backtracking.
 *
 */

#include <vector>

#ifndef YAPT_HH
#define YAPT_HH 1

class YAPError;

extern "C" {

X_API extern Term YAP_MkCharPTerm(char *n);
}

/**
 * @brief Generic class for Prolog Terms.
 *
 * The class YAPTerm provides the ability to construct and manipulate Prolog terms. Operations include:
 * 1. constructors that generate terms ranging from other terms to, source text.
 * 2. term type testing, extraction and construction.
 * 3. several built-ins for term manipulation, including unification, copying, and variant testing;
 *
 * The YAPTerm class has several sub-classes, mostly corresponding to diverse types of terms. Also, YAP
 * provides two utility sub-classes: lists and conjunctions.
 *
 ~~~
 a :- b(Z).
 ~~~
 * YAPTerm is implemented on top of the slot or handle mechanism. This provides protection against stack shifting or
 * garbage collection, but in YAP it requires the programmer to explicitely release the handles. This can be done through pop_t() or through synch points.
 */
class X_API YAPTerm {
  friend class YAPPredicate;
  friend class YAPPrologPredicate;
  friend class YAPQuery;
  friend class YAPModule;
  friend class YAPModuleProp;
  friend class YAPApplTerm;
  friend class YAPListTerm;
  friend class YAPConjunctiveTerm;

protected:
yhandle_t hdl; /// a handle to stack representation of the term, equivalent to SWI-Prolog's term_t; this is the only dynamic structure in a YAPTerm

public:

Term gt() { /// convert from YAPTerm to term (same as term())
    CACHE_REGS
    // fprintf(stderr,"?%d,%lx,%p\n",t,LOCAL_HandleBase[t], HR);
    // Yap_DebugPlWriteln(LOCAL_HandleBase[t]);
    return Yap_GetFromSlot(hdl);
  }; 

  Term pop_t() {/// get the Prolog term corresponding to the YAPTerm, and try to recover space 

    CACHE_REGS
    // fprintf(stderr,"?%d,%lx,%p\n",t,LOCAL_HandleBase[t], HR);
    // Yap_DebugPlWriteln(LOCAL_HandleBase[t]);
      return Deref(Yap_PopHandle(hdl));
  };

  void mk(Term t0) {  /// create a new YAPTerm from a term
    CACHE_REGS hdl = Yap_InitSlot(t0);
    // fprintf(stderr,"+%d,%lx,%p,%p",t,t0,HR,ASP); Yap_DebugPlWriteln(t0);
  };

  void put(Term t0) { /// copy a term to an YAPTerm
    CACHE_REGS
    Yap_PutInHandle(hdl, t0);
    // fprintf(stderr,"+%d,%lx,%p,%p",t,t0,HR,ASP); Yap_DebugPlWriteln(t0);
  };

YAPTerm(Term tn) { mk(tn); }; ///< private method to convert from Term (internal YAP representation) toYAPTerm 
#ifdef SWIGPYTHON
//   YAPTerm(struct _object *inp) {
// Term tinp = pythonToYAP(inp);
//  t = Yap_InitSlot(tinp);
//}
#endif
  YAPTerm() { hdl = 0; }; ///< do nothing constructor
  
  // YAPTerm(yhandle_t i) { t = i; };
  YAPTerm(void *ptr); ///< pointer to term
  
  YAPTerm(char *s) { /// parse string s and construct a term.
    Term tp = 0;
    mk(YAP_ReadBuffer(s, &tp));
  }

#if 1
  virtual ~YAPTerm(){ /// Term destructor, tries to recover slot
 
    
    
      //  fprintf(stderr,"-%d,%lx,%p ",t,LOCAL_HandleBase[t] ,HR);
      /*    if (!t)
            return;
          //          Yap_DebugPlWriteln(LOCAL_HandleBase[t]);
          LOCAL_HandleBase[t] = TermFreeTerm;
          while (LOCAL_HandleBase[LOCAL_CurSlot - 1] == TermFreeTerm) {
            LOCAL_CurSlot--;
          }
          */
  };
#endif
 
  YAPTerm(long int num) { /// construct a term out of an integer (if you know object type use YAPIntegerTerm)
    CACHE_REGS mk(MkIntegerTerm(num)); };

  YAPTerm(double num) { /// construct a term out of an double (if you know object type use
    CACHE_REGS mk(MkFloatTerm(num)); };
  
  YAPTerm(std::string &name, std::vector<YAPTerm>  ts) { /// extract the tag of a term, after dereferencing.
    arity_t arity = ts.size();
    Functor f = Yap_MkFunctor(Yap_LookupAtom(name.c_str()),arity);
    std::vector<Term> nts(arity);
    for (arity_t i=0;i<arity;i++)
      nts[i] = ts[i].gt();
    mk(Yap_MkApplTerm(f,arity,nts.data()));
    };
YAP_tag_t tag();


  Term deepCopy(); ///< copy the term ( term copy )

  inline int numberVars(int start, bool singletons = false) {   /// number variables in term (see nummbervars/3) 
        CACHE_REGS
    Functor f = Yap_MkFunctor(AtomOfTerm(getAtomicLocalPrologFlag(NUMBERVARS_FUNCTOR_FLAG)),1);
     return Yap_NumberVars(gt(), start,f, singletons, nullptr PASS_REGS);
  }
  inline Term term() { /// convert from YAPTerm to Term (internal YAP representation)
    return Deref(gt());
  }
  
  YAPTerm arg(int i) { /// fetch argument i from the term (i counts from 1)
    BACKUP_MACHINE_REGS();
    Term t0 = gt();
    YAPTerm tf;
    if (!IsApplTerm(t0) && !IsPairTerm(t0))
      return (Term)0;
    tf = YAPTerm(ArgOfTerm(i, t0));
    RECOVER_MACHINE_REGS();
    return tf;
  };

    inline void bind(Term b) { /// set a variable in internal representation
      CACHE_REGS
      LOCAL_HandleBase[hdl] = b;
  }
  inline void bind(YAPTerm *b) {/// set a variable
    CACHE_REGS
    LOCAL_HandleBase[hdl] = b->term(); }
  Term &operator[](arity_t n);  ///<  fetch a sub-term from YAPTerm to Term (internal YAP representation)
  // const YAPTerm *vars();
  
  virtual bool exactlyEqual(YAPTerm t1) {/// this term is == to t1
    bool out;
    BACKUP_MACHINE_REGS();
    out = Yap_eq(gt(), t1.term());
    RECOVER_MACHINE_REGS();
    return out;
  };

virtual bool unify(YAPTerm t1) { /// unify this term with t1
    intptr_t out;
    BACKUP_MACHINE_REGS();
    out = Yap_unify(gt(), t1.term());
    RECOVER_MACHINE_REGS();
    return out;
  };

  virtual bool unifiable(YAPTerm t1) { /// we can unify t and t1
    bool out;
    BACKUP_MACHINE_REGS();
    out = Yap_eq(gt(), t1.term());
    RECOVER_MACHINE_REGS();
    return out;
  };

  inline virtual YAP_Term variant(YAPTerm t1) {  /// t =@@= t1, the two terms are equal up to variable renaming
    intptr_t out;
    BACKUP_MACHINE_REGS();
    out = Yap_Variant(gt(), t1.term());
    RECOVER_MACHINE_REGS();
    return out;
  };

  virtual intptr_t hashTerm(size_t sz, size_t depth, bool variant) {  /// compute a number from a ground term
    intptr_t out;

    BACKUP_MACHINE_REGS();
    out = Yap_TermHash(gt(), sz, depth, variant);
    RECOVER_MACHINE_REGS();
    return out;
  };

  virtual bool isVar() { return IsVarTerm(gt()); }    ///< type check for unbound
  virtual bool isAtom() { return IsAtomTerm(gt()); } ///<  type check for atom
  virtual bool isInteger() { return IsIntegerTerm(gt());  } ///< type check for integer
  virtual bool isFloat() {     return IsFloatTerm(gt());  } ///< type check for floating-point
  virtual bool isString() {    return IsStringTerm(gt());  } ///< type check for a string " ... "
  virtual bool isCompound() {    return !(IsVarTerm(gt()) || IsNumTerm(gt()));}///<< structured term or list
  virtual bool isAppl() { return IsApplTerm(gt());}  ///< is a structured term
  virtual bool isPair() { return IsPairTerm(gt());} ///< is a pair term
  virtual bool isGround() { return Yap_IsGroundTerm(gt());} ///< term is ground
  virtual bool isList() { return Yap_IsListTerm(gt()); }     ///< term is a list


  
  // Yap_RepStreamFromId(int sno)
  virtual Term getArg(arity_t i); ///< extract the argument i of the term, where i in 1...arity

  virtual inline arity_t arity() { /// extract the arity of the term; variables have arity 0.
    Term t0 = gt();

    if (IsApplTerm(t0)) {
      Functor f = FunctorOfTerm(t0);
      if (IsExtensionFunctor(f))
        return 0;
      return ArityOfFunctor(f);
    } else if (IsPairTerm(t0)) {
      return 2;
    }
    return 0;
  }

  virtual const char *text() {  /// return a string with a textual representation of the term
    CACHE_REGS
    char *os;

    BACKUP_MACHINE_REGS();
    if (!(os = Yap_TermToBuffer(Yap_GetFromSlot(hdl), Number_vars_f))) {
      RECOVER_MACHINE_REGS();
      return 0;
    }
    RECOVER_MACHINE_REGS();
    return os;
  };

  inline void reset() {     /// discard all YAPTerms older than this.
    CACHE_REGS
    LOCAL_CurSlot =
      hdl;
  }
  

  inline yhandle_t handle() { return hdl; }  ///< return a handle to the term


  inline bool initialized() { return hdl != 0; } ///< whether the term actually refers to a live object
  
};

  
/**
 * @class YAPApplTerm Compound Term
 *
 * A compound term, with functor and fixed number of arguments,
 * also known as a function application.
 *
 * Notice that lists and big numbers do not belong to this class.
 */
class X_API YAPApplTerm : public YAPTerm {
  friend class YAPTerm;


public:
  /// There are very many ways to build one of these terms:
  ///
  ///  1. engine representation to YAPApplTerm
  YAPApplTerm(Term t0) { if (IsApplTerm(t0)) mk(t0); }
  ///  1. this is the way the engine builds App, but in C you need
  ///   to give the arity. Notice we build from the engine world.
  YAPApplTerm(Functor f, Term ts[]) {
    BACKUP_MACHINE_REGS();
    Term t0 = Yap_MkApplTerm(f, f->ArityOfFE, ts);
    mk(t0);
    RECOVER_MACHINE_REGS();
  };
  ///  1. similar to before, but wwe use objects. This is useful if we
  ///  already got the objects.
  YAPApplTerm(YAPFunctor f, YAPTerm ts[]);
  /// not really needed, but we may not want to look inside
  /// the vector.
  YAPApplTerm(const std::string s, std::vector<Term> ts);
  YAPApplTerm(const std::string s, std::vector<YAPTerm> ts);
#if 0
  /// 1. C++11: notice we do not check if f==list.length();
  /// it should be
  template<typename... T>
    YAPApplTerm(const Functor f, std::initializer_list<Term> list ) {
     BACKUP_MACHINE_REGS();
     *HR++ = (CELL)f;
   for( auto elem : list )
    {
      RESET_VARIABLE(HR);
      Yap_unify( elem, (CELL)HR );
      HR++;
    }
      RECOVER_MACHINE_REGS();
  };

  YAPApplTerm(const std::string s, std::initializer_list<Term> list ) {/// 2. C++11 way, but YAP gets the arity from the vector length
          CACHE_REGS

     BACKUP_MACHINE_REGS();
     Term *o = HR++;
   for( auto elem : list )
    {
      RESET_VARIABLE(HR);
      Yap_unify( elem, (CELL)HR );
      HR++;
    }
   o[0] = (CELL)Yap_MkFunctor(Yap_LookupAtom(s.c_str()), (HR-(o+1)));
 mk(AbsAppl(o));
      RECOVER_MACHINE_REGS();
  };

  YAPApplTerm(const char s[], std::initializer_list<Term> list ) {
     BACKUP_MACHINE_REGS();
     Term *o = HR++;
   for( auto elem : list )
   {
   RESET_VARIABLE(HR);
     Yap_unify( elem, (CELL)HR );
     HR++;
     }
    o[0] = (CELL)Yap_MkFunctor(Yap_LookupAtom(s), (HR-(o+1)));
 mk(AbsAppl(o));
      RECOVER_MACHINE_REGS();
  };

   YAPApplTerm(const std::string s, std::initializer_list<YAPTerm> list ) {/// 1. variadic C++11 where arguments are objects.
    CACHE_REGS

    BACKUP_MACHINE_REGS();
    Term *o = HR++;
   for( auto elem : list )
    {
      RESET_VARIABLE(HR);
      Yap_unify( elem.gt(), (CELL)HR );
      HR++;
    }
   o[0] = (CELL)Yap_MkFunctor(Yap_LookupAtom(s.c_str()), (HR-(o+1)));
   mk(AbsAppl(o));
      RECOVER_MACHINE_REGS();
  };
#else
   YAPApplTerm(const std::string s, Term a1 ) {
    CACHE_REGS
   BACKUP_MACHINE_REGS();
   Term *o = HR;
   RESET_VARIABLE(HR+1);
Yap_unify( a1, (CELL)(HR+1) );
HR+=2;

 o[0] = (CELL)(YAPFunctor(s.c_str(), 1).fun());
mk(AbsAppl(o));
RECOVER_MACHINE_REGS();
};
YAPApplTerm(const std::string s, Term a1, Term a2 ) {
        CACHE_REGS

BACKUP_MACHINE_REGS();
Term *o = HR;
RESET_VARIABLE(HR+1);
Yap_unify( a1, (CELL)(HR+1) );
RESET_VARIABLE(HR+2);
Yap_unify( a2, (CELL)(HR+2) );
HR+=3;

o[0] = (CELL)Yap_MkFunctor(Yap_LookupAtom(s.c_str()), 2);
mk(AbsAppl(o));
RECOVER_MACHINE_REGS();
};
YAPApplTerm(const std::string s, Term a1, Term a2, Term a3 ) {
        CACHE_REGS

BACKUP_MACHINE_REGS();
Term *o = HR;
RESET_VARIABLE(HR+1);
Yap_unify( a1, (CELL)(HR+1) );
RESET_VARIABLE(HR+2);
Yap_unify( a2, (CELL)(HR+2) );
RESET_VARIABLE(HR+3);
Yap_unify( a3, (CELL)(HR+3) );
HR+=4;
o[0] = (CELL)Yap_MkFunctor(Yap_LookupAtom(s.c_str()),3);
mk(AbsAppl(o));
RECOVER_MACHINE_REGS();
};
#endif

 YAPApplTerm(const std::string s, unsigned int arity) {  ///  1. build empty compound term, that is, all arguments are free variables.

    mk(Yap_MkNewApplTerm(Yap_MkFunctor(Yap_LookupAtom(s.c_str()), arity),
                         arity));
  };

 YAPApplTerm(YAPFunctor f);  ///    use the functor object. to construct a new object

  inline Functor functor() { return FunctorOfTerm(gt()); }
  inline YAPFunctor getFunctor() { return YAPFunctor(FunctorOfTerm(gt())); }

    Term getArg(arity_t i) {

    Term t0 = gt();
    Term tf;
    tf = ArgOfTerm(i, t0);
    RECOVER_MACHINE_REGS();
    return tf;
  };
  void putArg(int i, Term targ) {
    // BACKUP_MACHINE_REGS();
    Term t0 = gt();
    RepAppl(t0)[i] = Deref(targ);
    // RECOVER_MACHINE_REGS();
  };
  void putArg(int i, YAPTerm t) {
    // BACKUP_MACHINE_REGS();
    Term t0 = gt();
    RepAppl(t0)[i] = t.term();
    // RECOVER_MACHINE_REGS();
  };
  virtual bool isVar() { return false; }     /// type check for unbound
  virtual bool isAtom() { return false; }    ///  type check for atom
  virtual bool isInteger() { return false; } /// type check for integer
  virtual bool isFloat() { return false; }   /// type check for floating-point
  virtual bool isString() { return false; }  /// type check for a string " ... "
  virtual bool isCompound() { return true; } /// is a primitive term
  virtual bool isAppl() { return true; }     /// is a structured term
  virtual bool isPair() { return false; }    /// is a pair term
  virtual bool isGround() { return true; }   /// term is ground
  virtual bool isList() { return false; }    /// term is a list
};

/**
 * @class List Constructor Term
 */
class X_API YAPPairTerm : public YAPTerm {
  friend class YAPTerm;

public:
  YAPPairTerm(Term t0) {
    t0 = Deref(t0);
    if (IsPairTerm(t0) || t0 == TermNil)
      mk(t0);
    else
      Yap_ThrowError(TYPE_ERROR_LIST, t0, "YAPPairTerms");
  }
  YAPPairTerm(YAPTerm hd, YAPTerm tl);
  YAPPairTerm();
  Term getHead() { return (HeadOfTerm(gt())); }
  Term getTail() { return (TailOfTerm(gt())); }
  YAPTerm car() { return YAPTerm(HeadOfTerm(gt())); }
  bool nil() { return gt() == TermNil; }
  YAPPairTerm cdr() { return YAPPairTerm(TailOfTerm(gt())); }
  std::vector<Term> listToArray();
  std::vector<YAPTerm> listToVector();
};

/**
 * @class Number Term
 */
class X_API YAPNumberTerm : public YAPTerm {
public:
  YAPNumberTerm(){};
  bool isTagged() { return IsIntTerm(gt()); }
};

/**
 * @class Integer Term
 */
class X_API YAPIntegerTerm : public YAPNumberTerm {
public:
  YAPIntegerTerm(intptr_t i);
  intptr_t getInteger() { return IntegerOfTerm(gt()); };
};

/**
 * @class YAPFloatTerm Floating Point Term
 */
class X_API YAPFloatTerm : public YAPNumberTerm {
public:
  YAPFloatTerm(double dbl) {
    CACHE_REGS
    mk(MkFloatTerm(dbl)); };

  double getFl() { return FloatOfTerm(gt()); };
};

/**
 * @class YapListTerm  list
 */
class X_API YAPListTerm : public YAPTerm {
public:
 
  YAPListTerm() { mk(TermNil); /* else type_error */ } /// Create an empty list term.
 
  YAPListTerm(Term t0) { mk(t0); /* else type_error */ } /// Create a list term out of a standard term. Check if a valid operation.

  YAPListTerm(Term ts[], size_t n); /// Create a list term out of an array of terms.

  YAPListTerm(std::vector<Term>); /// Create a list term out of a vector of terms.

  YAPListTerm(std::vector<YAPTerm>); /// Create a list term out of a vector of YAPTerms.
size_t length() { /// Return the number of elements in a list term.
    Term *tailp;
    Term t1 = gt();
    return Yap_SkipList(&t1, &tailp);
  }
 
  Term &operator[](size_t n); /// Extract the nth element.

  Term car(); /// Extract the first element of a list.

  Term cdr(); /// Extract the tail elements of a list.

  Term dup(); /// copy a list.
 inline bool nil() {/// Check if the list is empty.
    return gt() == TermNil;
  }

  ;
};

/**
 * @class YAPConjunctiveTerm A conjunction of terms: eg, the body of a rule
 *
 */
class X_API YAPConjunctiveTerm : public YAPTerm {
public:
  
  YAPConjunctiveTerm() { mk(TermTrue); /* else type_error */ }  /// create a true term

  YAPConjunctiveTerm(Term t0) { mk(t0); /* else type_error */ }  ///Create a conjunctive term out of a term.

  YAPConjunctiveTerm(const Term ts[], size_t n);  /// Create a conjunctive term out of an array of terms.

  YAPConjunctiveTerm(std::vector<Term>); /// Create a conjunctive term out of an array of terms.
    size_t length() { /// Return the number of elements in a conjunction
	    size_t n=1;
    Term t1 = gt();
    while (IsApplTerm(t1) && FunctorOfTerm(t1)==FunctorComma) {
      t1 = ArgOfTerm(2,t1);
      n++;
    }
    return n;
  }

 
  Term &operator[](size_t n); /// Extract the nth element.
  
  Term car(); /// Extract the first element of a listconjunction.
 
  Term cdr(); /// Extract the tail elements of a conjunction.
};

/**
 * Represent UTF-8 text as Prolog text,
 */
class X_API YAPStringTerm : public YAPTerm {
public:
  YAPStringTerm(std::string &s);  /// your standard constructor

 private:
};

/**
 * Term Representation of an Atom
 */
class X_API YAPAtomTerm : public YAPTerm {
  friend class YAPModule;
  /// Constructor: receives a C-atom;
  YAPAtomTerm(Term t) : YAPTerm(t) { IsAtomTerm(t); }

public:
  YAPAtomTerm(Atom a) { mk(MkAtomTerm(a)); }
  /// Constructor: receives an atom;
  YAPAtomTerm(YAPAtom a) : YAPTerm() { mk(MkAtomTerm(a.a)); }
  /// Constructor: receives a sequence of UTF-8 codes;
  YAPAtomTerm(char s[]);
  /// Constructor: receives a sequence of up to n UTF-8 codes;
  YAPAtomTerm(char *s, size_t len);
  /// Constructor: receives a sequence of wchar_ts, whatever they may be;
  YAPAtomTerm(wchar_t *s);
  /// Constructor: receives a sequence of n wchar_ts, whatever they may be;
  YAPAtomTerm(wchar_t *s, size_t len);
  ///Constructor: receives a std::string;
  YAPAtomTerm(std::string s) { mk(MkAtomTerm(Yap_LookupAtom(s.c_str()))) ;};
  //   };
  bool isVar() { return false; }           /// type check for unbound
  bool isAtom() { return true; }           ///  type check for atom
  bool isInteger() { return false; }       /// type check for integer
  bool isFloat() { return false; }         /// type check for floating-point
  bool isString() { return false; }        /// type check for a string " ... "
  bool isCompound() { return false; }      /// is a primitive term
  bool isAppl() { return false; }          /// is a structured term
  bool isPair() { return false; }          /// is a pair term
  virtual bool isGround() { return true; } /// term is ground
  virtual bool isList() { return gt() == TermNil; } /// [] is a list
  /// Getter: outputs the atom;
  YAPAtom getAtom() { return YAPAtom(AtomOfTerm(gt())); }
  /// Getter: outputs the name as a sequence of ISO-LATIN1 codes;
  const char *text() { return (const char *)AtomOfTerm(gt())->StrOfAE; }
};

/**
 * @class YAPVarTerm Variable Term
 */
class X_API YAPVarTerm : public YAPTerm {
  friend class YAPTerm;

public:
  YAPVarTerm() { /// constructor, creates new variable
      CACHE_REGS

    mk(MkVarTerm()); };
  YAPVarTerm(Term t) {/// constructor from existing variables
    if (IsVarTerm(t)) {
      mk(t);
    }
  }

    CELL *getVar() { return VarOfTerm(gt());  /// get the slot where the variable is.
    }

    bool unbound() { return IsUnboundVar(VarOfTerm(gt())); } /// is this bound?
  inline bool isVar() { return true; }      
  inline bool isAtom() { return false; }     ///  type check for atom
  inline bool isInteger() { return false; }  /// type check for integer
  inline bool isFloat() { return false; }    /// type check for floating-point
  inline bool isString() { return false; }   /// type check for a string " ... "
  inline bool isCompound() { return false; } /// is a primitive term
  inline bool isAppl() { return false; }     /// is a structured term
  inline bool isPair() { return false; }     /// is a pair term
  inline bool isGround() { return false; }   /// term is ground
  inline bool isList() { return false; }     /// term is a list
};

    extern "C" {
      X_API extern  Term Yap_MkErrorTerm(yap_error_descriptor_t *);
    }

///
/// Prolog container for error descriptors
///
class X_API YAPErrorTerm : public YAPTerm {
    friend class YAPTerm;
public:
    YAPErrorTerm() : YAPTerm( Yap_MkErrorTerm(nullptr)) {};
    YAPErrorTerm(yap_error_descriptor_t *err) :YAPTerm( Yap_MkErrorTerm(err) ) {};
};

/// @}
#endif /* YAPT_HH */
