/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-2014	 *
*									 *
*************************************************************************/

/**

 @file absf.yap
 @author L.Damas, V.S.Costa
*/

/**
 @defgroup absf File Name Resolution
 @ingroup InputOutput

@{

 Support for file name resolution through absolute_file_name/3 and
  friends. These utility built-ins describe a list of directories that
  are used by load_files/2 to search. They include pre-compiled paths
  plus user-defined directories, directories based on environment
  variables and registry information to search for files.


*/
:-  system_module( '$absf', [
        absolute_file_name/3,
        add_to_path/1,
        add_to_path/2,
        path/1,
        remove_from_path/1], []).

:- multifile user:file_search_path/2.
:- dynamic user:file_search_path/2.

:- dynamic user:prolog_file_type/2.


% :- set_prolog_flag( verbose_file_search, true ).


absf_trace(Msg, Args ) :-
    current_prolog_flag( verbose_file_search, true ),
    catch(
	print_message( informational, absolute_file_path( Msg, Args ) ),
	  _,
		true
      ),
    !.
absf_trace( _, _ ).

absf_trace( File ) :-
    current_prolog_flag( verbose_file_search, true ),
    catch(
	print_message( informational, absolute_file_path( File ) ),
	  _,
	  true
      ),
    !.
absf_trace( _File ).

absf_trace_component( Prefix, List ) :-
    current_prolog_flag( verbose_file_search, true ),
    catch(
	print_message( informational, absolute_file_path_component( Prefix, List ) ),
	  _,
	  true
      ),
    !.
absf_trace_component( _, _ ).


validate_absf(_File, TrueFile, LOpts, LOpts, TrueFile) :-
	var(TrueFile),
	!,
	must_be_list(LOpts).
validate_absf(_File, LOpts, TrueFile, LOpts, TrueFile ).
	
enter_absf( File, LOpts, Opts, State ) :-
    !,
    (
    var(File)
    ->
    instantiation_error(File) ; true),
    absf_trace('input: ~w',File),
    absf_trace_component('     ', LOpts),
    abs_file_parameters(LOpts,Opts),
    current_prolog_flag( open_expands_filename, PreviousFileNameExpand ),
    current_prolog_flag( file_errors, PreviousFileErrors ),
    current_prolog_flag( verbose_file_search, PreviousVerbose ),
    working_directory(D0,D0),
    State = abs_entry(File, D0, PreviousFileErrors, PreviousVerbose, PreviousFileNameExpand ).

set_absf(Opts) :- !,
    get_abs_file_parameter( verbose_file_search, Opts,Verbose ),
    get_abs_file_parameter( expand, Opts, PreviousFileNameExpand ),
    get_abs_file_parameter( file_errors, Opts, FileErrors ),
    set_prolog_flag( file_errors, FileErrors ),
    set_prolog_flag( open_expands_filename, PreviousFileNameExpand ),
    set_prolog_flag( verbose_file_search,  Verbose ).

restore_absf(abs_entry(_OldF, D0, PreviousFileErrors, PreviousVerbose, PreviousFileNameExpand) ) :- !,
    working_directory(_,D0),
    set_prolog_flag( open_expands_filename, PreviousFileNameExpand ),
    set_prolog_flag( file_errors, PreviousFileErrors ),
    set_prolog_flag( verbose_file_search, PreviousVerbose ).

absf_port(answer, File, Opts, TrueFileName, State ) :-
    absf_port(exit, File, Opts, TrueFileName,State  ).
absf_port(exit, _File,  _Opts, TrueFileName, State ) :-
    restore_absf(State),
    absf_trace(' |------- exit: found  ~a', [TrueFileName]).
absf_port(redo, _File, Opts, _TrueFileName,  _State ):-
    absf_trace(' !------- retry', []),
    set_absf(Opts).
absf_port(fail, File,_Opts, TrueFileName, _State) :-
    absf_trace(' !------- failed.', []),
    % check if no solution
    current_prolog_flag( file_errors, error ),
    throw_file_error(error,error(existence_error(file,File),absolute_file_name(File, TrueFileName, [File]))).
absf_port(fail, _File,_Opts, _TrueFileName, State) :-
        restore_absf(State).
absf_port(!, _File, _Opts, _TrueFileName, _State ).
absf_port(exception(_),_File, _Opts, _TrueFileName, State ) :- 
    restore_absf(State).
