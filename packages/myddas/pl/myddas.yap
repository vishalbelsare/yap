:- module(myddas,[
		  db_open/5,
		  db_open/4,
%		  db_open/2,
%		  db_open/1,
%		  db_open/0,
		  db_close/1,
		  db_close/0,

		  db_verbose/1,
		  db_module/1,
		  db_is_database_predicate/3,
		  %#ifdef MYDDAS_STATS
%		 db_stats/1,
%		  db_stats/2,
%		  db_stats_time/2,
		  %#endif
		 db_sql/2,
		  db_sql/3,
		  db_sql_select/3,
		  db_prolog_select/2,
		  db_prolog_select/3,
		  db_prolog_select_multi/3,
		  db_command/2,
		  db_assert/2,
		  db_assert/1,
		  db_create_table/3,
		  db_export_view/4,
		  db_update/2,
		  db_describe/2,
		  db_describe/3,
		  db_show_tables/2,
		  db_show_tables/1,
	  db_get_attributes_types/2,
		  db_get_attributes_types/3,
		  db_number_of_fields/2,
		  db_number_of_fields/3,
      % myddas_shared.c
      c_db_initialize_myddas/0,
      c_db_connection_type/2,
      c_db_add_preds/4,
      c_db_preds_conn/4,
      c_db_connection/1,
      c_db_check_if_exists_pred/3,
      c_db_delete_predicate/3,
      c_db_multi_queries_number/2,
/*      %#ifdef MYDDAS_STATS
      c_db_stats/2,
      c_db_stats_walltime/1,
      c_db_stats_translate/2,
      c_db_stats_time/2,
      %#endif
      %#ifdef DEBUG
      c_db_check/0,
      %#endif
		  %#ifdef MYDDAS_TOP_LEVEL
          db_top_level/4,
		  db_top_level/5,
		  db_datalog_select/3,
		  %#endif
*/
				% myddas_assert_predicates.ypp
				% myddas_mysql.ypp,
          		  db_multi_queries_number/2
]).


:- load_foreign_files([], ['YAPmyddas'], c_db_initialize_myddas).

:- use_module('myddas/myddas_sqlite3').
:- use_module('myddas/myddas_odbc').
:- use_module('myddas/myddas_postgres').
:- use_module('myddas/myddas_mysql').

:- reexport(myddas/myddas_assert_predicates,[
					db_import/2,
					db_import/3,
					db_view/2,
					db_view/3,
					db_insert/2,
					db_insert/3,
					db_abolish/2,
					db_listing/0,
					db_listing/1
	      ]).

:- meta_predicate db_import(+,+,:), db_import(+,:).



:- use_module(myddas/myddas_util_predicates,[
				      '$prolog2sql'/3,
				      '$create_multi_query'/3,
				      '$get_multi_results'/4,
				      '$process_sql_goal'/4,
				      '$process_fields'/3,
				      '$get_values_for_insert'/3,
				      '$make_atom'/2,
				      '$write_or_not'/1,
				      '$abolish_all'/1,
				      '$make_a_list'/2,
				      '$get_table_name'/2,
				      '$get_values_for_update'/4,
				      '$extract_args'/4,
/*				      #if MYDDAS_STATS
				      '$make_stats_list'/2,
				      #endif
*/				     '$lenght'/2
				     ]).

:- use_module(myddas/myddas_errors,[
			     '$error_checks'/1

			    ]).

:- use_module(myddas/myddas_prolog2sql,[
				 translate/3,
				 queries_atom/2
				]).

:- use_module(library(lists),[
		  append/3,
      member/2
	      ]).

:- set_prolog_flag(verbose,silent).

:- multifile user:file_search_path/2.

:- dynamic user:file_search_path/2.

user:file_search_path(dataset, C) :-
    user:library_directory(Dir),
    (   current_prolog_flag(windows, true)
    ->  atomic_list_concat([Dir,data], ;, C)
    ;
    atomic_list_concat([Dir,data], :, C)
    ).
user:file_search_path(dataset, '.').


:- multifile user:prolog_file_type/2.

:- dynamic user:prolog_file_type/2.

user:prolog_file_type( db, dataset ).

