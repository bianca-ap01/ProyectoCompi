#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>

using namespace std;

class Token {
public:
    // Tipos de token
    enum Type {
        PLUS,    // +
        MINUS,   // -
        MUL,     // *
        DIV,     // /
        POW,     // **
        LPAREN,  // (
        RPAREN,  // )
        SQRT,    // sqrt
        NUM,     // Número
        ERR,     // Error
        ID,      // ID
        LE,      // <
        RETURN,  // return
        SEMICOL, // ;
        ASSIGN,  // =
        PRINT,   // printf
        IF, // if
        WHILE, // while
        DO, // do
        ELSE, // else
        FALSE, // false
        COMA, // ,
        TRUE, // true
        UNSIGNED, // unsigned
        INT, // int
        FLOAT, // float
        LONG, // long
        LBRACE, //{
        RBRACE, //}
        FOR,  // for
        MOD,  // %
        BACKSLASH,
        AUTO, // para inferencia de tipos -> auto
        QMARK, // para ternaria -> ?
        COL, // para ternaria -> :
    };

    // Atributos
    Type type;
    string text;

    // Constructores
    Token(Type type);
    Token(Type type, char c);
    Token(Type type, const string& source, int first, int last);

    // Sobrecarga de operadores de salida
    friend ostream& operator<<(ostream& outs, const Token& tok);
    friend ostream& operator<<(ostream& outs, const Token* tok);
};

#endif // TOKEN_H