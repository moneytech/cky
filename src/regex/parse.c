/***************************************************************************//**

  @file         parse.c

  @author       Stephen Brennan

  @date         Created Saturday, 26 July 2014

  @brief        Regular expression parsing functions.

  @copyright    Copyright (c) 2014, Stephen Brennan.
  All rights reserved.

  @copyright
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of Stephen Brennan nor the names of his contributors may
      be used to endorse or promote products derived from this software without
      specific prior written permission.

  @copyright
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL STEPHEN BRENNAN BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <stdbool.h>    // true, false

#include "regex.h"      // functions we're implementing
#include "fsm.h"        // for fsm operations
#include "libstephen.h" // for linked lists
#include "str.h"        // for get_escape

/**
   @brief Adjust the FSM according to its modifier, if any.

   When a character, character class, or parenthesized regex is read in, it
   could be followed by the modifiers `*`, `+`, or `?`.  This function adjusts
   the FSM for those modifiers, and adjusts the location pointer if one was
   present.

   @param new The newly read in FSM.
   @param regex The pointer to the pointer to the location in the regex.
 */
void regex_parse_check_modifier(fsm *new, const wchar_t **regex)
{
  fsm *f;

  switch ((*regex)[1]) {

  case L'*':
    fsm_kleene(new);
    (*regex)++;
    break;

  case L'+':
    f = fsm_copy(new);
    fsm_kleene(f);
    fsm_concat(new, f);
    fsm_delete(f, true);
    (*regex)++;
    break;

  case L'?':
    // Create the machine that accepts the empty string
    f = fsm_create();
    f->start = fsm_add_state(f, true);
    fsm_union(new, f);
    fsm_delete(f, true);
    (*regex)++;
    break;
  }
}

/**
   @brief Return a FSM that accepts any whitespace.
   @param type Positive or negative.
 */
fsm *regex_parse_create_whitespace_fsm(int type)
{
  fsm *f = fsm_create();
  int src = fsm_add_state(f, false);
  int dest = fsm_add_state(f, true);
  fsm_trans *ft = fsm_trans_create(6, type, dest);
  ft->start[0] = L' ';
  ft->end[0] = L' ';
  ft->start[1] = L'\f';
  ft->end[1] = L'\f';
  ft->start[2] = L'\n';
  ft->end[2] = L'\n';
  ft->start[3] = L'\r';
  ft->end[3] = L'\r';
  ft->start[4] = L'\t';
  ft->end[4] = L'\t';
  ft->start[5] = L'\v';
  ft->end[5] = L'\v';
  fsm_add_trans(f, src, ft);
  f->start = src;
  return f;
}

/**
   @brief Return a FSM for word characters (letters, numbers, and underscore).
 */
fsm *regex_parse_create_word_fsm(int type)
{
  fsm *f = fsm_create();
  int src = fsm_add_state(f, false);
  int dest = fsm_add_state(f, true);
  fsm_trans *ft = fsm_trans_create(4, type, dest);
  ft->start[0] = L'a';
  ft->end[0] = L'z';
  ft->start[1] = L'A';
  ft->end[1] = L'Z';
  ft->start[2] = L'_';
  ft->end[2] = L'_';
  ft->start[3] = L'0';
  ft->end[3] = L'9';
  fsm_add_trans(f, src, ft);
  f->start = src;
  return f;
}

/**
   @brief Return a FSM for digits.
 */
fsm *regex_parse_create_digit_fsm(int type)
{
  fsm *f = fsm_create();
  int src = fsm_add_state(f, false);
  int dest = fsm_add_state(f, true);
  fsm_trans *ft = fsm_trans_create(1, type, dest);
  ft->start[0] = L'0';
  ft->end[0] = L'9';
  fsm_add_trans(f, src, ft);
  f->start = src;
  return f;
}

/**
   @brief Returns an FSM for an escape sequence, outside of a character class.

   Basically, adds the \W, \w, \D, \d, \S, \s character classes to the already
   existing character escape sequences covered by get_escape().

   Expects that `*regex` points to the backslash in the escape sequence.  Always
   returns such that `*regex` points to the LAST character in the escape
   sequence.

   @param regex Pointer to the pointer to the backslash escape.
   @return FSM to accept the backslash escape sequence.
 */
fsm *regex_parse_outer_escape(const wchar_t **regex)
{
  char c;

  (*regex)++; // advance to the specifier
  switch (**regex) {
  case L's':
    return regex_parse_create_whitespace_fsm(FSM_TRANS_POSITIVE);
  case L'S':
    return regex_parse_create_whitespace_fsm(FSM_TRANS_NEGATIVE);
  case L'w':
    return regex_parse_create_word_fsm(FSM_TRANS_POSITIVE);
  case L'W':
    return regex_parse_create_word_fsm(FSM_TRANS_NEGATIVE);
  case L'd':
    return regex_parse_create_digit_fsm(FSM_TRANS_POSITIVE);
  case L'D':
    return regex_parse_create_digit_fsm(FSM_TRANS_NEGATIVE);
  default:
    c = get_escape(regex, L'e');
    // get_escape() leaves the pointer AFTER the last character in the escape.
    // We want it ON the last one.
    (*regex)--;
    return fsm_create_single_char(c);
  }
}

