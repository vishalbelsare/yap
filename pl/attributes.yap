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
* File:		atts.yap						 *
* Last rev:	8/2/88							 *
* mods:									 *
* comments:	attribute support for Prolog				 *
*									 *
*************************************************************************/
/**
  @file attributes.yap

  @addtogroup HPAtts

  @{

*/	

:- module(attributes,
	  [
	      call_residue/2,
	      attvars_residuals/3]
%	  [copy_term/3]
	 ).

:- dynamic existing_attribute/4.
:- dynamic modules_with_attributes/1.
:- dynamic attributed_module/3.

    :- multifile
        attributed_module/3.

:- dynamic existing_attribute/4.
:- dynamic modules_with_attributes/1.
:- dynamic attributed_module/3.

/** @pred copy_term(? _TI_,- _TF_,- _Goals_)

Term  _TF_ is a varia of the original term  _TI_, such that for
each variable  _V_ in the term  _TI_ there is a new variable  _V'_
in term  _TF_ without any attributes attached.  Attributed
variables are thus converted to standard variables.   _Goals_ is
unified with a list that represents the attributes.  The goal
`maplist(call, _Goals_)` can be called to recreate the
attributes.

`attribute_goals/1` in the module where the attribute is
defined.


*/

:- multifile woken_att_do/4.

prolog:copy_term(Term, Copy, Gs) :-
    copy_term(Term,Copy),
    term_attvars(Copy,Vs),
    (   Vs == []
    ->
    Gs=[]
    ;
    attvars_residuals(Vs, Gs, []),
    delete_attributes(Vs)
    ).

attvars_residuals([]) --> [].
attvars_residuals([V|Vs]) -->
	{ nonvar(V) }, !,
	attvars_residuals(Vs).
attvars_residuals([V|Vs]) -->
    { get_attrs(V, As) },
	attvar_residuals(As, V),
	!,
	attvars_residuals(Vs).
attvars_residuals([_|Vs]) -->
	attvars_residuals(Vs).

/** @pred Module:attribute_goal( -Var, Goal)

User-defined procedure, called to convert the attributes in  _Var_ to
a  _Goal_. Should fail when no interpretation is available.
 */
attvar_residuals(_ , V) -->
    { nonvar(V) },
    !.
%SWI
attvar_residuals([] , _V)--> !.
attvar_residuals(att(Module,_Value,As), V) -->
    { '$pred_exists'(attribute_goals(V, _,_),Module) },
    call(Module:attribute_goals(V )),
    !,
    attvar_residuals(As, V).   
attvar_residuals(att(_,_Value,As), V) -->
    attvar_residuals(As, V).   
	%SICStus
attvar_residuals(Attribute, V) -->
    { functor(Attribute,Module,Ar),
      Ar > 1
     },
    (
	{
	    '$pred_exists'(attribute_goal(V, Goal,_,_),Module),
	    call(Module:attribute_goal(V, Goal))
	}
    ->
	[Goal]
    ;
    []
    ),
    { arg(1, Attribute, As) },
    attvar_residuals(As, V).
attvar_residuals(_, _) --> [].
%
% wake_up_goal is called by the system whenever a suspended goal
% resumes.
%


/* The first case may happen if this variable was used for dif.
   In this case, we need a way to keep the original
 goal around
*/

% what to do when two variables bind together
%
unify_attributed_variable(New,V) :-
    nonvar(New),
    nonvar(V),
    !,
    V=New.
unify_attributed_variable(New,V) :-
    nonvar(New),
    attvar(V),
    !,
    unify_attributed_variable(V,New).    
unify_attributed_variable(V,New) :-
    get_attrs(V,Atts),
    !,
    bind_attvar(V),
    do_hook_attributes(Atts, New).
    
% SICStus 
unify_attributed_variable(V,New) :-
    attvar(V),
    !,
    woken_att_do(V, New, LGoals, DoNotBind),
    (DoNotBind = false ->  bind_attvar(V) ; true),
    lcall(LGoals).
%    '$wake_up_done'.
unify_attributed_variable_(V,V). % :-
%    '$wake_up_done'.


/**
  * @pred attr_unify_hook(Att0, Binding)
  *
  * Module specific hook predicate called after binding an attributed variable.
*/
do_hook_attributes([], _) :- !.
do_hook_attributes(Att0, Binding) :-
    Att0=att(Mod,Att,Atts),
    '$pred_exists'(attr_unify_hook(Att0, Binding),Mod),
    !,
    call(Mod:attr_unify_hook(Att, Binding)),
     do_hook_attributes( Atts, Binding).
do_hook_attributes(att(_,_,Atts), Binding) :-
    do_hook_attributes( Atts, Binding).


lcall([]).
lcall([Mod:Gls|Goals]) :-
    lcall2(Gls,Mod),
	lcall(Goals).

lcall2([], _).
lcall2([Goal|Goals], Mod) :-
!,
    call(Mod:Goal),
	lcall2(Goals, Mod).
lcall2((Goal,Goals), Mod) :-
    !,
    call(Mod:Goal),
	lcall2(Goals, Mod).
lcall2(Goal, Mod) :-
    !,
    call(Mod:Goal).



/** @pred call_residue_vars(: _G_, _L_)



Call goal  _G_ and unify  _L_ with a list of all constrained variables created <em>during</em> execution of  _G_:

```
  ?- dif(X,Z), call_residue_vars(dif(X,Y),L).
dif(X,Z), call_residue_vars(dif(X,Y),L).
L = [Y],
dif(X,Z),
dif(X,Y) ? ;

no
```
 */