absf_port(external_exception(_),_File, _Opts, _TrueFileName, State ) :-  
    restore_absf(State).

find_in_path(Name, Opts, File) :-
	    '$library'(Name, Opts, Name1),
	    absf_trace('library expansion  done: ~s',Name1),
	    '$path2atom'(Name1,Name2),
	    absf_trace('path to text done: ~s',Name2),
	    '$suffix'(Name2, Opts, _, Name3),
	    absf_trace('suffix  done ~s',Name3),
	    '$glob'(Name3,Opts,Name4),
	    absf_trace('glob  done ~s',Name4),
	    get_abs_file_parameter( expand, Opts, Exp ),
	    absf_trace('expansion  done ~s',Exp),
	    (Exp == true,
             expand_file_name(Name4,Names0),
	     absf_trace('expansion  done ~s',Names0),
             Names0 = [_|_]
            ->
             Names = Names0
	    ;
              Names = [Name4]
             ),
	    member(Name5,Names),
	    absf_trace('pick ~s', Name5),
	    clean_name(Name5,Opts,File),
	    get_abs_file_parameter( file_type, Opts, Type ),
	    get_abs_file_parameter( access, Opts, Access ),
	    '$check_file'(File, Type, Access),
	    ( get_abs_file_parameter( solutions, Opts, first ) -> ! ; true ).
find_in_path(user,_,user_input) :- !.
find_in_path(user_input,_,user_input) :- !.
find_in_path(user_output,_,user_ouput) :- !.
find_in_path(user_error,_,user_error) :- !.

clean_name(N0,_OPts,NF) :-
    is_absolute_file_name(N0),
    !,
    absolute_file_name(N0,NF).
clean_name(N0,_OPts,NF) :-
    absolute_file_name(N0,NF).    
clean_name(N0,_OPts,NF) :-
    recorded('$path',D,_),
    working_directory(D0,D),
    absolute_file_name(N0,NF),
    working_directory(_D,D0).    
    

%
% handle library(lists) or foreign(jpl)
%
'$library'(Name, _Opts, E) :-
    (
	Name =.. [Lib, P]
    ->
    '$lib2path'(Lib, P, E)
    ;
    Name = E
    ).

'$lib2path'(Lib, P0, E) :-
    user:file_search_path(Lib, FLib),
    atom(FLib),
    '$lib2path'(FLib, P0, E).
'$lib2path'(Lib, P0, E) :-
    user:file_search_path(Lib, FLib),
    FLib =..  [F2Lib,Extra],
    '$lib2path'(F2Lib, Extra/P0, E).
'$lib2path'(Lib, P0, Lib/P0).


'$path2atom'(A,A) :-
    atom(A),
    !.
'$path2atom'(S,A) :-
    string(S),
    !,
    atom_string(A,S).

'$path2atom'(As,A) :-
    '$cat_file_name'(As,L,[]),
    path_concat(L,A).


'$cat_file_name'(A/B, NA, IA	) :-
    
    !,
    '$cat_file_name'(A, NA, RA),
    '$cat_file_name'(B, RA, IA).
'$cat_file_name'(File, NFs, Fs) :-
    atom(File),
    !,
    NFs = [File|Fs].
'$cat_file_name'(SFile, NFs, Fs) :-
    string(SFile),
    !,
    atom_string(File,SFile),
    NFs = [File|Fs].

'$suffix'(F,_Opts,Ext,F) :-
     file_name_extension(_Base, Ext, F),
     atom_codes(Ext,[_|_]),
     !.
'$suffix'(F,Opts,Ext,NF) :-
    (
	get_abs_file_parameter( extensions, Opts, Exts ),
        member(Ext, Exts),
	absf_trace(' trying suffix ~a from ~w', [Ext,Exts])
    ;
    get_abs_file_parameter( file_type, Opts, Type ),
    ( Type == source -> NType = prolog ; NType = Type ),
    user:prolog_file_type(Ext, NType)
    ),
    absf_trace(' trying suffix ~a from type ~a', [Ext, NType]),
    atom_concat([F,
'.',Ext],NF).
'$suffix'(F,_Opts,'',F).

'$glob'(F,Opts,NF) :-
    get_abs_file_parameter( glob, Opts, G ),
    G \= '',
    !,
    path_concat([F,G],NF).
