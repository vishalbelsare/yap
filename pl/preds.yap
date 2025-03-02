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
* File:		preds.yap						 *
* Last rev:	8/2/88							 *
* mods:									 *
* comments:	Predicate Manipulation for YAP				 *
*									 *
*************************************************************************/

/**
 * @{
 * @defgroup Database The Clausal Data Base
 * @ingroup Builtins

Predicates in YAP may be dynamic or static. By default, when
consulting or reconsulting, predicates are assumed to be static:
execution is faster and the code will probably use less space.
Static predicates impose some restrictions: in general there can be no
addition or removal of  clauses for a procedure if it is being used in the
current execution.

Dynamic predicates allow programmers to change the Clausal Data Base with
the same flexibility as in C-Prolog. With dynamic predicates it is
always possible to add or remove clauses during execution and the
semantics will be the same as for C-Prolog. But the programmer should be
aware of the fact that asserting or retracting are still expensive operations,
and therefore he should try to avoid them whenever possible.

*/

:- system_module_( '$_preds', [abolish/1,
        abolish/2,
        assert/1,
        assert/2,
        assert_static/1,
        asserta/1,
        asserta/2,
        asserta_static/1,
        assertz/1,
        assertz/2,
        assertz_static/1,
        clause/2,
        clause/3,
        clause_property/2,
        compile_predicates/1,
        current_key/2,
        current_predicate/1,
        current_predicate/2,
        dynamic_predicate/2,
        hide_predicate/1,
        nth_clause/3,
        predicate_erased_statistics/4,
        predicate_property/2,
        predicate_statistics/4,
        retract/1,
        retract/2,
        retractall/1,
        stash_predicate/1,
        system_predicate/1,
        system_predicate/2,
        unknown/2], ['$assert_static'/5,
        '$assertz_dynamic'/4,
        '$clause'/4,
        '$current_predicate'/4,
        '$init_preds'/0,
        '$noprofile'/2,
        '$public'/2,
        '$unknown_error'/1,
        '$unknown_warning'/1]).

:- use_system_module( '$_boot', ['$check_head_and_body'/4,
        '$head_and_body'/2]).

:- use_system_module( '$_errors', [throw_error/2]).

:- use_system_module( '$_init', ['$do_log_upd_clause'/6,
        '$do_log_upd_clause0'/6,
        '$do_log_upd_clause_erase'/6,
        '$do_static_clause'/5]).

:- use_system_module( '$_modules', ['$imported_pred'/4,
        '$meta_predicate'/4,
        '$module_expansion'/5]).

:- use_system_module( '$_preddecls', ['$check_multifile_pred'/3,
        '$dynamic'/2]).

:- use_system_module( '$_strict_iso', ['$check_iso_strict_clause'/1]).



/** @pred  assert_static(: _C_)


Adds clause  _C_ to a static procedure. Asserting a static clause
for a predicate while choice-points for the predicate are available has
undefined results.


*/
assert_static(MC) :-
    strip_module(MC, M, C),
    '$compile'(C , assertz_static, C, M, 0, [] ).

/** @pred  asserta_static(: _C_)


Adds clause  _C_ as the first clause for a static procedure.


*/
asserta_static(MC) :-
    strip_module(MC, M, C),
    '$compile'(C , asserta_static, C, M, 0, [] ).


/** @pred  assertz_static(: _C_)


Adds clause  _C_ to the end of a static procedure.  Asserting a
static clause for a predicate while choice-points for the predicate are
available has undefined results.



The following predicates can be used for dynamic predicates and for
static predicates, if source mode was on when they were compiled:




*/
assertz_static(MC) :-
    strip_module(MC, M, C),
    '$compile'(C , assertz_static, C, M, 0, [] ).

