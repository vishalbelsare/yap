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
  * File:		signals.pl						 *
  * Last rev:								 *
  * mods:									 *
  * comments:	signal handling in YAP					 *
  *									 *
  *************************************************************************/

%%! @addtogroup OS
%%  @{
:- system_module_( '$_signals', [alarm/3,
				on_exception/3,
				on_signal/3,
				raise_exception/1,
				read_sig/0], []).

:- use_system_module( '$_boot', ['$meta_call'/2]).

:- use_system_module( '$_debug', ['$trace'/1]).


/** @pred  alarm(+ _Seconds_,+ _Callable_,+ _OldAlarm_)


  Arranges for YAP to be interrupted in  _Seconds_ seconds, or in
  [ _Seconds_| _MicroSeconds_]. When interrupted, YAP will execute
  _Callable_ and then return to the previous execution. If
  _Seconds_ is `0`, no new alarm is scheduled. In any event,
  any previously set alarm is canceled.

  The variable  _OldAlarm_ unifies with the number of seconds remaining
  until any previously scheduled alarm was due to be delivered, or with
  `0` if there was no previously scheduled alarm.

  Note that execution of  _Callable_ will wait if YAP is
  executing built-in predicates, such as Input/Output operations.

  The next example shows how  _alarm/3_ can be used to implement a
  simple clock:

  ~~~~~
  loop :- loop.

  ticker :- write('.'), flush_output,
  get_value(tick, yes),
  alarm(1,ticker,_).

  :- set_value(tick, yes), alarm(1,ticker,_), loop.
  ~~~~~

  The clock, `ticker`, writes a dot and then checks the flag
  `tick` to see whether it can continue ticking. If so, it calls
  itself again. Note that there is no guarantee that the each dot
  corresponds a second: for instance, if the YAP is waiting for
  user input, `ticker` will wait until the user types the entry in.

  The next example shows how alarm/3 can be used to guarantee that
  a certain procedure does not take longer than a certain amount of time:

  ~~~~~
  loop :- loop.

  :-   catch((alarm(10, throw(ball), _),loop),
  ball,
  format('Quota exhausted.~n',[])).
  ~~~~~
  In this case after `10` seconds our `loop` is interrupted,
  `ball` is thrown,  and the handler writes `Quota exhausted`.
  Execution then continues from the handler.

  Note that in this case `loop/0` always executes until the alarm is
  sent. Often, the code you are executing succeeds or fails before the
  alarm is actually delivered. In this case, you probably want to disable
  the alarm when you leave the procedure. The next procedure does exactly so:

  ~~~~~
  once_with_alarm(Time,Goal,DoOnAlarm) :-
  catch(execute_once_with_alarm(Time, Goal), alarm, DoOnAlarm).

  execute_once_with_alarm(Time, Goal) :-
  alarm(Time, alarm, _),
  ( call(Goal) -> alarm(0, alarm, _) ; alarm(0, alarm, _), fail).
  ~~~~~

  The procedure `once_with_alarm/3` has three arguments:
  the  _Time_ to wait before the alarm is
  sent; the  _Goal_ to execute; and the goal  _DoOnAlarm_ to execute
  if the alarm is sent. It uses catch/3 to handle the case the
  `alarm` is sent. Then it starts the alarm, calls the goal
  _Goal_, and disables the alarm on success or failure.


*/
/** @pred  on_signal(+ _Signal_,? _OldAction_,+ _Callable_)


  Set the interrupt handler for soft interrupt  _Signal_ to be
  _Callable_.  _OldAction_ is unified with the previous handler.

  Only a subset of the software interrupts (signals) can have their
  handlers manipulated through on_signal/3.
  Their POSIX names, YAP names and default behavior is given below.
  The "YAP name" of the signal is the atom that is associated with
  each signal, and should be used as the first argument to
  on_signal/3. It is chosen so that it matches the signal's POSIX
  name.

  on_signal/3 succeeds, unless when called with an invalid
  signal name or one that is not supported on this platform. No checks
  are made on the handler provided by the user.

  + sig_up (Hangup)
  SIGHUP in Unix/Linux; Reconsult the initialization files
  ~/.yaprc, ~/.prologrc and ~/prolog.ini.
  + sig_usr1 and sig_usr2 (User signals)
  SIGUSR1 and SIGUSR2 in Unix/Linux; Print a message and halt.


  A special case is made, where if  _Callable_ is bound to
  `default`, then the default handler is restored for that signal.

  A call in the form `on_signal( _S_, _H_, _H_)` can be used
  to retrieve a signal's current handler without changing it.

  It must be noted that although a signal can be received at all times,
  the handler is not executed while YAP is waiting for a query at the

  prompt. The signal will be, however, registered and dealt with as soon
  as the user makes a query.

  Please also note, that neither POSIX Operating Systems nor YAP guarantee
  that the order of delivery and handling is going to correspond with the
  order of dispatch.
*/
:- meta_predicate on_signal(0,?,:), alarm(+,0,-).

:- dynamic prolog:'$signal_handler'/1.

'$signal_handler'(sig_creep).
'$signal_handler'(sig_int) :-
    flush_output,
    '$clear_input'(user_input),
    prompt1('Action (h for help)'),
    get_char(user_input,C),
    int_action(C).
'$signal_handler'(sig_iti) :-
	'$thread_gfetch'(Goal),
	% if more signals alive, set creep flag
	'$current_module'(M0),
	('$execute_rlist'(Goal,M0),fail ; true).
'$signal_handler'(sig_trace) :-
	trace.
'$signal_handler'(sig_debug) :-
	debug.
'$signal_handler'(sig_alarm) :-
    throw(timeout).
'$signal_handler'(sig_vtalarm) :-
    throw(timeout).
'$signal_handler'(sig_hup).
%    '$reload'.
'$signal_handler'(sig_debug ) :-
	debug.
'$signal_handler'(sig_trace ) :-
	'$creep'.
'$signal_handler'(sig_vtalarm) :-
    throw(timeout).
'$signal_handler'(sig_usr1) :-
    throw(error(signal(usr1,[]),true)).
'$signal_handler'(sig_usr2) :-
    throw(error(signal(usr2,[]),true)).
'$signal_handler'(sig_pipe) :-
    throw(error(signal(pipe,[]),true)).
'$signal_handler'(sig_fpe) :-
    throw(error(signal(fpe,[]),true)).
'$signal_handler'(abort) :-
    abort.

int_action(s) :-
    statistics.
int_action(a) :-
    abort.
int_action(b) :-
    break.
int_action(e) :-
    halt.
int_action(d) :-
    debug.
int_action(g) :-
    yap_hacks:stack_dump.
int_action(t) :-
    trace,
    '$creep'.
int_action('T') :-
    start_low_level_trace.
int_action(h) :-
    format(user_error, "Please press one of:~n"),
    format(user_error, "    a for abort~n",[]),
    format(user_error, "    b for break~n",[]),
    format(user_error, "    c for continue~n",[]),
    format(user_error, "    d for enabling debug mode~n",[]),
    format(user_error, "    e for exit (halt)~n", []),
    format(user_error, "    g for a stack dump~n",[]),
    format(user_error, "    s for statistics~n",[]),
    format(user_error, "    t for trace\n").

'$execute_rlist'([_|L],M) :-
    '$execute_rlist'(L,M).
'$execute_rlist'([G|_],M) :-
    once(M:G),
    fail.



'$start_creep'(Mod:G) :-
    current_choice_point(CP),
    '$trace_goal'(G, Mod, outer ,_,CP).

'$no_creep_call'('$execute_clause'(G,Mod,Ref,CP),_) :- !,
        '$enable_debugging',
	'$execute_clause'(G,Mod,Ref,CP).
'$no_creep_call'('$execute0'(M:G),_) :- !,
	'$enable_debugging',
	'$execute_non_stop'(M:G).
'$no_creep_call'(G, M) :-
	'$enable_debugging',
	'$execute0'(M:G).






% reconsult init files. %
'$reload' :-
    (( exists('~/.yaprc') -> [-'~/.yaprc'] ; true ),
     ( exists('~/.prologrc') -> [-'~/.prologrc'] ; true ),
     ( exists('~/prolog.ini') -> [-'~/prolog.ini'] ; true )).
% die on signal default. %


on_signal(Signal,OldAction,NewAction) :-
    var(Signal), !,
    (nonvar(OldAction) -> throw(error(instantiation_error,on_signal/3)) ; true),
    '$signal'(Signal),
    on_signal(Signal, OldAction, NewAction).
on_signal(Signal,OldAction,default) :-
    '$reset_signal'(Signal, OldAction).
on_signal(_Signal,_OldAction,Action) :-
    var(Action), !,
    throw(error('SYSTEM_ERROR_INTERNAL','Somehow the meta_predicate declarations of on_signal are subverted!')).
on_signal(Signal,OldAction,Action) :-
    OldAction == Action,
    !,
    clause('$signal_handler'(Signal), OldAction).
on_signal(Signal,_OldAction,Action) :-
    ( Action = _M:Goal -> true ; throw(error(type_error(callable,Action),on_signal/3)) ),
    % the following disagrees with 13211-2:6.7.1.4 which disagrees with 13211-1:7.12.2a %
    % but the following agrees with 13211-1:7.12.2a %
    ( nonvar(M) -> true ; throw(error(instantiation_error,on_signal/3)) ),
    ( atom(M) -> true ; throw(error(type_error(callable,Action),on_signal/3)) ),
    ( nonvar(Goal) -> true ; throw(error(instantiation_error,on_signal/3)) ),
    retractall('$signal_handler'(Signal)),
    assert(('$signal_handler'(Signal) :- Action)).


alarm(Interval, Goal, Left) :-
	Interval == 0, !,
	alarm(0, 0, Left0, _),
	on_signal(sig_alarm, _, Goal),
	Left = Left0.
alarm(Interval, Goal, Left) :-
	integer(Interval), !,
	on_signal(sig_alarm, _, Goal),
	'$alarm'(Interval, 0, Left, _).
alarm(Number, Goal, Left) :-
	float(Number), !,
	Secs is integer(Number),
	USecs is integer((Number-Secs)*1000000) mod 1000000,
	on_signal(sig_alarm, _, Goal),
	alarm(Secs, USecs, Left, _).
alarm([Interval|USecs], Goal, [Left|LUSecs]) :-
	on_signal(sig_alarm, _, Goal),
	alarm(Interval, USecs, Left, LUSecs).

raise_exception(Ball) :- throw(Ball).

on_exception(Pat, G, H) :- catch(G, Pat, H).

read_sig :-
	recorded('$signal_handler',X,_),
	writeq(X),nl,
	fail.
read_sig.

				% %
				% make thes predicates non-traceable. %

:- '$set_no_trace'(current_choice_point(_DCP), prolog).
:- '$set_no_trace'(cut_by(_DCP), prolog).
:- '$set_no_trace'(true, prolog).
:- '$set_no_trace'('$call'(_,_,_,_), prolog).
:- '$set_no_trace'('$execute0'(_), prolog).
:- '$set_no_trace'('$execute_clause'(_,_,_,_), prolog).
:- '$set_no_trace'('$restore_regs'(_,_), prolog).
:- '$set_no_trace'('$undefp'(_), prolog).


%%! @}
