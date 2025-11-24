#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include <unordered_map>
#include <string>
#include <vector>
#include "ast.h"
#include "environment.h"
#include "semantic_types.h"

using namespace std;

class BinaryExp;
class NumberExp;
class Program;
class PrintStm;
class AssignStm;
class IfStm;
class ForStm;
class WhileStm;
class FunDec;
class ReturnStm;
class IdExp;
class TernaryExp;
class Body;
class VarDec;
class FcallExp;
class BoolExp;

class TypeVisitor {
public:

    // --- Nodos de nivel superior ---
    virtual void visit(Program* p) = 0;
    virtual void visit(Body* b) = 0;
    virtual void visit(VarDec* v) = 0;
    virtual void visit(FunDec* f) = 0;

    // --- Sentencias ---
    virtual void visit(PrintStm* stm) = 0;
    virtual void visit(AssignStm* stm) = 0;
    virtual void visit(ReturnStm* stm) = 0;
    // NUEVAS
    virtual void visit(IfStm* stm) = 0;
    virtual void visit(ForStm* stm) = 0;
    virtual void visit(WhileStm* stm) = 0;

    // --- Expresiones ---
    virtual Type* visit(BinaryExp* e) = 0;
    virtual Type* visit(NumberExp* e) = 0;
    virtual Type* visit(IdExp* e) = 0;
    virtual Type* visit(BoolExp* e) = 0;
    virtual Type* visit(FcallExp* e) = 0;
    // NUEVA
    virtual Type* visit(TernaryExp* e) = 0;
};



// ──────────────────────────────────────────────
//   CLASE TYPECHECKER
// ──────────────────────────────────────────────

class TypeChecker : public TypeVisitor {
private:
    struct FunctionInfo {
        Type* returnType;
        vector<Type*> paramTypes;
    };

    Environment<Type*> env;                        // Entorno de variables y sus tipos
    unordered_map<string, FunctionInfo> functions; // Entorno de funciones

    // Tipos básicos
    Type* intType;
    Type* longType;
    Type* floatType;
    Type* uIntType;
    Type* voidType;
    Type* boolType;

    // Tipo de retorno de la función actual
    Type* currentReturnType;

    // Registro de funciones
    void add_function(FunDec* fd);

public:
    TypeChecker();

    // Método principal de verificación
    void typecheck(Program* program);

    // --- Visitas de alto nivel ---
    void visit(Program* p) override;
    void visit(Body* b) override;
    void visit(VarDec* v) override;
    void visit(FunDec* f) override;

    // --- Sentencias ---
    void visit(PrintStm* stm) override;
    void visit(AssignStm* stm) override;
    void visit(ReturnStm* stm) override;
    // NUEVAS
    void visit(IfStm* stm) override;
    void visit(ForStm* stm) override;
    void visit(WhileStm* stm) override;

    // --- Expresiones ---
    Type* visit(BinaryExp* e) override;
    Type* visit(NumberExp* e) override;
    Type* visit(IdExp* e) override;
    Type* visit(FcallExp* e) override;
    Type* visit(BoolExp* e) override;
    // NUEVA
    Type* visit(TernaryExp* e) override;
};

#endif // TYPECHECKER_H
