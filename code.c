#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "knicc.h"

Map *local_var_map;
Map *global_map;
Vector *string_literal_vec;
Map *def_struct_map;

const char *regs64[6] = {
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8",
    "r9"
};

const char *regs32[6] = {
    "edi",
    "esi",
    "edx",
    "ecx",
    "e8",
    "e9"
};

void emit_stmt(Node *n);
void emit_expr(Node *n);

void emit_if_stmt(Node *n) {;
    emit_expr(n->if_stmt.expr);
    printf("  popq %%rax\n");
    printf("  cmpq $0, %%rax\n");
    printf("  je .L%d\n", n->if_stmt.label_no);
    emit_stmt(n->if_stmt.true_stmt);
    printf("  .L%d:\n", n->if_stmt.label_no);
}
void emit_if_else_stmt(Node *n) {
    emit_expr(n->if_stmt.expr);
    printf("  popq %%rax\n");
    printf("  cmpq $0, %%rax\n");
    printf("  je .L%d\n", n->if_stmt.label_no);
    emit_stmt(n->if_stmt.true_stmt);
    printf("  jmp .L%d\n", n->if_stmt.label_no+1);
    printf("  .L%d:\n", n->if_stmt.label_no);
    emit_stmt(n->if_stmt.else_stmt);
    printf("  .L%d:\n", n->if_stmt.label_no+1);
}

void emit_return_stmt(Node *n) {
    emit_expr(n->ret_stmt.expr);
    printf("\n  popq %%rax\n");
    printf("  movq %%rbp, %%rsp\n");
    printf("  popq %%rbp\n");
    printf("  ret\n");
}

void emit_while_stmt(Node *n) {
    printf(".L%d:\n", n->while_stmt.label_no);
    emit_expr(n->while_stmt.expr);
    printf("  popq %%rax\n");
    printf("  cmpq $0, %%rax\n");
    printf("  je .L%d\n", n->while_stmt.label_no + 1);
    emit_stmt(n->while_stmt.stmt); 
    printf("  jmp .L%d\n", n->while_stmt.label_no);
    printf("  .L%d:\n", n->while_stmt.label_no + 1);
}

void emit_for_stmt(Node *n) {
    emit_expr(n->for_stmt.init_expr);
    printf(".L%d:\n", n->for_stmt.label_no);
    emit_expr(n->for_stmt.cond_expr);
    printf("  popq %%rax\n");
    printf("  cmpq $0, %%rax\n");
    printf("  je .L%d\n", n->for_stmt.label_no + 1);
    emit_stmt(n->for_stmt.stmt);
    printf("  .L%d:\n", n->for_stmt.label_no + 2);
    emit_expr(n->for_stmt.loop_expr);
    printf("  jmp .L%d\n", n->for_stmt.label_no);
    printf(".L%d:\n", n->for_stmt.label_no + 1);
}

void emit_compound_stmt(Node *n) {
    Vector *stmts = n->stmts;
    for (int i = 0; i < stmts->length; i++) {
        Node *ast = vec_get(stmts, i);
        printf("\n// %d line\n", i+1);
        emit_stmt(ast);
    }
}

void emit_break(Node *n) {
    printf("  jmp .L%d\n", n->break_no);
}

void emit_continue(Node *n) {
    printf("  jmp .L%d\n", n->continue_label_no);
}

void emit_stmt(Node *n) {
    if (n->type == COMPOUND_STMT) emit_compound_stmt(n);
    else if (n->type == WHILE) emit_while_stmt(n);
    else if (n->type == FOR) emit_for_stmt(n);
    else if (n->type == IF_STMT) emit_if_stmt(n);
    else if (n->type == IF_ELSE_STMT) emit_if_else_stmt(n);
    else if (n->type == RETURN) emit_return_stmt(n);
    else if (n->type == BREAK) emit_break(n);
    else if (n->type == CONTINUE) emit_continue(n);
    else emit_expr(n);
}

void emit_global_var(void) {
    for (int i = 0; i < global_map->vec->length; i++) {
        char *key = ((KeyValue *)(vec_get(global_map->vec, i)))->key;
        printf("%s:\n", key);
        printf("  .zero 4\n");
    }
}

void emit_string_literal() {
    for (int i = 0; i < string_literal_vec->length; i++) {
        printf(".LC%d:\n", i);
        printf("  .asciz \"%s\"\n", (char *)vec_get(string_literal_vec, i));
        printf("\n");
    }
}

