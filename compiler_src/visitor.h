#ifndef VISITOR_H
#define VISITOR_H
// visitor de generacion de codigo asm y snapshots de stack

#include "ast.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
#include <set>
// Env
#include "environment.h"

using namespace std;

class BinaryExp;
class NumberExp;
class IdExp;
class BoolExp;
class TernaryExp;
class Program;
class PrintStm;
class WhileStm;
class IfStm;
class AssignStm;
class Body;
class VarDec;
class FcallExp;
class ReturnStm;
class FunDec;
class ForStm;

struct FrameVar {
    // variable con nombre, offset en stack, tipo y valor simbolico
    string name;
    int offset;
    string type;
    string value;
};

struct Frame {
    // frame activo (funcion) con su etiqueta y variables visibles
    string label;
    vector<FrameVar> vars;
};

struct Snapshot {
    // captura de estado del frame en un punto del codigo
    string label;
    vector<FrameVar> vars;
    int line;
    int idx;
    string func;
};

// Interfaz de Visitor
class Visitor {
public:
    // Expresiones
    virtual int visit(BinaryExp* exp) = 0;
    virtual int visit(NumberExp* exp) = 0;
    virtual int visit(IdExp* exp) = 0;
    virtual int visit(BoolExp* exp) = 0;
    virtual int visit(TernaryExp* exp) = 0;
    virtual int visit(FcallExp* fcall) = 0;

    // Programa / funciones / cuerpo
    virtual int visit(Program* p) = 0;
    virtual int visit(FunDec* fd) = 0;
    virtual int visit(Body* body) = 0;
    virtual int visit(VarDec* vd) = 0;

    // Sentencias
    virtual int visit(PrintStm* stm) = 0;
    virtual int visit(WhileStm* stm) = 0;
    virtual int visit(IfStm* stm) = 0;
    virtual int visit(AssignStm* stm) = 0;
    virtual int visit(ReturnStm* r) = 0;
    virtual int visit(ForStm* stm) = 0;
};


// Visitor que genera código asm x86-64
class GenCodeVisitor : public Visitor {
private:
    ostream& out;
    string stackPath;
    set<string> usedVars;  
    void markUsedVars(Exp* e);
    void markUsedVarsInBody(Body* b);

public:
    GenCodeVisitor(ostream& out, const string& stackPath = "") : out(out), stackPath(stackPath) {}

    int generar(Program* program);
    Environment<int> env;                       // offsets
    Environment<string> typeEnv;                // tipos (locals)
    unordered_map<string, bool> memoriaGlobal;  // globals: nombre -> bool
    unordered_map<string, string> globalTypes;   // tipos de globales
    int    offset        = -8;                   // offset actual en stack
    int    labelcont     = 0;                    // contador para labels unicos
    bool   entornoFuncion = false;               // estamos generando dentro de funcion
    string nombreFuncion;
    Frame  globalFrame{"globals"};
    Frame  currentFrame{"none"};
    vector<Snapshot> snapshots;                  // capturas de stack para el front
    map<string, FrameVar> currentVars;           // valores simbolicos actuales
    int snapshotCounter = 0;
    map<int, vector<string>> asmByLine;          // linea -> instrucciones
    int currentLine = -1;                        // linea fuente actual para emit

    // Expresiones
    int visit(BinaryExp* exp) override;
    int visit(NumberExp* exp) override;
    int visit(IdExp* exp) override;
    int visit(BoolExp* exp) override;
    int visit(TernaryExp* exp) override;
    int visit(FcallExp* fcall) override;

    // Programa / funciones / cuerpo
    int visit(Program* p) override;
    int visit(FunDec* fd) override;
    int visit(Body* body) override;
    int visit(VarDec* vd) override;

    // Sentencias
    int visit(PrintStm* stm) override;
    int visit(AssignStm* stm) override;
    int visit(WhileStm* stm) override;
    int visit(IfStm* stm) override;
    int visit(ReturnStm* r) override;
    int visit(ForStm* stm) override;

private:
    int preAsignarOffsets(Body* body, int startOffset);          // asigna offsets antes de generar
    void saveStack();                                            // guarda snapshots de stack en json
    void saveAsmMap();                                           // guarda asm por linea en stackpath+.asm.json
    void emit(const string& instr, int lineOverride = -1);       // escribe asm y lo asocia a linea actual
    void snapshot(const string& label, int line = -1);           // captura estado del frame para el front
    string constEval(Exp* e);                                    // eval simbolica simple para valores en stack
};

#endif // VISITOR_H



