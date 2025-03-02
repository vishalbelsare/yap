
/**
 * @file   maputils.yap
 * @author VITOR SANTOS COSTA <vsc@VITORs-MBP.lan>
 * @date   Tue Nov 17 22:48:58 2015
 * 
 * @brief  Auxiliary routines for map... libraries
 * 
 * 
*/
%%%%%%%%%%%%%%%%%%%%
% map utilities
%%%%%%%%%%%%%%%%%%%%

:- module(maputils,
	  [pred_name/4,
	   aux_preds/5,
	   append_args/3]).

:- use_module(library(lists)).

/**
* @addtogroup maplist
  *
  * Auxiliary routines
  *
  *@{
*/




:- dynamic number_of_expansions/1.

number_of_expansions(0).



append_args(Term, Args, NewTerm) :-
	Term =.. [Meta|OldArgs],
	append(OldArgs, Args, GoalArgs),
	NewTerm =.. [Meta|GoalArgs].

aux_preds(Meta, _, _, _, _) :-
	var(Meta), !,
	fail.
aux_preds(M:Meta, MetaVars, M:Pred, PredVars, M:Proto) :- !,
	aux_preds(Meta, MetaVars, Pred, PredVars, Proto).
aux_preds(Meta, MetaVars, Pred, PredVars, Proto) :-
	Meta =.. [F|Args],
	aux_args(Args, MetaVars, PredArgs, PredVars, ProtoArgs),
	Pred =.. [F|PredArgs],
	Proto =.. [F|ProtoArgs].

aux_args([], [], [], [], []).
aux_args([Arg|Args], MVars, [Arg|PArgs], PVars, [Arg|ProtoArgs]) :-
	ground(Arg), !,
	aux_args(Args, MVars, PArgs, PVars, ProtoArgs).
aux_args([Arg|Args], [Arg|MVars], [PVar|PArgs], [PVar|PVars], ['_'|ProtoArgs]) :-
	aux_args(Args, MVars, PArgs, PVars, ProtoArgs).

pred_name(Macro, Arity, P , Name) :-
        prolog_load_context(file, FullFileName),
	file_base_name( FullFileName, File ),
	prolog_load_context(term_position, Pos),
	stream_position_data( line_count, Pos, Line ), !,
	transformation_id(Id),
	atomic_concat(['$$$ for ',Macro,'/',Arity,', line ',Line,' in ',File,'(',P,') #',Id], Name).
pred_name(Macro, Arity, P , Name) :-
    transformation_id(Id),
	atomic_concat(['$$$__expansion__ for ',Macro,'/',Arity,'(',P,') #',Id], Name).

transformation_id(Id) :-
    retract(number_of_expansions(Id)),
    !,
    Id1 is Id+1,
    assert(number_of_expansions(Id1)).
transformation_id(0).

/**
  @}
*/