'$glob'(F,_Opts,F).

% always verify if a directory, but take care to only
% fail if it exists.
'$check_file'(F, directory, _) :-
    '$access_file'(F, exist),
    !,
    exists_directory(F).
'$check_file'(_F, _Type, none) :- !.
'$check_file'(F, _Type, exist) :-
    '$access_file'(F, exist). % if it has a type cannot be a directory..
'$check_file'(F, _Type, Access) :-
    '$access_file'(F, Access),
    \+ exists_directory(F). % if it has a type cannot be a directory..

%
%
'$system_library_directories'(library, Dir) :-
	user:library_directory( Dir ).
%	'$split_by_sep'(0, 0, Dirs, Dir).
'$system_library_directories'(foreign, Dir) :-
    user:foreign_directory( Dir ).
% compatibility with old versions
%
% search the current directory  first.
'$system_library_directories'(commons, Dir) :-
	user:commons_directory( Dir ).


% enumerate all paths separated by a path_separator.
'$paths'(Cs, C) :-
    atom(Cs),
    (	current_prolog_flag(windows, true) -> Sep = ';' ; Sep = ':' ),
    sub_atom(Cs, N0, 1, N, Sep),
    !,
    (
	     sub_atom(Cs,0,N0,_,C)
     ;
        sub_atom(Cs,_,N,0,RC),
        '$paths'(RC, C)
    ).
'$paths'(S, S).


/**
  @pred path(-Directories:list) is det,deprecated

 YAP specific procedure that returns a list of user-defined directories
 in the library search-path.We suggest using user:file_search_path/2 for
 compatibility with other Prologs.
*/
path(Path) :-
	findall(X,'$in_path'(X),Path).


%% 3. honor user definition
'$in_path'(X) :-
    current_prolog_flag(os_argv, All),
    '$append'(_, ['-p',Dir0|_Alphas], All),
    absolute_file_name(Dir0, X,[file_type(directory),solutions(all),
                                expand(true),file_errors(fail)]).
'$in_path'(X) :-
    recorded('$path',Path,_),
    ( Path = ''  -> X = '.' ; X = Path ).


/**
  @pred add_to_path(+Directory:atom) is det,deprecated

  YAP-specific predicate to include directory in library search path.
  We suggest using user:file_search_path/2 for
   compatibility with other Prologs.
*/
add_to_path(New) :-
	add_to_path(New,last).

/**
  @pred add_to_path(+Directory:atom, +Position:atom) is det,deprecated

  YAP-specific predicate to include directory in front or back of
  library search path. We suggest using user:file_search_path/2 for
  compatibility with other Prologs and more extensive functionality.
*/
add_to_path(New,Pos) :-
	atom(New), !,
	check_path(New,Str),
	atom_codes(Path,Str),
	'$add_to_path'(Path,Pos).

'$add_to_path'(New,_) :-
	recorded('$path',New,R),
	erase(R),
	fail.
'$add_to_path'(New,last) :-
	!,
	recordz('$path',New,_).
'$add_to_path'(New,first) :-
	recorda('$path',New,_).

/**  @pred   remove_from_path(+Directory:atom) is det,deprecated

*/
remove_from_path(New) :- check_path(New,Path),

 			recorded('$path',Path,R), erase(R).

check_path(At,SAt) :-
    atom(At),
    !,
    atom_codes(At,S),
    check_path(S,SAt).
check_path([],[]).
check_path([Ch],[Ch]) :- '$dir_separator'(Ch), !.
check_path([Ch],[Ch,A]) :- !, integer(Ch), '$dir_separator'(A).
check_path([N|S],[N|SN]) :-
    integer(N),
    check_path(S,SN).


