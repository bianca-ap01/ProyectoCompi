#include <iostream>
#include "ast.h"
#include <unordered_map>
#include <algorithm>
#include "visitor.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////////
// Métodos accept del AST
///////////////////////////////////////////////////////////////////////////////////

int BinaryExp::accept(Visitor* visitor) { return visitor->visit(this); }
int NumberExp::accept(Visitor* visitor) { return visitor->visit(this); }
int IdExp::accept(Visitor* visitor)     { return visitor->visit(this); }
int BoolExp::accept(Visitor* visitor)   { return visitor->visit(this); }
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
    saveStack();
    return 0;
}

void GenCodeVisitor::saveStack() {
    if (stackPath.empty()) return;

    // Si no hay snapshots, al menos añade globals
    if (snapshots.empty() && !globalFrame.vars.empty()) {
        snapshots.push_back(Snapshot{"globals", globalFrame.vars, 0});
    }

    std::ofstream json(stackPath, std::ios::trunc);
    if (!json.is_open()) return;

    json << "[";
    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto& fr = snapshots[i];
        json << "{\"label\":\"" << fr.label << "\",\"line\":" << fr.line << ",\"vars\":[";
        for (size_t v = 0; v < fr.vars.size(); ++v) {
            const auto& var = fr.vars[v];
            json << "{\"name\":\"" << var.name << "\",\"value\":\"" << var.value << "\","
                 << "\"offset\":" << var.offset << ",\"type\":\"" << var.type << "\"}";
            if (v + 1 < fr.vars.size()) json << ",";
        }
        json << "]}";
        if (i + 1 < snapshots.size()) json << ",";
    }
    json << "]";
}

// Pre-pase opcional (actualmente no se usa porque reservamos stack por bloque)
void GenCodeVisitor::preAsignarOffsets(Body* /*body*/) {
    return;
}

int GenCodeVisitor::visit(Program* program) {
    env.add_level();
    out << ".data\n";
    out << "print_fmt: .string \"%ld \\n\"" << endl;

    // Declaraciones globales del AST
    for (auto dec : program->vdlist) {
        dec->accept(this);
    }

    // Reservar memoria para cada global: .quad 0
    for (auto it = memoriaGlobal.begin(); it != memoriaGlobal.end(); ++it) {
        out << it->first << ": .quad 0" << endl;
    }

    out << ".text\n";

    // Funciones
    for (auto dec : program->fdlist) {
        dec->accept(this);
    }

    out << ".section .note.GNU-stack,\"\",@progbits" << endl;
    env.remove_level();
    return 0;
}

int GenCodeVisitor::visit(VarDec* vd) {
    for (auto& var : vd->vars) {
        if (!entornoFuncion) {
            // global
            memoriaGlobal[var] = true;
            // Capturamos en frame global
            FrameVar fv{var, 0, vd->type, "?"};
            globalFrame.vars.push_back(fv);
        } else {
            // local: solo asignamos offset, código lo genera FunDec con initializers
            if (!env.check(var)) {
                int currentOffset = offset;
                env.add_var(var, currentOffset);
                offset -= 8;
                if (currentFrame.label != "none") {
                    FrameVar fv{var, currentOffset, vd->type, "?"};
                    currentFrame.vars.push_back(fv);
                    currentVars[var] = fv;
                    snapshot("decl " + var, vd->line);
                }
            }
        }
    }
    return 0;
}

int GenCodeVisitor::visit(NumberExp* exp) {
    if (exp->isFloat) {
        cerr << "[GenCode] Literales float no soportados aún en generación de código." << endl;
        exit(1);
    }
    out << " movq $" << exp->value << ", %rax" << endl;
    return 0;
}

int GenCodeVisitor::visit(BoolExp* exp) {
    out << " movq $" << exp->valor << ", %rax" << endl;
    return 0;
}