/**
   @brief Create a FSM for a character class.

   Reads a character class (pointed by `*regex`), which it then converts to a
   single transition FSM.

   @param regex Pointer to pointer to location in string!
   @returns FSM for character class.
 */
fsm *regex_parse_char_class(const wchar_t **regex)
{
  #define NORMAL 0
  #define RANGE 1

  smb_ll start, end;
  DATA d;
  int type = FSM_TRANS_POSITIVE, state = NORMAL, index = 0;
  smb_ll_iter iter;
  fsm *f;
  int src, dest;
  fsm_trans *ft;

  ll_init(&start);
  ll_init(&end);

  // Detect whether the character class is positive or negative
  (*regex)++;
  if (**regex == L'^') {
    type = FSM_TRANS_NEGATIVE;
    (*regex)++;
  }

  // Loop through characters in the character class, recording each start-end
  // pair for the transition.
  for ( ; **regex != L']'; (*regex)++) {
    if (**regex == L'-') {
      state = RANGE;
    } else {
      // Get the correct character
      if (**regex == L'\\') {
        printf("Read escape.\n");
        (*regex)++;
        d.data_llint = get_escape(regex, L'e');
        (*regex)--;
      } else {
        d.data_llint = **regex;
      }
      // Put it in the correct place
      if (state == NORMAL) {
        ll_append(&start, d);
        ll_append(&end, d);
      } else {
        // Modify the last transition if this is a range
        ll_set(&end, ll_length(&end)-1, d);
        state = NORMAL;
      }
    }
  }

  if (state == RANGE) {
    // The last hyphen was meant to be literal.  Yay!
    d.data_llint = L'-';
    ll_append(&start, d);
    ll_append(&end, d);
  }

  // Now, create an fsm and fsm_trans, and write the recorded pairs into the
  // allocated start and end buffers.
  f = fsm_create();
  src = fsm_add_state(f, false);
  dest = fsm_add_state(f, true);
  f->start = src;
  ft = fsm_trans_create(ll_length(&start), type, dest);

  for (iter = ll_get_iter(&start); ll_iter_valid(&iter); ll_iter_next(&iter)) {
    d = ll_iter_curr(&iter);
    ft->start[index] = (wchar_t) d.data_llint;
    index++;
  }
  index = 0;
  for (iter = ll_get_iter(&end); ll_iter_valid(&iter); ll_iter_next(&iter)) {
    d = ll_iter_curr(&iter);
    ft->end[index] = (wchar_t) d.data_llint;
    index++;
  }

  fsm_add_trans(f, src, ft);

  ll_destroy(&start);
  ll_destroy(&end);
  return f;
}

/**
   @brief The workhorse recursive helper to regex_parse().

   Crawls through a regex, holding a FSM of the expression so far.  Single
   characters are concatenated as simple 0 -> 1 machines.  Parenthesis initiate
   a recursive call.  Pipes union the current regex with everything after.

   Needs to be called with a non-NULL value for both, which is why there is a
   top level function.

   @param regex Local pointer to the location in the regular expression.
   @param final Pointer to the parent's location.
   @return An FSM for the regular expression.
 */
fsm *regex_parse_recursive(const wchar_t *regex, const wchar_t **final)
{
  // Initial FSM is a machine that accepts the empty string.
  int i = 0;
  fsm *curr = fsm_create();
  fsm *new;
  curr->start = fsm_add_state(curr, true);

  // ASSUME THAT ALL PARENS, ETC ARE BALANCED!

  for ( ; *regex; regex++) {
    switch (*regex) {

    case L'(':
      new = regex_parse_recursive(regex + 1, &regex);
      regex_parse_check_modifier(new, &regex);
      fsm_concat(curr, new);
      fsm_delete(new, true);
      break;

    case L')':
      *final = regex;
      return curr;
      break;

    case L'|':
      // Need to pass control back up without attempting to check modifiers.
      new = regex_parse_recursive(regex + 1, &regex);
      fsm_union(curr, new);
      fsm_delete(new, true);
      *final = regex;
      return curr;
      break;

    case L'[':
      new = regex_parse_char_class(&regex);
      regex_parse_check_modifier(new, &regex);
      fsm_concat(curr, new);
      fsm_delete(new, true);
      break;

    case L'\\':
      new = regex_parse_outer_escape(&regex);
      regex_parse_check_modifier(new, &regex);
      fsm_concat(curr, new);
      fsm_delete(new, true);
      break;

    default:
      // A regular letter
      new = fsm_create_single_char(*regex);
      regex_parse_check_modifier(new, &regex);
      fsm_concat(curr, new);
      fsm_delete(new, true);
      break;
    }
  }
  *final = regex;
  return curr;
}

/**
   @brief Construct a FSM to accept the given regex.
   @param regex Regular expression string.
   @return A FSM that can be used to decide the language of the regex.
 */
fsm *regex_parse(const wchar_t *regex){
  return regex_parse_recursive(regex, &regex);
}