/** @pred  clause(+ _H_, _B_) is iso


A clause whose head matches  _H_ is searched for in the
program. Its head and body are respectively unified with  _H_ and
 _B_. If the clause is a unit clause,  _B_ is unified with
 _true_.

This predicate is applicable to static procedures compiled with
`source` active, and to all dynamic procedures.*/
clause(V0,Q) :-
    must_be_callable( V0 ),
    '$yap_strip_module'(V0, M, V),
    '$predicate_type'(V,M,Type),
    '$clause'(Type,V,M,Q,_R).

/** @pred  clause(+ _H_, _B_,- _R_)

The same as clause/2, plus  _R_ is unified with the
reference to the clause in the database. You can use instance/2
to access the reference's value. Note that you may not use
erase/1 on the reference on static procedures.
*/
clause(P,Q,R) :-
	'$instance_module'(R,M0), !,
	instance(R,T0),
	( T0 = (H :- B) -> Q = B ; H=T0, Q = true),
    '$yap_strip_module'(P, M, T),
    '$yap_strip_module'(M0:H, M1, H1),
    (
     M == M1
    ->
    H1 = T
    ;
     M1:H1 = T
    ).
clause(V0,Q,R) :-
    '$imported_predicate'(V0,ExportingMod:V),
    '$predicate_type'(V,ExportingMod,Type),
    '$clause'(Type,V,ExportingMod,Q,R).


'$clause'(exo_procedure,P,M,true,exo(P)) :-
	'$execute0grep'(M:P).
'$clause'(mega_procedure,P,M,true,mega(P)) :-
	'$execute0'(M:P).
'$clause'(updatable_procedure, P,M,Q,R) :-
	'$log_update_clause'(P,M,Q,R).
'$clause'(source_procedure,P,M,Q,R) :-
    '$static_clause'(P,M,Q,R).
'$clause'(dynamic_procedure,P,M,Q,R) :-
	'$some_recordedp'(M:P), !,
	'$recordedp'(M:P,(P:-Q),R).
'$clause'(system_procedure,P,M,Q,R) :-
	\+ '$undefined'(P,M),
	functor(P,Name,Arity),
	throw_error(permission_error(access,system_procedure,Name/Arity),
	      clause(M:P,Q,R)).
'$clause'(private_procedure,P,M,Q,R) :-
	functor(P,Name,Arity),
	throw_error(permission_error(access,private_procedure,Name/Arity),
	      clause(M:P,Q,R)).
				    
'$init_preds' :-
	once('$do_static_clause'(_,_,_,_,_)),
	fail.
'$init_preds' :-
	once('$do_log_upd_clause'(_,_,_,_,_,_)),
	fail.
'$init_preds' :-
	once('$do_log_upd_clause_erase'(_,_,_,_,_,_)),
	fail.
'$init_preds'.

/** @pred  nth_clause(+ _H_, _I_,- _R_)


Find the  _I_th clause in the predicate defining  _H_, and give
a reference to the clause. Alternatively, if the reference  _R_ is
given the head  _H_ is unified with a description of the predicate
and  _I_ is bound to its position.


*/
nth_clause(V,I,R) :-
	'$imported_predicate'(V,M2:P2),
	'$nth_clause'(P2, M2, I, R).


'$nth_clause'(P,M,I,R) :-
	var(I), var(R), !,
	'$clause'(_,P,M,_,R),
	'$fetch_nth_clause'(P,M,I,R).
'$nth_clause'(P,M,I,R) :-
	'$fetch_nth_clause'(P,M,I,R).

/** @pred  abolish(+ _P_,+ _N_)

Completely delete the predicate with name _P_ and arity _N_. It will
remove both static and dynamic predicates. All state on the predicate,
including whether it is dynamic or static, multifile, or
meta-predicate, will be lost.
*/
abolish(N0,A) :-
    must_be_of_type(integer,A),
    strip_module(N0, Mod, N), !,
    must_be_atom(N),
    '$abolish'(N,A,Mod).

'$abolish'(N,A,M) :-
	( recorded('$predicate_defs','$predicate_defs'(N,A,M,_),R) -> erase(R) ),
	fail.