int GenCodeVisitor::visit(IdExp* exp) {
    if (memoriaGlobal.count(exp->value))
        out << " movq " << exp->value << "(%rip), %rax" << endl;
    else
        out << " movq " << env.lookup(exp->value) << "(%rbp), %rax" << endl;
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
            out << " setl %al" << endl;
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
        //out << " movq %rax, " << memoria[stm->id] << "(%rbp)" << endl;
        out << " movq %rax, " << env.lookup(stm->id) << "(%rbp)" << endl;
    if (currentVars.count(stm->id)) {
        currentVars[stm->id].value = constEval(stm->e);
    }
    if (entornoFuncion && currentFrame.label != "none") {
        snapshot("assign " + stm->id, stm->line);
    }
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
    env.add_level();

    // Reservar espacio para las variables declaradas en este bloque
    int oldOffset    = offset;
    int reservaLocal = 0;

    for (auto dec : b->declarations) {
        for (const auto& var : dec->vars) {
            int currentOffset = offset;
            env.add_var(var, currentOffset);
            offset -= 8;
            reservaLocal += 8;

            // Capturar variables locales para visualización de stack
            if (entornoFuncion && currentFrame.label != "none") {
                FrameVar fv{var, currentOffset, dec->type, "?"};
                currentFrame.vars.push_back(fv);
                currentVars[var] = fv;
                snapshot("decl " + var, dec->line);
            }
        }
    }
    if (reservaLocal > 0) {
        out << " subq $" << reservaLocal << ", %rsp" << endl;
    }

    // Inicializadores locales en orden de aparición
    for (auto dec : b->declarations) {
        auto varIt = dec->vars.begin();
        for (size_t i = 0; i < dec->initializers.size() && varIt != dec->vars.end(); ++i, ++varIt) {
            Exp* init = dec->initializers[i];
            if (!init) continue;
            const string& varName = *varIt;

            init->accept(this); // → %rax

            if (memoriaGlobal.count(varName)) {
                out << " movq %rax, " << varName << "(%rip)" << endl;
            } else {
                out << " movq %rax, " << env.lookup(varName) << "(%rbp)" << endl;
            }

            // Si es un literal numérico, guardamos el valor simbólico
            if (currentVars.count(varName)) {
                currentVars[varName].value = constEval(init);
            }
        }
    }

    // Sentencias del bloque
    for (auto s : b->StmList) {
        s->accept(this);
        // Snapshot por línea para mostrar el avance aun si no cambia memoria
        if (entornoFuncion && currentFrame.label != "none" && s && s->line > 0) {
            // Evitar duplicar snapshot de asignaciones (ya se capturan con label assign id)
            if (dynamic_cast<AssignStm*>(s) == nullptr) {
                snapshot("line " + to_string(s->line), s->line);
            }
        }
    }

    if (reservaLocal > 0) {
        out << " addq $" << reservaLocal << ", %rsp" << endl;
    }
    offset = oldOffset; // restaurar para bloques hermanos
    env.remove_level();
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
    env.add_level();
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
    env.remove_level();
    return 0;
    
}

int GenCodeVisitor::visit(FunDec* f) {
    entornoFuncion = true;
    //memoria.clear();
    env.clear();
    env.add_level();
    offset = -8;
    nombreFuncion = f->nombre;
    currentFrame = Frame{f->nombre, {}};
    snapshots.clear();

    vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    out << ".globl " << f->nombre << endl;
    out << f->nombre << ":" << endl;
    out << " pushq %rbp" << endl;
    out << " movq %rsp, %rbp" << endl;

    // Parámetros: asignar offsets y reservar espacio para guardarlos
    int size = f->Pnombres.size();
    int reservaArgs = size * 8;
    if (reservaArgs > 0) {
        out << " subq $" << reservaArgs << ", %rsp" << endl;
    }
    for (int i = 0; i < size; i++) {
        int currentOffset = offset;
        env.add_var(f->Pnombres[i], currentOffset);
        out << " movq " << argRegs[i] << ", " << currentOffset << "(%rbp)" << endl;
        offset -= 8;
        FrameVar fv{f->Pnombres[i], currentOffset, (i < (int)f->Ptipos.size() ? f->Ptipos[i] : "param"), "?"};
        currentFrame.vars.push_back(fv);
        currentVars[fv.name] = fv;
    }

    // Cuerpo completo (declaraciones con inicializadores y sentencias)
    f->cuerpo->accept(this);

    out << ".end_" << f->nombre << ":" << endl;
    out << "leave" << endl;
    out << "ret" << endl;

    entornoFuncion = false;
    env.remove_level();
    snapshot(f->nombre + " (final)", 0);
    currentVars.clear();
    currentFrame = Frame{"none", {}};
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

void GenCodeVisitor::snapshot(const std::string& label, int line) {
    if (currentFrame.label == "none") return;

    // Insertamos un marcador de snapshot en el ASM para ubicar progreso
    out << "# SNAP " << label;
    if (line > 0) {
        out << " line " << line;
    }
    out << "\n";

    std::vector<FrameVar> vars;
    for (auto &fv : currentFrame.vars) {
        auto it = currentVars.find(fv.name);
        if (it != currentVars.end()) {
            fv.value = it->second.value;
        }
        vars.push_back(fv);
    }
    std::sort(vars.begin(), vars.end(), [](const FrameVar& a, const FrameVar& b){
        return a.offset > b.offset;
    });
    snapshots.push_back(Snapshot{label, vars, line});
}

std::string GenCodeVisitor::constEval(Exp* e) {
    if (auto num = dynamic_cast<NumberExp*>(e)) {
        return std::to_string(num->value);
    }
    return "?";
}