:- include( myddas/myddas_core ).

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
  * File:		myddas.yap	                                         *
  * Last rev:							         *
  * mods:									 *
  * comments:	Global predicates for the MyDDAS Interface		 *
  *									 *
  *************************************************************************/

#if DEBUG
:- set_prolog_flag(single_var_warnings,on).
:- set_prolog_flag(write_strings,on).
#endif

#define SWITCH(Contype, G)     \
	( Contype == mysql ->      \
	  myddas_my:my_ ## G                 \
	;                          \
	  Contype == sqlite3 ->    \
	  myddas_sqlite3:sqlite3_ ## G            \
	;                          \
	  Contype == postgres ->   \
	  myddas_postgres:postgres_ ## G            \
	;                          \
	  Contype == odbc ->       \
	  myddas_odbc:odbc_ ## G               \
	)

	#define C_SWITCH(Contype, G)     \
		( Contype == mysql ->      \
		  myddas_my:c_my_ ## G                  \
		;                          \
		  Contype == sqlite3 ->    \
		  myddas_sqlite3:c_sqlite3_ ## G             \
		;                          \
		  Contype == postgres ->   \
		  myddas_postgres:c_postgres_ ## G           \
		;                          \
		  Contype == odbc ->       \
		  myddas_odbc:c_odbc_ ## G               \
		)



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_open/5
				% db_open/4
				%


%% @pred db_open(+Interface,-Handle,+HostDb,+User,+Password)
%% @pred db_open(+Interface,+HostDb,+User,+Password)
%
% Connect to a database-server, or open a file as a database, Parameters are:
%
% _Interface_ : a supported server, it may be one of mysql, odbc, postgres, sqlite3.
% _Handle_ : a name that refers to the database-conectionn. By default is `myddas`,
% _Connection Info': either a sequence Host/Db/Port/Socket or a file name. You can use xb0 for the defaukt port and sockets,
% _UserId_; the use identifier, ignored by sqlite3;
% _Password_ : access control, ignored by sqlite3,
%
%
%

#if MYDDAS_MYSQL
db_open(mysql,Connection,Host/Db/Port/Socket,User,Password) :- !,
	'$error_checks'(db_open(mysql,Connection,Host/Db/Port/Socket,User,Password)),
	c_db_my_connect(Host,User,Password,Db,Port,Socket,Con),
	set_value(Connection,Con).
db_open(mysql,Connection,Host/Db/Port,User,Password) :-
	integer(Port),!,
	db_open(mysql,Connection,Host/Db/Port/_,User,Password).  % Var to be NULL, the  default socket
db_open(mysql,Connection,Host/Db/Socket,User,Password) :- !,
	db_open(mysql,Connection,Host/Db/0/Socket,User,Password). % 0 is default port
db_open(mysql,Connection,Host/Db,User,Password) :-
	db_open(mysql,Connection,Host/Db/0/_,User,Password).  % 0 is default port and Var to be NULL, the default socket
#endif

#if MYDDAS_POSTGRES
db_open(postgres,Connection,Host/Db/Port/Socket,User,Password) :- !,
	'$error_checks'(db_open(postgres,Connection,Host/Db/Port/Socket,User,Password)),
	c_db_my_connect(Host,User,Password,Db,Port,Socket,Con),
	set_value(Connection,Con).
db_open(postgres,Connection,Host/Db/Port,User,Password) :-
	integer(Port),!,
	db_open(postgres,Connection,Host/Db/Port/_,User,Password).  % Var to be NULL, the  default socket
db_open(postgres,Connection,Host/Db/Socket,User,Password) :- !,
    db_open(postgres,Connection,Host/Db/0/Socket,User,Password). % 0 is default port
db_open(postgres,Connection,Host/Db,User,Password) :-
    db_open(postgres,Connection,Host/Db/0/_,User,Password).  % 0 is default port and Var to be NULL, the default socpket
#endif

#if MYDDAS_ODBC
db_open(odbc,Connection,ODBCEntry,User,Password) :-
    '$error_checks'(db_open(odbc,Connection,ODBCEntry,User,Password)),
    c_odbc_connect(ODBCEntry,User,Password,Con),
    set_value(Connection,Con).
#endif

