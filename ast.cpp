#include "ast.h"
#include <iostream>

using namespace std;

// ------------------ Exp ------------------
Exp::~Exp() {}

string Exp::binopToChar(BinaryOp op) {
    switch (op) {
        case PLUS_OP:  return "+";
        case MINUS_OP: return "-";
        case MUL_OP:   return "*";
        case DIV_OP:   return "/";
        case POW_OP:   return "**";
        case LE_OP:    return "<";
        default:       return "?";
    }
}

// ------------------ BinaryExp ------------------
BinaryExp::BinaryExp(Exp* l, Exp* r, BinaryOp o)
    : left(l), right(r), op(o) {}

BinaryExp::~BinaryExp() {
    delete left;
    delete right;
}

// ------------------ NumberExp ------------------
NumberExp::NumberExp(int v) : value(v) {}

NumberExp::~NumberExp() {}

// ------------------ IdExp ------------------
IdExp::IdExp(string v) : value(std::move(v)) {}

IdExp::~IdExp() {}

// ------------------ Stm base ------------------
Stm::~Stm() {}

// ------------------ PrintStm ------------------
PrintStm::PrintStm(Exp* expresion)
    : e(expresion) {}

PrintStm::~PrintStm() {
    delete e;
}

// ------------------ AssignStm ------------------
AssignStm::AssignStm(string variable, Exp* expresion)
    : id(std::move(variable)), e(expresion) {}

AssignStm::~AssignStm() {
    delete e;
}

// ------------------ IfStm ------------------
IfStm::IfStm(Exp* c, Body* t, Body* e)
    : condition(c), then(t), els(e) {}

// destructor inline vacío en el .h

// ------------------ WhileStm ------------------
WhileStm::WhileStm(Exp* c, Body* body)
    : condition(c), b(body) {}

// destructor inline vacío en el .h

// ------------------ ForStm ------------------
ForStm::ForStm(Stm* i, Exp* c, Stm* s, Body* body)
    : init(i), condition(c), step(s), b(body) {}

// ------------------ ReturnStm ------------------
ReturnStm::ReturnStm() : e(nullptr) {}

ReturnStm::ReturnStm(Exp* exp) : e(exp) {}

// destructor inline vacío en el .h

// ------------------ FcallExp ------------------
FcallExp::FcallExp() {}

FcallExp::FcallExp(const string& n, const vector<Exp*>& args)
    : nombre(n), argumentos(args) {}

// destructor inline vacío en el .h

// ------------------ TernaryExp ------------------
TernaryExp::TernaryExp(Exp* c, Exp* t, Exp* e)
    : condition(c), thenExp(t), elseExp(e) {}

// destructor inline vacío en el .h

// ------------------ VarDec ------------------
VarDec::VarDec() {}

VarDec::~VarDec() {
    for (Exp* init : initializers) {
        delete init;
    }
}

// ------------------ Body ------------------
Body::Body() {
    declarations = list<VarDec*>();
    StmList      = list<Stm*>();
}

Body::~Body() {
    for (Stm* s : StmList) {
        delete s;
    }
    for (VarDec* v : declarations) {
        delete v;
    }
}

// ------------------ FunDec ------------------
FunDec::FunDec()
    : kind(TYPE_INT), type("int"), nombre(""), cuerpo(nullptr) {}

FunDec::~FunDec() {
    delete cuerpo;
}

// ------------------ Program ------------------
Program::Program() {}

Program::~Program() {
    for (VarDec* v : vdlist) {
        delete v;
    }
    for (FunDec* f : fdlist) {
        delete f;
    }
}