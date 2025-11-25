#include <iostream>
#include <fstream>
#include <string>
#include "scanner.h"
#include "parser.h"
#include "ast.h"
#include "visitor.h"
#include <cstring>
#include "debugger.h"
using namespace std;

int main(int argc, const char* argv[]) {
    // Verificar número de argumentos
    if (argc < 2) {
        cout << "Número incorrecto de argumentos.\n";
        cout << "Uso: " << argv[0] << " <archivo_de_entrada>" << endl;
        return 1;
    }

    // Abrir archivo de entrada
    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cout << "No se pudo abrir el archivo: " << argv[1] << endl;
        return 1;
    }

    // Leer contenido completo del archivo en un string
    string input, line;
    while (getline(infile, line)) {
        input += line + '\n';
    }
    infile.close();

    // Crear instancias de Scanner 
    Scanner scanner1(input.c_str());

    // Crear instancias de Parser
    Parser parser(&scanner1);

    // Parsear y generar AST
    
    Program* program = parser.parseProgram();
    
    if (argc >= 3 && strcmp(argv[2], "--debug") == 0) {
        
        // Imprimimos la cabecera DOT para Graphviz
        cout << "digraph MemoryFlow {" << endl;
        cout << "  rankdir=LR;" << endl;
        cout << "  node [shape=plain, fontname=\"Arial\"];" << endl;
        cout << "  edge [color=\"#aaaaaa\"];" << endl;

        DebuggerVisitor debugger;
        if (program) {
            program->accept(&debugger);
        }
        
        cout << "}" << endl; // Cierre del grafo
        
        // ¡IMPORTANTE! Terminamos aquí para no generar ensamblador
        // Usamos _Exit(0) para una salida limpia y rápida
        _Exit(0); 
    }
    cout << "Generando codigo ensamblador en inputs/" << argv[1] << endl; // Mensaje opcional
        string inputFile(argv[1]);
        size_t dotPos = inputFile.find_last_of('.');
        string baseName = (dotPos == string::npos) ? inputFile : inputFile.substr(0, dotPos);
        string outputFilename = baseName + ".s";
        string stackFilename  = baseName + "_stack.json";
        ofstream outfile(outputFilename);
        if (!outfile.is_open()) {
            cerr << "Error al crear el archivo de salida: " << outputFilename << endl;
            return 1;
        }

    cout << "Generando codigo ensamblador en " << outputFilename << endl;
    GenCodeVisitor codigo(outfile, stackFilename);
    codigo.generar(program);
    outfile.close();
    
    return 0;
}
