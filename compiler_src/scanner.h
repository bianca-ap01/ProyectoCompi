#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include "token.h"

class Scanner {
private:
    std::string input;
    int first;
    int current;
    int line;
    int col;
    void advanceChar();

public:
    Scanner(const char* in_s);
    Token* nextToken();
    ~Scanner();
};

// Ejecutar scanner (opcional, para depurar)
int ejecutar_scanner(Scanner* scanner, const std::string& inputFile);

#endif // SCANNER_H
