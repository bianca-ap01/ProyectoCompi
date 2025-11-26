#include <iostream>
#include "ast.h"
#include <unordered_map>
#include <algorithm>
#include "visitor.h"

using namespace std;

// Utilidad: intentar parsear un long long desde string
static bool tryParseLong(const std::string& s, long long& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        out = std::stoll(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

static std::string jsonEscape(const std::string& s) {
    std::string r;
    r.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': r += "\\\\"; break;
            case '"':  r += "\\\""; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:   r += c; break;
        }
    }
    return r;
}

static int sizeOfType(const std::string& t) {
    // Usamos slots de 8 bytes para simplificar alineación y evitar solapamientos
    (void)t;
    return 8;
}

static bool is32Bit(const std::string& t) {
    return false; // usaremos movq siempre para locales
}

static std::string getVarType(const std::map<std::string, FrameVar>& vars, const std::string& name) {
    auto it = vars.find(name);
    if (it != vars.end()) return it->second.type;
    return "int";
}

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
    saveAsmMap();
    return 0;
}

void GenCodeVisitor::emit(const std::string& instr, int lineOverride) {
    int line = (lineOverride >= 0) ? lineOverride : currentLine;
    out << instr << endl;
    if (line >= 0) {
        asmByLine[line].push_back(instr);
    }
}