void emit_toplevel(Vector *n) {
    emit_string_literal();
    printf(".data\n");
    emit_global_var();
    printf(".text\n");
    printf(".global main\n");
    for (int i = 0; i < n->length; i++) {
        Node *ast = vec_get(n, i);
        if (ast->type == GLOBAL_DECL || ast->type == ENUM_DECL || ast->type == STRUCT_DECL) continue;
        else emit_func_def(ast);
    }
}

void emit_func_def(Node *n) {
    printf("%s:\n", n->func_def.name);
    printf("  pushq %%rbp\n");
    printf("  movq %%rsp, %%rbp\n");
    local_var_map = n->func_def.map;
    printf("  subq $%d, %%rsp\n", n->func_def.offset);
    for (int i = 0; i < n->func_def.parameters->length; i++) { // 関数の引数処理
        Node *arg = (Node *)vec_get(n->func_def.parameters, i);
        Var *v = (Var *)((KeyValue *)find_by_key(n->func_def.map, arg->var_decl.name)->value);
        if (v->is_pointer) printf("  movq %%%s, -%d(%%rbp)\n", regs64[i], v->offset);
        else if (v->type == TYPE_CHAR) {
            printf("  movq %%%s, %%rax\n", regs64[i]);
            printf("  movb %%al, -%d(%%rbp)\n", v->offset);            
        } else printf("  movl %%%s, -%d(%%rbp)\n", regs32[i], v->offset);
    }
    for (int i = 0; i < n->stmts->length; i++) {
        Node *ast = vec_get(n->stmts, i);
        emit_stmt(ast);
    }
    emit_func_ret(n);
}

void emit_func_ret(Node *n) {
    printf("\n  popq %%rax\n");
    printf("  movq %%rbp, %%rsp\n");
    printf("  popq %%rbp\n");
    printf("  ret\n");
}

void gen_operands(void) {
    printf("  popq %%rax\n");
    printf("  popq %%rdx\n");
}

void emit_add(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    if (n->left->type == IDENTIFIER) {
        KeyValue *kv = find_by_key(local_var_map, n->left->literal);
        if (kv != NULL) {
            Var *v = ((Var *)(kv->value));
            printf("  pushq $%d\n", add_sub_ptr(v));
            gen_operands();
            printf("  imul %%rdx, %%rax\n");
            printf("  pushq %%rax\n");
        }
    }
    gen_operands();
    printf("  addq %%rdx, %%rax\n");
    printf("  pushq %%rax\n");
}

void emit_sub(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    gen_operands();
    printf("  subq %%rax, %%rdx\n");
    printf("  pushq %%rdx\n");
}

void emit_multi(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    gen_operands();
    printf("  imul %%rdx, %%rax\n");
    printf("  pushq %%rax\n");
}

void emit_div(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    printf("  popq %%rcx\n");
    printf("  popq %%rax\n");
    printf("  cltd\n");
    printf("  idivq %%rcx\n");
    printf("  pushq %%rax\n");
}

void emit_mod(Node *n) {
    emit_div(n);
    printf("  popq %%rax\n");
    printf("  pushq %%rdx\n");
}

void emit_assign(Node *n) {
    Var *v;
    emit_lvalue_code(n->left);
    emit_expr(n->right);
    // 代入を実行
    printf("  popq %%rbx\n");
    printf("  popq %%rax\n");
    v = get_first_var(local_var_map, n);
    if (v != NULL){
        if (v->is_pointer)  printf("  movq %%rbx, (%%rax)\n");
        else if (v->type == TYPE_INT) printf("  movl %%ebx, (%%rax)\n");
        else if (v->type == TYPE_CHAR) printf("  movb %%bl, (%%rax)\n");
        else printf("  movl %%ebx, (%%rax)\n");
    } else printf("  movq %%rbx, (%%rax)\n");
    printf("  pushq %%rbx\n");
}

void emit_func_call(Node *n) {
    for (int i = 0; i < n->func_call.argc; i++) {
        emit_expr(n->func_call.argv[i]);
        printf("  popq %%rax\n");
        printf("  movq  %%rax,  %%%s\n", regs64[i]);
    }
    printf("  call %s\n", n->func_call.name);
    printf("  pushq %%rax\n");
}

