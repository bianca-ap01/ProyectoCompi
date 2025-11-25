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
    int line;
    int col;

    Token(Type type, int line = -1, int col = -1);
    Token(Type type, char c, int line = -1, int col = -1);
    Token(Type type, const std::string& source, int first, int last, int line = -1, int col = -1);

    friend std::ostream& operator<<(std::ostream& outs, const Token& tok);
    friend std::ostream& operator<<(std::ostream& outs, const Token* tok);
};

#endif // TOKEN_H