call_residue_vars(Goal,Residue) :-
	all_attvars(Vs0),
	call(Goal),
	all_attvars(Vs),
	% this should not be actually strictly necessary right now.
	% but it makes it a safe bet.
	sort(Vs, Vss),
	sort(Vs0, Vs0s),
	'$ord_remove'(Vss, Vs0s, Residue).

'$ord_remove'([], _, []).
'$ord_remove'([V|Vs], [], [V|Vs]).
'$ord_remove'([V1|Vss], [V2|Vs0s], Residue) :-
	( V1 == V2 ->
	  '$ord_remove'(Vss, Vs0s, Residue)
	;
	  V1 @< V2 ->
	  Residue = [V1|ResidueF],
	  '$ord_remove'(Vss, [V2|Vs0s], ResidueF)
	;
	  '$ord_remove'([V1|Vss], Vs0s, Residue)
	).

/** @pred attribute_goals(+ _Var_,- _Gs_,+ _GsRest_)



This nonterminal, if it is defined in a module, is used by  _copy_term/3_
to project attributes of that module to residual goals. It is also
used by the toplevel to obtain residual goals after executing a query.

 
Normal user code should deal with put_attr/3, get_attr/3 and del_attr/2.
The routines in this section fetch or set the entire attribute list of a
variables. Use of these predicates is anticipated to be restricted to
printing and other special purpose operations.

*/



module_has_attributes(Mod) :-
    attributed_module(Mod, _, _), !.

/*

list([])     --> [].
list([L|Ls]) --> [L], list(Ls).

dot_list((A,B)) --> !, dot_list(A), dot_list(B).
dot_list(A)	--> [A].
*/

prolog:delete_attributes(Term) :-
	term_attvars(Term, Vs),
	delete_attributes_(Vs).

delete_attributes_([]).
delete_attributes_([V|Vs]) :-
	del_attrs(V),
	delete_attributes_(Vs).



/** @pred call_residue(: _G_, _L_)


Call goal  _G_. If subgoals of  _G_ are still blocked, return
a list containing these goals and the variables they are blocked in. The
goals are then considered as unblocked. The next example shows a case
where dif/2 suspends twice, once outside call_residue/2,
and the other inside:

```
?- dif(X,Y),
       call_residue((dif(X,Y),(X = f(Z) ; Y = f(Z))), L).

X = f(Z),
L = [[Y]-dif(f(Z),Y)],
dif(f(Z),Y) ? ;

Y = f(Z),
L = [[X]-dif(X,f(Z))],
dif(X,f(Z)) ? ;

no
```
The system only reports one invocation of dif/2 as having
suspended.


*/
call_residue(Goal,Residue) :-
	var(Goal), !,
	throw_error(instantiation_error,call_residue(Goal,Residue)).
call_residue(Module:Goal,Residue) :-
	atom(Module), !,

	call_residue(Goal,Module,Residue).
call_residue(Goal,Residue) :-
	'$current_module'(Module),
	call_residue(Goal,Module,Residue).

call_residue(Goal,Module,Residue) :-
	prolog:call_residue_vars(Module:Goal,NewAttVars),
	run_project_attributes(NewAttVars, Module:Goal),
	prolog:copy_term(Goal, Goal, Residue).

delayed_goals(G, Vs, NVs, Gs) :-
	project_delayed_goals(G),
%	term_factorized([G|Vs], [_|NVs], Gs).
	copy_term([G|Vs], [_NG|NVs], GsW),
	sort(GsW,Gs).

project_delayed_goals(G) :-
% SICStus compatible step,
% just try to simplify store  by projecting constraints
% over query variables.
% called by top_level to find out about delayed goals
	all_attvars(LAV),
	LAV = [_|_],
	run_project_attributes(LAV, G), !.
project_delayed_goals(_).


attributed(G, Vs) :-
	variables_in_term(G, LAV,[]),
	att_vars(LAV, Vs).

att_vars([], []).
att_vars([V|LGs], [V|AttVars]) :- attvar(V), !,
	att_vars(LGs, AttVars).
att_vars([_|LGs], AttVars) :-
	att_vars(LGs, AttVars).

% make sure we set the suspended goal list to its previous state!
% make sure we have installed a SICStus like constraint solver.

/** @pred Module:project_attributes( +AttrVars, +Goal)



Given a goal _Goal_ with variables  _QueryVars_ and list of attributed
variables  _AttrVars_, project all attributes in  _AttrVars_ to
 _QueryVars_. Although projection is constraint system dependent,
typically this will involve expressing all constraints in terms of
 _QueryVars_ and considering all remaining variables as existentially
quantified.

Projection interacts with attribute_goal/2 at the Prolog top
level. When the query succeeds, the system first calls
project_attributes/2. The system then calls
attribute_goal/2 to get a user-level representation of the
constraints. Typically, project_attributes/2 will convert from the
original constraints into a set of new constraints on the projection,
and these constraints are the ones that will have an
attribute_goal/2 handler.
 */
run_project_attributes(AllVs, G) :-
	findall(Mod,current_predicate(project_attributes,Mod:project_attributes(AttIVs, AllVs)),Mods),
terms:variables_in_term(G, InputVs,[]),
	pick_att_vars(InputVs, AttIVs),
	project_module( Mods, AttIVs, AllVs).

pick_att_vars([],[]).
pick_att_vars([V|L],[V|NL]) :- attvar(V), !,
	pick_att_vars(L,NL).
pick_att_vars([_|L],NL) :-
	pick_att_vars(L,NL).

project_module([], _LIV, _LAV).
project_module([Mod|LMods], LIV, LAV) :-
	call(Mod:project_attributes(LIV, LAV)), !,
	all_attvars(NLAV),
	project_module(LMods,LIV,NLAV).
project_module([_|LMods], LIV, LAV) :-
	project_module(LMods,LIV,LAV).

%% @}

