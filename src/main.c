/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Created Tuesday, 13 May 2014

  @brief        CKY parser project main program.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See the LICENSE.txt file for details.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <assert.h>

#include "libstephen/base.h"
#include "libstephen/ad.h"
#include "libstephen/util.h"
#include "libstephen/cb.h"
#include "libstephen/str.h"
#include "libstephen/fsm.h"
#include "libstephen/regex.h"
#include "gram.h"
#include "lex.h"

void simple_gram(void);
void regex(void);
void search(void);
void dot(void);
void lex(char*);

/**
   @brief Print the help message for the main program.
 */
void help(char *name)
{
  printf("Usage: %s [TESTS]\n", name);
  puts("Runs tests on CKY parser and regex engine.");
  puts("");
  puts("Tests:");
  puts("  -g, --simple-gram       create and print a grammar");
  puts("  -e, --regex             input regex and test strings");
  puts("  -s, --search            regex search file");
  puts("  -d, --dot               create graphviz dot from regex");
  puts("  -l, --lex [FILE]        perform lexical analysis");
  puts("");
  puts("Misc:");
  puts("  -h, --help              display this help message and exit");
}

/**
   @brief Main entry point of the program.

   Currently, the program has no set interface or function.  I modify it to test
   out certain features I have just developed.  As time goes on, I'll add some
   sort of real test code.

   @param argc Number of command line arguments
   @param argv Array of command line arguments
   @return The program's exit code.
 */
int main(int argc, char **argv)
{
  smb_ad data;
  bool executed = false;

  arg_data_init(&data);
  process_args(&data, argc - 1, argv + 1);

  if (check_flag(&data, 'h') || check_long_flag(&data, "help")) {
    help(argv[0]);
    arg_data_destroy(&data);
    exit(0);
  }

  if (check_flag(&data, 'g') || check_long_flag(&data, "simple-gram")) {
    simple_gram();
    executed = true;
  }
  if (check_flag(&data, 'e') || check_long_flag(&data, "regex")) {
    regex();
    executed = true;
  }
  if (check_flag(&data, 's') || check_long_flag(&data, "search")) {
    search();
    executed = true;
  }
  if (check_flag(&data, 'd') || check_long_flag(&data, "dot")) {
    dot();
    arg_data_destroy(&data);
    return 0; // exit silently
  }
  if (check_flag(&data, 'l') || check_long_flag(&data, "lex")) {
    char *filename;
    filename = get_flag_parameter(&data, 'l');
    if (filename == NULL)
      filename = get_long_flag_parameter(&data, "lex");
    lex(filename);
    executed = true;
  }

  if (!executed) {
    help(argv[0]);
    arg_data_destroy(&data);
    exit(1);
  }

  arg_data_destroy(&data);
  return 0;
}

/**
   @brief Read a regex from stdin and print to stdout a dot representation.
 */
void dot(void)
{
  smb_status status = SMB_SUCCESS;
  wchar_t *str;
  fsm *compiled_fsm;

  str = smb_read_linew(stdin, &status);
  compiled_fsm = regex_parse(str);
  smb_free(str);
  fsm_dot(compiled_fsm, stdout);
  fsm_delete(compiled_fsm, true);
}

/**
   @brief Interactively search a file for a pattern.
 */
void search(void)
{
  char *filename;
  FILE *file;
  char *regex;
  wchar_t *wregex;
  fsm *regex_fsm;
  char *input;
  wchar_t *winput;
  size_t len;
  smb_al *results;
  regex_hit *hit;
  smb_status status = SMB_SUCCESS;
  int i;

  printf("Input Filename: ");
  filename = smb_read_line(stdin, &status);
  file = fopen(filename, "r");
  smb_free(filename);
  if (file == NULL) {
    perror("Error opening file: ");
    return;
  }

  printf("Input Regex: ");
  regex = smb_read_line(stdin, &status);
  len = mbstowcs(NULL, regex, 0);
  wregex = smb_new(wchar_t, len + 1);
  if (mbstowcs(wregex, regex, len+1) != len) {
    PRINT_ERROR_LOC;
    fprintf(stderr, "Error converting to UCS 4.\n");
    smb_free(regex);
    smb_free(wregex);
    return;
  }
  smb_free(regex);
  regex_fsm = regex_parse(wregex);
  smb_free(wregex);

  input = read_file(file);
  fclose(file);
  len = mbstowcs(NULL, regex, 0);
  winput = smb_new(wchar_t, len + 1);
  if (mbstowcs(winput, input, len+1) != len) {
    PRINT_ERROR_LOC;
    fprintf(stderr, "Error converting to UCS 4.\n");
    fsm_delete(regex_fsm, true);
    smb_free(input);
    smb_free(winput);
    return;
  }
  smb_free(input);

  results = fsm_search(regex_fsm, winput, false, false);
  for (i = 0; i < al_length(results); i++) {
    hit = (regex_hit *)al_get(results, i, &status).data_ptr;
    printf("=> Hit at index %d, length %d\n", hit->start, hit->length);
    printf("   \"%.*ls\"\n", hit->length, winput + hit->start);
    regex_hit_delete(hit);
  }

  smb_free(winput);
  fsm_delete(regex_fsm, true);
  al_delete(results);
}