void GenCodeVisitor::saveStack() {
    if (stackPath.empty()) return;

    // Si no hay snapshots, al menos añade globals
    if (snapshots.empty() && !globalFrame.vars.empty()) {
        snapshots.push_back(Snapshot{"globals", globalFrame.vars, 0, snapshotCounter++});
    }

    std::ofstream json(stackPath, std::ios::trunc);
    if (!json.is_open()) return;

    json << "[";
    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto& fr = snapshots[i];
        json << "{\"label\":\"" << fr.label << "\",\"line\":" << fr.line << ",\"idx\":" << fr.idx << ",\"vars\":[";
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

void GenCodeVisitor::saveAsmMap() {
    if (stackPath.empty()) return;
    std::string asmMapPath = stackPath + ".asm.json";
    std::ofstream json(asmMapPath, std::ios::trunc);
    if (!json.is_open()) return;
    json << "{";
    bool first = true;
    for (const auto& kv : asmByLine) {
        if (!first) json << ",";
        first = false;
        json << "\"" << kv.first << "\":[";
        for (size_t i = 0; i < kv.second.size(); ++i) {
            json << "\"" << jsonEscape(kv.second[i]) << "\"";
            if (i + 1 < kv.second.size()) json << ",";
        }
        json << "]";
    }
    json << "}";
}

// Pre-pase opcional (actualmente no se usa porque reservamos stack por bloque)
void GenCodeVisitor::preAsignarOffsets(Body* /*body*/) {
    return;
}

int GenCodeVisitor::visit(Program* program) {
    currentLine = -1;
    env.add_level();
    emit(".data");
    emit("print_fmt: .string \"%ld \\n\"");

    // Declaraciones globales del AST
    for (auto dec : program->vdlist) {
        dec->accept(this);
    }

    // Reservar memoria para cada global: .quad 0
    for (auto it = memoriaGlobal.begin(); it != memoriaGlobal.end(); ++it) {
        emit(it->first + ": .quad 0");
    }

    emit(".text");

    // Funciones
    for (auto dec : program->fdlist) {
        dec->accept(this);
    }

    emit(".section .note.GNU-stack,\"\",@progbits");
    env.remove_level();
    return 0;
}

int GenCodeVisitor::visit(VarDec* vd) {
    currentLine = vd->line;
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
                offset -= sizeOfType(vd->type);
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
    emit(" movq $" + std::to_string(exp->value) + ", %rax");
    return 0;
}

int GenCodeVisitor::visit(BoolExp* exp) {
    emit(" movq $" + std::to_string(exp->valor) + ", %rax");
    return 0;
}

int GenCodeVisitor::visit(IdExp* exp) {
    std::string vtype = getVarType(currentVars, exp->value);
    if (memoriaGlobal.count(exp->value))
        emit(" movq " + exp->value + "(%rip), %rax");
    else
        emit(" movq " + std::to_string(env.lookup(exp->value)) + "(%rbp), %rax");
    return 0;
}

int GenCodeVisitor::visit(BinaryExp* exp) {
    // left → pila
    exp->left->accept(this);
    emit(" pushq %rax");

    // right → %rax, copiamos a %rcx
    exp->right->accept(this);
    emit(" movq %rax, %rcx");
    emit(" popq %rax"); // left en %rax

    switch (exp->op) {
        case PLUS_OP:
            emit(" addq %rcx, %rax");
            break;
        case MINUS_OP:
            emit(" subq %rcx, %rax");
            break;
        case MUL_OP:
            emit(" imulq %rcx, %rax");
            break;
        case DIV_OP:
            emit(" cqto");       // extiende signo a RDX:RAX
            emit(" idivq %rcx"); // cociente → RAX
            break;
        case POW_OP: {
            int lbl = labelcont++;
            emit(" movq %rax, %r8");    // base
            emit(" movq %rcx, %r9");    // exponente
            emit(" movq $1, %rax");     // res = 1
            emit("pow_loop_" + std::to_string(lbl) + ":");
            emit(" cmpq $0, %r9");
            emit(" jle pow_end_" + std::to_string(lbl));
            emit(" imulq %r8, %rax");
            emit(" decq %r9");
            emit(" jmp pow_loop_" + std::to_string(lbl));
            emit("pow_end_" + std::to_string(lbl) + ":");
            break;
        }
        case LE_OP:
            emit(" cmpq %rcx, %rax");
            emit(" movl $0, %eax");
            emit(" setl %al");
            emit(" movzbq %al, %rax");
            break;
    }
    return 0;
}

int GenCodeVisitor::visit(TernaryExp* exp) {
    int label = labelcont++;

    // condición
    exp->condition->accept(this);
    emit(" cmpq $0, %rax");
    emit(" je ternary_else_" + std::to_string(label));

    // then
    exp->thenExp->accept(this);
    emit(" jmp ternary_end_" + std::to_string(label));

    // else
    emit("ternary_else_" + std::to_string(label) + ":");
    exp->elseExp->accept(this);

    emit("ternary_end_" + std::to_string(label) + ":");
    return 0;
}

int GenCodeVisitor::visit(AssignStm* stm) {
    currentLine = stm->line;
    stm->e->accept(this);
    std::string vtype = getVarType(currentVars, stm->id);
    if (memoriaGlobal.count(stm->id))
        emit(" movq %rax, " + stm->id + "(%rip)");
    else
        emit(" movq %rax, " + std::to_string(env.lookup(stm->id)) + "(%rbp)");
    if (currentVars.count(stm->id)) {
        currentVars[stm->id].value = constEval(stm->e);
    }
    if (entornoFuncion && currentFrame.label != "none") {
        snapshot("assign " + stm->id, stm->line);
    }
    return 0;
}

int GenCodeVisitor::visit(PrintStm* stm) {
    currentLine = stm->line;
    stm->e->accept(this);
    emit(" movq %rax, %rsi");
    emit(" leaq print_fmt(%rip), %rdi");
    emit(" movl $0, %eax");
    emit(" call printf@PLT");
    return 0;
}

int GenCodeVisitor::visit(Body* b) {
    env.add_level();

    // Reservar espacio para las variables declaradas en este bloque
    int oldOffset    = offset;
    int reservaLocal = 0;
    int blockLine = -1;

    for (auto dec : b->declarations) {
        currentLine = dec->line;
        if (blockLine == -1) blockLine = dec->line;
        for (const auto& var : dec->vars) {
            int sz = sizeOfType(dec->type);
            int currentOffset = offset;
            env.add_var(var, currentOffset);
            offset -= sz;
            reservaLocal += sz;

            // Capturar variables locales para visualización de stack
            if (entornoFuncion && currentFrame.label != "none") {
                FrameVar fv{var, currentOffset, dec->type, "?"};
                currentFrame.vars.push_back(fv);
                currentVars[var] = fv;
                snapshot("decl " + var, dec->line);
            }
        }
    }
    if (blockLine == -1 && !b->StmList.empty() && b->StmList.front()) {
        blockLine = b->StmList.front()->line;
    }
    if (reservaLocal > 0) {
        // Ajuste de stack considerado parte del prólogo del bloque (no de una línea específica)
        emit(" subq $" + std::to_string(reservaLocal) + ", %rsp", -1);
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
                emit(" movq %rax, " + varName + "(%rip)");
            } else {
                emit(" movq %rax, " + std::to_string(env.lookup(varName)) + "(%rbp)");
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
        emit(" addq $" + std::to_string(reservaLocal) + ", %rsp", -1);
    }
    offset = oldOffset; // restaurar para bloques hermanos
    env.remove_level();
    return 0;
}

int GenCodeVisitor::visit(IfStm* stm) {
    currentLine = stm->line;
    int label = labelcont++;
    stm->condition->accept(this);
    emit(" cmpq $0, %rax");
    emit(" je else_" + std::to_string(label));
    stm->then->accept(this);
    emit(" jmp endif_" + std::to_string(label));
    emit("else_" + std::to_string(label) + ":");
    if (stm->els) stm->els->accept(this);
    emit("endif_" + std::to_string(label) + ":");
    return 0;
}

int GenCodeVisitor::visit(WhileStm* stm) {
    currentLine = stm->line;
    int label = labelcont++;
    emit("while_" + std::to_string(label) + ":");
    stm->condition->accept(this);
    emit(" cmpq $0, %rax");
    emit(" je endwhile_" + std::to_string(label));
    stm->b->accept(this);
    emit(" jmp while_" + std::to_string(label));
    emit("endwhile_" + std::to_string(label) + ":");
    return 0;
}

int GenCodeVisitor::visit(ReturnStm* stm) {
    currentLine = stm->line;
    if (stm->e) {
        stm->e->accept(this);
    }
    emit(" jmp .end_" + nombreFuncion);
    return 0;
}

int GenCodeVisitor::visit(ForStm* stm) {
    currentLine = stm->line;
    env.add_level();
    // init
    if (stm->init) {
        stm->init->accept(this);
    }

    int label = labelcont++;

    emit("for_" + std::to_string(label) + ":");

    // condición
    if (stm->condition) {
        stm->condition->accept(this);
        emit(" cmpq $0, %rax");
        emit(" je endfor_" + std::to_string(label));
    }

    // cuerpo
    if (stm->b) {
        stm->b->accept(this);
    }

    // paso
    if (stm->step) {
        stm->step->accept(this);
    }

    emit(" jmp for_" + std::to_string(label));
    emit("endfor_" + std::to_string(label) + ":");
    env.remove_level();
    return 0;
    
}

int GenCodeVisitor::visit(FunDec* f) {
    currentLine = -1;
    entornoFuncion = true;
    //memoria.clear();
    env.clear();
    env.add_level();
    offset = -8;
    nombreFuncion = f->nombre;
    snapshotCounter = 0;
    currentFrame = Frame{f->nombre, {}};
    snapshots.clear();

    vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    emit(".globl " + f->nombre);
    emit(f->nombre + ":");
    emit(" pushq %rbp", -1);
    emit(" movq %rsp, %rbp", -1);

    // Parámetros: asignar offsets y reservar espacio para guardarlos
    int size = f->Pnombres.size();
    int reservaArgs = size * 8;
    if (reservaArgs > 0) {
        emit(" subq $" + std::to_string(reservaArgs) + ", %rsp", -1);
    }
    for (int i = 0; i < size; i++) {
        int currentOffset = offset;
        env.add_var(f->Pnombres[i], currentOffset);
        emit(" movq " + argRegs[i] + ", " + std::to_string(currentOffset) + "(%rbp)");
        offset -= 8;
        FrameVar fv{f->Pnombres[i], currentOffset, (i < (int)f->Ptipos.size() ? f->Ptipos[i] : "param"), "?"};
        currentFrame.vars.push_back(fv);
        currentVars[fv.name] = fv;
    }

    // Snapshot inicial con los parámetros ya guardados
    if (!f->Pnombres.empty()) {
        snapshot(f->nombre + " params", 0);
    }

    // Cuerpo completo (declaraciones con inicializadores y sentencias)
    f->cuerpo->accept(this);

    emit(".end_" + f->nombre + ":", -1);
    emit("leave", -1);
    emit("ret", -1);

    entornoFuncion = false;
    env.remove_level();
    // No snapshot final para evitar clutter en UI
    currentVars.clear();
    currentFrame = Frame{"none", {}};
    return 0;
}

int GenCodeVisitor::visit(FcallExp* exp) {
    vector<std::string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    int size = exp->argumentos.size();

    for (int i = 0; i < size; i++) {
        exp->argumentos[i]->accept(this);
        emit(" movq %rax, " + argRegs[i]);
    }

    emit(" call " + exp->nombre);
    // RAX ya tiene retorno
    return 0;
}

void GenCodeVisitor::snapshot(const std::string& label, int line) {
    if (currentFrame.label == "none") return;

    // Marcador de inicio de snapshot: las instrucciones siguientes pertenecen a este idx
    out << "# SNAPIDX " << snapshotCounter << " " << label;
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
    snapshots.push_back(Snapshot{label, vars, line, snapshotCounter});
    snapshotCounter++;
}

std::string GenCodeVisitor::constEval(Exp* e) {
    // Evalúa expresiones simples para mostrar valores simbólicos en el stack
    if (auto num = dynamic_cast<NumberExp*>(e)) {
        return std::to_string(num->value);
    }
    if (auto b = dynamic_cast<BoolExp*>(e)) {
        return std::to_string(b->valor);
    }
    if (auto id = dynamic_cast<IdExp*>(e)) {
        auto it = currentVars.find(id->value);
        if (it != currentVars.end()) {
            return it->second.value;
        }
        return "?";
    }
    if (auto bin = dynamic_cast<BinaryExp*>(e)) {
        std::string lstr = constEval(bin->left);
        std::string rstr = constEval(bin->right);
        long long lval, rval;
        if (!tryParseLong(lstr, lval) || !tryParseLong(rstr, rval)) {
            return "?";
        }
        long long res = 0;
        switch (bin->op) {
            case PLUS_OP:  res = lval + rval; break;
            case MINUS_OP: res = lval - rval; break;
            case MUL_OP:   res = lval * rval; break;
            case DIV_OP:   if (rval == 0) return "?"; res = lval / rval; break;
            case POW_OP:   // potencia simple iterativa
                res = 1;
                for (long long i = 0; i < rval; ++i) res *= lval;
                break;
            case LE_OP:    res = (lval < rval) ? 1 : 0; break;
            default: return "?";
        }
        return std::to_string(res);
    }
    // Llamadas o casos no evaluables
    if (dynamic_cast<FcallExp*>(e)) {
        return "call"; // no evaluamos llamadas en compile-time
    }
    return "?";
}
