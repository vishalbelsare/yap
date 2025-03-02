/**
*   @file yapq.hh
 *
 *   @defgroup FLI_YAP-cplus-query-handling Query Handling in the YAP interface.
 *   @brief Engine and Query Management
 *
 *   @ingroup FLI_YAP-cplus-interface
 *
 * @{
 *
 * These classes wrap  engine and query. An engine is an environment where we
 * can rum Prolog, that is, where we can run queries.
 *
 * Also, supports callbacks and engine configuration.
 *
 */

#ifndef YAPQ_HH
#define YAPQ_HH 1


class X_API YAPPredicate;

/**
   Queries and engines
*/

#if __ANDROID__

#endif

/**
 * @brief Queries
 *
 * interface to a YAP Query;
 * uses an SWI-like status info internally.
 */
class X_API YAPQuery : public X_API YAPPredicate {
  bool q_open;
  int q_state;
  // yhandle_t q_handles;
  int q_flags;
  YAP_dogoalinfo q_h;
  YAPPairTerm *names;
  Term goal;
  CELL *nts;
  // temporaries
  YAPError *e;

  inline void setNext() { // oq = LOCAL_execution;
          CACHE_REGS

    //  LOCAL_execution = this;
    q_open = true;
    q_state = 0;
    q_flags = true; // PL_Q_PASS_EXCEPTION;

    q_h.p = P;
    q_h.cp = CP;
    // make sure this is safe
    q_h.CurSlot = LOCAL_CurSlot;
  };

  void openQuery();

  PredEntry *rewriteUndefQuery();

public:
  YAPQuery() {
    goal = TermTrue;
    openQuery();
  };
  inline ~YAPQuery() { close(); }
  ///< main constructor, uses a predicate and an array of terms
  ///<
  ///< It is given a YAPPredicate _p_ , and an array of terms that must have at
  ///< least
  ///< the same arity as the functor.
  YAPQuery(YAPPredicate p, YAPTerm t[]);
  ///<
  ///< full constructor,
  ///<
  ///<
  ///< It is given a functor, module, and an array of terms that must have at
  ///< least
  ///< the same arity as the functor.
  YAPQuery(YAPFunctor f, YAPTerm mod, YAPTerm t[]);
  ///< often, this is more efficient
  ///<
  YAPQuery(YAPFunctor f, YAPTerm mod, Term t[]);
  ///< functor/term constructor,
  ///<
  ///< It is given a functor, and an array of terms that must have at least
  ///< the same arity as the functor. Works within the current module.
  // YAPQuery(YAPFunctor f, YAPTerm t[]);
  ///< string constructor without varnames
  ///<
  ///< It is given a string, calls the parser and obtains a Prolog term that
  ///< should be a callable
  ///< goal.
  inline YAPQuery(const char *s) : YAPPredicate(s, goal, names, (nts = a1_ptr())) {
    __android_log_print(ANDROID_LOG_INFO, "YAPDroid", "got game %ld",
                        LOCAL_CurSlot);

    openQuery();
  };
  
  // inline YAPQuery() : YAPPredicate(s, tgoal, tnames)
  // {
  //     __android_log_print(ANDROID_LOG_INFO, "YAPDroid", "got game %ld",
  //     if (!ap)
  //         return;
  //     __android_log_print(ANDROID_LOG_INFO, "YAPDroid", "%s", vnames.text());
  //     goal = YAPTerm(tgoal);
  //     names = YAPPairTerm(tnames);
  //     openQuery(tgoal);
  // };
  ///< string constructor with just an atom
  ///<
  ///< It i;
  ///<};
  ///< build a query from a term
  YAPQuery(YAPTerm t) : YAPPredicate((goal = t.term()), (nts = a1_ptr())) {
    BACKUP_MACHINE_REGS();
    openQuery();
    names = new YAPPairTerm(TermNil);
    RECOVER_MACHINE_REGS();
  }
  ///< set flags for query execution, currently only for exception handling
  void setFlag(int flag) { q_flags |= flag; }
  ///< reset flags for query execution, currently only for exception handling
  void resetFlag(int flag) { q_flags &= ~flag; }
  ///< first query
  ///<
  ///< actually implemented by calling the next();
  inline bool first() { return next(); }
  ///< ask for the next solution of the current query
  ///< same call for every solution
  bool next();
  ///< an alias to avoid saying next()
  inline bool answer() { return next(); }
  ///< does this query have open choice-points?
  ///< or is it deterministic?
  bool deterministic();
  ///< represent the top-goal
  const char *text();
  ///< remove alternatives in the current search space, and finish the current
  ///< query
  ///< finish the current query: undo all bindings.
  void close();
  ///< query variables.
  void cut();
  Term namedVars() { return names->term(); };
  YAPPairTerm *namedVarTerms() { return names; };
  ///< query variables, but copied out
  std::vector<Term> namedVarsVector() { return names->listToArray(); };
  ///< convert a ref to a binding.
  YAPTerm getTerm(yhandle_t t);
  ///< simple YAP Query;
  ///< just calls YAP and reports success or failure, Useful when we just
  ///< want things done, eg YAPCommand("load_files(library(lists), )")
  inline bool command() {
    bool rc = next();
    close();
    return rc;
  };

