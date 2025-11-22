#include <iostream>
#include "ast.h"
#include "visitor.h"
#include <unordered_map>

using namespace std;

///////////////////////////////////////////////////////////////////////////////////
// Métodos accept del AST
///////////////////////////////////////////////////////////////////////////////////

int BinaryExp::accept(Visitor* visitor) { return visitor->visit(this); }
int NumberExp::accept(Visitor* visitor) { return visitor->visit(this); }
int IdExp::accept(Visitor* visitor)     { return visitor->visit(this); }
int TernaryExp::accept(Visitor* v)      { return v->visit(this); }
int FcallExp::accept(Visitor* v)        { return v->visit(this); }

int Program::accept(Visitor* visitor)   { return visitor->visit(this); }
int FunDec::accept(Visitor* visitor)    { return visitor->visit(this); }
int Body::accept(Visitor* visitor)      { return visitor->visit(this); }
int VarDec::accept(Visitor* visitor)    { return visitor->visit(this); }

int PrintStm::accept(Visitor* visitor)  { return visitor->visit(this); }
int AssignStm::accept(Visitor* visitor) { return visitor->visit(this); }
int IfStm::accept(Visitor* visitor)     { return visitor->visit(this); }
int WhileStm::accept(Visitor* visitor)  { return visitor->visit(this); }
int ReturnStm::accept(Visitor* visitor) { return visitor->visit(this); }
int ForStm::accept(Visitor* visitor)     { return visitor->visit(this); }


///////////////////////////////////////////////////////////////////////////////////
// GenCodeVisitor
///////////////////////////////////////////////////////////////////////////////////

int GenCodeVisitor::generar(Program* program) {
    program->accept(this);
    return 0;
}

int GenCodeVisitor::visit(Program* program) {
    out << ".data\n";
    out << "print_fmt: .string \"%ld \\n\"" << endl;

    // Declaraciones globales del AST
    for (auto dec : program->vdlist) {
        dec->accept(this);
    }

    // Reservar memoria para cada global: .quad 0
    for (auto& [var, _] : memoriaGlobal) {
        out << var << ": .quad 0" << endl;
    }

    out << ".text\n";

    // Funciones
    for (auto dec : program->fdlist) {
        dec->accept(this);
    }

    out << ".section .note.GNU-stack,\"\",@progbits" << endl;
    return 0;
}

int GenCodeVisitor::visit(VarDec* vd) {
    for (auto& var : vd->vars) {
        if (!entornoFuncion) {
            // global
            memoriaGlobal[var] = true;
        } else {
            // local: solo asignamos offset, código lo genera FunDec con initializers
            memoria[var] = offset;
            offset -= 8;
        }
    }
    return 0;
}

int GenCodeVisitor::visit(NumberExp* exp) {
    out << " movq $" << exp->value << ", %rax" << endl;
    return 0;
}

int GenCodeVisitor::visit(IdExp* exp) {
    if (memoriaGlobal.count(exp->value))
        out << " movq " << exp->value << "(%rip), %rax" << endl;
    else
        out << " movq " << memoria[exp->value] << "(%rbp), %rax" << endl;
    return 0;
}

int GenCodeVisitor::visit(BinaryExp* exp) {
    // left → pila
    exp->left->accept(this);
    out << " pushq %rax" << endl;

    // right → %rax, copiamos a %rcx
    exp->right->accept(this);
    out << " movq %rax, %rcx" << endl;
    out << " popq %rax" << endl; // left en %rax

    switch (exp->op) {
        case PLUS_OP:
            out << " addq %rcx, %rax" << endl;
            break;
        case MINUS_OP:
            out << " subq %rcx, %rax" << endl;
            break;
        case MUL_OP:
            out << " imulq %rcx, %rax" << endl;
            break;
        case DIV_OP:
            out << " cqto" << endl;       // extiende signo a RDX:RAX
            out << " idivq %rcx" << endl; // cociente → RAX
            break;
        case POW_OP: {
            int lbl = labelcont++;
            out << " movq %rax, %r8" << endl;    // base
            out << " movq %rcx, %r9" << endl;    // exponente
            out << " movq $1, %rax" << endl;     // res = 1
            out << "pow_loop_" << lbl << ":" << endl;
            out << " cmpq $0, %r9" << endl;
            out << " jle pow_end_" << lbl << endl;
            out << " imulq %r8, %rax" << endl;
            out << " decq %r9" << endl;
            out << " jmp pow_loop_" << lbl << endl;
            out << "pow_end_" << lbl << ":" << endl;
            break;
        }
        case LE_OP:
            out << " cmpq %rcx, %rax" << endl;
            out << " movl $0, %eax" << endl;
            out << " setle %al" << endl;
            out << " movzbq %al, %rax" << endl;
            break;
    }
    return 0;
}

int GenCodeVisitor::visit(TernaryExp* exp) {
    int label = labelcont++;

    // condición
    exp->condition->accept(this);
    out << " cmpq $0, %rax" << endl;
    out << " je ternary_else_" << label << endl;

    // then
    exp->thenExp->accept(this);
    out << " jmp ternary_end_" << label << endl;

    // else
    out << "ternary_else_" << label << ":" << endl;
    exp->elseExp->accept(this);

    out << "ternary_end_" << label << ":" << endl;
    return 0;
}

