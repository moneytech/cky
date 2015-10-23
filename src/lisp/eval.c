/***************************************************************************//**

  @file         eval.c

  @author       Stephen Brennan

  @date         Created Wednesday, 21 October 2015

  @brief        Lisp evaluation routines. maybe.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <assert.h>

#include "libstephen/str.h"
#include "lex.h"
#include "lisp.h"

// forward-declaration
lisp_value *lisp_evaluate(lisp_value *expression, lisp_scope *scope);

/**
   @brief Return a list containing each item in a list, evaluated.
   @param list List of items to evaluate.
   @param scope Scope to evaluate each list item within.
   @returns A list of the evaluated items!
 */
static lisp_list *lisp_evaluate_list(lisp_list *list, lisp_scope *scope)
{
  if (list == NULL) return NULL;
  lisp_list *l = (lisp_list*) tp_list.tp_alloc();
  l->value = lisp_evaluate(list->value, scope);
  l->next = lisp_evaluate_list(list->next, scope);
  return l;
}

/**
   @brief Return the value of a piece of lisp code!
   @param expression The code to evaluate.
   @param scope The scope to evaluate the code within.
 */
lisp_value *lisp_evaluate(lisp_value *expression, lisp_scope *scope)
{
  smb_status st = SMB_SUCCESS;
  lisp_funccall *call;
  lisp_value *rv, *func;
  lisp_list *args;
  lisp_identifier *id;

  if (expression->type == &tp_int ||
      expression->type == &tp_atom ||
      expression->type == &tp_list ||
      expression->type == &tp_builtin) {
    lisp_incref(expression);
    rv = expression;
  } else if (expression->type == &tp_funccall) {
    call = (lisp_funccall *)expression;
    func = lisp_evaluate((lisp_value*)call->function, scope);
    args = lisp_evaluate_list(call->arguments, scope);
    if (func->type == &tp_builtin) {
      lisp_builtin *builtin = (lisp_builtin*) func;
      rv = builtin->function(args);
      lisp_decref((lisp_value*)args);
      lisp_decref(func);
    } else {
      printf("error in evaluation\n");
    }
  } else {
    id = (lisp_identifier*)expression;
    rv = ht_get(&scope->table, PTR(id->value), &st).data_ptr;
    assert(st == SMB_SUCCESS);
    lisp_incref(rv); // we are returning a new reference not owned by scope
    return rv;
  }
  return rv;
}

lisp_value *lisp_run(wchar_t *str)
{
  smb_status st = SMB_SUCCESS;
  // given string, return list of tokens (with strings if necessary)
  smb_ll *tokens = lisp_lex(str);
  // then, parse it to a (list of) lisp_value
  smb_iter it = ll_get_iter(tokens);
  lisp_value *code = lisp_parse(&it);
  // then, evaluate that
  lisp_scope *scope = lisp_create_globals();
  lisp_value *res = lisp_evaluate(code, scope);
  res->type->tp_print(res, stdout, 0);
  lisp_decref(res);
  lisp_decref(code);
  ht_destroy(&scope->table);
  smb_free(scope);
  it = ll_get_iter(tokens);
  while (it.has_next(&it)) {
    smb_free(it.next(&it, &st).data_ptr);
    assert(st == SMB_SUCCESS);
  }
  ll_delete(tokens);
  return res;
}

/**
   @brief This is the demo for main.c.
 */
void lisp(void)
{
  wchar_t *file = read_filew(stdin);
  lisp_run(file);
  smb_free(file);
}