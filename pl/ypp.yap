/**
 * @file   ypp.yap
 * @author Nuno Fonseca (nunofonseca@acm.org), Tiago Soares
 * @date  2005-05-14
 * 
 * @brief  Yap PreProcessing
 * 
 * 
*/
%====================================================================================
%
% YPP: Yap PreProcessing
%
% Author: Nuno Fonseca (nunofonseca@acm.org)
% Date: 
% $Id: ypp.yap,v 1.4 2006-03-07 17:30:47 tiagosoares Exp $
%
%====================================================================================

%====================================================================================
% Module declarations
%====================================================================================

:-system_module(ypp,[
	      ypp_state/1,	% ypp_state(on|off)
	      ypp_define/2,     % persistent defines  ypp_define(+NAME,VALUE)
	      ypp_undefine/1,   % persistent defines  ypp_undefine(+NAME)
	      ypp_extcmd/1,	% ypp_extcmd(?command)
	      ypp_consult/1,    % similiar to standard consult but with preprocessing
	      ypp_reconsult/1
		],
		[]
        ).

/**
* @defgroup Ypp Yap PreProcessing
* @ingroup YAPLibrary
*
* @ Interface to the `C` preprocessor.
*/


%====================================================================================
% Public Predicates
%====================================================================================

ypp_state(State):-
	ground(State),!,
	set_state(State).

ypp_state(State):-
	get_state(State),
	!.
%% @pred ypp_define(Name,Value).
%
% Define symbol _Name_

ypp_define(Name,Value):-
	ground(Name),ground(Value),
	store_define(Name,Value).

%% @pred ypp_undefine(Name).
%
% Retire symbol _Name_

ypp_undefine(Name):-
	ground(Name),
	del_define(Name).

ypp_extcmd(Cmd):-
	ground(Cmd),!,
	eraseall('____ypp_extcmd'),
	recorda('____ypp_extcmd',Cmd,_).
ypp_extcmd(Cmd):-
	\+ ground(Cmd),
	recorded('____ypp_extcmd',Cmd,_).

%% @pred ypp_consult(File)
%
% call ypp on _File_ and then consult the result.

ypp_consult(File):-
	(get_state(on)->ypp_file(File,NFile);NFile=File),
	consult(NFile).

%% @pred ypp_consult(File)
%
% call ypp on _File_ and then reconsult the result.
ypp_reconsult(File):-
	(get_state(on)->ypp_file(File,NFile);NFile=File),
	reconsult(NFile).
%====================================================================================
% Private Predicates
%====================================================================================

set_state(on):-set_value('____ypp_state',1).
set_state(off):-set_value('____ypp_state',0).

get_state(State):-
	get_value('____ypp_state',Val),
	(Val==0->State=off;State=on).


store_define(Name,_Value):-
	recorded('___ypp',def(Name,_),Ref),
        erase(Ref),
        fail.
store_define(Name,Value):-
	system_variable( Name ),
	getenv( Name, Value ),
	fail.
store_define(Name,Value):-
	recordz('___ypp',def(Name,Value),_),
	fail.
store_define(_Name,_Value).

system_variable( 'YAPLIBDIR' ).
system_variable( 'YAPSHAREDIR' ).
system_variable( 'YAPBINDIR' ).


del_define(Name):-
	(recorded('___ypp',def(Name,_),Ref)->erase(Ref);true),
	!.

%defines2string(-DefineString)
defines2string(S):-
	findall(d(Name,Val),recorded('___ypp',def(Name,Val),_),AllDefs),
	mergedefs(AllDefs,'-D',S),
	!.

mergedefs([],_,'').
mergedefs([d(Name,Val)|RDefs],Pref,S):-
	mergedefs(RDefs,Pref,S1),
	atom_concat([Pref,Name,'=',Val,' ',S1],S).


ypp_file(File,PPFile):-
	% ppfile
	atom_concat([File,'.ypp'],PPFile),
	% Cmdline
	defines2string(Defs),ypp_extcmd(Cmd),
	atom_concat([Cmd,' ',PPFile,' ',Defs,' ',File],Cmdline),
	write(Cmdline),nl,
%	current_module(M1,M2),
%	write(M1:M2),nl,
	system(Cmdline),
	!.

% initialization
:-set_state(on),
	ypp_extcmd('cpp -P -E -w -o ').
%	ypp_extcmd('gpp -o').
