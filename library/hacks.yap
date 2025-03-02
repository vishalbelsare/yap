/**
 * @file   library/hacks.yap
 * @author VITOR SANTOS COSTA <vsc@VITORs-MBP.lan>
 * @date   Tue Nov 17 19:00:25 2015
 *
 * @brief  Prolog hacking
 *
 *
*/

/**
  * @addtogroup Hacks 
  * @{
  * @brief Manipulate the Prolog stacks, including setting and resetting
  * choice-points.
  *
**/

:- module(yap_hacks, [
	      trace/1,
	      alarm/3,
	      choicepoint/7,
	      code_location/3,
	      continuation/4,
	      current_choice_points/1,
	      current_continuations/1,
	      disable_interrupts/0,
	      display_stack_info/4,
	      display_stack_info/6,
	      enable_interrupts/0,
	      export_beautify/2 as beautify,
	      stack_dump/0,
	      stack_dump/1,
	      virtual_alarm/3,
              fully_strip_module/3,
	      exception_property/3,
  yap_error_descriptor/2,
  ctrace/1,
  fully_strip_module/3,
ctrace/1,
	      context_variables/1
				%		      cut_at/1,
				%		      cut_by/1,
				%		      display_pc/4,
				%		      parent_choicepoint/1,
				%		      parent_choicepoint/2,
          ]).

/**
 * @pred ctrace(Goal)
 *
 * This predicate is only available if the YAP
 * compile option was set. It generates a
 * step-by-step trace of the execution of _Goal_
 *
 */



/**
 * @pred stack_dump
 *
 * Write the current ancestor stack to the outout. Ancestors may have:
 * - terminated
 * - still have sub-goals to execute, if so, they left an _environment_
 * - still have clauses they may nacktrack to; if so, they left a _choice point_
 *
 */
stack_dump :-
	stack_dump(-1).

/**
 * @pred stack_dump(+N)
 *
 * Report the last _N_ entries in the stack (see stack_dump/0)
 */

stack_dump(Max) :-
	current_choice_points(CPs),
	current_continuations([Env|Envs]),
	continuation(Env,_,ContP,_),
	length(CPs, LCPs),
	length(Envs, LEnvs),
	format(user_error,'~n~n~tStack Dump~t~40+~n~nAddress~tChoiceP~16+ Cur/Next Clause        Goal~n',[LCPs,LEnvs]),
	display_stack_info(CPs, Envs, Max, ContP).

display_stack_info(CPs,Envs,Lim,PC) :-
	display_stack_info(CPs,Envs,Lim,PC,Lines,[]),
	flush_output(user_output),
	flush_output(user_error),
	run_formats(Lines, user_error).


run_formats([], _).
run_formats([Com-Args|StackInfo], Stream) :-
	format(Stream, Com, Args),
	run_formats(StackInfo, Stream).

code_location(Info,Where,Location) :-
	integer(Where) , !,
	pred_for_code(Where,Name,Arity,Mod,Clause),
	construct_code(Clause,Name,Arity,Mod,Info,Location).
code_location(Info,_,Info).

construct_code(-1,Name,Arity,Mod,Where,Location) :- !,
	number_codes(Arity,ArityCode),
	atom_codes(ArityAtom,ArityCode),
	atom_concat([Where,' at ',Mod,':',Name,'/',ArityAtom,' at indexing code'],Location).
construct_code(0,_,_,_,Location,Location) :- !.
construct_code(Cl,Name,Arity,Mod,Where,Location) :-
	number_codes(Arity,ArityCode),
	atom_codes(ArityAtom,ArityCode),
	number_codes(Cl,ClCode),
	atom_codes(ClAtom,ClCode),
	atom_concat([Where,' at ',Mod,':',Name,'/',ArityAtom,' (clause ',ClAtom,')'],Location).

'$prepare_loc'(Info,Where,Location) :- integer(Where), !,
	pred_for_code(Where,Name,Arity,Mod,Clause),
	construct_code(Clause,Name,Arity,Mod,Info,Location).
'$prepare_loc'(Info,_,Info).



display_pc(PC, PP, Source) -->
	{ integer(PC) },
	{ pred_for_code(PC,Name,Arity,Mod,Clause) },
	pc_code(Clause, PP, Name, Arity, Mod, Source).

pc_code(0,_PP,_Name,_Arity,_Mod, 'top level or system code' - []) --> !.
pc_code(-1,_PP,Name,Arity,Mod, '~a:~q/~d' - [Mod,Name,Arity]) --> !,
	{ functor(S, Name,Arity),
	nth_clause(Mod:S,1,Ref),
	clause_property(Ref, file(File)),
	clause_property(Ref, line_count(Line)) },
	[ '~a:~d:0, ' - [File,Line] ].
