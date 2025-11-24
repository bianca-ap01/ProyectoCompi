#include "typechecker.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace std;

// ===========================================================
//   Métodos accept específicos para el verificador de tipos
// ===========================================================

Type* NumberExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* IdExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* BinaryExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* FcallExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* BoolExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* TernaryExp::accept(TypeVisitor* v) { return v->visit(this); }

void AssignStm::accept(TypeVisitor* v) { v->visit(this); }
void PrintStm::accept(TypeVisitor* v) { v->visit(this); }
void ReturnStm::accept(TypeVisitor* v) { v->visit(this); }
void IfStm::accept(TypeVisitor* v) { v->visit(this); }
void ForStm::accept(TypeVisitor* v) { v->visit(this); }
void WhileStm::accept(TypeVisitor* v) { v->visit(this); }

void VarDec::accept(TypeVisitor* v) { v->visit(this); }
void FunDec::accept(TypeVisitor* v) { v->visit(this); } 
void Body::accept(TypeVisitor* v) { v->visit(this); }
void Program::accept(TypeVisitor* v) { v->visit(this); }

// ===========================================================
//   Constructor del TypeChecker
// ===========================================================

TypeChecker::TypeChecker() {
    intType = new Type(Type::INT);
    longType = new Type(Type::LONG);
    floatType = new Type(Type::FLOAT);
    voidType = new Type(Type::VOID);
    uIntType = new Type(Type::UINT);
    boolType = new Type(Type::BOOL);
    currentReturnType = nullptr;
}

// ===========================================================
//   Registrar funciones globales
// ===========================================================

void TypeChecker::add_function(FunDec* fd) {
    if (functions.find(fd->nombre) != functions.end()) {
        cerr << "Error: función '" << fd->nombre << "' ya fue declarada." << endl;
        exit(0);
    }

    Type* returnType = new Type();
    if (!returnType->set_basic_type(fd->type)) {
        cerr << "Error: tipo de retorno no válido en función '" << fd->nombre << "'." << endl;
        exit(0);
    }

    FunctionInfo info;
    info.returnType = returnType;
    for (const auto& ptype : fd->Ptipos) {
        Type* pt = new Type();
        if (!pt->set_basic_type(ptype)) {
            cerr << "Error: tipo de parámetro inválido en función '" << fd->nombre << "'." << endl;
            exit(0);
        }
        info.paramTypes.push_back(pt);
    }

    functions[fd->nombre] = info;
}

// ===========================================================
//   Método principal de verificación
// ===========================================================

void TypeChecker::typecheck(Program* program) {
    if (program) program->accept(this);
    cout << "Revisión exitosa" << endl;
}

// ===========================================================
//   Nivel superior: Programa y Bloque
// ===========================================================

void TypeChecker::visit(Program* p) {
    // Primero registrar funciones
    for (auto f : p->fdlist)
        add_function(f);

    env.add_level();
    for (auto v : p->vdlist)
        v->accept(this);  
    for (auto f : p->fdlist)
        f->accept(this);  
    env.remove_level();
}

void TypeChecker::visit(Body* b) {
    env.add_level();
    for (auto v : b->declarations)
        v->accept(this); 
    for (auto s : b->StmList)
        s->accept(this); 
    env.remove_level();
}

// ===========================================================
//   Declaraciones
// ===========================================================

void TypeChecker::visit(VarDec* v) {
    bool isAuto = (v->type == "auto" || v->kind == TYPE_AUTO);

    if (isAuto) {
        Type* inferred = nullptr;
        size_t idx = 0;
        for (const auto& id : v->vars) {
            if (idx >= v->initializers.size() || v->initializers[idx] == nullptr) {
                cerr << "Error: 'auto' requiere inicializador para '" << id << "'." << endl;
                exit(0);
            }
            Type* initType = v->initializers[idx]->accept(this);
            if (!inferred) {
                inferred = new Type(initType->ttype);
            } else if (!inferred->match(initType)) {
                cerr << "Error: los inicializadores de 'auto' no coinciden en tipo." << endl;
                exit(0);
            }

            if (env.check(id)) {
                cerr << "Error: variable '" << id << "' ya declarada." << endl;
                exit(0);
            }
            env.add_var(id, new Type(inferred->ttype));
            ++idx;
        }
    } else {
        Type base;
        if (!base.set_basic_type(v->type)) {
            cerr << "Error: tipo de variable no válido." << endl;
            exit(0);
        }

        size_t idx = 0;
        for (const auto& id : v->vars) {
            if (env.check(id)) {
                cerr << "Error: variable '" << id << "' ya declarada." << endl;
                exit(0);
            }
            if (idx < v->initializers.size() && v->initializers[idx]) {
                Type* initType = v->initializers[idx]->accept(this);
                if (!initType->match(&base)) {
                    cerr << "Error: tipo de inicializador incompatible con '" << id << "'." << endl;
                    exit(0);
                }
            }
            env.add_var(id, new Type(base.ttype));
            ++idx;
        }
    }
}