'$abolish'(N,A,M) :- functor(T,N,A),
		( '$is_dynamic'(T, M) -> '$abolishd'(T,M) ;
	      	 /* else */	      '$abolishs'(T,M) ).

/** @pred  abolish(+ _PredSpec_) is iso


Deletes the predicate given by  _PredSpec_ from the database. If
§§ _PredSpec_ is an unbound variable, delete all predicates for the
current module. The
specification must include the name and arity, and it may include module
information. Under <tt>iso</tt> language mode this built-in will only abolish
dynamic procedures. Under other modes it will abolish any procedures.


*/
abolish(X0) :-
    current_prolog_flag(language,iso), !,
    must_be_predicate_indicator(X0,M,N,A),
    '$new_abolish'(N,A,M).
abolish(X0) :-
    strip_module(X0,M,X),
    '$old_abolish'(X,M).

'$new_abolish'(Na,Ar, M) :-
	functor(H, Na, Ar),
	'$is_dynamic'(H, M), !,
	'$abolishd'(H, M).
'$new_abolish'(Na,Ar, M) :- % succeed for undefined procedures.
	functor(T, Na, Ar),
	'$undefined'(T, M), !.
'$new_abolish'(Na,Ar, M) :-
	throw_error(permission_error(modify,static_procedure,Na/Ar),abolish(M:Na/Ar)).
'$new_abolish'(Na,Ar, M) :-
    throw_error(type_error(predicate_indicator,Na/Ar),abolish(M:Na/Ar)).


'$check_error_in_module'(M, Msg) :-
	var(M), !,
	throw_error(instantiation_error, Msg).
'$check_error_in_module'(M, Msg) :-
	\+ atom(M), !,
	throw_error(type_error(atom,M), Msg).

'$old_abolish'(V,M) :- var(V), !,
         % current_prolog_flag(language, sicstus) ->
	    throw_error(instantiation_error,abolish(M:V))
	%;
	%    '$abolish_all_old'(M)
	%)
	.
'$old_abolish'(N/A, M) :- !,
	'$abolish'(N, A, M).
'$old_abolish'(A,M) :- atom(A), !,
	%( current_prolog_flag(language, iso) ->
	  throw_error(type_error(predicate_indicator,A),abolish(M:A))
	%;
	%    '$abolish_all_atoms_old'(A,M)
	%)
	.
'$old_abolish'([], _) :- !.
'$old_abolish'([H|T], M) :- !,  '$old_abolish'(H, M), '$old_abolish'(T, M).
'$old_abolish'(T, M) :-
	throw_error(type_error(predicate_indicator,T),abolish(M:T)).

'$abolishs'(G, M) :- '$is_system_predicate'(G,M), !,
	functor(G,Name,Arity),
	throw_error(permission_error(modify,static_procedure,Name/Arity),abolish(M:G)).
'$abolishs'(G, Module) :-
	current_prolog_flag(language, sicstus), % only do this in sicstus mode
	'$undefined'(G, Module),
	functor(G,Name,Arity),
	print_message(warning,no_match(abolish(Module:Name/Arity))).
'$abolishs'(T, M) :-
	retractall('$import'(_,M,_,T,_,_)),
	fail.
'$abolishs'(G, M) :-
	'$purge_clauses'(G, M), fail.
'$abolishs'(_, _).

/**  @pred stash_predicate(+ _Pred_)
Make predicate  _Pred_ invisible to new code, and to current_predicate/2,
`listing`, and friends. New predicates with the same name and
functor can be declared.
 **/
stash_predicate(P0) :-
    must_be_predicate_indicator(P0,M,N,A),
    '$stash_predicate2'(N,A, M).

'$stash_predicate2'(N,A, M) :- !,
	functor(S,N,A),
	'$stash_predicate'(S, M) .

