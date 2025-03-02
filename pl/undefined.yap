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
* File:		undefined.yap						 *
* Last rev:	8/2/88							 *
* mods:									 *
* comments:	Predicate Undefined for YAP				 *
*									 *
*************************************************************************/

/**
 *
 * @defgroup Undefined_Procedures Handling Undefined Procedures
@ingroup YAPControl
@{

A predicate in a module is said to be undefined if there are no clauses
defining the predicate, and if the predicate has not been declared to be
dynamic. What YAP does when trying to execute undefined predicates can
be specified in three different ways:


+ By setting an YAP flag, through the set_prolog_flag/2 or
set_prolog_flag/2 built-ins. This solution generalizes the
ISO standard by allowing module-specific behavior.
+ By using the unknown/2 built-in (this deprecated solution is
compatible with previous releases of YAP).
+ By defining clauses for the hook predicate
`user:unknown_predicate_handler/3`. This solution is compatible
with SICStus Prolog.


*/

/**  @pred  unknown_predicate_handler(+ _Call_, + _M_, - _N_)

In YAP, the default action on undefined predicates is to output an
`error` message. Alternatives are to silently `fail`, or to print a
`warning` message and then fail.  This follows the ISO Prolog standard
where the default action is `error`.

The user:unknown_predicate_handler/3 hook was first introduced in
SICStus Prolog. It allows redefining the answer for specifici
calls. As an example. if undefined/1 is:

```
undefined(A) :-
	     format('Undefined predicate: ~w~n',[A]), fail.

:- assert(user:unknown_predicate_handler(U,M,undefined(M:U)) )

call to a predicate for which no clauses were defined will result in
the output of a message of Undefined predicate.

```
Undefined predicate:
```
followed by the failure of that call.
*/
:- multifile user:unknown_predicate_handler/3.
:- dynamic user:unknown_predicate_handler/3.

'$undefp'(G0) :-
    '$exit_undefp',
    '$undefp__'(G0, NewG),
    call(NewG).

'$undefp__'(MGoal, FMGoal) :-
    '$imported_predicate'(MGoal,FMGoal),
    !.
'$undefp__'(M:G, NewG) :-
    '$number_of_clauses'(unknown_predicate_handler(_,_,_), user, N ),
    N > 0,
    user:unknown_predicate_handler(G, M, NewG),
    !.
'$undefp__'(M:G, _) :-
    '$undefp_flag'(M:G).

'$undefp_flag'(G) :-
    current_prolog_flag(unknown, Flag),
    '$undef_error'(Flag,G),
    fail.

'$undef_error'(error,  ModGoal) :-
	'$yap_strip_module'(ModGoal, M, G),
	functor( G, N, A),
	throw(error(existence_error(procedure,M:N/A),ModGoal)).
'$undef_error'(warning,  ModGoal) :-
    '$yap_strip_module'(ModGoal, M, G),
	functor( G, N, A),
	print_warning( error(existence_error(procedure,M:N/A),ModGoal) ).
% no need for code at this point.
%'$undef_error'(fail,_) :-
%	fail.

/** @pred  unknown(- _O_,+ _N_)

The unknown predicate, informs about what the user wants to be done
  when there are no clauses for a predicate. Using unknown/3 is
  strongly deprecated. We recommend setting the `unknown` prolog
  flag for generic behaviour, and calling the hook
  user:unknown_predicate_handler/3 to fine-tune specific cases
  undefined goals.

*/

unknown(P, NP) :-
    current_prolog_flag( unknown, P ),
    set_prolog_flag( unknown, NP ).

/**
@}
*/


