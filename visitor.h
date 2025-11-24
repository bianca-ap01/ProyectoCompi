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

/*
class DebuggerVisitor : public Visitor {
public:
    // Entorno que guarda el VALOR REAL (int)
    Environment<int> envValues;
    
    // Entorno auxiliar que guarda el OFFSET DE MEMORIA (string "-8(%rbp)")
    Environment<string> envOffsets;

    int currentOffset = -8; // Simulamos el stack pointer

    // Helpers
    void logTrace(string accion, string var, string mem, int val) {
        // Formato CSV simple para que Python lo lea fácil
        // ACTION, VAR, MEM_LOC, NEW_VALUE
        cout << "TRACE| " << accion << " | " << var << " | " << mem << " | " << val << endl;
    }

    // --- Expresiones (Calculan valores) ---
    int visit(BinaryExp* exp) override {
        int l = exp->left->accept(this);
        int r = exp->right->accept(this);
        switch(exp->op) {
            case PLUS_OP: return l + r;
            case MINUS_OP: return l - r;
            case MUL_OP: return l * r;
            case DIV_OP: return (r!=0)? l/r : 0;
            case LE_OP: return l < r;
            default: return 0;
        }
    }
    
    int visit(NumberExp* exp) override { return exp->value; }
    
    int visit(IdExp* exp) override { return envValues.lookup(exp->value); }

    // --- Statements (Ejecutan y generan la tabla) ---

    int visit(VarDec* vd) override {
        for(string var : vd->vars) {
            // 1. Asignar offset simulado (como haría el assembly)
            string memoryLoc = to_string(currentOffset) + "(%rbp)";
            envOffsets.add_var(var, memoryLoc);
            currentOffset -= 8;

            // 2. Inicializar valor en 0
            envValues.add_var(var, 0);

            // 3. Reportar a la tabla
            logTrace("DECLARACION", var, memoryLoc, 0);
        }
        return 0;
    }

    int visit(AssignStm* stm) override {
        // 1. Calcular nuevo valor
        int val = stm->e->accept(this);
        
        // 2. Actualizar memoria real
        envValues.update(stm->id, val);

        // 3. Buscar dónde está guardado (el offset)
        string memLoc = envOffsets.lookup(stm->id);

        // 4. Reportar
        logTrace("ASIGNACION", stm->id, memLoc, val);
        return 0;
    }

    int visit(PrintStm* stm) override {
        int val = stm->e->accept(this);
        cout << "OUTPUT| PANTALLA | - | - | " << val << endl;
        return 0;
    }
    
    // --- Boilerplate de flujo ---
    int visit(Program* p) override { return 0; } // Asume lógica en Main
    int visit(Body* b) override {
        envValues.add_level();
        envOffsets.add_level();
        for(auto* dec : b->declarations) dec->accept(this);
        for(auto* stm : b->StmList) stm->accept(this);
        envValues.remove_level();
        envOffsets.remove_level();
        return 0;
    }
    int visit(FunDec* fd) override { 
        currentOffset = -8; // Reset del stack frame al entrar a función
        fd->cuerpo->accept(this); 
        return 0; 
    }

    // Vacíos por ahora para que compile
    int visit(TernaryExp* e) override { return 0; }
    int visit(FcallExp* e) override { return 0; }
    int visit(IfStm* s) override { 
        if(s->condition->accept(this)) s->then->accept(this);
        else if(s->els) s->els->accept(this);
        return 0; 
    }
    int visit(WhileStm* s) override { return 0; }
    int visit(ReturnStm* s) override { return 0; }
    int visit(ForStm* s) override { return 0; }
};
*/
#endif