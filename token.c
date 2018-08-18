#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "knicc.h"

char *find_token_name(TokenType t) {
    char *s;
    switch (t) {
        case INT:
            s = "INT";
            break;
        case IDENT:
            s = "IDENT";
            break;
        case SEMICOLON:
            s = "SEMICOLON";
            break;
        case ADD:
            s = "ADD";
            break;
        case SUB:
            s = "SUB";
            break;
        case MULTI:
            s = "MULTI";
            break;
        case ASSIGN:
            s = "ASSIGN";
            break;
        case Eq:
            s = "Eq";
            break;
        case Less:
            s = "Less";
            break;
        case _EOF:
            s = "EOF";
            break;
        case LParen:
            s = "LParen";
            break;
        case RParen:
            s = "RParen";
            break;
        case LBrace:
            s = "LBrace";
            break;
        case RBrace:
            s = "RBrace";
            break;
        case COMMA:
            s = ",";
            break;
        case If:
            s = "If";
            break;
        case Else:
            s = "Else";
            break;
        case While:
            s = "While";
            break;
        case Return:
            s = "Return";
            break;
        case COMPOUND_STMT:
            s = "COMPOUND_STMT";
            break;
        case For:
            s = "for";
            break;
        case DEC_INT:
            s = "DEC_INT";
            break;
        case Deref:
            s = "Deref";
            break;
        case Ref:
            s = "Ref";
            break;
        case FUNC_CALL:
            s = "FUNC_CALL";
            break;
        case FUNC_DECL:
            s = "FUNC_DECL";
            break;
        default:
            s = "UNEXPECTED TOKEN";
            printf("type: %d\n", t);
            break;
    }
    return s;
}

TokenType spacial_char(char c) {
    switch (c) {
        case '&': return Ref;
        case ';': return SEMICOLON;
        case '+': return ADD;
        case '-': return SUB;
        case '*': return MULTI;
        case '=': return ASSIGN;
        case '(': return LParen;
        case ')': return RParen;
        case '{': return LBrace;
        case '}': return RBrace;
        case ',': return COMMA;
        case '<': return Less;
        case '>': return More;
        case '[': return LBracket;
        case ']': return RBracket;
        default: return NOT_FOUND;
    }
}

TokenType keyword(char *s) {
    if (strcmp("if", s) == 0) return If;
    if (strcmp("else", s) == 0) return Else;
    if (strcmp("while", s) == 0) return While;
    if (strcmp("for", s) == 0) return For;
    if (strcmp("int", s) == 0) return DEC_INT;
    if (strcmp("return", s) == 0) return Return;
    return IDENT;
}

Token new_token(char *literal, TokenType kind) {
    Token t;
    strcpy(t.literal, literal);
    t.type = kind;
    return t;
}

void debug_token(Token t) {
    printf("Token.type: %s, literal: %s\n", find_token_name(t.type), t.literal);
}

bool is_binop(TokenType type) {
    switch (type) {
        case ADD:
        case SUB:
        case MULTI:
        case ASSIGN:
        case Less:
        case More:
            return true;
        default:
            return false;
    }
}

bool is_unaryop(TokenType type) {
    switch (type) {
        case Ref:
        case MULTI:
        case Deref:
            return true;
        default:
            return false;
    }
}
