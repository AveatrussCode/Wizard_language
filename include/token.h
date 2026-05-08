#ifndef TOKEN_H
#define TOKEN_H
#include <string.h>

typedef enum {
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, CLASS, ELSE, FALSE, FOR, IF, OR,
    PRINT, RETURN, THIS,
    TRUE, VAR, WHILE, OTHER, EOF
} TokenType;

struct token {
    TokenType type;
    string lexema;
    liral literal;
    int line;

    token(TokenType _type, string _lexema, liral _literal, int _line){
        type = _type;
        lexema = _lexema;
        literal = _literal;
        line = _line;
    }
};

#endif