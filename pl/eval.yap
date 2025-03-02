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
* File:		eval.yap						 *
* Last rev:								 *
* mods:									 *
* comments:	optimise disjunction handling				 *
*									 *
*************************************************************************/

/**
  * @file   eval.yap
  * @author VITOR SANTOS COSTA <vsc@VITORs-MBP-2.lan>
  * @date   Thu Oct 19 11:52:48 2017
  *
  * @brief  Compiling expressions
  *
  * @defgroup CompiledExpressions Compiled Form of Arithmetic Expressions in Prolog
  * @ingroup Directives
  * @{
  *
*/
:- system_module_( '$_eval', [], ['$full_clause_optimisation'/4]).

:- use_system_module( terms, [new_variables_in_term/3,
        variables_within_term/3]).



'$add_extra_safe'('$plus'(_,_,V)) --> !, [V].
'$add_extra_safe'('$minus'(_,_,V)) --> !, [V].
'$add_extra_safe'('$times'(_,_,V)) --> !, [V].
'$add_extra_safe'('$div'(_,_,V)) --> !, [V].
'$add_extra_safe'('$and'(_,_,V)) --> !, [V].
'$add_extra_safe'('$or'(_,_,V)) --> !, [V].
'$add_extra_safe'('$sll'(_,_,V)) --> !, [V].
'$add_extra_safe'('$slr'(_,_,V)) --> !, [V].
'$add_extra_safe'(C=D,A,B) :-
   !,
   ( compound(C) ->
     '$variables_in_term'(C,E,A)
   ;
     E=A
   ),
   ( compound(D) ->
     '$variables_in_term'(D,B,E)
   ;
     B=E
   ).
'$add_extra_safe'(_) --> [].


'$gen_equals'([], [], _, O, O).
'$gen_equals'([V|Commons],[NV|NCommons], LV0, O, NO) :- V == NV, !,
	'$gen_equals'(Commons,NCommons, LV0, O, NO).
'$gen_equals'([V|Commons],[NV|NCommons], LV0, O, OO) :-
	'$vmember'(V,LV0),
         OO = (V=NV,'$safe'(NV),NO),
	'$gen_equals'(Commons,NCommons, LV0, O, NO).
'$gen_equals'([V|Commons],[NV|NCommons], LV0, O, OO) :-
         OO = (V=NV,NO),
	'$gen_equals'(Commons,NCommons, LV0, O, NO).

'$safe_guard'((A,B), M) :- !,
	    '$safe_guard'(A, M),
	    '$safe_guard'(B, M).
'$safe_guard'((A;B), M) :- !,
	    '$safe_guard'(A, M),
	    '$safe_guard'(B, M).
'$safe_guard'(A, M) :- !,
	    '$safe_builtin'(A, M).

'$safe_builtin'(G, Mod) :-
	'$predicate_flags'(G, Mod, Fl, Fl),
	Fl /\ 0x00008880 =\= 0.


'$localise_disj_vars'((B;B2), M, (NB ; NB2), LV, LV0, LEqs) :- !,
	'$localise_vars'(B, M, NB, LV, LV0, LEqs),
	'$localise_disj_vars'(B2, M, NB2, LV, LV0, LEqs).
'$localise_disj_vars'(B2, M, NB, LV, LV0, LEqs) :-
	'$localise_vars'(B2, M, NB, LV, LV0, LEqs).

'$localise_vars'((A->B), M, (A->NB), LV, LV0, LEqs) :-
	'$safe_guard'(A, M), !,
	'$variables_in_term'(A, LV, LV1),
	'$localise_vars'(B, M,  NB, LV1, LV0, LEqs).
'$localise_vars'((A;B), M, (NA;NB), LV1, LV0, LEqs) :- !,
	'$localise_vars'(A, M,  NA, LV1, LV0, LEqs),
	'$localise_disj_vars'(B, M,  NB, LV1, LV0, LEqs).
