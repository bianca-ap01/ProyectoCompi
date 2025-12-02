#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include "ast.h"
#include "visitor.h"

using namespace std;

// ======================================================================
//   Utilidades
// ======================================================================
// emit escribe asm y lo guarda por linea en asmByLine para la visualizacion
// sizeOfType usa slots de 8 bytes para evitar solapamientos; is32Bit decide movq/movl

static bool tryParseLong(const string& s, long long& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        out = stoll(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

static string jsonEscape(const string& s) {
    string r;
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

static int sizeOfType(const string& t) {
    if (t == "bool") return 1;
    if (t == "int" || t == "unsigned int") return 4;
    if (t == "float") return 4;
    return 8; // long y por defecto
}

static bool is32Bit(const string& t) {
    return (t == "bool" || t == "int" || t == "unsigned int" || t == "float");
}

static bool isUnsigned(const string& t) {
    return (t == "unsigned int");
}

static bool isFloatType(const string& t) {
    return (t == "float");
}

static string getVarType(const map<string, FrameVar>& vars, const string& name) {
    auto it = vars.find(name);
    if (it != vars.end()) return it->second.type;
    return "int";
}

static string findGlobalType(const Frame& globals, const string& name) {
    for (const auto& fv : globals.vars) {
        if (fv.name == name) return fv.type;
    }
    return "int";
}

// ======================================================================
//   Métodos accept del AST
// ======================================================================

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
int ForStm::accept(Visitor* visitor)    { return visitor->visit(this); }

// ======================================================================
//   GenCodeVisitor
// ======================================================================

int GenCodeVisitor::generar(Program* program) {
    program->accept(this);
    saveStack();
    saveAsmMap();
    return 0;
}

void GenCodeVisitor::emit(const string& instr, int lineOverride) {
    int line = (lineOverride >= 0) ? lineOverride : currentLine;
    out << instr << endl;
    // guardamos tambien prologo (-1) para que aparezca en front
    if (line >= -1) {
        asmByLine[line].push_back(instr);
    }
}

void GenCodeVisitor::saveStack() {
    if (stackPath.empty()) return;

    // si no hay snapshots, al menos guardar globals
    if (snapshots.empty() && !globalFrame.vars.empty()) {
        snapshots.push_back(Snapshot{"globals", globalFrame.vars, 0, snapshotCounter++, "global"});
    }

    ofstream json(stackPath, ios::trunc);
    if (!json.is_open()) return;

    json << "[";
    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto& fr = snapshots[i];
        json << "{\"label\":\"" << jsonEscape(fr.label) << "\",\"line\":" << fr.line << ",\"idx\":" << fr.idx
             << ",\"func\":\"" << jsonEscape(fr.func) << "\",\"vars\":[";
        for (size_t v = 0; v < fr.vars.size(); ++v) {
            const auto& var = fr.vars[v];
            json << "{\"name\":\"" << jsonEscape(var.name) << "\",\"value\":\"" << jsonEscape(var.value) << "\","
                << "\"offset\":" << var.offset << ",\"type\":\"" << jsonEscape(var.type) << "\"}";
            if (v + 1 < fr.vars.size()) json << ",";
        }
        json << "]}";
        if (i + 1 < snapshots.size()) json << ",";
    }
    json << "]";
}

void GenCodeVisitor::saveAsmMap() {
    if (stackPath.empty()) return;
    // asmByLine sale de emit: agrupa instrucciones por linea de codigo fuente
    // se escribe en stackPath+".asm.json" para que el front lo use directo
    string asmMapPath = stackPath + ".asm.json";
    ofstream json(asmMapPath, ios::trunc);
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

// Pre-pase: asignar offsets a todas las variables locales
int GenCodeVisitor::preAsignarOffsets(Body* body, int startOffset) {
    int localOffset = startOffset;
    for (auto dec : body->declarations) {
        for (const auto& var : dec->vars) {
            if (usedVars.count(var)){
                int sz = sizeOfType(dec->type);
                int align = (sz == 8) ? 8 : 4;
                localOffset -= sz;
                int misalign = (-localOffset) % align;
                if (misalign != 0) localOffset -= (align - misalign);
                env.add_var(var, localOffset);
                typeEnv.add_var(var, dec->type);
            localOffset -= sz;
            }
            
        }
    }
    for (auto s : body->StmList) {
        if (auto ifs = dynamic_cast<IfStm*>(s)) {
            if (ifs->then) localOffset = preAsignarOffsets(ifs->then, localOffset);
            if (ifs->els)  localOffset = preAsignarOffsets(ifs->els,  localOffset);
        } else if (auto wh = dynamic_cast<WhileStm*>(s)) {
            if (wh->b) localOffset = preAsignarOffsets(wh->b, localOffset);
        } else if (auto fs = dynamic_cast<ForStm*>(s)) {
            if (fs->b) localOffset = preAsignarOffsets(fs->b, localOffset);
        }
    }
    return localOffset;
}

int GenCodeVisitor::visit(Program* program) {
    currentLine = -1;
    env.add_level();
    typeEnv.add_level();
    emit(".data");
    emit("print_int: .string \"%d \\n\"");
    emit("print_uint: .string \"%u \\n\"");
    emit("print_long: .string \"%ld \\n\"");
    emit("print_float: .string \"%f \\n\"");
    emit("print_bool: .string \"%d \\n\"");

    // Declaraciones globales
    for (auto dec : program->vdlist) {
        dec->accept(this);
    }
    for (auto it = memoriaGlobal.begin(); it != memoriaGlobal.end(); ++it) {
        emit(it->first + ": .quad 0");
    }

    emit(".text");
    for (auto dec : program->fdlist) {
        dec->accept(this);
    }
    emit(".section .note.GNU-stack,\"\",@progbits");
    env.remove_level();
    typeEnv.remove_level();
    return 0;
}

int GenCodeVisitor::visit(VarDec* vd) {
    currentLine = vd->line;
    for (auto& var : vd->vars) {
        if (!entornoFuncion) {
            memoriaGlobal[var] = true;
            globalTypes[var] = vd->type;
            FrameVar fv{var, 0, vd->type, "?"};
            globalFrame.vars.push_back(fv);
        } else {
            if (env.check(var) && currentFrame.label != "none") {
                int off = env.lookup(var);
                FrameVar fv{var, off, vd->type, "?"};
                currentFrame.vars.push_back(fv);
                currentVars[var] = fv;
                snapshot("decl " + var, vd->line);
            }
        }
    }
    return 0;
}

// Helpers de carga/almacenamiento
static string movLoad(const string& t) {
    if (t == "bool") return " movzbq ";
    if (is32Bit(t)) return " movl ";
    return " movq ";
}
static string movStore(const string& t) {
    if (t == "bool") return " movb ";
    if (is32Bit(t)) return " movl ";
    return " movq ";
}

int GenCodeVisitor::visit(NumberExp* exp) {
    string t = Type::type_to_string(exp->literalType);
    if (isFloatType(t)) {
        union { float f; uint32_t u; } fb;
        fb.f = static_cast<float>(exp->fvalue);
        emit(" movl $" + to_string(fb.u) + ", %eax");
        emit(" movd %eax, %xmm0");
        return 0;
    }
    if (t == "bool") {
        emit(" movb $" + to_string(exp->value) + ", %al");
        emit(" movzbq %al, %rax");
        return 0;
    }
    if (is32Bit(t)) {
        emit(" movl $" + to_string(exp->value) + ", %eax");
    } else {
        emit(" movq $" + to_string(exp->value) + ", %rax");
    }
    return 0;
}

int GenCodeVisitor::visit(BoolExp* exp) {
    emit(" movb $" + to_string(exp->valor) + ", %al");
    emit(" movzbq %al, %rax");
    return 0;
}

int GenCodeVisitor::visit(IdExp* exp) {
    string t = typeEnv.check(exp->value) ? typeEnv.lookup(exp->value) : findGlobalType(globalFrame, exp->value);
    if (isFloatType(t)) {
        if (memoriaGlobal.count(exp->value))
            emit(" movss " + exp->value + "(%rip), %xmm0");
        else
            emit(" movss " + to_string(env.lookup(exp->value)) + "(%rbp), %xmm0");
        return 0;
    }
    if (t == "bool") {
        if (memoriaGlobal.count(exp->value))
            emit(" movzbq " + exp->value + "(%rip), %rax");
        else
            emit(" movzbq " + to_string(env.lookup(exp->value)) + "(%rbp), %rax");
        return 0;
    }
    bool use32 = is32Bit(t);
    if (memoriaGlobal.count(exp->value)) {
        emit(string(use32 ? " movl " : " movq ") + exp->value + "(%rip), " + (use32 ? "%eax" : "%rax"));
    } else {
        int off = env.lookup(exp->value);
        emit(string(use32 ? " movl " : " movq ") + to_string(off) + "(%rbp), " + (use32 ? "%eax" : "%rax"));
    }
    return 0;
}

int GenCodeVisitor::visit(BinaryExp* exp) {
    string vstr = constEval(exp);
    long long v;
    if (tryParseLong(vstr, v)) {
        exp->cont = 1;
        exp->valor = static_cast<int>(v);
        if (v >= INT32_MIN && v <= INT32_MAX) {
            emit(" movl $" + to_string(v) + ", %eax");
        } else {
            emit(" movq $" + to_string(v) + ", %rax");
        }
        return 0;
    }

    bool leftLit  = dynamic_cast<NumberExp*>(exp->left) || dynamic_cast<BoolExp*>(exp->left);
    bool rightLit = dynamic_cast<NumberExp*>(exp->right) || dynamic_cast<BoolExp*>(exp->right);
    if (leftLit && rightLit) {
        string lstr = constEval(exp->left);
        string rstr = constEval(exp->right);
        long long lval, rval;
        if (tryParseLong(lstr, lval) && tryParseLong(rstr, rval)) {
            long long res = 0;
            bool ok = true;
            switch (exp->op) {
                case PLUS_OP:  res = lval + rval; break;
                case MINUS_OP: res = lval - rval; break;
                case MUL_OP:   res = lval * rval; break;
                case DIV_OP:   if (rval == 0) ok = false; else res = lval / rval; break;
                case POW_OP:
                    if (rval < 0) ok = false;
                    else {
                        res = 1;
                        for (long long i = 0; i < rval; ++i) res *= lval;
                    }
                    break;
                case LE_OP:    res = (lval < rval) ? 1 : 0; break;
                default: ok = false; break;
            }
            if (ok) {
                exp->cont = 1;
                exp->valor = static_cast<int>(res);
                if (res >= INT32_MIN && res <= INT32_MAX) {
                    emit(" movl $" + to_string(res) + ", %eax");
                } else {
                    emit(" movq $" + to_string(res) + ", %rax");
                }
                return 0;
            }
        }
    }
    
    
    // Evaluar left
    


    exp->left->accept(this);
    // Guardar resultado left
    Type::TType lt = exp->left->inferredType;
    Type::TType rt = exp->right->inferredType;
    bool floatOp = (lt == Type::FLOAT) || (rt == Type::FLOAT);
    if (floatOp) {
        emit(" subq $16, %rsp"); // espacio para guardar xmm0
        emit(" movdqu %xmm0, (%rsp)");
        exp->right->accept(this); // right en xmm0
        emit(" movdqu %xmm0, %xmm1");
        emit(" movdqu (%rsp), %xmm0"); // left en xmm0
        emit(" addq $16, %rsp");
        switch (exp->op) {
            case PLUS_OP: emit(" addss %xmm1, %xmm0"); break;
            case MINUS_OP: emit(" subss %xmm1, %xmm0"); break;
            case MUL_OP: emit(" mulss %xmm1, %xmm0"); break;
            case DIV_OP: emit(" divss %xmm1, %xmm0"); break;
            case LE_OP:
                emit(" ucomiss %xmm1, %xmm0");
                emit(" setb %al");
                emit(" movzbq %al, %rax");
                break;
            default: break;
        }
        return 0;
    }

    emit(" pushq %rax");      // left en stack
    exp->right->accept(this); // right en %rax
    emit(" movq %rax, %rcx"); // right en rcx
    emit(" popq %rax");       // left en rax

    bool use32 = (lt == Type::INT || lt == Type::UINT) && (rt == Type::INT || rt == Type::UINT);
    bool unsignedOp = (lt == Type::UINT || rt == Type::UINT);

    switch (exp->op) {
        case PLUS_OP:
            emit((use32 ? " addl %ecx, %eax" : " addq %rcx, %rax"));
            break;
        case MINUS_OP:
            emit((use32 ? " subl %ecx, %eax" : " subq %rcx, %rax"));
            break;
        case MUL_OP:
            emit((use32 ? " imull %ecx, %eax" : " imulq %rcx, %rax"));
            break;
        case DIV_OP:
            if (use32) {
                emit(" cltd");
                emit(" idivl %ecx");
            } else {
                emit(" cqto");
                emit(" idivq %rcx");
            }
            break;
        case LE_OP:
            emit((use32 ? " cmpl %ecx, %eax" : " cmpq %rcx, %rax"));
            emit(" movl $0, %eax");
            emit(unsignedOp ? " setb %al" : " setl %al");
            emit(" movzbq %al, %rax");
            break;
        default:
            break;
    }
    return 0;
}

int GenCodeVisitor::visit(TernaryExp* exp) {
    int label = labelcont++;
    exp->condition->accept(this);
    emit(" cmpq $0, %rax");
    emit(" je ternary_else_" + to_string(label));
    exp->thenExp->accept(this);
    emit(" jmp ternary_end_" + to_string(label));
    emit("ternary_else_" + to_string(label) + ":");
    exp->elseExp->accept(this);
    emit("ternary_end_" + to_string(label) + ":");
    return 0;
}

int GenCodeVisitor::visit(AssignStm* stm) {
    currentLine = stm->line;
    stm->e->accept(this);
    string vtype = typeEnv.check(stm->id) ? typeEnv.lookup(stm->id) : globalTypes[stm->id];
    string store = movStore(vtype);
    if (memoriaGlobal.count(stm->id)) {
        if (isFloatType(vtype)) emit(" movss %xmm0, " + stm->id + "(%rip)");
        else emit(store + (store == " movb " ? "%al" : (is32Bit(vtype) ? "%eax" : "%rax")) + ", " + stm->id + "(%rip)");
    } else {
        int off = env.lookup(stm->id);
        if (isFloatType(vtype)) emit(" movss %xmm0, " + to_string(off) + "(%rbp)");
        else emit(store + (store == " movb " ? "%al" : (is32Bit(vtype) ? "%eax" : "%rax")) + ", " + to_string(off) + "(%rbp)");
    }
    if (currentVars.count(stm->id)) currentVars[stm->id].value = constEval(stm->e);
    if (entornoFuncion && currentFrame.label != "none") snapshot("assign " + stm->id, stm->line);
    return 0;
}

int GenCodeVisitor::visit(PrintStm* stm) {
    currentLine = stm->line;
    stm->e->accept(this);
    // determinar tipo
    string t = "int";
    if (auto num = dynamic_cast<NumberExp*>(stm->e)) t = Type::type_to_string(num->literalType);
    else if (auto id = dynamic_cast<IdExp*>(stm->e)) t = typeEnv.check(id->value) ? typeEnv.lookup(id->value) : globalTypes[id->value];
    string fmt = "print_int";
    if (t == "unsigned int") fmt = "print_uint";
    else if (t == "long") fmt = "print_long";
    else if (t == "float") fmt = "print_float";
    else if (t == "bool") fmt = "print_bool";

    if (t == "float") {
        emit(" cvtss2sd %xmm0, %xmm0"); // promocionar a double para printf
        emit(" movl $1, %eax");
        emit(" leaq " + fmt + "(%rip), %rdi");
    } else {
        emit(" movq %rax, %rsi");
        emit(" leaq " + fmt + "(%rip), %rdi");
        emit(" movl $0, %eax");
    }
    emit(" call printf@PLT");
    if (entornoFuncion && currentFrame.label != "none") {
        snapshot("print", stm->line);
    }
    return 0;
}

int GenCodeVisitor::visit(Body* b) {
    for (auto dec : b->declarations) {
        dec->accept(this);
    }
    // Inicializadores
    for (auto dec : b->declarations) {
        auto varIt = dec->vars.begin();
        for (size_t i = 0; i < dec->initializers.size() && varIt != dec->vars.end(); ++i, ++varIt) {
            Exp* init = dec->initializers[i];
            if (!init) continue;
            const string& varName = *varIt;
            string vtype = typeEnv.check(varName) ? typeEnv.lookup(varName) : findGlobalType(globalFrame, varName);
            init->accept(this);
            if (isFloatType(vtype)) {
                if (memoriaGlobal.count(varName)) emit(" movss %xmm0, " + varName + "(%rip)");
                else emit(" movss %xmm0, " + to_string(env.lookup(varName)) + "(%rbp)");
            } else {
                string store = movStore(vtype);
                string reg = (store == " movb " ? "%al" : (is32Bit(vtype) ? "%eax" : "%rax"));
                if (memoriaGlobal.count(varName))
                    emit(store + reg + ", " + varName + "(%rip)");
                else
                    emit(store + reg + ", " + to_string(env.lookup(varName)) + "(%rbp)");
            }
            if (currentVars.count(varName)) currentVars[varName].value = constEval(init);
        }
    }
    for (auto s : b->StmList) {
        s->accept(this);
    }
    return 0;
}

int GenCodeVisitor::visit(IfStm* stm) {
    currentLine = stm->line;

    if (entornoFuncion && currentFrame.label != "none") snapshot("if", stm->line);

    string cval = constEval(stm->condition);
    long long v;

    if (tryParseLong(cval, v)) {
        if (v != 0) {
            if (stm->then) stm->then->accept(this);
        } else {
            if (stm->els) stm->els->accept(this);
        }
        return 0;
    }
    else{
        int label = labelcont++;
        stm->condition->accept(this);
        emit(" cmpq $0, %rax");
        emit(" je else_" + to_string(label));
        stm->then->accept(this);
        emit(" jmp endif_" + to_string(label));
        emit("else_" + to_string(label) + ":");
        if (stm->els) stm->els->accept(this);
        emit("endif_" + to_string(label) + ":");
    }
    return 0;
    
}

int GenCodeVisitor::visit(WhileStm* stm) {
    currentLine = stm->line;
    if (entornoFuncion && currentFrame.label != "none") snapshot("while", stm->line);
    int label = labelcont++;
    emit("while_" + to_string(label) + ":");
    stm->condition->accept(this);
    emit(" cmpq $0, %rax");
    emit(" je endwhile_" + to_string(label));
    stm->b->accept(this);
    emit(" jmp while_" + to_string(label));
    emit("endwhile_" + to_string(label) + ":");
    return 0;
}

int GenCodeVisitor::visit(ReturnStm* stm) {
    currentLine = stm->line;
    if (stm->e) stm->e->accept(this);
    emit(" jmp .end_" + nombreFuncion);
    if (entornoFuncion && currentFrame.label != "none") {
        snapshot("return", stm->line);
    }
    return 0;
}

int GenCodeVisitor::visit(ForStm* stm) {
    currentLine = stm->line;
    if (entornoFuncion && currentFrame.label != "none") snapshot("for", stm->line);
    env.add_level();
    typeEnv.add_level();
    if (stm->init) stm->init->accept(this);
    int label = labelcont++;
    emit("for_" + to_string(label) + ":");
    if (stm->condition) {
        stm->condition->accept(this);
        emit(" cmpq $0, %rax");
        emit(" je endfor_" + to_string(label));
    }
    if (stm->b) stm->b->accept(this);
    if (stm->step) stm->step->accept(this);
    emit(" jmp for_" + to_string(label));
    emit("endfor_" + to_string(label) + ":");
    env.remove_level();
    typeEnv.remove_level();
    return 0;
}

int GenCodeVisitor::visit(FunDec* f) {
    currentLine = -1;
    entornoFuncion = true;
    env.clear();
    typeEnv.clear();
    env.add_level();
    typeEnv.add_level();
    offset = -8;
    nombreFuncion = f->nombre;
    currentFrame = Frame{f->nombre, {}};

    vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    vector<string> argRegsXmm = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5"};

    int funcLine = -1;
    if (f->cuerpo) {
        if (!f->cuerpo->declarations.empty()) funcLine = f->cuerpo->declarations.front()->line;
        else if (!f->cuerpo->StmList.empty()) funcLine = f->cuerpo->StmList.front()->line;
    }
    currentLine = funcLine;
    emit(".globl " + f->nombre, funcLine-1);
    emit(f->nombre + ":", funcLine-1);
    emit(" pushq %rbp", funcLine-1);
    emit(" movq %rsp, %rbp", funcLine-1);

    // Preasignar offsets de locales
    int funcOffset = offset;
    // Parametros
    for (int i = 0; i < (int)f->Pnombres.size(); ++i) {
        string ptype = (i < (int)f->Ptipos.size()) ? f->Ptipos[i] : "int";
        int sz = sizeOfType(ptype);
        int align = (sz == 8) ? 8 : 4;
        int misalign = (-funcOffset) % align;
        if (misalign != 0) funcOffset -= (align - misalign);
        env.add_var(f->Pnombres[i], funcOffset);
        typeEnv.add_var(f->Pnombres[i], ptype);
        FrameVar fv{f->Pnombres[i], funcOffset, ptype, "?"};
        currentFrame.vars.push_back(fv);
        currentVars[fv.name] = fv;
        funcOffset -= sz;
    }
    usedVars.clear();
    if (f->cuerpo) {
        markUsedVarsInBody(f->cuerpo);

        for (auto &dec : f->cuerpo->declarations) {
            for (auto init : dec->initializers) {
                if (init) markUsedVars(init);
            }
        }

        funcOffset = preAsignarOffsets(f->cuerpo, funcOffset);
    }
    int totalStack = -funcOffset - 8;
    int align16 = totalStack % 16;
    if (align16 != 0) totalStack += (16 - align16);
    if (totalStack > 0) {
        emit(" subq $" + to_string(totalStack) + ", %rsp", funcLine-1);
        offset = -8 - totalStack;
    }

    // Guardar parametros en sus slots
    int floatIdx = 0, intIdx = 0;
    vector<string> argRegs32 = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    vector<string> argRegs8  = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
    for (int i = 0; i < (int)f->Pnombres.size(); ++i) {
        string ptype = typeEnv.lookup(f->Pnombres[i]);
        int destOff = env.lookup(f->Pnombres[i]);
        if (isFloatType(ptype)) {
            if (floatIdx < (int)argRegsXmm.size())
                emit(" movss " + argRegsXmm[floatIdx] + ", " + to_string(destOff) + "(%rbp)", funcLine-1);
            floatIdx++;
        } else {
            string movInstr = movStore(ptype);
            string srcReg;
            if (movInstr == " movb ")
                srcReg = (intIdx < (int)argRegs8.size()) ? argRegs8[intIdx] : "%dil";
            else if (movInstr == " movl ")
                srcReg = (intIdx < (int)argRegs32.size()) ? argRegs32[intIdx] : "%edi";
            else
                srcReg = (intIdx < (int)argRegs.size()) ? argRegs[intIdx] : "%rdi";

            emit(movInstr + srcReg + ", " + to_string(destOff) + "(%rbp)", funcLine-1);
            intIdx++;
        }
    }

    // snapshot inicial: prolog (linea -1 y/o justo antes de la declaracion) para cualquier funcion
    snapshot("prolog", funcLine > 0 ? funcLine - 1 : -1);

    if (f->cuerpo) f->cuerpo->accept(this);

    emit(".end_" + f->nombre + ":");
    emit("leave");
    emit("ret");

    entornoFuncion = false;
    env.remove_level();
    typeEnv.remove_level();
    currentVars.clear();
    currentFrame = Frame{"none", {}};
    return 0;
}

int GenCodeVisitor::visit(FcallExp* exp) {
    vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    vector<string> argRegsXmm = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5"};
    int intIdx = 0, floatIdx = 0;
    for (auto* a : exp->argumentos) {
        a->accept(this);
        Type::TType at = a->inferredType;
        if (at == Type::FLOAT) {
            if (floatIdx < (int)argRegsXmm.size())
                emit(" movaps %xmm0, " + argRegsXmm[floatIdx]);
            floatIdx++;
        } else {
            if (intIdx < (int)argRegs.size())
                emit(" movq %rax, " + argRegs[intIdx]);
            intIdx++;
        }
    }
    if (floatIdx > 0) emit(" movl $" + to_string(floatIdx) + ", %eax"); else emit(" movl $0, %eax");
    emit(" call " + exp->nombre);
    return 0;
}

void GenCodeVisitor::snapshot(const string& label, int line) {
    if (currentFrame.label == "none") return;
    out << "# SNAPIDX " << snapshotCounter << " " << label;
    if (line > 0) out << " line " << line;
    out << "\n";
    vector<FrameVar> vars;
    for (auto &fv : currentFrame.vars) {
        auto it = currentVars.find(fv.name);
        if (it != currentVars.end()) fv.value = it->second.value;
        vars.push_back(fv);
    }
    sort(vars.begin(), vars.end(), [](const FrameVar& a, const FrameVar& b){ return a.offset > b.offset; });
    snapshots.push_back(Snapshot{label, vars, line, snapshotCounter, nombreFuncion.empty() ? "global" : nombreFuncion});
    snapshotCounter++;
}

string GenCodeVisitor::constEval(Exp* e) {
    if (auto num = dynamic_cast<NumberExp*>(e)) {
        return to_string(num->value);
    }
    if (auto b = dynamic_cast<BoolExp*>(e)) {
        return to_string(b->valor);
    }
    if (auto id = dynamic_cast<IdExp*>(e)) {
        auto it = currentVars.find(id->value);
        if (it != currentVars.end()) return it->second.value;
        return "?";
    }
    if (auto bin = dynamic_cast<BinaryExp*>(e)) {
        string lstr = constEval(bin->left);
        string rstr = constEval(bin->right);
        long long lval, rval;
        if (!tryParseLong(lstr, lval) || !tryParseLong(rstr, rval)) return "?";
        long long res = 0;
        switch (bin->op) {
            case PLUS_OP:  res = lval + rval; break;
            case MINUS_OP: res = lval - rval; break;
            case MUL_OP:   res = lval * rval; break;
            case DIV_OP:   if (rval == 0) return "?"; res = lval / rval; break;
            case POW_OP:
                res = 1; for (long long i = 0; i < rval; ++i) res *= lval; break;
            case LE_OP:    res = (lval < rval) ? 1 : 0; break;
            default: return "?";
        }
        return to_string(res);
    }
    if (dynamic_cast<FcallExp*>(e)) return "call";
    return "?";
}

void GenCodeVisitor::markUsedVars(Exp* e) {
    if (auto id = dynamic_cast<IdExp*>(e)) {
        usedVars.insert(id->value);
    } else if (auto bin = dynamic_cast<BinaryExp*>(e)) {
        markUsedVars(bin->left);
        markUsedVars(bin->right);
    } else if (auto tern = dynamic_cast<TernaryExp*>(e)) {
        markUsedVars(tern->condition);
        markUsedVars(tern->thenExp);
        markUsedVars(tern->elseExp);
    } else if (auto fcall = dynamic_cast<FcallExp*>(e)) {
        for (auto arg : fcall->argumentos)
            markUsedVars(arg);
    }
}

void GenCodeVisitor::markUsedVarsInBody(Body* b) {
    for (auto stm : b->StmList) {
        if (auto assign = dynamic_cast<AssignStm*>(stm)) {
            markUsedVars(assign->e);
        } else if (auto print = dynamic_cast<PrintStm*>(stm)) {
            markUsedVars(print->e);
        } else if (auto ifs = dynamic_cast<IfStm*>(stm)) {
            markUsedVars(ifs->condition);
            if (ifs->then) markUsedVarsInBody(ifs->then);
            if (ifs->els) markUsedVarsInBody(ifs->els);
        } else if (auto wh = dynamic_cast<WhileStm*>(stm)) {
            markUsedVars(wh->condition);
            if (wh->b) markUsedVarsInBody(wh->b);
        } else if (auto ret = dynamic_cast<ReturnStm*>(stm)) {
            if (ret->e) markUsedVars(ret->e);
        } else if (auto fs = dynamic_cast<ForStm*>(stm)) {
            if (fs->condition) markUsedVars(fs->condition);
            // fs->step es una Stm* (p.ej. AssignStm). manejarlo según su tipo:
            if (fs->step) {
                if (auto s_assign = dynamic_cast<AssignStm*>(fs->step)) {
                    markUsedVars(s_assign->e);
                } else if (auto s_print = dynamic_cast<PrintStm*>(fs->step)) {
                    markUsedVars(s_print->e);
                } else if (auto s_ret = dynamic_cast<ReturnStm*>(fs->step)) {
                    if (s_ret->e) markUsedVars(s_ret->e);
                } else if (auto s_if = dynamic_cast<IfStm*>(fs->step)) {
                    markUsedVars(s_if->condition);
                    if (s_if->then) markUsedVarsInBody(s_if->then);
                    if (s_if->els) markUsedVarsInBody(s_if->els);
                } // añadir más casos si tu lenguaje permite otros tipos de step
            }
            if (fs->b) markUsedVarsInBody(fs->b);
        }
    }
}