pc_code(Cl,Name,Arity,Mod, 'clause ~d for ~a:~q/~d'-[Cl,Mod,Name,Arity]) -->
	{ Cl > 0 },
	{ functor(S, Name,Arity),
	nth_clause(Mod:S,Cl,Ref),
	clause_property(Ref, file(File)),
	clause_property(Ref, line_count(Line)) },
	[ '~a:~d:0, ' - [File,Line] ].

display_stack_info(_,_,0,_) --> !.
display_stack_info([],[],_,_) --> [].
display_stack_info([CP|CPs],[],I,_) -->
	show_cp(CP, '.'),
	{ I1 is I-1 },
	display_stack_info(CPs,[],I1,_).
display_stack_info([],[Env|Envs],I,Cont) -->
	show_env(Env, Cont, NCont),
	{ I1 is I-1 },
	display_stack_info([], Envs, I1, NCont).
display_stack_info([CP|LCPs],[Env|LEnvs],I,Cont) -->
	{
	 yap_hacks:continuation(Env, _, NCont, CB),
	 I1 is I-1
	},
	( { CP == Env, CB < CP } ->
	    % if we follow choice-point and we cut to before choice-point
	    % we are the same goal
	   show_cp(CP, ''), %
           display_stack_info(LCPs, LEnvs, I1, NCont)
	;
          { CP > Env } ->
	   show_cp(CP, ' < '),
	   display_stack_info(LCPs,[Env|LEnvs],I1,Cont)
	;
	   show_env(Env,Cont,NCont),
	   display_stack_info([CP|LCPs],LEnvs,I1,NCont)
	).

show_cp(CP, Continuation) -->
	{ yap_hacks:choicepoint(CP, Addr, Mod, Name, Arity, Goal, ClNo) },
	( { Goal = (_;_) }
          ->
	  { scratch_goal(Name,Arity,Mod,Caller) },
	  [ '0x~16r~t*~16+ ~d~16+ ~q ~n'-
		[Addr, ClNo, Caller] ]

	    ;
	  [ '0x~16r~t *~16+~a ~d~16+ ~q:' -
		[Addr, Continuation, ClNo, Mod]]
	),
	{ current_prolog_flag( debugger_print_options, Opts) },
	{ export_beautify(Mod:Goal,G)},
	['~@.~n' -  write_term(G,Opts)].

show_env(Env,_Cont,NCont) -->
	{
	 yap_hacks:continuation(Env, Addr, NCont, _),
	format('0x~16r 0x~16r~n',[Env,NCont]),
	 yap_hacks:cp_to_predicate(xoxuxoCont, Mod, Name, Arity, ClId)
	},
        [ '0x~16r~t  ~16+ ~d~16+ ~q:' -
		[Addr, ClId, Mod] ],
	{scratch_goal(Name, Arity, Mod, G)},
	{ current_prolog_flag( debugger_print_options, Opts) },
	['~@.~n' - write_term(G,Opts)].


/**
 * @pred virtual_alarm(+Interval, 0:Goal, -Left)
 *
 * Activate  an alarm to execute _Goal_ in _Interval_ seconds. If the alarm was active,
 * bind _Left_ to the previous value.
 *
 * If _Interval_ is 0, disable the current alarm.
 */
virtual_alarm(Interval, Goal, Left) :-
	Interval == 0, !,
	virtual_alarm(0, 0, Left0, _),
	on_signal(sig_vtalarm, _, Goal),
	Left = Left0.
virtual_alarm(Interval, Goal, Left) :-
	integer(Interval), !,
	on_signal(sig_vtalarm, _, Goal),
	virtual_alarm(Interval, 0, Left, _).
virtual_alarm([Interval|USecs], Goal, [Left|LUSecs]) :-
	on_signal(sig_vtalarm, _, Goal),
	virtual_alarm(Interval, USecs, Left, LUSecs).


/** @pred hacks:context_variables(-NamedVariables)
  Access variable names.

  Unify NamedVariables with a list of terms _Name_=_V_
  giving the names of the variables occurring in the last term read.
  Notice that variable names option must have been on.
*/


yap_hacks:scratch_goal(N,0,Mod,Mod:N) :-
	!.
yap_hacks:scratch_goal(N,A,Mod,NG) :-
	list_of_qmarks(A,L),
	G=..[N|L],
	(
	  beautify_goal(G,Mod,[NG],[])
	;
	  G = NG
	),
	!.

list_of_qmarks(0,[]) :- !.
list_of_qmarks(I,[?|L]) :-
	I1 is I-1,
	list_of_qmarks(I1,L).

fully_strip_module( T, M, TF) :-
    '$yap_strip_module'( T, M, TF).


yap_hacks:export_beautify(A,NA) :-
    beautify(A,NA).
    
