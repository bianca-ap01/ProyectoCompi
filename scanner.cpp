#include "scanner.h"
#include "token.h"
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>

Scanner::Scanner(const char* s)
    : input(s), first(0), current(0) {}

Scanner::~Scanner() {}

Token* Scanner::nextToken() {
    while (true) {
        if (current >= static_cast<int>(input.size())) {
            return new Token(Token::END);
        }

        char c = input[current];

        // Saltar espacios en blanco
        if (std::isspace(static_cast<unsigned char>(c))) {
            current++;
            continue;
        }

        first = current;

        // Línea de preprocesador: #include, #define, etc. -> ignorar
        if (c == '#') {
            current++;
            while (current < static_cast<int>(input.size()) &&
                   input[current] != '\n') {
                current++;
            }
            if (current < static_cast<int>(input.size()) &&
                input[current] == '\n') {
                current++;
            }
            continue;
        }

        // Literales de cadena "..."
        if (c == '"') {
            current++; // saltar comilla de apertura
            while (current < static_cast<int>(input.size()) &&
                   input[current] != '"' &&
                   input[current] != '\n') {
                current++;
            }
            if (current < static_cast<int>(input.size()) &&
                input[current] == '"') {
                current++; // consumir comilla de cierre
            }
            return new Token(Token::STRING, input, first, current);
        }

        // Números (solo enteros por ahora)
        if (std::isdigit(static_cast<unsigned char>(c))) {
            current++;
            while (current < static_cast<int>(input.size()) &&
                   std::isdigit(static_cast<unsigned char>(input[current]))) {
                current++;
            }
            return new Token(Token::NUM, input, first, current);
        }

        // Identificadores y palabras clave
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            current++;
            while (current < static_cast<int>(input.size()) &&
                   (std::isalnum(static_cast<unsigned char>(input[current])) ||
                    input[current] == '_')) {
                current++;
            }
            std::string lex = input.substr(first, current - first);

            if (lex == "return")   return new Token(Token::RETURN,  lex[0]);
            if (lex == "if")       return new Token(Token::IF,      lex[0]);
            if (lex == "else")     return new Token(Token::ELSE,    lex[0]);
            if (lex == "while")    return new Token(Token::WHILE,   lex[0]);
            if (lex == "for")      return new Token(Token::FOR,     lex[0]);
            if (lex == "do")       return new Token(Token::DO,      lex[0]);
            if (lex == "printf")   return new Token(Token::PRINT,   lex[0]);
            if (lex == "true")     return new Token(Token::TRUE,    lex[0]);
            if (lex == "false")    return new Token(Token::FALSE,   lex[0]);
            if (lex == "unsigned") return new Token(Token::UNSIGNED,lex[0]);
            if (lex == "int")      return new Token(Token::INT,     lex[0]);
            if (lex == "float")    return new Token(Token::FLOAT,   lex[0]);
            if (lex == "long")     return new Token(Token::LONG,    lex[0]);
            if (lex == "auto")     return new Token(Token::AUTO,    lex[0]);

            // Identificador genérico
            Token* t = new Token(Token::ID);
            t->text = lex;
            return t;
        }

        // Operadores y signos de puntuación
        current++;
        switch (c) {
            case '+': return new Token(Token::PLUS, c);
            case '-': return new Token(Token::MINUS, c);
            case '*':
                if (current < static_cast<int>(input.size()) &&
                    input[current] == '*') {
                    current++;
                    return new Token(Token::POW, '*');
                }
                return new Token(Token::MUL, c);
            case '/': return new Token(Token::DIV, c);
            case '%': return new Token(Token::MOD, c);
            case '(': return new Token(Token::LPAREN, c);
            case ')': return new Token(Token::RPAREN, c);
            case '{': return new Token(Token::LBRACE, c);
            case '}': return new Token(Token::RBRACE, c);
            case ';': return new Token(Token::SEMICOL, c);
            case ',': return new Token(Token::COMA, c);
            case '<': return new Token(Token::LE, c);
            case '>': return new Token(Token::GT, c);
            case '=': return new Token(Token::ASSIGN, c);
            case '?': return new Token(Token::QMARK, c);
            case ':': return new Token(Token::COL, c);
            case '\\':return new Token(Token::BACKSLASH, c);
            case '.': return new Token(Token::DOT, c);
            default:
                // Carácter no reconocido
                return new Token(Token::ERR, c);
        }
    }
}

// Versión sencilla de ejecutar_scanner (opcional)
int ejecutar_scanner(Scanner* scanner, const std::string& inputFile) {
    std::ofstream outFile(inputFile + ".tokens.txt");
    if (!outFile.is_open()) {
        std::cerr << "No se pudo abrir archivo de salida de tokens\n";
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