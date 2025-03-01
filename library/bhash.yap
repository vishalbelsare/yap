%% -*- Prolog -*-

/**
 * @file   bhash.yap
 * @author VITOR SANTOS COSTA <vsc@VITORs-MBP.lan>
 * @date   Tue Nov 17 01:11:29 2015
 * 
 * @brief  Backtrackable Hash Tables
 * 
 * 
*/

:- source.
:- yap_flag(unknown,error).
:- style_check(all).

:- module(b_hash, [   b_hash_new/1,
		      b_hash_new/2,
		      b_hash_new/4,
		      b_hash_lookup/3,
		      b_hash_update/3,
		      b_hash_update/4,
		      b_hash_insert_new/4,
		      b_hash_insert/4,
		      b_hash_size/2,
		      b_hash_code/2,
		      is_b_hash/1,
        	      b_hash_to_list/2,
		      b_hash_values_to_list/2,
		      b_hash_keys_to_list/2
		  ]).

/**
 * @defgroup bhash Backtrackable Hash Tables
 * @ingroup YAPLibrary
 *
 * @{

This library implements hash-tables, that associate keys with values.
It requires the hash key to be any ground term, but the value term can
take any value.

. The library can be loaded as

:- use_module( library( bhash ) ).

The library's code uses backtrackable updates and an array to store
the terms. Implicit keys are generated by term_hash/4 (note that we cannot guarantee there will be no collisions).

*/

:- use_module(library(terms), [ term_hash/4 ]).


:- meta_predicate(b_hash_new(-,+,3,2)).

/**
 * Initial hash table size: should be a prime number>?
 */
array_default_size(2048).

/** @pred is_b_hash( +Hash )

True if term _Hash_ is a hash table.
*/
is_b_hash(V) :- var(V), !, fail.
is_b_hash(hash(_,_,_,_,_)).

/** @pred b_hash_new( -NewHash )

Create a empty hash table _NewHash_, with size obtained from array__default_size/1, by default 2048 entries.
*/
b_hash_new(hash(Keys, Vals, Size, N, _, _)) :-
    array_default_size(Size),
    array(Keys, Size),
    array(Vals, Size),
    create_mutable(0, N).

/** @pred b_hash_new( -_NewHash_, +_Size_ )

Create a empty hash table, with size _Size_ entries.
*/
b_hash_new(hash(Keys, Vals, Size, N, _, _), Size) :-
    array(Keys, Size),
    array(Vals, Size),
    create_mutable(0, N).

/** @pred b_hash_new( -_NewHash_, +_Size_, :_Hash_, :_Cmp_ )

Create a empty hash table, with size _Size_ entries.
_Hash_ defines a partition function, and _Cmp_ defined a comparison function.
*/
b_hash_new(hash(Keys,Vals, Size, N, HashF, CmpF), Size, HashF, CmpF) :-
    array(Keys, Size),
    array(Vals, Size),
    create_mutable(0, N).

/**
  @pred  b_hash_size( +_Hash_, -_Size_ )

_Size_ unifies with the size of the hash table _Hash_.
*/
b_hash_size(hash(_, _, Size, _, _, _), Size).

/**
  @pred b_hash_lookup( +_Key_, ?_Val_, +_Hash_ )

Search the ground term _Key_ in table _Hash_ and unify _Val_ with the associated entry.
*/
b_hash_lookup(Key, Val, hash(Keys, Vals, Size, _, F, CmpF)):-
    hash_f(Key, Size, Index, F),
    fetch_key(Keys, Index, Size, Key, CmpF, ActualIndex),
    array_element(Vals, ActualIndex, Mutable),
    get_mutable(Val, Mutable).

fetch_key(Keys, Index, Size, Key, CmpF, ActualIndex) :-
    array_element(Keys, Index, El),
    nonvar(El),
    (
	cmp_f(CmpF, El, Key)
    ->
    Index = ActualIndex
    ;
    I1 is (Index+1) mod Size,
    fetch_key(Keys, I1, Size, Key, CmpF, ActualIndex)
    ).

