#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include "token.h"
using namespace std;
class Scanner {
private:
    string input;
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
int ejecutar_scanner(Scanner* scanner, const string& inputFile);

#endif // SCANNER_H