/** @pred hide_predicate(+ _Pred_)
Make predicate  _Pred_ invisible to `current_predicate/2`,
`listing`, and friends.

 **/


/** @pred  predicate_property( _P_, _Prop_) is iso


For the predicates obeying the specification  _P_ unify  _Prop_
  with a property of  _P_. These properties may be:

+ `built_in `
true for built-in predicates,

+ `dynamic`
true if the predicate is dynamic

+ `static `
true if the predicate is static

+ `meta_predicate( _M_) `
true if the predicate has a meta_predicate declaration  _M_.

+ `multifile `
true if the predicate was declared to be multifile

+ `imported_from( _Mod_) `
true if the predicate was imported from module  _Mod_.

+ `file(_File_) `
true if the predicate was declared in file  _File_. Unavailable for multi-file predicates.

+ `exported `
true if the predicate is exported in the current module.

+ `public`
true if the predicate is public; note that all dynamic predicates are
public.

+ `tabled `
true if the predicate isd tabled; note that only static predicates can
be tabled in YAP.

+ `source (predicate_property flag) `
true if source for the predicate is available.

+ `number_of_clauses( _ClauseCount_) `
Number of clauses in the predicate definition. Always one if external
or built-in.

*/
predicate_property(Pred,Prop) :-
    '$yap_strip_module'(Pred, M, P),
    (var(M)
    ->
	'$all_current_modules'(M)
       ;
      true
    ),
    (var(P) %
    ->
    module_predicate(M,N,Ar,_),
      functor(P,N,Ar)
    ;
    true
    ),
    (
    '$pred_exists'(P,prolog)
    ->
	'$predicate_property'(P,prolog,Prop)
    ;
      '$is_proxy_predicate'(P,M)
      ->
(
	'$import_chain'(M,P,M0,P0),
      '$pred_exists'(P0,M0),
      (Prop = imported_from(M0)
	;
	'$predicate_property'(P0,M0,Prop)
      )
)
    ;
    '$predicate_property'(P,M,Prop)
    ).

'$predicate_property'(P,M,meta_predicate(Q)) :-
    functor(P,Na,Ar),
    functor(Q,Na,Ar),
    recorded('$m', meta_predicate(M,Q),_).
'$predicate_property'(P,M,Prop) :-
    '$predicate_type'(P,M,Type),
    (Type == undefined -> !,fail;
     Type == system_procedure -> Prop=built_in;
     Type == updatable_procedure ->
	 (
	     Prop=dynamic
		 ;
		 Prop = source
		 ;
		 '$is_thread_local'(P,M)
		 ->
		 Prop = (thread_local)
	 )
     ;
     (
	 Prop=static
     ;
     Type == mega_procedure -> Prop=mega
     
     )
    ).

'$predicate_property'(P,M,file(File)) :-
    \+ '$is_multifile'(P, M),
    M\=prolog,
    '$owner_file'(P,M,File).
'$predicate_property'(P,M,line_count(Line)) :-
    M\=prolog,
    '$owner_file_line'(P,M,Line).
'$predicate_property'(P,M,multifile) :-
	'$is_multifile'(P,M).
'$predicate_property'(P,M,source) :-
	'$has_source'(P,M).
'$predicate_property'(P,M,tabled) :-
    '$is_tabled'(P,M).
'$predicate_property'(P,M,public) :-
	'$is_public'(P,M).
'$predicate_property'(P,Mod,number_of_clauses(NCl)) :-
    '$number_of_clauses'(P,Mod,
			 NCl).

/**
  @pred  predicate_statistics( _P_, _NCls_, _Sz_, _IndexSz_)

Given predicate  _P_,  _NCls_ is the number of clauses for
 _P_,  _Sz_ is the amount of space taken to store those clauses
(in bytes), and  _IndexSz_ is the amount of space required to store
indices to those clauses (in bytes).
*/
predicate_statistics(V,NCls,Sz,ISz) :- var(V), !,
	throw_error(instantiation_error,predicate_statistics(V,NCls,Sz,ISz)).
