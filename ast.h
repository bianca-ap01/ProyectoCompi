#ifndef AST_H
#define AST_H

#include <string>
#include <list>
#include <ostream>
#include <vector>

using namespace std;

class Visitor;
class VarDec;

// Operadores binarios soportados
enum BinaryOp {
    PLUS_OP,
    MINUS_OP,
    MUL_OP,
    DIV_OP,
    POW_OP,
    LE_OP
};

enum TypeKind {
    TYPE_UINT,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_LONG,
    TYPE_AUTO,
};

// Clase abstracta Exp
class Exp {
public:
    virtual int  accept(Visitor* visitor) = 0;
    virtual ~Exp() = 0;  // Destructor puro → clase abstracta
    static string binopToChar(BinaryOp op);  // Conversión operador → string
};

// Expresión binaria
class BinaryExp : public Exp {
public:
    Exp* left;
    Exp* right;
    BinaryOp op;
    int accept(Visitor* visitor) override;
    BinaryExp(Exp* l, Exp* r, BinaryOp op);
    ~BinaryExp();
};

// Expresión numérica
class NumberExp : public Exp {
public:
    int value;
    int accept(Visitor* visitor) override;
    NumberExp(int v);
    ~NumberExp();
};

// Expresión de identificador
class IdExp : public Exp {
public:
    string value;
    int accept(Visitor* visitor) override;
    IdExp(string v);
    ~IdExp();
};

class Stm{
public:
    virtual int accept(Visitor* visitor) = 0;
    virtual ~Stm() = 0;
};

class VarDec{
public:
    TypeKind kind;
    string type;
    list<string> vars;
    vector<Exp*> initializers; // opcional: para soportar int x = 1;
    VarDec();
    int accept(Visitor* visitor);
    ~VarDec();
};

class Body{
public:
    list<Stm*> StmList;
    list<VarDec*> declarations;
    int accept(Visitor* visitor);
    Body();
    ~Body();
};

class IfStm: public Stm {
public:
    Exp* condition;
    Body* then;
    Body* els; // puede ser nullptr

    IfStm(Exp* condition, Body* then, Body* els);
    int accept(Visitor* visitor) override;
    ~IfStm(){};
};

class WhileStm: public Stm {
public:
    Exp* condition;
    Body* b;

    WhileStm(Exp* condition, Body* b);
    int accept(Visitor* visitor) override;
    ~WhileStm(){};
};

class ForStm: public Stm {
public:
    Stm*  init;      // sentencia de inicialización (ej. i = 0;)
    Exp*  condition; // condición del for (ej. i < 10)
    Stm*  step;      // sentencia de incremento (ej. i = i + 1;)
    Body* b;         // cuerpo del for

    ForStm(Stm* init, Exp* condition, Stm* step, Body* b);
    int accept(Visitor* visitor) override;
    ~ForStm() {}; 
};

class AssignStm: public Stm {
public:
    string id;
    Exp* e;

    AssignStm(string, Exp*);
    ~AssignStm();
    int accept(Visitor* visitor) override;
};

class PrintStm: public Stm {
public:
    Exp* e;

    PrintStm(Exp*);
    ~PrintStm();
    int accept(Visitor* visitor) override;
};

class ReturnStm: public Stm {
public:
    Exp* e; // puede ser nullptr

    ReturnStm();
    ReturnStm(Exp* e);
    ~ReturnStm(){};
    int accept(Visitor* visitor) override;
};

class FcallExp: public Exp {
public:
    string nombre;
    vector<Exp*> argumentos;

    int accept(Visitor* visitor) override;
    FcallExp();
    FcallExp(const string& nombre, const vector<Exp*>& args);
    ~FcallExp(){};
};

class FunDec{
public:
    TypeKind kind;
    string type;
    string nombre;
    Body* cuerpo;
    vector<string> Ptipos;
    vector<string> Pnombres;

    int accept(Visitor* visitor);
    FunDec();
    ~FunDec();
};

class Program{
public:
    list<VarDec*> vdlist;
    list<FunDec*> fdlist;

    Program();
    ~Program();
    int accept(Visitor* visitor);
};

class TernaryExp : public Exp {
public:
    Exp* condition;
    Exp* thenExp;
    Exp* elseExp;

    TernaryExp(Exp* condition, Exp* thenExp, Exp* elseExp);
    int accept(Visitor* visitor) override;
    ~TernaryExp(){};
};

#endif // AST_H