/**

@pred absolute_file_name( -File:atom, +Path:atom, +Options:list) is nondet

_Options_ is a list of options to guide the conversion:

  -  extensions(+ _ListOfExtensions_)

     List of file-name suffixes to add to try adding to the file. The
     Default is the empty suffix, `''`.  For each extension,
     absolute_file_name/3 will first add the extension and then verify
     the conditions imposed by the other options.  If the condition
     fails, the next extension of the list is tried.  Extensions may
     be specified both with dot, as `.ext`, or without, as plain
     `ext`.

  -  relative_to(+ _FileOrDir_ )

     Resolve the path relative to the given directory or directory the
     holding the given file.  Without this option, paths are resolved
     relative to the working directory (see working_directory/2) or,
     if  _Spec_  is atomic and absolute_file_name/3 is executed
     in a directive, it uses the current source-file as reference.

  -  access(+ _Mode_ )

     Imposes the condition access_file( _File_ ,  _Mode_ ).   _Mode_  is one of `read`, `write`, `append`, `exist` or
     `none` (default).

     See also access_file/2.

  -  file_type(+ _Type_ )

     Defines suffixes matching one of several pre-specified type of files. Default mapping is as follows:

       1.  `txt` implies `[ '' ]`,

       2.  `prolog` implies `['.yap', '.pl', '.prolog', '']`,

       3.  `executable`  implies `['.so', ',dylib', '.dll']` depending on the Operating system,

       4.  `qly` implies `['.qly', '']`,

       5.  `directory` implies `['']`,

       6.  The file-type `source` is an alias for `prolog` designed to support compatibility with SICStus Prolog. See also prolog_file_type/2.

     Notice that this predicate only
     returns non-directories, unless the option `file_type(directory)` is
     specified, or unless `access(none)`.

  -  file_errors(`fail`/`error`)

     If `error` (default), throw  `existence_error` exception
     if the file cannot be found.  If `fail`, stay silent.

  -  solutions(`first`/`all`)

     If `first` (default), commit to the first solution.  Otherwise
     absolute_file_name will enumerate all solutions via backtracking.

  -  expand(`true`/`false`)

     If `true` (default is `false`) and _Spec_ is atomic, call
     expand_file_name/2 followed by member/2 on _Spec_ before
     proceeding.  This is originally a SWI-Prolog extension, but
     whereas SWI-Prolog implements its own conventions, YAP uses the
     shell's `glob` primitive.

     Notice that in `glob` mode YAP will fail if it cannot find a matching file, as `glob`
     implicitely tests for existence when checking for patterns.

  -  glob(`Pattern`)

     If  _Pattern_ is atomic, add the pattern as a suffix to the current expansion, and call
     expand_file_name/2 followed by member/2 on the result. This is originally  a SICStus Prolog exception.

     Both `glob` and `expand` rely on the same underlying
     mechanism. YAP gives preference to `glob`.

  -  verbose_file_search(`true`/`false`)

     If `true` (default is `false`) output messages during
     search. This is often helpful when debugging. Corresponds to the
     SWI-Prolog flag `verbose_file_search` (also available in YAP).


Compatibility considerations to common argument-order in ISO as well
as SICStus absolute_file_name/3 forced us to be flexible here.
If the last argument is a list and the second not, the arguments are
swapped, thus the call

```
?- absolute_file_name( 'pl/absf.yap', [], Path)
```
is valid as well.
*/

absolute_file_name(File,TrueFileName0,LOpts0) :-
    %   must_be_of_type( atom, File ),
    % look for solutions
    validate_absf( File, TrueFileName0, LOpts0, LOpts, TrueFileName),
    gated_call(
	enter_absf( File, LOpts, Opts, State),
		   find_in_path(File, Opts,TrueFileName),
		   Port,
		   absf_port(Port, File, Opts, TrueFileName, State)
	).


/**
 * @pred exists_source(_File_, _FullPath_)
 *
 * if the path _File_ matches a readable Prolog file in the file system. 
 * unify its full path with _FullPath_.
 * 
 */
exists_source(Source, Path) :-
	absolute_file_name(Source, Path,
			   [ file_type(prolog),
			     access(read),
			     file_errors(fail)
			   ]).

/**
 * @pred file_exists(_File_)
 *
 * Succeeds if the path _File_ matches an entry in the file system. 
 * 
 */
file_exists(File) :-    
    absolute_file_name( File, _, [access(exist),
				 file_errors(fail)]).

/**	exists_source(+Source) is semidet.

	True if Source (a term  valid   for  load_files/2) exists. Fails
	without error if this is not the case. The predicate is intended
	to be used with  :-  if,  as   in  the  example  below. See also
	source_exports/2.

```
	:- if(exists_source(library(error))).
	:- use_module_library(error).
	:- endif.
```
*/

exists_source(Source) :-
	exists_source(Source, _Path).

/** @} */