#if MYDDAS_SQLITE3
db_open(sqlite3,Connection,File,User,Password) :-
    absolute_file_name(File,Db,[access(write),file_type(myddas),expand(true)]),
    '$error_checks'(db_open(sqlite3,Connection,Db,User,Password)),
    c_sqlite3_connect(Db,User,Password,Con),
    set_value(Connection,Con).
db_open(sqlite3,File,User,Password) :-
    absolute_file_name(File,Db,[access(write),file_type(myddas),expand(true)]),
    '$error_checks'(db_open(sqlite3,Connection,Db,User,Password)),
    c_sqlite3_connect(Db,User,Password,Con),
    set_value(myddas,Con).
#endif
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% db_close/1
%% db_close/0
%
% close a connection _Con_: all its resources are returned, and all undefined
% predicates are abolished. Default is to close `myddas`.
db_close:-
    db_close(myddas).
db_close(Protocol):-
    '$error_checks'(db_close(Protocol)),
    get_value(Protocol,Con),
    c_db_connection_type(Con,ConType),
    ( '$abolish_all'(Con) ;
      set_value(Protocol,[]), % "deletes" atom
      C_SWITCH( ConType, disconnect(Con) )
    ).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% db_verbose/1
%
%
db_verbose(X):-
    var(X),!,
    get_value(db_verbose,X).
db_verbose(N):-!,
    set_value(db_verbose,N).
%default value
:- set_value(db_verbose,0).
:- set_value(db_verbose_filename,myddas_queries).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% db_module/1
%
%
db_module(X):-
    var(X),!,
    get_value(db_module,X).
db_module(ModuleName):-
    set_value(db_module,ModuleName).
% default value
:- db_module(user).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% db_is_database_predicate(+,+,+)
%
%
db_is_database_predicate(Module,PredName,Arity):-
    '$error_checks'(db_is_database_predicate(PredName,Arity,Module)),
    c_db_check_if_exists_pred(PredName,Arity,Module).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#if MYDDAS_STATS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% db_stats(+,-)
%
%
db_stats(List):-
    db_stats(myddas,List).

db_stats(Protocol,List):-
    '$error_checks'(db_stats(Protocol,List)),
    NumberOfStats = 10,
    '$make_a_list'(NumberOfStats,ListX1),
    ( var(Protocol) ->
      c_db_stats(0,ListX1)
    ;
    get_value(Protocol,Conn),
    c_db_stats(Conn,ListX1)
    ),
    '$make_stats_list'(ListX1,List).

#if  DEBUG
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% db_stats_time(+,-)
% Reference is C pointer (memory reference)
				%
db_stats_time(Reference,Time):-
	'$error_checks'(db_stats_time(Reference,Time)),
	c_db_stats_time(Reference,Time).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif /* DEBUG */

#endif /* MYDDAS_STATS */


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_sql(+,+,-)
				%
				%

				%compatibility
db_sql_select(Protocol,SQL,LA):-
	db_sql(Protocol,SQL,LA).

db_sql(SQL,LA):-
	db_sql(myddas,SQL,LA).

db_sql(Connection,SQL,LA):-
	'$error_checks'(db_sql(Connection,SQL,LA)),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
	db_sql_(ConType, Con, SQL, LA).

db_sql_(ConType, Con, SQL,LA):-
	'$write_or_not'(SQL),
	( ConType == mysql ->
	  my_result_set(Mode),
	  c_db_my_query(SQL,ResultSet,Con,Mode,Arity)
	;ConType == postgres ->
	  postgres_result_set(Mode),
	  c_postgres_query(SQL,ResultSet,Con,Mode,Arity)
	;ConType == sqlite3 ->
	  sqlite3_result_set(Mode),
	  c_sqlite3_query(SQL,ResultSet,Con,Mode,Arity)
	;
	  c_odbc_query(SQL,ResultSet,Arity,LA,Con),
	  c_odbc_number_of_fields_in_query(SQL,Con,Arity)
	),
	'$make_a_list'(Arity,LA),
	SWITCH( ConType, row(ResultSet,Arity,LA) ).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_prolog_select(+,+,+)
				%
				%
db_prolog_select(LA,DbGoal):-
	db_prolog_select(myddas,LA,DbGoal).
