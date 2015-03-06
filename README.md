# cky

## About

- Author: Stephen Brennan
- License: Revised BSD
- Library Dependencies: [libstephen](https://github.com/brenns10/libstephen)

## Rationale

After learning a lot about regular expressions, grammars, and normal forms in my
EECS 343 (Theoretical Computer Science) class, I decided it would be a fun
adventure to implement regular expressions and the CKY parsing algorithm in C.
And, since nobody likes to specify their grammars in CNF, I would also have to
implement the algorithm to convert a grammar into CNF.

## Theory

Regular expressions are a way of defining a language of strings.  They are only
capable of defining a particular class of languages: the regular languages.  The
regular languages share the characteristic that they can be accepted by a Finite
State Machine.  There are proofs that show that any language accepted by a
Finite State Machine can be defined by a regular expression.  More importantly,
there are algorithms that take a regular expression and give a Finite State
Machine that accepts the language.  This is one of the algorithms implemented in
this project.

Theoretical regular expressions are less feature rich than computer regular
expressions.  My implementation will favor a balance between the two:

- The syntax will follow computer regular expressions (not the formal syntax).

- Not all common features of regular expressions will be allowed.  I currently
  plan to allow '.' as a universal character, '?' as an optional character, some
  character class shortcuts, and possibly explicit number selection via brackets
  '{10}'.

The CKY parsing algorithm is a `O(n^3)` algorithm that parses strings to answer
whether they can be generated by a context-free grammar, given in Chomsky Normal
Form.  Chomsky Normal Form is a form of a grammar with the following
restrictions: rules may be in the form `A --> b`, where `b` is a terminal, or `A
--> B C`, where `B` and `C` are nonterminals.  Any context-free grammar can be
represented in CNF.

There exist other parsing algorithms that run asymptotically faster.  However,
they cannot parse an arbitrary grammar.  Additionally, I'm mainly looking to
implement an algorithm I learned, not reinvent the wheel.

## Goals

At a high level, I want a program that can read a grammar and parse a file using
it.  This is a pretty big deal, and I haven't figured out all the architecture
behind that.

For now, I'm concerned with the regular expression side of things.

## Current State

- I've created some grammar data structures.  These data structures aren't
  really used for anything yet.

- I've created FSM data structures, and functions for deterministic and
  nondeterministic simulation.

- I've created functions that will export FSMs to text and graphviz formats, as
  well as import from the same text format.

- I've created functions that will operate on FSMs in order to create larger
  machines.

- I've created a function that will read a regular expression and output a FSM.

- I've created a function that will take a file and a regular expression, and it
  will find all matches.

## Future

I plan on the following:

- Cleaning up the code in fsm.c and regex.c.  It's a bit messy right now.  The
  code 'just works'.

- Unit testing.  I have a unit testing framework, but coming up with tests was
  left behind in favor of practical demonstrations of functionality.  Now, I'm
  assured of the functionality, and I desperately need tests.

- Adding convenience functions that work for UTF-8 and UCS-4.  Most of my
  parsing is done in UCS-4 for it's nifty indexing features.  So, when functions
  call to have text searched, they first need to convert to UCS-4.  I can handle
  that for them!

- Multiple compile targets.  I plan on using these so I can make a grep-like
  tool for searching files, other binaries for other ideas, and a test binary.

- Further in the future, I plan on actually implementing a parser.  It will use
  my regex engine as the lexical scanner.