  //> access to input argument i as a term
  Term x(int i) {
          CACHE_REGS

    return XREGS[i];
  }
  //>  access to input argument as a YAPTerm
  YAPTerm X(int i) {
          CACHE_REGS


    return YAPTerm(XREGS[i]); }

  //> unify term t with argumentr i
  bool output( Term t, int i) {
      CACHE_REGS
    return Yap_unify(XREGS[i], t);
  };
  bool output( YAPTerm t, int i) {
          CACHE_REGS

    return t.unify(XREGS[i]); };

  //> ensure at least cells cells are available
  bool ensureStorage( size_t cells) {
      CACHE_REGS

    return Yap_dogcl(cells*sizeof(CELL) PASS_REGS);
  };
  //>  ensure memory but take care to first save the terms in
  //>  ts. YAPTerms do no require this.
  bool ensureStorage( size_t cells, std::vector<Term> ts)    
  {
          CACHE_REGS

    return Yap_dogcl(cells*sizeof(CELL) PASS_REGS); };
};



/// This class implements a callback Prolog-side. It will be inherited by the
/// Java or Python
/// class that actually implements the callback.
class X_API YAPCallback {
public:
  virtual ~YAPCallback() {}
  virtual void run() { LOG("callback"); }
  virtual void run(char *s) {}
};

/// @brief  arguments used  to initialise a new engine
struct X_API YAPEngineArgs : YAP_init_args {

public:
  YAPEngineArgs() {
    memset(this,0,sizeof(YAPEngineArgs));
    // const std::string *s = new std::string("startup.yss");
    Embedded = true;
    install = false;
    Yap_InitDefaults(&this->start, nullptr, 0, nullptr);
#if YAP_PYTHON
    Embedded = true;
    python_in_python = Py_IsInitialized();
#endif
#if __ANDROID__
#endif
  };

  inline void setEmbedded(bool fl) { Embedded = fl; };

  inline bool getEmbedded() { return Embedded; };

  inline void setStackSize(bool fl) { StackSize = fl; };

  inline bool getStackSize() { return StackSize; };

  inline void setTrailSize(bool fl) { TrailSize = fl; };

  inline bool getTrailSize() { return TrailSize; };

  inline bool getMStackSize() { return StackSize; };

  inline void setMaxTrailSize(bool fl) { MaxTrailSize = fl; };

  inline bool getMaxTrailSize() { return MaxTrailSize; };

  inline void createSavedState(bool fl) { install = fl; };

  inline bool creatingSavedState() { return install; };

};

/**
 * @brief YAP Engine: creates an execution environment
 where we can  run goals.
 *
 *
 */
class YAPEngine {
private:
  YAPEngineArgs *engine_args;
  YAPCallback *_callback;
  YAPError yerror;
  void doInit(YAP_file_type_t BootMode, YAPEngineArgs *cargs);
  YAPError e;
  PredEntry *rewriteUndefEngineQuery(PredEntry *ap, Term &t, Term tmod);

public:
  ///< construct a new engine; may use a variable number of arguments
  std::string port = "call";
  YAPEngine(YAPEngineArgs cargs=YAPEngineArgs()) {
    engine_args =& cargs;
    // doInit(cargs->boot_file_type);
    __android_log_print(
    ANDROID_LOG_INFO, "YAPDroid", "start engine  ");
#ifdef __ANDROID__
    doInit(YAP_PL, &cargs);

#else
    doInit(YAP_QLY, &cargs);
#endif
  }; ///< construct a new engine, including aaccess to callbacks
  ///< construct a new engine using argc/argv list of arguments
  YAPEngine(int argc, char *argv[],
            YAPCallback *callback = (YAPCallback *)NULL);
  ///< kill engine
  ~YAPEngine() { delYAPCallback(); };
  ///< remove current callback
  void delYAPCallback() { _callback = 0; };
  ///< set a new callback
  void setYAPCallback(YAPCallback *cb) {
    delYAPCallback();
    _callback = cb;
  };
  ///< execute the callback.
  ///</void run() { if (_callback) _callback.run(); }
  ///< execute the callback with a text argument.
  void run(char *s) {
    if (_callback)
      _callback->run(s);
  }
  ///< stop yap
  void close() { Yap_exit(0); }