db_prolog_select(Connection,LA,DbGoal):-
	length(LA,Arity),
	Name=viewname,
	functor(ViewName,Name,Arity),
				% build arg list for viewname/Arity
	ViewName=..[Name|LA],
	'$prolog2sql'(ViewName,DbGoal,SQL),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
    db_sql_(ConType, Con, SQL,LA).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_prolog_select_multi(+,+,-)
				% db_prolog_select_multi(guest,[(ramos(A,C),A=C),(ramos(D,B),B=10)],[[A],[D,B]]).
				%
db_prolog_select_multi(Connection,DbGoalsList,ListOfResults) :-
	'$error_checks'(db_prolog_select_multi(Connection,DbGoalsList,ListOfResults)),
	'$create_multi_query'(ListOfResults,DbGoalsList,SQL),

	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
	'$write_or_not'(SQL),
	C_SWITCH(ConType, query(SQL,ResultSet,Con,Mode,_) ),
	'$get_multi_results'(Con,ConType,ResultSet,ListOfResults).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_command/2
				%
				%
db_command(Connection,SQL):-
	'$error_checks'(db_command(Connection,SQL)),
	get_value(Connection,Con),
	'$write_or_not'(SQL),
	c_db_connection_type(Con,ConType),
	( ConType == mysql ->
	  db_my_result_set(Mode),
	  c_db_my_query(SQL,_,Con,Mode,_)
	;
	  true
	).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_assert/2
				% db_assert/1
				%
db_assert(PredName):-
	db_assert(myddas,PredName).

db_assert(Connection,PredName):-
	translate(PredName,PredName,Code),
	'$error_checks'(db_insert2(Connection,PredName,Code)),
	'$get_values_for_insert'(Code,ValuesList,RelName),
	'$make_atom'(['INSERT INTO `',RelName,'` VALUES '|ValuesList],SQL),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
	'$write_or_not'(SQL),
	( ConType == mysql ->
	  db_my_result_set(Mode),
	  c_db_my_query(SQL,_,Con,Mode,_)
	;ConType == postgres ->
	  postgres_result_set(Mode),
	  c_postgres_query(SQL,_,Con,Mode,_)
	;ConType == sqlite3 ->
	  sqlite3_result_set(Mode),
	  myddas_myddas_sqlite3:c_sqlite3_query(SQL,_,Con,Mode,_)
	;
	  c_odbc_query(SQL,_,_,_,Con)
	).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_create_table/3
				% FieldsList = [field(Name,Type,Null,Key,DefaultValue)]
				% Example [field(campo1,'char(12)',y,y,a),field(campo2,int,y,y,0)]
				% TODO Test with ODBC & Type Checks
db_create_table(Connection,TableName,FieldsInf):-
	'$error_checks'(db_create_table(Connection,TableName,FieldsInf)),
	get_value(Connection,Con),

	'$process_fields'(FieldsInf,FieldString,KeysSQL),
	'$make_atom'(['CREATE TABLE `',TableName,'` ( ',FieldString,KeysSQL,' )'],FinalSQL),

	c_db_connection_type(Con,ConType),
	'$write_or_not'(FinalSQL),
	( ConType == mysql ->
	  db_my_result_set(Mode),
	  c_db_my_query(FinalSQL,_,Con,Mode,_)
	;ConType == posgres ->
	  postgres_result_set(Mode),
	  c_postsgres_query(FinalSQL,_,Con,Mode,_)
	;ConType == sqlite3 ->
	  sqlite3_result_set(Mode),
	  myddas_myddas_sqlite3:c_sqlite3_query(FinalSQL,_,Con,Mode,_)
	;
	  c_odbc_query(FinalSQL,_,_,_,Con)
	).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_export_view/4
				% TODO Test with ODBC
				%