void emit_ident(Node *n) {
    KeyValue *kv = ((KeyValue *)(find_by_key(local_var_map, n->literal)));
    if (kv != NULL) {
        Var *v = kv->value;
        if (v->array_size > 0 || v->type == TYPE_STRUCT) printf("  leaq -%d(%%rbp), %%rax\n", v->offset);
        else if (v->is_pointer) printf("  movq -%d(%%rbp), %%rax\n", v->offset);
        else if (v->type == TYPE_INT) printf("  movl -%d(%%rbp), %%eax\n", v->offset);
        else if (v->type == TYPE_CHAR) printf("  movzbl -%d(%%rbp), %%eax\n", v->offset);
    } else printf("  movq %s(%%rip), %%rax\n", n->literal);
    printf("  pushq %%rax\n");
}

void emit_or(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    printf("  popq %%rdx\n");
    printf("  popq %%rax\n");
    printf("  orl %%edx, %%eax\n");
    printf("  pushq %%rax\n"); 
}

void emit_and(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    printf("  popq %%rdx\n");
    printf("  popq %%rax\n");
    printf("  andl %%edx, %%eax\n");
    printf("  pushq %%rax\n"); 
}

void emit_boolean_op(Node *n) {
    emit_expr(n->left);
    emit_expr(n->right);
    gen_operands();
    char *m;
    if (n->type == EQ) m = "sete";
    if (n->type == NOTEQ) m = "setne";
    if (n->type == LESS) m = "setl";
    if (n->type == LESSEQ) m = "setle";
    if (n->type == MORE) m = "setg";
    if (n->type == MOREEQ) m = "setge";
    printf("  cmpq %%rax, %%rdx\n");
    printf("  %s %%al\n", m);
    printf("  movzbl %%al, %%eax\n");
    printf("  pushq %%rax\n");
}

void emit_ref(Node *n) {
    Var *v = get_first_var(local_var_map, n);
    printf("  leaq -%d(%%rbp),  %%rax\n", v->offset);
    printf("  pushq %%rax\n");
}

void emit_deref(Node *n) {
    emit_expr(n->left); // スタックのトップに p+12 とかのアドレスが乗ってる
    // emit_expr(n->right); segfault
    // printf("  movq %d(%%rbp), %%rax\n", var->offset);
    printf("  popq %%rax\n");
    printf("  pushq (%%rax)\n");
}

void emit_string(Node *n) {
    for (int i = 0; i < string_literal_vec->length; i++) {
        if (strcmp(vec_get(string_literal_vec, i), n->literal) == 0) {
            printf("  leaq .LC%d(%%rip), %%rax\n", i);
            break;
        }
    }
    printf("  pushq %%rax\n");
}

void emit_dot(Node *n) {
    emit_expr(n->left);
    printf("popq %%rbx\n");
    Var *v = get_first_var(local_var_map, n->left);
    printf("  addq  $%d, %%rax\n", get_offset_member(v, n->right));
    printf("  movl (%%rax), %%eax\n");
    printf("  pushq %%rax\n");
}

void emit_expr(Node *n) {
    if (n == NULL) return;
    if (n->type == INT) printf("  pushq $%d\n", n->ival);
    if (n->type == VAR_DECL) return;
    if (n->type == IDENTIFIER) emit_ident(n);
    if (n->type == ADD) emit_add(n);
    if (n->type == SUB) emit_sub(n);
    if (n->type == MULTI) emit_multi(n);
    if (n->type == DIV) emit_div(n);
    if (n->type == MOD) emit_mod(n);
    if (n->type == ASSIGN) emit_assign(n);
    if (n->type == OR) emit_or(n);
    if (n->type == AND) emit_and(n);
    if (EQ <= n->type && n->type <= MOREEQ) emit_boolean_op(n);
    if (n->type == FUNC_CALL) emit_func_call(n);
    if (n->type == REF) emit_ref(n);
    if (n->type == DEREF) emit_deref(n);
    if (n->type == STRING) emit_string(n);
    if (n->type == DOT) emit_dot(n);
}

void emit_lvalue_code(Node *n) {
    if (n->type == DEREF) {
        emit_expr(n->left);
    } else if (n->type == IDENTIFIER) {
        KeyValue *kv = ((KeyValue *)(find_by_key(local_var_map, n->literal)));
        if (kv != NULL){
            Var *v = ((Var *)(kv->value));
            printf("  leaq -%d(%%rbp), %%rax\n", v->offset);
        }
        else printf("  leaq %s(%%rip), %%rax\n", n->literal);
    } else if (n->type == DOT) {
        emit_ref(n->left);
        Var *v = get_first_var(local_var_map, n->left);
        printf("  addq  $%d, %%rax\n", get_offset_member(v, n->right));
    }
    printf("  pushq %%rax\n");
}