/**
  @pred b_hash_update( +_Key_, +_Hash_, +NewVal )

Update to the value associated with the ground term _Key_ in table _Hash_ to _NewVal_.
*/
b_hash_update(Hash, Key, NewVal):-
    Hash = hash(Keys, Vals, Size, _, F, CmpF),
    hash_f(Key,Size,Index,F),
    fetch_key(Keys, Index, Size, Key, CmpF, ActualIndex),
    array_element(Vals, ActualIndex, Mutable),
    update_mutable(NewVal, Mutable).

/**
  @pred b_hash_update( +_Key_, -_OldVal_, +_Hash_, +NewVal )

Update to the value associated with the ground term _Key_ in table _Hash_ to _NewVal_, and unify _OldVal_ with the current value.
*/
b_hash_update(Hash, Key, OldVal, NewVal):-
    Hash = hash(Keys, Vals, Size, _, F, CmpF),
    hash_f(Key,Size,Index,F),
    fetch_key(Keys, Index, Size, Key, CmpF, ActualIndex),
    array_element(Vals, ActualIndex, Mutable),
    get_mutable(OldVal, Mutable),
    update_mutable(NewVal, Mutable).

/** b_hash_insert(+_Hash_, +_Key_, _Val_, +_NewHash_ )

Insert the term _Key_-_Val_ in table _Hash_ and unify _NewHash_ with the result. If ground term _Key_ exists, update the dictionary.
*/
b_hash_insert(Hash, Key, NewVal, NewHash):-
    Hash = hash(Keys, Vals, Size, N, F, CmpF),
    hash_f(Key,Size,Index,F),
    find_or_insert(Keys, Index, Size, N, CmpF, Vals, Key, NewVal, Hash, NewHash).

find_or_insert(Keys, Index, Size, N, CmpF, Vals, Key, NewVal, Hash, NewHash) :-
    array_element(Keys, Index, El),
    (
	var(El)
    ->
    add_element(Keys, Index, Size, N, Vals, Key, NewVal, Hash, NewHash)
    ;
    cmp_f(CmpF, El, Key)
    ->
    % do rb_update
    array_element(Vals, Index, Mutable),
    update_mutable(NewVal, Mutable),
    Hash = NewHash
    ;
    I1 is (Index+1) mod Size,
    find_or_insert(Keys, I1, Size, N, CmpF, Vals, Key, NewVal, Hash, NewHash)
    ).

/**
  @pred b_hash_insert_new(+_Hash_, +_Key_, _Val_, +_NewHash_ )

Insert the term _Key_-_Val_ in table _Hash_ and unify _NewHash_ with the result. If ground term _Key_ exists, fail.
*/
b_hash_insert_new(Hash, Key, NewVal, NewHash):-
    Hash = hash(Keys, Vals, Size, N, F, CmpF),
    hash_f(Key,Size,Index,F),
    find_or_insert_new(Keys, Index, Size, N, CmpF, Vals, Key, NewVal, Hash, NewHash).

find_or_insert_new(Keys, Index, Size, N, CmpF, Vals, Key, NewVal, Hash, NewHash) :-
    array_element(Keys, Index, El),
    (
	var(El)
    ->
    add_element(Keys, Index, Size, N, Vals, Key, NewVal, Hash, NewHash)
    ;
    cmp_f(CmpF, El, Key)
    ->
    fail
    ;
    I1 is (Index+1) mod Size,
    find_or_insert_new(Keys, I1, Size, N, CmpF, Vals, Key, NewVal, Hash, NewHash)
    ).

add_element(Keys, Index, Size, N, Vals, Key, NewVal, Hash, NewHash) :-
    get_mutable(NEls, N),
    NN is NEls+1,
    update_mutable(NN, N),
    array_element(Keys, Index, Key),
    update_mutable(NN, N),
    array_element(Vals, Index, Mutable),
    create_mutable(NewVal, Mutable),
    (
	NN > Size/3
    ->
    expand_array(Hash, NewHash)
    ;
    Hash = NewHash
    ).

expand_array(Hash, NewHash) :-
    Hash == NewHash, !,
    Hash = hash(Keys, Vals, Size, _X, F, _CmpF),
    new_size(Size, NewSize),
    array(NewKeys, NewSize),
    array(NewVals, NewSize),
    copy_hash_table(Size, Keys, Vals, F, NewSize, NewKeys, NewVals),
    /* overwrite in place */
    setarg(1, Hash, NewKeys),
    setarg(2, Hash, NewVals),
    setarg(3, Hash, NewSize).