db_export_view(Connection,TableViewName,SQLorDbGoal,FieldsInf):-
	'$error_checks'(db_export_view(Connection,TableViewName,SQLorDbGoal,FieldsInf)),
	get_value(Connection,Con),
	'$process_sql_goal'(TableViewName,SQLorDbGoal,TableName,SQL),

				% Case there's some information about the
				% attribute fields of the relation given
				% by the user
	( FieldsInf == [] ->
	  '$make_atom'(['CREATE TABLE ',TableName,' AS ',SQL],FinalSQL)
	;
	  '$process_fields'(FieldsInf,FieldString,KeysSQL),
	  '$make_atom'(['CREATE TABLE ',TableName,' (',FieldString,KeysSQL,') AS ',SQL],FinalSQL)
	),

	c_db_connection_type(Con,ConType),
	'$write_or_not'(FinalSQL),
	( ConType == mysql ->
	  db_my_result_set(Mode),
	  c_db_my_query(FinalSQL,_,Con,Mode,_)
	;
	  c_odbc_query(FinalSQL,_,_,_,Con)
	).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_update/2
				% BUG: db_update dosen't work for this case, just an example
				% db_update(my1,edge(1,3)-edge(99,99)).
				% The case where the set condition is "set" to change all the fields
db_update(Connection,WherePred-SetPred):-
				%TODO: error_checks
	get_value(Connection,Conn),

				% Match and Values must be "unifiable"
	functor(WherePred,PredName,Arity),
	functor(SetPred,PredName,Arity),

	functor(NewRelation,PredName,Arity),

	'$extract_args'(WherePred,1,Arity,WhereArgs),
	'$extract_args'(SetPred,1,Arity,SetArgs),

	copy_term(WhereArgs,WhereArgsTemp),
	NewRelation=..[PredName|WhereArgsTemp],
	translate(NewRelation,NewRelation,Code),

	'$get_values_for_update'(Code,SetArgs,SetCondition,WhereCondition),

	'$get_table_name'(Code,TableName),
	append(SetCondition,WhereCondition,Conditions),
	'$make_atom'(['UPDATE `',TableName,'` '|Conditions],SQL),
	'$write_or_not'(SQL),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
	( ConType == mysql ->
	db_my_result_set(Mode),
	  c_db_my_query(SQL,_,Conn,Mode,_)
	;
	  ConType == mysql ->
	postgres_result_set(Mode),
	  c_postgres_query(SQL,_,Conn,Mode,_)
	;
	ConType == sqlite3 ->
	sqlite3_result_set(Mode),
	 myddas_sqlite3:c_sqlite3_query(SQL,_,Conn,Mode,_)
	;
	  ConType == odbc ->
	odbc_result_set(Mode),
	  c_odbc_query(SQL,_,Conn,Mode,_)
	).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
/**
 *
@pred db_get_attributes_types(+Conn,+RelationName,-ListOfFields)
@pred  db_get_attributes_types(RelationName,ListOfFields)
Y
ou can use the predicate db_get_attributes_types/2 or db_get_attributes_types/3, to know what
are the names and attributes types of the fields of a given relation. For example:

```
?- db_get_attributes_types(myddas,’Hello World’,LA).
LA = [’Number’,integer,’Name’,string,’Letter’,string] ?
yes
˜˜˜˜
where `Hello World` is the name of the relation and `myddas` is the connection identifier.
 */
db_get_attributes_types(RelationName,TypesList) :-
	db_get_attributes_types(myddas,RelationName,TypesList).
db_get_attributes_types(Connection,RelationName,TypesList) :-
	'$error_checks'(db_get_attributes_types(Connection,RelationName,TypesList)),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
    C_SWITCH(ConType, get_attributes_types(RelationName,Con,TypesList) ).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/**

@pred db_number_of_fields(+Conn,+RelationName,-Arity).
@pred  db_number_of_fields(+RelationName,-Arity).

You can use the predicate db number of fields/2 or db number of fields/3, to know what is the arity
of a given relation.

@Example
```
?- db_number_of_fields(myddas,’Hello World’,Arity).
Arity = 3 ?
yes
˜˜˜˜
where `Hello World` is the name of the relation and `myddas` is the connection identifier.
 */
				%
db_number_of_fields(RelationName,Arity) :-
	db_number_of_fields(myddas,RelationName,Arity).
db_number_of_fields(Connection,RelationName,Arity) :-
	'$error_checks'(db_number_of_fields(Connection,RelationName,Arity)),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
    C_SWITCH(ConType, number_of_fields(RelationName,Con,Arity) ).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
				% db_multi_queries_number(+,+)
				% TODO: EVERITHING
				%
