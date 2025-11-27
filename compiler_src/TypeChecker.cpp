#include "TypeChecker.h"
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
        Type* inferred = nullptr; // Tipo inferido por los inicializadores
        size_t indice = 0; // Índice para inicializadores
        for (const auto& id : v->vars) { // se itera en cada variable
            if (indice >= v->initializers.size() || v->initializers[indice] == nullptr) { // si no tiene inicializador
                cerr << "Error: 'auto' requiere inicializador para '" << id << "'." << endl; // error
                exit(0);
            }
            Type* initType = v->initializers[indice]->accept(this); // obtener tipo del inicializador
            if (!inferred) { // si es el primer inicializador
                inferred = new Type(initType->ttype); // establecer tipo inferido
            } else if (!inferred->match(initType)) { // si no coincide con el tipo inferido
                cerr << "Error: los inicializadores de 'auto' no coinciden en tipo." << endl;
                exit(0);
            }

            if (env.check(id)) {
                cerr << "Error: variable '" << id << "' ya declarada." << endl;
                exit(0);
            }
            env.add_var(id, new Type(inferred->ttype));
            // Anotar el tipo inferido en el AST para uso posterior (gencode)
            v->type = Type::type_to_string(inferred->ttype);
            ++indice;
        }
    } else {
        Type base;
        if (!base.set_basic_type(v->type)) {
            cerr << "Error: tipo de variable no válido." << endl;
            exit(0);
        }

        size_t indice = 0;
        for (const auto& id : v->vars) {
            if (indice < v->initializers.size() && v->initializers[indice]) {
                Type* initType = v->initializers[indice]->accept(this);
                bool compatible = initType->match(&base) ||
                                  (base.match(longType) && initType->match(intType)) ||
                                  (base.match(uIntType) && (initType->match(intType) || initType->match(uIntType))) ||
                                  (base.match(floatType) && (initType->match(intType) || initType->match(floatType)));
                if (!compatible) {
                    cerr << "Error: tipo de inicializador incompatible con '" << id << "'." << endl;
                    exit(0);
                }
            }
            if (env.check(id)) {
                env.update(id, new Type(base.ttype));
            } else {
                env.add_var(id, new Type(base.ttype));
            }
            ++indice;
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
    if (!(t->match(intType) || t->match(boolType) || t->match(longType)
        || t->match(uIntType) || t->match(floatType))) {
        cerr << "Error: tipo inválido en print." << endl;
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
    } else {
        bool compatible = varType->match(expType) ||
                          (varType->match(longType) && expType->match(intType)) ||
                          (varType->match(uIntType) && (expType->match(uIntType) || expType->match(intType))) ||
                          (varType->match(floatType) && (expType->match(floatType) || expType->match(intType)));
        if (!compatible) {
            cerr << "Error: tipos incompatibles en asignación a '" << stm->id << "'." << endl;
            exit(0);
        }
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
    env.add_level();

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

    env.remove_level();
}

// ===========================================================
//   Expresiones
// ===========================================================

Type* TypeChecker::visit(BinaryExp* e) {
    Type* left = e->left->accept(this);
    Type* right = e->right->accept(this);

    bool leftIsUInt = left->match(uIntType);
    bool rightIsUInt = right->match(uIntType);
    bool leftIsInt = left->match(intType);
    bool rightIsInt = right->match(intType);
    bool leftIsLong = left->match(longType);
    bool rightIsLong = right->match(longType);
    bool leftIsFloat = left->match(floatType);
    bool rightIsFloat = right->match(floatType);

    switch (e->op) {
        case PLUS_OP: 
        case MINUS_OP: 
        case MUL_OP: 
        case DIV_OP: 
        case POW_OP:
            if (leftIsFloat || rightIsFloat) { e->inferredType = e->resultType = floatType->ttype; return floatType; }
            if (leftIsLong && rightIsLong)   { e->inferredType = e->resultType = longType->ttype;  return longType; }
            if ((leftIsLong && rightIsInt) || (leftIsInt && rightIsLong)) { e->inferredType = e->resultType = longType->ttype; return longType; }
            if (leftIsInt && rightIsInt)     { e->inferredType = e->resultType = intType->ttype;   return intType; }
            if (leftIsUInt && rightIsUInt)   { e->inferredType = e->resultType = uIntType->ttype;  return uIntType; }
            if ((leftIsInt && rightIsUInt) || (leftIsUInt && rightIsInt)) { e->inferredType = e->resultType = intType->ttype; return intType; } // mezcla -> int
            if ((leftIsLong && rightIsUInt) || (leftIsUInt && rightIsLong)) { e->inferredType = e->resultType = longType->ttype; return longType; }
            cerr << "Error: operacion aritmetica requiere tipos numericos compatibles." << endl;
            exit(0);
        case LE_OP:
            if (leftIsFloat || rightIsFloat) { e->inferredType = boolType->ttype; return boolType; }
            if ((leftIsInt && rightIsInt) ||
                (leftIsLong && rightIsLong) ||
                (leftIsLong && rightIsInt) ||
                (leftIsInt && rightIsLong) ||
                (leftIsUInt && rightIsUInt) ||
                (leftIsInt && rightIsUInt) ||
                (leftIsUInt && rightIsInt) ||
                (leftIsLong && rightIsUInt) ||
                (leftIsUInt && rightIsLong)) {
                e->inferredType = e->resultType = boolType->ttype;
                return boolType;
            }
            cerr << "Error: comparacion requiere operandos numericos compatibles." << endl;
            exit(0);
        default:
            cerr << "Error: operador binario no soportado." << endl;
            exit(0);
    }
}

Type* TypeChecker::visit(NumberExp* e) {
    Type* t = nullptr;
    if (e->isFloat) t = floatType;
    else if (e->isUnsigned) t = uIntType;
    else t = e->isLong ? longType : intType;
    e->inferredType = t->ttype;
    e->literalType = t->ttype;
    return t;
}

Type* TypeChecker::visit(BoolExp* e) { e->inferredType = boolType->ttype; return boolType; }

Type* TypeChecker::visit(IdExp* e) {
    if (!env.check(e->value)) {
        cerr << "Error: variable '" << e->value << "' no declarada." << endl;
        exit(0);
    }
    Type* t = env.lookup(e->value);
    e->inferredType = t->ttype;
    e->resolvedType = t->ttype;
    return t;
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

    e->inferredType = it->second.returnType->ttype;
    e->returnType = it->second.returnType->ttype;
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

    e->inferredType = thenType->ttype;
    e->resultType = thenType->ttype;
    return thenType;
}
