#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>

class Token {
public:
    // Tipos de token
    enum Type {
        PLUS,       // +
        MINUS,      // -
        MUL,        // *
        DIV,        // /
        POW,        // **
        MOD,        // %
        LPAREN,     // (
        RPAREN,     // )
        LBRACE,     // {
        RBRACE,     // }
        SEMICOL,    // ;
        COMA,       // ,
        LE,         // <
        GT,         // >
        ASSIGN,     // =
        QMARK,      // ?
        COL,        // :
        BACKSLASH,  // '\'

        // Palabras clave
        RETURN,     // return
        IF,         // if
        ELSE,       // else
        WHILE,      // while
        FOR,        // for
        DO,         // do
        PRINT,      // printf
        TRUE,       // true
        FALSE,      // false
        UNSIGNED,   // unsigned
        INT,        // int
        FLOAT,      // float
        LONG,       // long
        AUTO,       // auto

        // Literales e identificadores
        NUM,        // número entero
        ID,         // identificador
        STRING,     // literal de cadena "..."

        // Otros
        HASHTAG,    // #
        DOT,        // .
        END,        // EOF
        ERR         // error léxico
    };

    Type type;
    std::string text;

    Token(Type type);
    Token(Type type, char c);
    Token(Type type, const std::string& source, int first, int last);

    friend std::ostream& operator<<(std::ostream& outs, const Token& tok);
    friend std::ostream& operator<<(std::ostream& outs, const Token* tok);
};

#endif // TOKEN_H