'$localise_vars'(((A,B),C), M, NG, LV, LV0, LEqs) :- !,
	'$flatten_bd'((A,B),C,NB),
	'$localise_vars'(NB, M, NG, LV, LV0, LEqs).
'$localise_vars'((!,B), M, (!,NB), LV, LV0, LEqs) :- !,
	'$localise_vars'(B, M,  NB, LV, LV0, LEqs).
'$localise_vars'((X=Y,B), M, (X=Y,NB1), LV, LV0, LEqs) :-
	var(X), var(Y), !,
	'$localise_vars'(B, M,  NB1, LV, LV0, [X,Y|LEqs]).
'$localise_vars'((G,B), M, (G,NB1), LV, LV0, LEqs) :-
	'$safe_builtin'(G, M), !,
	'$variables_in_term'(G, LV, LV1),
	'$add_extra_safe'(G, NLV0, LV0),
	'$localise_vars'(B, M,  NB1, LV1, NLV0, LEqs).
'$localise_vars'((G1,B1), _, O, LV, LV0, LEqs) :- !,
	terms:variables_within_term(LV, B1, Commons),
	terms:new_variables_in_term(LV, B1, New),
	copy_term(Commons+New+LEqs+B1, NCommons+NNew+NLEqs+NB1),
	NNew = New,
	NLEqs = LEqs,
	'$gen_equals'(Commons, NCommons, LV0, (G1,NB1), O).
'$localise_vars'(G, _, G, _, _, _).

'$flatten_bd'((A,B),R,NB) :- !,
	'$flatten_bd'(B,R,R1),
	'$flatten_bd'(A,R1,NB).
'$flatten_bd'(A,R,(A,R)).

% the idea here is to make global variables in disjunctions
% local.
'$localise_vars_opt'(H, M, (B1;B2), (NB1;NB2)) :-
	'$variables_in_term'(H, [], LV),
	'$localise_vars'(B1, M, NB1, LV, LV, []),
	'$localise_disj_vars'(B2, M, NB2, LV, LV, []).


%, portray_clause((H:-BF))
%'$full_clause_optimisation'(_H, _M, B, B).
%:-
%	'$localise_vars_opt'(H, M, B0, BF), !.

/**
 * @pred current_evaluable_property(P)
 *
 * _P_ must be a valid arithetic functor or constant.
 *
 * It can be used to test or to enuerate.
*/
current_evaluable_property(P) :-
    var(P),
    !,
    (
    '$current_evaluable_property_2'(P)
    ;
'$current_evaluable_property_1'(P)
;
'$current_evaluable_property_0'(P)
).
current_evaluable_property(P) :-
functor(P,_N,A),
(A==2
->
    '$is_evaluable_property2'(P)
    ;
    A==1
    ->
'$is_evaluable_property1'(P)
;
'$is_evaluable_property0'(P)
).

/**
 * @pred evaluable_property(P, Type)
 *
 * True if _P_ is an arithmetic functor;  _Type_ in YAP always unifies with
 *`static` and `built_in`.
 *
 * The main functor of _P_ must be instantiated.
 */
evaluable_property(P, Type) :-
must_be_callable(P),
(var(Type) -> (Type=built_in;Type=static) ;
 atom(Type) ->
      ('$valid_evaluable_type'(Type)
      ->
      true
      ;
      throw_error(domain_error,evaluable_property, Type)
)     
;      
 	throw_error(type_error/(atom,Type),evaluable_property(P, Type) )
).

'$valid__evaluable_type'(dynamic).
'$valid__evaluable_type'(static).
'$valid__evaluable_type'(built_in).
'$valid__evaluable_type'(foreign).


'$current_evaluable_property_2'(P) :-
     between(0,100,I),
     ('$current_evaluable_property2'(I,P) -> true ; !, fail).

'$current_evaluable_property_1'(P) :-
     between(0,100,I),
     ('$current_evaluable_property1'(I,P) -> true ; !, fail).
'$current_evaluable_property_0'(P) :-
     between(0,100,I),
     ('$current_evaluable_property0'(I,P) -> true ; !, fail).

%% @}
