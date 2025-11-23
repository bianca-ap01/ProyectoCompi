#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <string>
// Env
#include "environment.h"

using namespace std;

class BinaryExp;
class NumberExp;
class IdExp;
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

// Interfaz de Visitor
class Visitor {
public:
    // Expresiones
    virtual int visit(BinaryExp* exp) = 0;
    virtual int visit(NumberExp* exp) = 0;
    virtual int visit(IdExp* exp) = 0;
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

public:
    GenCodeVisitor(std::ostream& out) : out(out) {}

    int generar(Program* program);
    Environment<int> env;
    //unordered_map<string, int>  memoria;        // locals: nombre → offset
    unordered_map<string, bool> memoriaGlobal;  // globals: nombre → bool
    int    offset        = -8;
    int    labelcont     = 0;
    bool   entornoFuncion = false;
    string nombreFuncion;

    // Expresiones
    int visit(BinaryExp* exp) override;
    int visit(NumberExp* exp) override;
    int visit(IdExp* exp) override;
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
    void preAsignarOffsets(Body* body);
};

#endif // VISITOR_H
