#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
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
    std::string name;
    int offset;
    std::string type;
    std::string value;
};

struct Frame {
    std::string label;
    std::vector<FrameVar> vars;
};

struct Snapshot {
    std::string label;
    std::vector<FrameVar> vars;
    int line;
    int idx;
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
    std::ostream& out;
    std::string stackPath;

public:
    GenCodeVisitor(std::ostream& out, const std::string& stackPath = "") : out(out), stackPath(stackPath) {}

    int generar(Program* program);
    Environment<int> env;                       // offsets
    Environment<std::string> typeEnv;           // tipos (locals)
    unordered_map<string, bool> memoriaGlobal;  // globals: nombre -> bool
    unordered_map<string, std::string> globalTypes;
    int    offset        = -8;
    int    labelcont     = 0;
    bool   entornoFuncion = false;
    string nombreFuncion;
    Frame  globalFrame{"globals"};
    Frame  currentFrame{"none"};
    std::vector<Snapshot> snapshots;
    std::map<std::string, FrameVar> currentVars;
    int snapshotCounter = 0;
    std::map<int, std::vector<std::string>> asmByLine;
    int currentLine = -1;

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
    // Recorrido previo para asignar offsets a todas las variables locales (incluidas anidadas)
    int preAsignarOffsets(Body* body, int startOffset);
    void saveStack();
    void saveAsmMap();
    void emit(const std::string& instr, int lineOverride = -1);
    void snapshot(const std::string& label, int line = -1);
    std::string constEval(Exp* e);
};

#endif // VISITOR_H