predicate_statistics(P0,NCls,Sz,ISz) :-
	strip_module(P0, M, P),
	'$predicate_statistics'(P,M,NCls,Sz,ISz).

'$predicate_statistics'(M:P,_,NCls,Sz,ISz) :- !,
	'$predicate_statistics'(P,M,NCls,Sz,ISz).
'$predicate_statistics'(P,M,NCls,Sz,ISz) :-
	'$is_log_updatable'(P, M), !,
	'$lu_statistics'(P,NCls,Sz,ISz,M).
'$predicate_statistics'(P,M,_,_,_) :-
	'$is_system_predicate'(P,M), !, fail.
'$predicate_statistics'(P,M,_,_,_) :-
	'$undefined'(P,M), !, fail.
'$predicate_statistics'(P,M,NCls,Sz,ISz) :-
	'$static_pred_statistics'(P,M,NCls,Sz,ISz).

/** @pred  predicate_erased_statistics( _P_, _NCls_, _Sz_, _IndexSz_)


Given predicate  _P_,  _NCls_ is the number of erased clauses for
 _P_ that could not be discarded yet,  _Sz_ is the amount of space
taken to store those clauses (in bytes), and  _IndexSz_ is the amount
of space required to store indices to those clauses (in bytes).

 */
predicate_erased_statistics(P,NCls,Sz,ISz) :-
        var(P), !,
	current_predicate(_,P),
	predicate_erased_statistics(P,NCls,Sz,ISz).
predicate_erased_statistics(P0,NCls,Sz,ISz) :-
	strip_module(P0,M,P),
	'$predicate_erased_statistics'(M:P,NCls,Sz,_,ISz).

/** @pred  current_predicate( _A_, _P_)

Defines the relation:  _P_ is a currently defined predicate whose name is the atom  _A_.
*/
current_predicate(A,T0) :-
    '$yap_strip_module'(T0, M, T),
    (var(M) -> '$all_current_modules'(M);  must_be_atom(M)),
    (nonvar(T) ->
	functor(T,A,Ar),
        functor_predicate(M,A,Ar,user)
    ;atom(A) ->
    atom_functor(A,Ar),
    functor(T,A,Ar),
    functor_predicate(M,A,Ar,user)
    ;
    module_predicate(M,A,Ar,user),
    functor(T,A,Ar)
    ).

:- meta_predicate system_predicate(:), system_predicate(?,:).


/** @pred  system_predicate( ?_P_ )

Defines the relation:  indicator _P_ refers to a currently defined system predicate.
*/
system_predicate(T0) :-
    '$yap_strip_module'(T0, M, T),    
    (      var(M) -> '$all_current_modules'(M);  must_be_atom(M)
    ),
    (
      var(T) -> module_predicate(_,A,Ar,system)
	;
	T = A//Ar, nonvar(A) ->
	atom_functor(A,Ar),
	functor_predicate(M,A,Ar0,system),
	Ar is Ar0+2
    ;
    T = A/Ar, nonvar(A) -> 
	atom_functor(A,Ar),
	functor_predicate(M,A,Ar,system);
	throw_error(type_error(predicate_indicator,T),
                system_predicate(T))
    ).



/** @pred  system_predicate( ?A, ?P )

  Succeeds if _A_ is the name of the system predicate _P_. It can be
  used to test or  to enumerate all system predicates.
*/

system_predicate(A, P0) :-
    may_bind_to_type(atom,A),
    may_bind_to_type(callable,P0),
    '$yap_strip_module'(P0, M, P),

    (
	nonvar(P)
	  ->
	  functor(P,N,A),
	  functor_predicate(M,N,A,system)
    ;
    (      var(M) -> '$all_current_modules'(M);  must_be_atom(M)),
    module_predicate(A, Na, Ar, system),
	  functor(P,Na,Ar)
    ).

    


