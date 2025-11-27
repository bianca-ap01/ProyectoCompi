#include "scanner.h"
#include "token.h"
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>

Scanner::Scanner(const char* s)
    : input(s), first(0), current(0), line(1), col(1) {}

Scanner::~Scanner() {}

void Scanner::advanceChar() {
    if (current < static_cast<int>(input.size()) && input[current] == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
    current++;
}

Token* Scanner::nextToken() {
    while (true) {
        if (current >= static_cast<int>(input.size())) {
            return new Token(Token::END, line, col);
        }

        char c = input[current];
        int tokenLine = line;
        int tokenCol  = col;

        // Saltar espacios en blanco
        if (isspace(static_cast<unsigned char>(c))) {
            advanceChar();
            continue;
        }

        first = current;

        // Comentarios // y /* */
        if (c == '/' && current + 1 < static_cast<int>(input.size())) {
            char next = input[current + 1];
            if (next == '/') {
                // comentario de línea
                while (current < static_cast<int>(input.size()) && input[current] != '\n') {
                    advanceChar();
                }
                continue;
            } else if (next == '*') {
                // comentario de bloque
                advanceChar(); // consume '/'
                advanceChar(); // consume '*'
                while (current + 1 < static_cast<int>(input.size())) {
                    if (input[current] == '*' && input[current + 1] == '/') {
                        advanceChar(); // '*'
                        advanceChar(); // '/'
                        break;
                    }
                    advanceChar();
                }
                continue;
            }
        }

        // Línea de preprocesador: #include, #define, etc. -> ignorar
        if (c == '#') {
            advanceChar();
            while (current < static_cast<int>(input.size()) &&
                   input[current] != '\n') {
                advanceChar();
            }
            if (current < static_cast<int>(input.size()) &&
                input[current] == '\n') {
                advanceChar();
            }
            continue;
        }

        // Literales de cadena "..."
        if (c == '"') {
            advanceChar(); // saltar comilla de apertura
            while (current < static_cast<int>(input.size()) &&
                   input[current] != '"' &&
                   input[current] != '\n') {
                advanceChar();
            }
            if (current < static_cast<int>(input.size()) &&
                input[current] == '"') {
                advanceChar(); // consumir comilla de cierre
            }
            return new Token(Token::STRING, input, first, current, tokenLine, tokenCol);
        }

        // Números (solo enteros por ahora)
        if (isdigit(static_cast<unsigned char>(c))) {
            advanceChar();
            while (current < static_cast<int>(input.size()) &&
                   isdigit(static_cast<unsigned char>(input[current]))) {
                advanceChar();
            }
            if (current < static_cast<int>(input.size()) &&
                input[current] == '.' &&
                current + 1 < static_cast<int>(input.size()) &&
                isdigit(static_cast<unsigned char>(input[current + 1]))) {
                advanceChar(); // consumir '.'
                while (current < static_cast<int>(input.size()) &&
                       isdigit(static_cast<unsigned char>(input[current]))) {
                    advanceChar();
                }
            }
            if (current < static_cast<int>(input.size()) &&
                (input[current] == 'L' || input[current] == 'l' ||
                 input[current] == 'U' || input[current] == 'u' ||
                 input[current] == 'F' || input[current] == 'f')) {
                advanceChar(); // consumir sufijo de tipo
            }
            return new Token(Token::NUM, input, first, current, tokenLine, tokenCol);
        }

        // Identificadores y palabras clave
        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            advanceChar();
            while (current < static_cast<int>(input.size()) &&
                   (isalnum(static_cast<unsigned char>(input[current])) ||
                    input[current] == '_')) {
                advanceChar();
            }
            string lex = input.substr(first, current - first);

            if (lex == "return")   return new Token(Token::RETURN,  lex[0], tokenLine, tokenCol);
            if (lex == "if")       return new Token(Token::IF,      lex[0], tokenLine, tokenCol);
            if (lex == "else")     return new Token(Token::ELSE,    lex[0], tokenLine, tokenCol);
            if (lex == "while")    return new Token(Token::WHILE,   lex[0], tokenLine, tokenCol);
            if (lex == "for")      return new Token(Token::FOR,     lex[0], tokenLine, tokenCol);
            if (lex == "do")       return new Token(Token::DO,      lex[0], tokenLine, tokenCol);
            if (lex == "printf")   return new Token(Token::PRINT,   lex[0], tokenLine, tokenCol);
            if (lex == "true")     return new Token(Token::TRUE,    lex[0], tokenLine, tokenCol);
            if (lex == "false")    return new Token(Token::FALSE,   lex[0], tokenLine, tokenCol);
            if (lex == "unsigned") return new Token(Token::UNSIGNED,lex[0], tokenLine, tokenCol);
            if (lex == "int")      return new Token(Token::INT,     lex[0], tokenLine, tokenCol);
            if (lex == "float")    return new Token(Token::FLOAT,   lex[0], tokenLine, tokenCol);
            if (lex == "long")     return new Token(Token::LONG,    lex[0], tokenLine, tokenCol);
            if (lex == "auto")     return new Token(Token::AUTO,    lex[0], tokenLine, tokenCol);

            // Identificador genérico
            Token* t = new Token(Token::ID, tokenLine, tokenCol);
            t->text = lex;
            return t;
        }

        // Operadores y signos de puntuación
        advanceChar();
        switch (c) {
            case '+': return new Token(Token::PLUS, c, tokenLine, tokenCol);
            case '-': return new Token(Token::MINUS, c, tokenLine, tokenCol);
            case '*':
                if (current < static_cast<int>(input.size()) &&
                    input[current] == '*') {
                    advanceChar();
                    return new Token(Token::POW, '*', tokenLine, tokenCol);
                }
                return new Token(Token::MUL, c, tokenLine, tokenCol);
            case '/': return new Token(Token::DIV, c, tokenLine, tokenCol);
            case '%': return new Token(Token::MOD, c, tokenLine, tokenCol);
            case '(': return new Token(Token::LPAREN, c, tokenLine, tokenCol);
            case ')': return new Token(Token::RPAREN, c, tokenLine, tokenCol);
            case '{': return new Token(Token::LBRACE, c, tokenLine, tokenCol);
            case '}': return new Token(Token::RBRACE, c, tokenLine, tokenCol);
            case ';': return new Token(Token::SEMICOL, c, tokenLine, tokenCol);
            case ',': return new Token(Token::COMA, c, tokenLine, tokenCol);
            case '<': return new Token(Token::LE, c, tokenLine, tokenCol);
            case '>': return new Token(Token::GT, c, tokenLine, tokenCol);
            case '=': return new Token(Token::ASSIGN, c, tokenLine, tokenCol);
            case '?': return new Token(Token::QMARK, c, tokenLine, tokenCol);
            case ':': return new Token(Token::COL, c, tokenLine, tokenCol);
            case '\\':return new Token(Token::BACKSLASH, c, tokenLine, tokenCol);
            case '.': return new Token(Token::DOT, c, tokenLine, tokenCol);
            default:
                // Carácter no reconocido
                return new Token(Token::ERR, c, tokenLine, tokenCol);
        }
    }
}

// Versión sencilla de ejecutar_scanner (opcional)
int ejecutar_scanner(Scanner* scanner, const string& inputFile) {
    ofstream outFile(inputFile + ".tokens.txt");
    if (!outFile.is_open()) {
        cerr << "No se pudo abrir archivo de salida de tokens\n";
        return 1;
    }

    while (true) {
        Token* tok = scanner->nextToken();
        if (!tok) break;
        outFile << *tok << "\n";
        if (tok->type == Token::END || tok->type == Token::ERR) {
            delete tok;
            break;
        }
        delete tok;
    }

    outFile.close();
    return 0;
}
