#ifndef AST_H
#define AST_H

#include <string>
#include <unordered_map>
#include <list>
#include <ostream>
#include <vector>
#include "semantic_types.h"

using namespace std;

class Visitor;
class VarDec;
class TypeVisitor; // nuevo forward declaration

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
    Type::TType inferredType = Type::NOTYPE; // tipo inferido tras el typecheck
public:
    virtual int  accept(Visitor* visitor) = 0;
    virtual ~Exp() = 0;  // Destructor puro → clase abstracta
    static string binopToChar(BinaryOp op);  // Conversión operador → string

    // --- NUEVO ---
    virtual Type* accept(TypeVisitor* visitor) = 0; // Para verificador de tipos
};

// Expresión binaria
class BinaryExp : public Exp {
public:
    Exp* left;
    Exp* right;
    BinaryOp op;
    Type::TType resultType = Type::NOTYPE;
    int accept(Visitor* visitor) override;
    BinaryExp(Exp* l, Exp* r, BinaryOp op);
    ~BinaryExp();
    // --- NUEVO ---
    Type* accept(TypeVisitor* visitor);
};

// Expresión numérica
class NumberExp : public Exp {
public:
    long long value;
    double fvalue;
    bool isFloat;
    bool isLong;
    bool isUnsigned;
    Type::TType literalType = Type::NOTYPE;
    int accept(Visitor* visitor) override;
    NumberExp(long long v, double fv, bool isFloatLiteral, bool isLongLiteral, bool isUnsignedLiteral);
    ~NumberExp();
    // --- NUEVO ---
    Type* accept(TypeVisitor* visitor);
};

// Expresión de identificador
class IdExp : public Exp {
public:
    string value;
    Type::TType resolvedType = Type::NOTYPE;
    int accept(Visitor* visitor) override;
    IdExp(string v);
    ~IdExp();
    // --- NUEVO ---
    Type* accept(TypeVisitor* visitor);
};

class BoolExp : public Exp {
public:
    int valor;

    BoolExp(){};
    ~BoolExp(){};

    int accept(Visitor* visitor) override;
    // --- NUEVO ---
    Type* accept(TypeVisitor* visitor) override;
};

class Stm{
public:
    int line = 0;
    virtual int accept(Visitor* visitor) = 0;
    virtual ~Stm() = 0;
    // --- NUEVO ---
    virtual void accept(TypeVisitor* visitor) = 0;
};

class VarDec{
public:
    TypeKind kind;
    string type;
    list<string> vars;
    vector<Exp*> initializers; // opcional: para soportar int x = 1;
    int line = 0;
    VarDec(int line = 0);
    int accept(Visitor* visitor);
    ~VarDec();
    // --- NUEVO ---
    void accept(TypeVisitor* visitor);
};

class Body{
public:
    list<Stm*> StmList;
    list<VarDec*> declarations;
    int accept(Visitor* visitor);
    Body();
    ~Body();
    // --- NUEVO ---
    void accept(TypeVisitor* visitor);
};

class IfStm: public Stm {
public:
    Exp* condition;
    Body* then;
    Body* els; // puede ser nullptr

    IfStm(Exp* condition, Body* then, Body* els, int line = 0);
    int accept(Visitor* visitor) override;
    ~IfStm(){};
    // --- NUEVO ---
    void accept(TypeVisitor* visitor) override;
};

class WhileStm: public Stm {
public:
    Exp* condition;
    Body* b;

    WhileStm(Exp* condition, Body* b, int line = 0);
    int accept(Visitor* visitor) override;
    ~WhileStm(){};
    // --- NUEVO ---
    void accept(TypeVisitor* visitor) override;
};

class ForStm: public Stm {
public:
    Stm*  init;      // sentencia de inicialización (ej. i = 0;)
    Exp*  condition; // condición del for (ej. i < 10)
    Stm*  step;      // sentencia de incremento (ej. i = i + 1;)
    Body* b;         // cuerpo del for

    ForStm(Stm* init, Exp* condition, Stm* step, Body* b, int line = 0);
    int accept(Visitor* visitor) override;
    ~ForStm() {}; 
    // --- NUEVO ---
    void accept(TypeVisitor* visitor) override;
};

class AssignStm: public Stm {
public:
    string id;
    Exp* e;

    AssignStm(string, Exp*, int line = 0);
    ~AssignStm();
    int accept(Visitor* visitor) override;
    // --- NUEVO ---
    void accept(TypeVisitor* visitor) override;
};

class PrintStm: public Stm {
public:
    Exp* e;

    PrintStm(Exp*, int line = 0);
    ~PrintStm();
    int accept(Visitor* visitor) override;
    // --- NUEVO ---
    void accept(TypeVisitor* visitor) override;
};

class ReturnStm: public Stm {
public:
    Exp* e; // puede ser nullptr

    ReturnStm();
    ReturnStm(Exp* e, int line = 0);
    ~ReturnStm(){};
    int accept(Visitor* visitor) override;
    // --- NUEVO ---
    void accept(TypeVisitor* visitor) override;
};

class FcallExp: public Exp {
public:
    string nombre;
    vector<Exp*> argumentos;
    Type::TType returnType = Type::NOTYPE;

    int accept(Visitor* visitor) override;
    FcallExp();
    FcallExp(const string& nombre, const vector<Exp*>& args);
    ~FcallExp(){};
    // --- NUEVO ---
    Type* accept(TypeVisitor* visitor) override;
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
    // --- NUEVO ---
    void accept(TypeVisitor* visitor);
};

class Program{
public:
    list<VarDec*> vdlist;
    list<FunDec*> fdlist;

    Program();
    ~Program();
    int accept(Visitor* visitor);
    // --- NUEVO ---
    void accept(TypeVisitor* visitor);
};

class TernaryExp : public Exp {
public:
    Exp* condition;
    Exp* thenExp;
    Exp* elseExp;
    Type::TType resultType = Type::NOTYPE;

    TernaryExp(Exp* condition, Exp* thenExp, Exp* elseExp);
    int accept(Visitor* visitor) override;
    ~TernaryExp(){};
    // --- NUEVO ---
    Type* accept(TypeVisitor* visitor);
};

#endif // AST_H