void TypeChecker::visit(FunDec* f) {
    env.add_level();

    auto it = functions.find(f->nombre);
    if (it == functions.end()) {
        cerr << "Error interno: firma de función '" << f->nombre << "' no registrada." << endl;
        exit(0);
    }

    if (it->second.paramTypes.size() != f->Pnombres.size()) {
        cerr << "Error: número de parámetros no coincide en función '" << f->nombre << "'." << endl;
        exit(0);
    }

    currentReturnType = it->second.returnType;

    for (size_t i = 0; i < f->Pnombres.size(); ++i) {
        Type* pt = new Type();
        if (!pt->set_basic_type(f->Ptipos[i])) {
            cerr << "Error: tipo de parámetro inválido en función '" << f->nombre << "'." << endl;
            exit(0);
        }
        if (!pt->match(it->second.paramTypes[i])) {
            cerr << "Error: el tipo declarado del parámetro '" << f->Pnombres[i]
                 << "' no coincide con la firma de la función." << endl;
            exit(0);
        }
        env.add_var(f->Pnombres[i], pt);
    }
    f->cuerpo->accept(this); 
    env.remove_level();
    currentReturnType = nullptr;
}

// ===========================================================
//   Sentencias
// ===========================================================

void TypeChecker::visit(PrintStm* stm) {
    Type* t = stm->e->accept(this);
    if (!(t->match(intType) || t->match(boolType))) {
        cerr << "Error: tipo inválido en print (solo int o bool)." << endl;
        exit(0);
    }
}

void TypeChecker::visit(AssignStm* stm) {
    if (!env.check(stm->id)) {
        cerr << "Error: variable '" << stm->id << "' no declarada." << endl;
        exit(0);
    }

    Type* varType = env.lookup(stm->id);
    Type* expType = stm->e->accept(this);

    if (varType->ttype == Type::AUTO) {
        varType->ttype = expType->ttype;
    } else if (!varType->match(expType)) {
        cerr << "Error: tipos incompatibles en asignación a '" << stm->id << "'." << endl;
        exit(0);
    }
}

void TypeChecker::visit(ReturnStm* stm) {
    if (!currentReturnType) {
        cerr << "Error: 'return' fuera de una función." << endl;
        exit(0);
    }

    if (stm->e == nullptr) {
        if (!currentReturnType->match(voidType)) {
            cerr << "Error: la función espera un valor de retorno." << endl;
            exit(0);
        }
        return;
    }

    Type* t = stm->e->accept(this);
    if (!currentReturnType->match(t)) {
        cerr << "Error: tipo de retorno incompatible con la función." << endl;
        exit(0);
    }
}

void TypeChecker::visit(IfStm* stm) {
    Type* cond = stm->condition->accept(this);
    if (!cond->match(boolType)) {
        cerr << "Error: la condición del if debe ser booleana." << endl;
        exit(0);
    }
    stm->then->accept(this);
    if (stm->els) stm->els->accept(this);
}

void TypeChecker::visit(WhileStm* stm) {
    Type* cond = stm->condition->accept(this);
    if (!cond->match(boolType)) {
        cerr << "Error: la condición del while debe ser booleana." << endl;
        exit(0);
    }
    stm->b->accept(this);
}

void TypeChecker::visit(ForStm* stm) {
    if (stm->init) stm->init->accept(this);

    if (stm->condition) {
        Type* cond = stm->condition->accept(this);
        if (!cond->match(boolType)) {
            cerr << "Error: la condición del for debe ser booleana." << endl;
            exit(0);
        }
    }

    if (stm->b) stm->b->accept(this);
    if (stm->step) stm->step->accept(this);
}

// ===========================================================
//   Expresiones
// ===========================================================

Type* TypeChecker::visit(BinaryExp* e) {
    Type* left = e->left->accept(this);
    Type* right = e->right->accept(this);

    switch (e->op) {
        case PLUS_OP: 
        case MINUS_OP: 
        case MUL_OP: 
        case DIV_OP: 
        case POW_OP:
            if (!(left->match(intType) && right->match(intType))) {
                cerr << "Error: operación aritmética requiere operandos int." << endl;
                exit(0);
            }
            return intType;
        case LE_OP:
            if (!(left->match(intType) && right->match(intType))) {
                cerr << "Error: operación aritmética requiere operandos int." << endl;
                exit(0);
            }
            return boolType;
        default:
            cerr << "Error: operador binario no soportado." << endl;
            exit(0);
    }
}

Type* TypeChecker::visit(NumberExp* e) { return intType; }

Type* TypeChecker::visit(BoolExp* e) { return boolType; }

Type* TypeChecker::visit(IdExp* e) {
    if (!env.check(e->value)) {
        cerr << "Error: variable '" << e->value << "' no declarada." << endl;
        exit(0);
    }
    return env.lookup(e->value);
}

Type* TypeChecker::visit(FcallExp* e) {
    auto it = functions.find(e->nombre);
    if (it == functions.end()) {
        cerr << "Error: llamada a función no declarada '" << e->nombre << "'." << endl;
        exit(0);
    }

    if (e->argumentos.size() != it->second.paramTypes.size()) {
        cerr << "Error: cantidad de argumentos incorrecta en llamada a '" << e->nombre << "'." << endl;
        exit(0);
    }

    for (size_t i = 0; i < e->argumentos.size(); ++i) {
        Type* argType = e->argumentos[i]->accept(this);
        if (!argType->match(it->second.paramTypes[i])) {
            cerr << "Error: el argumento " << i << " de '" << e->nombre << "' no coincide con el tipo esperado." << endl;
            exit(0);
        }
    }

    return it->second.returnType;
}

Type* TypeChecker::visit(TernaryExp* e) {
    Type* cond = e->condition->accept(this);
    if (!cond->match(boolType)) {
        cerr << "Error: la condición del operador ternario debe ser booleana." << endl;
        exit(0);
    }

    Type* thenType = e->thenExp->accept(this);
    Type* elseType = e->elseExp->accept(this);

    if (!thenType->match(elseType)) {
        cerr << "Error: las ramas del operador ternario deben tener el mismo tipo." << endl;
        exit(0);
    }

    return thenType;
}