db_multi_queries_number(Connection,Number) :-
	'$error_checks'(db_multi_queries_number(Connection,Number)),
	get_value(Connection,Con),
	c_db_multi_queries_number(Con,Number).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



db_result_set(X)  :-
	'$error_checks'(db_result_set(X) ),
	get_value(myddas,Con),
	c_db_connection_type(Con,DBMS),
	DBMS:result_set(X).

db_datalog_describe(X)  :-
	'$error_checks'(db_datalog_describe(X) ),
	get_value(myddas,Con),
	c_db_connection_type(Con,DBMS),
	 DBMS:datalog_describe(X).

db_datalog_describe(Connection,Y)  :-
	'$error_checks'(db_datalog_describe(Connection,Y) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,DBMS),
	 DBMS:datalog_describe(Connection,Y).

/**
	 @pred db_describe(+,+,?)
	 @pred db_describe(+,?)

	 The db describe/3 predicate does the same action as db_datalog_describe/2 predicate but with one
	 major difference. The results are returned by backtracking. For example, the last query done:

	 ~~~~
	 ?- db_describe(myddas,’Hello World’,Term).
	 Term = tableInfo(’Number’,int(11),’YES’,’’,null(0),’’) ? ;
	 Term = tableInfo(’Name’,char(10),’YES’,’’,null(1),’’) ? ;
	 Term = tableInfo(’Letter’,char(1),’YES’,’’,null(2),’’) ? ;
	 no
	 ~~~~
	 */
db_describe(Y,Z)  :-
    db_describe(myddas,Y,Z).
db_describe(Connection,Y,Z)  :-
 	'$error_checks'(db_describe(Connection,Y,Z) ),
 	get_value(Connection,Con),
 	c_db_connection_type(Con,ConType),
 	SWITCH(ConType, describe(Connection,Y,Z) ).


db_datalog_show_tables  :-
	db_datalog_show_tables(myddas).
db_show_tables :-
	'$error_checks'(db_datalog_show_tables),
	get_value(myddas,Con),
	c_db_connection_type(Con,DBMS),
	DBMS:datalog_show_tables.

db_datalog_show_tables(Connection)  :-
	'$error_checks'(db_datalog_show_tables(Connection) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,DBMS),
	switch( DBMS, datalog_show_tables(Connection) ).


/**
	@pred db_show_tables(+,?).
	@pred db_show_tables(?).

	The db show tables/2 predicate does the same action as db show tables/1 predicate but with one
	major difference. The results are returned by backtracking.

	 For example, the last query done:
	~~~~`
	`?- db_show_tables(myddas,Table).
	Table = table(’Hello World’) ? ;
	~~~~
``*/
	db_show_tables(Y)  :-
		db_show_tables(myddas,Y).
			db_show_tables(Connection,Y)  :-
	'$error_checks'(db_show_tables(Connection,Y) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,ConType),
	 SWITCH( ConType, show_tables(Connection,Y) ).

db_show_database(Connection,Y)  :-
	'$error_checks'(db_show_database(Connection,Y) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,DBMS),
	 DBMS:show_database(Connection,Y).

db_show_databases(Connection,Y)  :-
	'$error_checks'(db_show_databases(Connection,Y) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,DBMS),
	 DBMS:show_databases(Connection,Y).

db_show_databases(X)  :-
	'$error_checks'(db_show_databases(X) ),
	get_value(myddas,Con),
	c_db_connection_type(Con,DBMS),
	 DBMS:show_databases(X).

db_change_database(Connection,Y)  :-
	'$error_checks'(db_change_database(Connection,Y) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,DBMS),
	DBMS:change_database(Connection,Y).

db_call_procedure(Connection,Y,Z,W)  :-
	'$error_checks'(db_call_procedure(Connection,Y,Z,W) ),
	get_value(Connection,Con),
	c_db_connection_type(Con,DBMS),
	DBMS:call_procedure(Connection,Y,Z,W).

db_call_procedure(X,Y,Z)  :-
	'$error_checks'(db_call_procedure(X,Y,Z) ),
	get_value(myddas,Con),
	c_db_connection_type(Con,DBMS),
     DBMS:call_procedure(X,Y,Z).