/**
  @pred  current_predicate( F ) is iso

  True if _F_ is the predicate indicator for a currently defined user or
  library predicate.The indicator  _F_ is of the form _Mod_:_Na_/_Ar_ or _Na/Ar_,
  where the atom _Mod_ is the module of the predicate,
 _Na_ is the name of the predicate, and  _Ar_ its arity.
*/
current_predicate(T0) :-
    '$yap_strip_module'(T0, M, T),
    (
      var(M) -> '$all_current_modules'(M)
	;
	must_be_atom(M)
    ),
    (
	T=A/Ar, var(A) ->
	module_predicate(M,A,Ar,user)
    ;
    T = A//Ar, var(A) ->
  	module_predicate(M,A,Ar0,user),
     Ar is Ar0+2
;
    T = A/Ar, nonvar(A) -> atom_functor(A,Ar),
    functor_predicate(M,A,Ar,user)

    ;
    T = A//Ar, nonvar(A) -> atom_functor(A,Ar),
	  functor_predicate(M,A,Ar0,user),
	  Ar is Ar0-2
    ;
    error(type_error(predicate_indicator,T),
               current_predicate(T))
   ).

% do nothing for now.
'$noprofile'(_, _).

'$ifunctor'(Pred,Na,Ar) :-
	(Ar > 0 ->
	    functor(Pred, Na, Ar)
	;
	     Pred = Na
	 ).


/** @pred  compile_predicates(: _ListOfNameArity_)



Compile a list of specified dynamic predicates (see dynamic/1 and
assert/1 into normal static predicates. This call tells the
Prolog environment the definition will not change anymore and further
calls to assert/1 or retract/1 on the named predicates
raise a permission error. This predicate is designed to deal with parts
 the program that is generated at runtime but does not change during
the remainder of the program execution.
 */
compile_predicates(Ps) :-
	'$current_module'(Mod),
	'$compile_predicates'(Ps, Mod, compile_predicates(Ps)).

'$compile_predicates'(V, _, Call) :-
	var(V), !,
	throw_error(instantiation_error,Call).
'$compile_predicates'(M:Ps, _, Call) :-
	'$compile_predicates'(Ps, M, Call).
'$compile_predicates'([], _, _).
'$compile_predicates'([P|Ps], M, Call) :-
	'$compile_predicate'(P, M, Call),
	'$compile_predicates'(Ps, M, Call).

'$compile_predicate'(P, _M, Call) :-
	var(P), !,
	throw_error(instantiation_error,Call).
'$compile_predicate'(M:P, _, Call) :-
	'$compile_predicate'(P, M, Call).
'$compile_predicate'(Na/Ar, Mod, _Call) :-
	functor(G, Na, Ar),
	findall([G|B],clause(Mod:G,B),Cls),
	abolish(Mod:Na,Ar),
	'$add_all'(Cls, Mod).

'$add_all'([], _).
'$add_all'([[G|B]|Cls], Mod) :-
	assert_static(Mod:(G:-B)),
	'$add_all'(Cls, Mod).


clause_property(ClauseRef, file(FileName)) :-
	instance_property(ClauseRef, 2, FileName).
clause_property(ClauseRef, source(FileName)) :-
    instance_property(ClauseRef, 2, SourceName ),
    ( recorded('$includes',(FileName->SourceName), _) 
    ->
    true
    ;
    FileName = SourceName
    ).
clause_property(ClauseRef, line_count(LineNumber)) :-
	instance_property(ClauseRef, 4, LineNumber),
	LineNumber > 0.
clause_property(ClauseRef, fact) :-
	instance_property(ClauseRef, 3, true).
clause_property(ClauseRef, erased) :-
	instance_property(ClauseRef, 0, true).
clause_property(ClauseRef, predicate(PredicateIndicator)) :-
	instance_property(ClauseRef, 1, PredicateIndicator).


/**
@}
*/
