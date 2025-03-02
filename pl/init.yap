/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
**	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		init.yap						 *
* Last rev:								 *
* mods:									 *
* comments:	initializing the full prolog system			 *
*									 *
*************************************************************************/


'$init_globals' :-
	% set_prolog_flag(break_level, 0),
	% '$set_read_error_handler'(error), let the user do that
	nb_setval('$chr_toplevel_show_store',false).

'$init_consult' :-
    '$conditional_compilation_init',
    set_prolog_flag(optimise, true ),
	nb_setval('$assert_all',off),
 	nb_setval('$initialization_goals',off),
	nb_setval('$included_file',[]),
	\+ '$undefined'('$init_preds',prolog),
	'$init_preds',
	fail.
'$init_consult'.

'$init_win_graphics' :-
    '$undefined'(window_title(_,_), system), !.
'$init_win_graphics' :-
    load_files([library(win_menu)], [silent(true),if(not_loaded)]),
    fail.
'$init_win_graphics'.

'$init_or_threads' :-
	'$c_yapor_workers'(W), !,
	'$start_orp_threads'(W).
'$init_or_threads'.

'$start_orp_threads'(1) :- !.
'$start_orp_threads'(W) :-
	thread_create('$c_worker',_,[detached(true)]),
	W1 is W-1,
	'$start_orp_threads'(W1).

'$version' :-
	'$version_specs'(Specs),
	print_message(informational, Specs).

'$version_specs'(version(YAP,VersionGit,AT,Saved)) :-
      current_prolog_flag(version_git,VersionGit),
      current_prolog_flag(compiled_at,AT),
      current_prolog_flag(version_data, YAP),
      current_prolog_flag(resource_database, Saved ).

/**
  * Initialise a Prolog engine.
  *
  * Must be called after restoring.
  */
init_prolog :-
    % do catch as early as possible
    '$init_consult',
    '$version',
    '$init_preds',
    set_prolog_flag(open_expands_filename, true),
    set_prolog_flag(file_errors, false),
    set_prolog_flag(verbose_file_search, false),
%    set_prolog_flag( source_mode, true),
    %set_prolog_flag(file_name_variables, OldF),
    '$init_globals',
    set_value('$gc',on),
    ('$exit_undefp' -> true ; true),
    prompt1(' ?- '),
    set_prolog_flag(debug, false),
    % simple trick to find out if this is we are booting from Prolog.
    % boot from a saved state
    '$init_from_saved_state_and_args', %start_low_level_trace,

    '$db_clean_queues'(0),
				% this must be executed from C-code.
				%	'$startup_saved_state',
    set_input(user_input),
    set_output(user_output),
    '$init_or_threads',
    '$run_at_thread_start'.


% then we can execute the programs.
'$startup_goals' :-
    module(user),
    fail.
'$startup_goals' :-
    recorded('$startup_goal',G,_),
    catch(once(user:G),_Error,error_handler),
    fail.
		'$startup_goals' :-
			get_value('$init_goal',GA),
			GA \= [],
			set_value('$init_goal',[]),
			'$run_atom_goal'(GA),
			fail.
			'$startup_goals' :-
				get_value('$top_level_goal',GA),
				GA \= [],
				set_value('$top_level_goal',[]),
				'$run_atom_goal'(GA),
				fail.
'$startup_goals' :-
    recorded('$restore_flag', goal(Module:GA), R),
    erase(R),
    catch(once(Module:GA),_Error,error_handler),
    fail.
'$startup_goals' :-
	get_value('$myddas_goal',GA), GA \= [],
	set_value('$myddas_goal',[]),
	get_value('$myddas_user',User), User \= [],
	set_value('$myddas_user',[]),
	get_value('$myddas_db',Db), Db \= [],
	set_value('$myddas_db',[]),
	get_value('$myddas_host',HostT),
	( HostT \= [] ->
	  Host = HostT,
	  set_value('$myddas_host',[])
	;
	  Host = localhost
	),
	get_value('$myddas_pass',PassT),
	( PassT \= [] ->
	  Pass = PassT,
	  set_value('$myddas_pass',[])
	;
	  Pass = ''
	),
	use_module(library(myddas)),
	call(db_open(mysql,myddas,Host/Db,User,Pass)),
	'$myddas_import_all',
	fail.
'$startup_goals' :-
    '__NB_getval__'('$top_level_goal',G,fail),
    G \= [],
    nb_setval('$top_level_goal',[]),
    catch(once(G),_Error,error_handler),
    fail.
'$startup_goals'.

 %
 % MYDDAS: Import all the tables from one database
 %

 '$myddas_import_all':-
	 call(db_my_show_tables(myddas,table(Table))),
	 call(db_import(myddas,Table,Table)),
	 fail.
 '$myddas_import_all'.

% use if we come from a save_program and we have SWI's shlib
'$init_from_saved_state_and_args' :-
	current_prolog_flag(hwnd, _HWND),
	load_files(library(win_menu), [silent(true)]),
	fail.
'$init_from_saved_state_and_args' :-
	recorded('$reload_foreign_libraries',_G,R),
	erase(R),
	shlib:reload_foreign_libraries,
	fail.
% this should be done before -l kicks in.
'$init_from_saved_state_and_args' :-
	current_prolog_flag(fast_boot, false),
	  ( exists('~/.yaprc') -> load_files('~/.yaprc', []) ; true ),
	  ( exists('~/.prologrc') -> load_files('~/.prologrc', []) ; true ),
	  ( exists('~/prolog.ini') -> load_files('~/prolog.ini', []) ; true ),
	  fail.
% use if we come from a save_program and we have a goal to execute
'$init_from_saved_state_and_args' :-
	get_value('$consult_on_boot',X), X \= [],
	set_value('$consult_on_boot',[]),
	'$do_startup_reconsult'(X),
	fail.
'$init_from_saved_state_and_args' :-
	recorded('$restore_flag', init_file(M:B), R),
	erase(R),
	'$do_startup_reconsult'(M:B),
	fail.
'$init_from_saved_state_and_args' :-
	recorded('$restore_flag', unknown(M:B), R),
	erase(R),
	set_prolog_flag(M:unknown,B),
	fail.
'$init_from_saved_state_and_args' :-
	'$startup_goals',
	fail.
'$init_from_saved_state_and_args' :-
	recorded('$restore_goal',G,R),
	erase(R),
	prompt(_,'| '),
	catch(once(user:G),Error,user:'$Error'(Error)),
	fail.

'$init_path_extensions' :-
	get_value('$extend_file_search_path',P), !,
	P \= [],
	set_value('$extend_file_search_path',[]),
	'$extend_file_search_path'(P).
'$init_path_extensions'.
