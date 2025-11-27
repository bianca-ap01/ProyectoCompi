#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"
#include "TypeChecker.h"

using namespace std;

// flujo principal: leer fuente, tokenizar, parsear, verificar tipos y generar asm + snapshots
int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cout << "numero incorrecto de argumentos\n";
        cout << "uso: " << argv[0] << " <archivo_de_entrada>" << endl;
        return 1;
    }

    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cout << "no se pudo abrir el archivo: " << argv[1] << endl;
        return 1;
    }

    string input, line;
    while (getline(infile, line)) {
        input += line + '\n';
    }
    infile.close();

    Scanner scanner1(input.c_str());
    Parser parser(&scanner1);
    Program* program = parser.parseProgram();

    string inputFile(argv[1]);
    size_t dotPos = inputFile.find_last_of('.');
    string baseName = (dotPos == string::npos) ? inputFile : inputFile.substr(0, dotPos);
    string outputFilename = baseName + ".s";
    ofstream outfile(outputFilename);
    if (!outfile.is_open()) {
        cerr << "error al crear el archivo de salida: " << outputFilename << endl;
        return 1;
    }

    cout << "\n=== verificacion de tipos ===\n";
    TypeChecker tc;
    tc.typecheck(program);

    cout << "generando asm en " << outputFilename << endl;
    string stackFilename = baseName + "_stack.json";
    GenCodeVisitor codigo(outfile, stackFilename);
    codigo.generar(program);
    outfile.close();
    
    return 0;
}
