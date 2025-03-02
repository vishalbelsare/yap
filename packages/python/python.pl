% % % -*-Mode : Prolog; -*-
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%    Author:        Vitor Santos Costa
%    E-mail:        vsc@dcc.fc.up.pt
%    Copyright (C): Universidade do Porto
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  This file is part of the YAP Python Interface
%  distributed according to Perl Artistic License
%  check LICENSE file for distribution license
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% file python.pl
%%%

:- module(python,
	  [
	   end_python/0,
	   python_run_file/1,
	   python_run_command/1,
	   python_run_script/2,
	   python_assign/2,
	   python_assign_indexed/3,
	   python_represents/2,
	   python_import/1,
	   array_to_python_list/4,
	   array_to_python_tuple/4,
	   array_to_python_view/5,
	   python/2,
	   python_threaded/0,
	   prolog_list_to_python_list/3,
	   python_clear_errors/0,
	   python_string_to/1,
	   op(100,fy,$),
	   op(950,fy,:=),
	   op(950,yfx,:=),
%	   op(950,fx,<-),
%	   op(950,yfx,<-),
	   op(50, yf, []),
	   op(50, yf, '()'),
	   op(100, xfy, '.'),
	   op(100, fy, '.'), 
	   (:=)/2,
        (:=)/1,
	%        (<-)/1,
	%        (<-)/2,
	'()'/1, '{}'/1
	  ]).


/** @defgroup YAP_YAP_Py4YAP A C-based  Prolog interface to python.
    @ingroup YAP_YAP_python
b

@{

  @author               Vitor Santos Costa
  @version      0:0:5, 2012/10/8
  @license      Perl Artistic License

This is an interface to allow calling Python from Prolog. Please look
at the YAP4PY SWIG package if you want to embedd Prolog with Python.

The interface should be activated by consulting the python library. It
immediately boots a Python image.

To best define the interface, one has to address two opposite goals:
    - make it as similar to python as possible
    - make all embedded language interfaces (python, R, Java) as
similar as possible.

   YAP supports the following translation between Prolog and Python:

| *Prolog*      | *Pyhon*       |          *Prolog Examples*             |
|:-------------:|:-------------:|---------------------------------------:|
| Numbers       | Numbers       | 2.3
|               |               | 1545
|               |               |
| Atom          | Symbols       | var
| $Atom         |               | $var [ = var]
| `string`      | 'string'      | \`hello\`
| "string"      |        '      | "hello"
|               |               |
| Atom(...)     | Symb(...)     | f( a, b, named=v)
| E.F(...)      | E.F (...)     | mod.f( a) [ = [mod\|f(a)] ]
| Atom()        |               | f() [ = '()'(f) ]
| Lists         | Lists         | [1,2,3]
| t(....)       | Tuples        | t(1,2,3) to (1,2,3)
| (..., ...)    |               | (1,2,3)[ = (1,(2,3))]
| {.=., .=.}    | Dict          | {\`one\`: 1, \`two\`: 2, \`three\`: 3}

*/



/************************************************************************************************************


Python interface

Data types arebb

     Python                Prolog
     string                atoms
     numbers		       numbers
     lists		           lists
     tuples                t(...)
     generic objs	        __pointer__(Address)

     $var refers to the attribute __main__.var

*************************************************************************************************************/


:- use_module(library(lists)).
:- use_module(library(apply_macros)).
:- use_module(library(charsio)).
:- dynamic python_mref_cache/2, python_obj_cache/2.

:-	   op(100,fy,'$'),
	   op(950,fy,:=),
	   op(950,yfx,:=),
%	   op(950,fx,<-),
%	   op(950,yfx,<-),
	   op(50, yf, []),
	   op(50, yf, '()'),
	   op(100, xfy, '.'),
	   op(100, fy, '.').

	   :-  multifile (<-)/1, (<-)/2,
			 '()'/1, '{}'/1,
			 (:=)/1,
			 (:=)/2.

:- initialization( load_foreign_files([],['YAPPython'], init_python_dll), now ).


import( F ) :- catch( python:python_import(F), _, fail ).

user:dot_qualified_goal(Fs) :- catch( python:python_proc(Fs), _, fail ).

F() :-
    python:python_proc(F() ).


{F} :-
    python:python_proc({F} ).

 := (P1 , P2 ) :- !,
    := P1,
    := P2.

:= F :- catch( python:python_proc(F), _, fail ).

V := F :-
    python:python_assign(F, V).

/*
user:(<- F) :-
	catch( python:python_proc(F), _, fail ).

user:(V <- F) :-
	V := F.
*/

python(Exp, Out) :-
	Out := Exp.


start_python :-
	python:python_import('inspect'),
	at_halt(end_python).

add_cwd_to_python :-
    unix(getcwd(Dir)),
    sys.path.append( Dir),
    sys.argv[0] := `yap`.

load_library(Lib,Module) :-
    load_files(library(Lib), [module(Module)]).

load_file(File,Lib) :-
    load_files(File, [module(Lib)]).


load_text(FileAtom,Module) :-
    atom(FileAtom),
    !,
    load_files(Module:atom(FileAtom), []).

load_text(FileString,Module) :-
    string(FileString),
    load_files(Module:string(FileString), []).


%% @}