/**
   @brief A light test of my regular expression parser.

   Prompts for a regex, and then loops prompting for test strings.  Runs the
   regex on each test string and reports to stdout whether they are accepted or
   rejected.
 */
void regex(void)
{
  wchar_t *str;
  smb_status status = SMB_SUCCESS;
  fsm * compiled_fsm;

  printf("Input Regex: ");
  str = smb_read_linew(stdin, &status);
  puts("Parsing...");
  compiled_fsm = regex_parse(str);
  smb_free(str);
  printf("Parsed!  Do you wish to see the FSM? [y/n]: ");

  str = smb_read_linew(stdin, &status);
  if (str[0] == L'y' || str[0] == L'Y') {
    puts("");
    fsm_print(compiled_fsm, stdout);
  } else { puts(""); }
  smb_free(str);

  puts("");

  while (true) {
    printf("Input Test String: ");
    str = smb_read_linew(stdin, &status);
    if (wcscmp(str, L"exit") == 0) {
      smb_free(str);
      break;
    }
    printf(fsm_sim_nondet(compiled_fsm, str) ? "Accepted.\n\n" : "Rejected.\n\n");
    smb_free(str);
  }
  fsm_delete(compiled_fsm, true);
}

/**
   @brief Tests creating a grammar, and printing it out.
 */
void simple_gram(void)
{
  char *start = "start";
  char *plus = "+";
  char *minus = "-";
  char *number = "NUMBER";

  cfg *gram = cfg_create();
  int nStart = cfg_add_symbol(gram, start, false);
  int nPlus = cfg_add_symbol(gram, plus, true);
  int nMinus = cfg_add_symbol(gram, minus, true);
  int nNumber = cfg_add_symbol(gram, number, true);

  cfg_rule *rule_plus = cfg_rule_create(nStart, 3);
  cfg_rule *rule_minus = cfg_rule_create(nStart, 3);
  cfg_rule *rule_number = cfg_rule_create(nStart, 1);

  rule_plus->rhs[0] = nStart;
  rule_plus->rhs[1] = nPlus;
  rule_plus->rhs[2] = nStart;
  rule_minus->rhs[0] = nStart;
  rule_minus->rhs[1] = nMinus;
  rule_minus->rhs[2] = nStart;
  rule_number->rhs[0] = nNumber;

  cfg_add_rule(gram, rule_plus);
  cfg_add_rule(gram, rule_minus);
  cfg_add_rule(gram, rule_number);

  cfg_print(gram);
  cfg_delete(gram, false);
}

/**
  @brief Open a lexer description file, and then lex from stdin.
  @param filename Lexer description file.
 */
void lex(char *filename)
{
  smb_lex *lex = lex_create();
  smb_status status = SMB_SUCCESS;
  wcbuf desc;
  wint_t wc;
  int idx = 0;
  FILE *f = fopen(filename, "r");
  wcb_init(&desc, 2048);

  while ((wc = fgetwc(f)) != WEOF) {
    wcb_append(&desc, wc);
  }
  fclose(f);

  lex_load(lex, desc.buf, &status);
  assert(status == SMB_SUCCESS);

  while (!feof(stdin)) {
    int length;
    DATA token;
    wchar_t *tokstr;
    tokstr = lex_fyylex(lex, stdin, &token, &length, &status);
    printf("%ls: at index=%d, length=%d\n", (wchar_t*)token.data_ptr, idx,
           length);
    if (wcscmp(token.data_ptr, L"whitespace") != 0) {
      printf("  => \"%ls\"\n", tokstr);
    }
    idx += length;
    smb_free(tokstr);
  }
  wcb_destroy(&desc);
  lex_delete(lex);
}