  ///< execute the callback with a text argument.
  bool hasError() {
    CACHE_REGS
    return LOCAL_Error_TYPE != YAP_NO_ERROR; }
  ///< build a query on the engine
  YAPQuery *query(const char *s) { return new YAPQuery(s); };
  ///< build a query from a term
  YAPQuery *query(YAPTerm t) { return new YAPQuery(t); };
  ///< build a query from a Prolog term (internal)
  YAPQuery *qt(Term t) { return new YAPQuery(YAPTerm(t)); };
  ///< given a handle, fetch a term from the engine
  inline YAPTerm getTerm(yhandle_t h) { return YAPTerm(h); }
  ///< current directory for the engine
  bool call(YAPPredicate ap, YAPTerm ts[]);
  ///< current directory for the engine
  bool goal(YAPTerm Yt, YAPModule module, bool release = false) {
    return mgoal(Yt.term(), module.term(), release);
  };
  ///< ru1n a goal in a module.
  ///<
  ///< By default, memory will only be fully
  ///< recovered on backtracking. The release option ensures
  ///< backtracking is called at the very end.
  bool mgoal(Term t, Term tmod, bool release = false);
  ///< current directory for the engine

    bool goal(YAPTerm t, bool release = false) { return goal(t.term(), release); }
    bool goal(Term t, bool release = false) {
    return mgoal(t, YAP_CurrentModule(), release);
  }
  ///< reset Prolog state
  void reSet();
  ///< assune that there are no stack pointers, just release memory
  ///< for last execution
  void release();

  ///< call load_files to load a file in a module
  bool load_file(std::string  FileName, std::string module="user" )
  {
    CACHE_REGS
    YAPTerm name = YAPAtomTerm(FileName);
    YAPTerm lf =  YAPApplTerm("load_files", {name, YAPListTerm()});
    return mgoal(lf.term(), CurrentModule, true);
      }
  ///< call load_files to load a library(file) in a module
  bool load_library(std::string  FileName)
  {
    YAPTerm name = YAPAtomTerm(FileName);
    std::vector<YAPTerm> ts = {name};
    name = YAPApplTerm("library",ts);

    YAPTerm lf =  YAPApplTerm(std::string("load_files"), {name, YAPPairTerm(TermNil)});
    return goal(lf, YAPModule());
  };
  Term top_level(std::string s);
  Term next_answer(YAPQuery*&);

  //> call a deterninistic predicate: the user will construct aterm of
  //> arity N-1. YAP adds an extra variable which will have the
  //> output.
  Term fun(Term t);
  YAPTerm funCall(YAPTerm t) { return YAPTerm(fun(t.term())); };

  
#if 0
template<class STREAM>
struct STDIOAdapter
{
    static FILE* yield(STREAM* stream)
    {
      // assert(stream != NULL);

        static cookie_io_functions_t Cookies =
        {
            .read  = NULL,
            .write = cookieWrite,
            .seek  = NULL,
            .close = cookieClose
        };

        return fopencookie(stream, "w", Cookies);
    };

     static ssize_t cookieWrite(void* cookie,
        const char* buf,
        size_t size)
    {
        if(cookie == NULL)
            return -1;
1
        STREAM* writer = static_cast <STREAM*>(cookie);

        writer->write(buf, size);

        return size;
    }

     static int cookieClose(void* cookie)
    {
         return EOF;
    }

  // STDIOAdapter


 bool load_stream(std::iostream Stream, bool library=false, std::string module=nullptr)
  {
    FILE* fp = STDIOAdapter<std::iostream>::yield(&Stream);
    if (module.empty()) {
      module = RepAtom(AtomOfTerm(CurrentModule))->StrOfAE;
    }
    

    YAPTerm stream = YAPApplTerm("stream", YAPListTerm({ YAPIntegerTerm(fileno(fp)) })),
      mod = YAPApplTerm("module", {YAPAtomTerm(module)});
    YAPTerm  	 lf =  YAPApplTerm("load_files", {YAPAtomTerm("jupyter"), YAPListTerm ({stream,mod)}});
    return goal(lf, YAPAtomTerm(module), true);
  }
#endif
///< load a string as if  it was a file.
  bool load_text(std::string text, std::string *module=nullptr)
  {
    YAPTerm s = YAPStringTerm(text);
    YAPModule mod;
    if (module !=nullptr)
      mod =YAPModule(*module);
    else
      mod = YAPModule();

    return goal(YAPApplTerm("load_files",{YAPApplTerm("string",{s})}), mod
, true);
  }
   


  const char *currentDir() {
    char dir[1024];
    std::string *s = new std::string(Yap_getcwd(dir, 1024 - 1));
    return s->c_str();
  };
  ///< report YAP version as a string
  const char *version() {
    std::string *s = new std::string(Yap_version());
    return s->c_str();
  };
  //Term fun(YAPTerm t) { return fun(t.term()); };
  //> set a StringFlag, usually a path
  //>
  bool setStringFlag(std::string arg, std::string path) {
    return Yap_set_flag(MkAtomTerm(Yap_LookupAtom(arg.data())),
                        MkAtomTerm(Yap_LookupAtom(path.data())));
  };

};

#endif /* YAPQ_HH */

/// @}
