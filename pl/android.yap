/**
 @file android.yap

@defgroup YAPAndroid Android Support
@ingroup YapExtensions
@{
*/
%:- module(android,
%	[text_to_query/2]).
	
:- initialization(set_prolog_flag(verbose,normal)).

:- meta_predicate text_to_query( :, '-' ).

text_to_query( MString, Status ) :-
    strip_module( user:MString, Mod, String ),
    top_query(android:query_( Mod:String, Status, _ ) ).

query_( Mod:String, Status, Bindings ) :-
  	atomic_to_term( String, Goal, VarNames ),
	(
	is_list(Goal) -> G = consult( Goal ) ; G = Goal ),
 	query_to_answer( Mod:G, VarNames, Status, Bindings).


user:file_search_path(data, '/data/data/pt.up.yap/files/Yap/myddas').


%% @} 