expand_array(Hash, hash(NewKeys, NewVals, NewSize, X, F, CmpF)) :-
    Hash = hash(Keys, Vals, Size, X, F, CmpF),
    new_size(Size, NewSize),
    array(NewKeys, NewSize),
    array(NewVals, NewSize),
    copy_hash_table(Size, Keys, Vals, F, NewSize, NewKeys, NewVals).

new_size(Size, NewSize) :-
    Size > 1048576, !,
    NewSize is Size+1048576.
new_size(Size, NewSize) :-
    NewSize is Size*2.

copy_hash_table(0, _, _, _, _, _, _) :- !.
copy_hash_table(I1, Keys, Vals, F, Size, NewKeys, NewVals) :-
    I is I1-1,
    array_element(Keys, I, Key),
    nonvar(Key), !,
    array_element(Vals, I, Val),
    insert_el(Key, Val, Size, F, NewKeys, NewVals),
    copy_hash_table(I, Keys, Vals, F, Size, NewKeys, NewVals).
copy_hash_table(I1, Keys, Vals, F, Size, NewKeys, NewVals) :-
    I is I1-1,
    copy_hash_table(I, Keys, Vals, F, Size, NewKeys, NewVals).

insert_el(Key, Val, Size, F, NewKeys, NewVals) :-
    hash_f(Key,Size,Index, F),
    find_free(Index, Size, NewKeys, TrueIndex),
    array_element(NewKeys, TrueIndex, Key),
    array_element(NewVals, TrueIndex, Val).

find_free(Index, Size, Keys, NewIndex) :-
    array_element(Keys, Index, El),
    (
	var(El)
    ->
    NewIndex = Index
    ;
    I1 is (Index+1) mod Size,
    find_free(I1, Size, Keys, NewIndex)
    ).

hash_f(Key, Size, Index, F) :-
    var(F), !,
    term_hash(Key,-1,Size,Index).
hash_f(Key, Size, Index, F) :-
    call(F, Key, Size, Index).

cmp_f(F, A, B) :-
    var(F), !,
    A == B.
cmp_f(F, A, B) :-
    call(F, A, B).

/**
  @pred b_hash_to_list(+_Hash_, -_KeyValList_ )

The term _KeyValList_ unifies with a list containing all terms _Key_-_Val_ in the hash table.
*/
b_hash_to_list(hash(Keys, Vals, _, _, _, _), LKeyVals) :-
    Keys =.. [_|LKs],
    Vals =.. [_|LVs],
    mklistpairs(LKs, LVs, LKeyVals).

/**
  @pred b_key_to_list(+_Hash_, -_KeyList_ )

The term _KeyList_ unifies with a list containing all keys in the hash table.
*/
b_hash_keys_to_list(hash(Keys, _, _, _, _, _), LKeys) :-
    Keys =.. [_|LKs],
    mklistels(LKs, LKeys).

/**
  @pred b_key_to_list(+_Hash_, -_ValList_ )

The term _`valList_ unifies with a list containing all values in the hash table.
*/
b_hash_values_to_list(hash(_, Vals, _, _, _, _), LVals) :-
    Vals =.. [_|LVs],
    mklistvals(LVs, LVals).

mklistpairs([], [], []).
mklistpairs([V|LKs], [_|LVs], KeyVals) :-
		var(V),
		!,
		mklistpairs(LKs, LVs, KeyVals).
mklistpairs([K|LKs], [V|LVs], [(K-VV)|KeyVals]) :- 
    get_mutable(VV, V),
    mklistpairs(LKs, LVs, KeyVals).

mklistels([],  []).
mklistels([V|Els], NEls) :- var(V), !,
			    mklistels(Els, NEls).
mklistels([K|Els], K.NEls) :- 
    mklistels(Els, NEls).

mklistvals([],  []).
mklistvals([V|Vals], NVals) :- var(V), !,
			       mklistvals(Vals, NVals).
mklistvals([K|Vals], [KK|NVals]) :- 
    get_mutable(KK, K),
    mklistvals(Vals, NVals).

/**
@}
*/