%%
% @pred beautify(Goal, ModuleProcessGoal)
%
% This helper routine should be called with a Prolog
% goal or clause body It will push the modules inside
% the Prolog connectives so that the goal becomes a little
% more easier to understand.
%
beautify(Goal, NicerGoal) :- 
    current_source_module(M,M),
    beautify(Goal, M, NicerGoal).

beautify((A,B),M,(CA,CB)) :-
    !,
    beautify(A,M,CA),
    beautify(B,M,CB).
 beautify((A;B),M,(CA;CB)) :-
    !,
    beautify(A,M,CA),
    beautify(B,M,CB).
beautify((A->B),M,(CA->CB)) :-
    !,
    beautify(A,M,CA),
    beautify(B,M,CB).
beautify((A *->B),M,(CA *->CB)) :-
    !,
    beautify(A,M,CA),
    beautify(B,M,CB).
beautify(M:A,_,CA) :-
    !,
    beautify(A,M,CA).
beautify(A,prolog, NA) :-
    beautify_goal(A,prolog,[NA],[]),
    !.
beautify(A,prolog,A) :-
    !.
beautify(A,M,CA) :-
    current_source_module(M,M),
    !,
    beautify(A,CA).
beautify(A,M,M:A).

beautify_goal('$yes_no'(G,_Query), (?-G)) -->
	!,
	{ Call =.. [(?), G] },
	[Call].
beautify_goal('$do_yes_no'(G,Mod), prolog) -->
	[Mod:G].
beautify_goal(query(G,VarList), prolog) -->
	[query(G,VarList)].
beautify_goal('$enter_top_level', prolog) -->
	['TopLevel'].
% The user should never know these exist.
beautify_goal('$csult'(Files,Mod),prolog) -->
	[reconsult(Mod:Files)].
beautify_goal('$use_module'(Files,Mod,Is),prolog) -->
	[use_module(Mod,Files,Is)].
beautify_goal('$continue_with_command'(reconsult,V,P,G,Source),prolog) -->
	['Assert'(G,V,P,Source)].
beautify_goal('$continue_with_command'(consult,V,P,G,Source),prolog) -->
	['Assert'(G,V,P,Source)].
beautify_goal('$continue_with_command'(top,V,P,G,_),prolog) -->
	['Query'(G,V,P)].
beautify_goal('$continue_with_command'(Command,V,P,G,Source),prolog) -->
	['TopLevel'(Command,G,V,P,Source)].
beautify_goal('$system_catch'(G,Mod,Exc,Handler),prolog) -->
	[catch(Mod:G, Exc, Handler)].
beautify_goal('$catch'(G,Exc,Handler),prolog) -->
	[catch(G, Exc, Handler)].
beautify_goal('$execute_command'(Query,M,V,P,Option,Source),prolog) -->
	[toplevel_query(M:Query, V, P, Option, Source)].
beautify_goal('$process_directive'(Gs,_Mode,_VL),prolog) -->
	[(:- Gs)].
beautify_goal('$loop'(Stream,Option),prolog) -->
	[execute_load_file(Stream, consult=Option)].
beautify_goal('$load_files'(Files,Opts,?),prolog) -->
	[load_files(Files,Opts)].
beautify_goal('$load_files'(_,_,Name),prolog) -->
	[Name].
beautify_goal('$reconsult'(Files,Mod),prolog) -->
	[reconsult(Mod:Files)].
beautify_goal('$undefp'(Mod:G),prolog) -->
	['CallUndefined'(Mod:G)].
beautify_goal('$undefp'(?),prolog) -->
	['CallUndefined'(?:?)].
beautify_goal(repeat,prolog) -->
	[repeat].
beautify_goal('$recorded_with_key'(A,B,C),prolog) -->
	[recorded(A,B,C)].
beautify_goal('$findall_with_common_vars'(Templ,Gen,Answ),prolog) -->
	[findall(Templ,Gen,Answ)].
beautify_goal('$bagof'(Templ,Gen,Answ),prolog) -->
	[bagof(Templ,Gen,Answ)].
beautify_goal('$setof'(Templ,Gen,Answ),prolog) -->
	[setof(Templ,Gen,Answ)].
beautify_goal('$findall'(T,G,S,A),prolog) -->
	[findall(T,G,S,A)].
beautify_goal('$listing'(G,M,_Stream),prolog) -->
	[listing(M:G)].
beautify_goal('$call'(G,_CP,?,M),prolog) -->
	[call(M:G)].
beautify_goal('$call'(_G,_CP,G0,M),prolog) -->
	[call(M:G0)].
beautify_goal('$current_predicate'(Na,M,S,_),prolog) -->
	[current_predicate(Na,M:S)].
beautify_goal('$list_clauses'(Stream,M,Pred),prolog) -->
	[listing(Stream,M:Pred)].

    %% @}