int GenCodeVisitor::visit(AssignStm* stm) {
    stm->e->accept(this);
    if (memoriaGlobal.count(stm->id))
        out << " movq %rax, " << stm->id << "(%rip)" << endl;
    else
        out << " movq %rax, " << memoria[stm->id] << "(%rbp)" << endl;
    return 0;
}

int GenCodeVisitor::visit(PrintStm* stm) {
    stm->e->accept(this);
    out <<
        " movq %rax, %rsi\n"
        " leaq print_fmt(%rip), %rdi\n"
        " movl $0, %eax\n"
        " call printf@PLT\n";
    return 0;
}

int GenCodeVisitor::visit(Body* b) {
    // Ojo: en funciones, las declaraciones ya se procesan en FunDec::visit
    for (auto dec : b->declarations) {
        dec->accept(this);
    }
    for (auto s : b->StmList) {
        s->accept(this);
    }
    return 0;
}

int GenCodeVisitor::visit(IfStm* stm) {
    int label = labelcont++;
    stm->condition->accept(this);
    out << " cmpq $0, %rax" << endl;
    out << " je else_" << label << endl;
    stm->then->accept(this);
    out << " jmp endif_" << label << endl;
    out << "else_" << label << ":" << endl;
    if (stm->els) stm->els->accept(this);
    out << "endif_" << label << ":" << endl;
    return 0;
}

int GenCodeVisitor::visit(WhileStm* stm) {
    int label = labelcont++;
    out << "while_" << label << ":" << endl;
    stm->condition->accept(this);
    out << " cmpq $0, %rax" << endl;
    out << " je endwhile_" << label << endl;
    stm->b->accept(this);
    out << " jmp while_" << label << endl;
    out << "endwhile_" << label << ":" << endl;
    return 0;
}

int GenCodeVisitor::visit(ReturnStm* stm) {
    if (stm->e) {
        stm->e->accept(this);
    }
    out << " jmp .end_" << nombreFuncion << endl;
    return 0;
}

int GenCodeVisitor::visit(ForStm* stm) {
    // init
    if (stm->init) {
        stm->init->accept(this);
    }

    int label = labelcont++;

    out << "for_" << label << ":" << endl;

    // condición
    if (stm->condition) {
        stm->condition->accept(this);
        out << " cmpq $0, %rax" << endl;
        out << " je endfor_" << label << endl;
    }

    // cuerpo
    if (stm->b) {
        stm->b->accept(this);
    }

    // paso
    if (stm->step) {
        stm->step->accept(this);
    }

    out << " jmp for_" << label << endl;
    out << "endfor_" << label << ":" << endl;

    return 0;
}

int GenCodeVisitor::visit(FunDec* f) {
    entornoFuncion = true;
    memoria.clear();
    offset = -8;
    nombreFuncion = f->nombre;

    vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    out << ".globl " << f->nombre << endl;
    out << f->nombre << ":" << endl;
    out << " pushq %rbp" << endl;
    out << " movq %rsp, %rbp" << endl;

    // Parámetros
    int size = f->Pnombres.size();
    for (int i = 0; i < size; i++) {
        memoria[f->Pnombres[i]] = offset;
        out << " movq " << argRegs[i] << ", " << offset << "(%rbp)" << endl;
        offset -= 8;
    }

    // Declaraciones locales: solo offset
    for (auto dec : f->cuerpo->declarations) {
        dec->accept(this);
    }

    // Reservar espacio de locals en el stack
    int reserva = -offset - 8;
    if (reserva > 0) {
        out << " subq $" << reserva << ", %rsp" << endl;
    }

    // Inicializadores locales (incluye auto x = expr;)
    for (auto dec : f->cuerpo->declarations) {
        auto varIt = dec->vars.begin();
        for (size_t i = 0; i < dec->initializers.size() && varIt != dec->vars.end(); ++i, ++varIt) {
            Exp* init = dec->initializers[i];
            if (!init) continue;
            const string& varName = *varIt;

            init->accept(this); // → %rax

            if (memoriaGlobal.count(varName)) {
                out << " movq %rax, " << varName << "(%rip)" << endl;
            } else {
                out << " movq %rax, " << memoria[varName] << "(%rbp)" << endl;
            }
        }
    }

    // Sentencias
    for (auto s : f->cuerpo->StmList) {
        s->accept(this);
    }

    out << ".end_" << f->nombre << ":" << endl;
    out << "leave" << endl;
    out << "ret" << endl;

    entornoFuncion = false;
    return 0;
}

int GenCodeVisitor::visit(FcallExp* exp) {
    vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    int size = exp->argumentos.size();

    for (int i = 0; i < size; i++) {
        exp->argumentos[i]->accept(this);
        out << " movq %rax, " << argRegs[i] << endl;
    }

    out << " call " << exp->nombre << endl;
    // RAX ya tiene retorno
    return 0;
}