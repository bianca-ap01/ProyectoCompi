#ifndef DEBUGGER_H
#define DEBUGGER_H
#include <iterator> // <-- Añade esto si no está
#include "visitor.h"
#include "environment.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // <-- ¡Necesario para std::find!
#include <unordered_set>
using namespace std;

class DebuggerVisitor : public Visitor {
public:
    // SIMULACIÓN DE MEMORIA
    Environment<int> envValues;       // Guarda el VALOR real (ej: 10, 20)
    Environment<string> envOffsets;   // Guarda la DIRECCIÓN simulada (ej: "-8(%rbp)")
    vector<string> currentScopeVars;
    vector<string> visibleVars;
    int stepCounter = 0;
    // VARIABLES DE ESTADO (Imitando a GenCodeVisitor)
    int offset = -8;                  // El stack pointer simulado
    bool entornoFuncion = false;      // Para saber si estamos en local o global

    // Helper para imprimir el log que lee Python
    /*void log(string accion, string var, string mem, int val) {
        cout << "TRACE|" << accion << "|" << var << "|" << mem << "|" << val << endl;
    }*/

    // --- FUNCIÓN CLAVE: DIBUJAR ESTADO ACTUAL ---
    // Crea un nodo de Graphviz que muestra la tabla de memoria actual
    void dumpState(string title, string highlightVar = "") {
        int id = stepCounter++;
        
        // Inicio de la tabla HTML
        cout << "  step" << id << " [label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" BGCOLOR=\"#ffffff\">" << endl;
        
        // Cabecera del paso
        cout << "    <TR><TD COLSPAN=\"3\" BGCOLOR=\"#333333\"><FONT COLOR=\"white\"><B>" 
            << title << "</B></FONT></TD></TR>" << endl;
        cout << "    <TR><TD BGCOLOR=\"#eeeeee\"><B>Var</B></TD><TD BGCOLOR=\"#eeeeee\"><B>Addr</B></TD><TD BGCOLOR=\"#eeeeee\"><B>Val</B></TD></TR>" << endl;

        // Filas de variables
        if (visibleVars.empty()) {
            cout << "    <TR><TD COLSPAN=\"3\">Wait...</TD></TR>" << endl;
        } else {
            // Usamos un set para no imprimir variables repetidas
            // y un mapa para encontrar la última instancia válida (shadowing)
            unordered_set<string> printedVars;
            unordered_map<string, int> lastIndex;
            
            for (int k = 0; k < visibleVars.size(); ++k) {
                lastIndex[visibleVars[k]] = k;
            }

            for (int k = 0; k < visibleVars.size(); ++k) {
                string var = visibleVars[k];
                
                // Solo mostramos la última instancia (la más interna/reciente)
                if (k != lastIndex[var]) continue;
                
                // Evitamos duplicados si por alguna razón quedaron residuos
                if (printedVars.count(var)) continue;
                printedVars.insert(var);

                string mem = envOffsets.check(var) ? envOffsets.lookup(var) : "?";
                int val = envValues.check(var) ? envValues.lookup(var) : 0;
                
                string attr = (var == highlightVar) ? " BGCOLOR=\"yellow\"" : "";
                
                cout << "    <TR>"
                     << "<TD" << attr << ">" << var << "</TD>"
                     << "<TD" << attr << ">" << mem << "</TD>"
                     << "<TD" << attr << ">" << val << "</TD>"
                     << "</TR>" << endl;
            }
        
        }
        cout << "  </TABLE>>];" << endl;

        // Flecha conectando con el paso anterior
        if (id > 0) {
            cout << "  step" << (id-1) << " -> step" << id << ";" << endl;
        }
    }
    // ==========================================
    // 1. ESTRUCTURA DEL PROGRAMA (Igual que GenCodeVisitor)
    // ==========================================
    int visit(Program* p) override {
        dumpState("Start");        // Inicializamos entornos
        envValues.add_level();
        envOffsets.add_level();

        // 1. Declaraciones Globales (vdlist)
        // En tu visitor.cpp las globales se tratan diferente (.quad 0).
        // Aquí las marcamos como "GLOBAL"
        entornoFuncion = false;
        for (auto* dec : p->vdlist) {
            dec->accept(this);
        }

        // 2. Ejecutar SOLO el main (fdlist)
        // A diferencia del compilador que genera código para todas,
        // el debugger busca el punto de entrada 'main'.
        bool mainFound = false;
        for (auto* func : p->fdlist) {
            if (func->nombre == "main") {
                func->accept(this);
                mainFound = true;
                break;
            }
        }

        envValues.remove_level();
        envOffsets.remove_level();
        return 0;
    }

    // ==========================================
    // 2. FUNCIONES (Manejo del Stack Frame)
    // ==========================================
    int visit(FunDec* fd) override {
        // Nivel 1: Función (Main)
        int savedOffset = offset; 
        offset = -8; 
        entornoFuncion = true;
        
        // Guardar scope vars anterior (probablemente globales)
        vector<string> savedScopeVars = currentScopeVars;
        currentScopeVars.clear(); // Limpiar para nuevas locales de función

        envValues.add_level(); envOffsets.add_level();
        dumpState("Enter " + fd->nombre);
        
        if (fd->cuerpo) fd->cuerpo->accept(this);
        
        // Limpieza de variables locales de la función
        for (const string& var : currentScopeVars) {
            for (int k = visibleVars.size() - 1; k >= 0; --k) {
                if (visibleVars[k] == var) {
                    visibleVars.erase(visibleVars.begin() + k);
                    break;
                }
            }
        }

        envValues.remove_level(); envOffsets.remove_level();
        entornoFuncion = false; 
        offset = savedOffset;
        currentScopeVars = savedScopeVars; // Restaurar
        return 0;
    }

    int visit(Body* b) override {
        // El Body en tu parser generalmente NO crea nuevo nivel de stack en C simple,
        // pero sí un scope lógico. Lo manejamos transparente aquí.
        vector<string> savedVisibleVars = visibleVars;
        vector<string> savedScopeVars = currentScopeVars;
        int savedOffset = offset;
        currentScopeVars.clear();
        
        envValues.add_level();
        envOffsets.add_level();
        // Declaraciones locales (VarDec)
        for (auto* dec : b->declarations) {
            dec->accept(this);
        }
        // Sentencias
        for (auto* stm : b->StmList) {
            stm->accept(this);
        }
        // 1. Deshacer visibleVars para este scope
        // --- LIMPIEZA CORREGIDA (Eliminar desde el final) ---
        for (const string& var : currentScopeVars) {
            // Buscamos de atrás hacia adelante para eliminar la instancia más reciente
            for (int i = visibleVars.size() - 1; i >= 0; --i) {
                if (visibleVars[i] == var) {
                    visibleVars.erase(visibleVars.begin() + i);
                    break; // Eliminamos solo una instancia y pasamos a la siguiente variable
                }
            }
        }
        offset = savedOffset;

        envValues.remove_level();
        envOffsets.remove_level();
        currentScopeVars = savedScopeVars;
        
        return 0;
    }

    // ==========================================
    // 3. DECLARACIÓN DE VARIABLES (Asignación de Offsets)
    // ==========================================
// En debugger.h (dentro de DebuggerVisitor)
int visit(VarDec* vd) override {
        auto initIt = vd->initializers.begin();

        for (const string& var : vd->vars) {
            string mem;
            if (entornoFuncion) {
                mem = to_string(offset) + "(%rbp)";
                offset -= 8; 
            } else {
                mem = var + "(%rip)"; 
            }
            
            int initialValue = 0;
            Exp* initializer = nullptr;
            
            if (initIt != vd->initializers.end()) {
                initializer = *initIt;
                ++initIt;
            }

            if (initializer) {
                initialValue = initializer->accept(this); 
            }
            
            envOffsets.add_var(var, mem);
            envValues.add_var(var, initialValue);
            
            visibleVars.push_back(var);
            currentScopeVars.push_back(var); 
            
            dumpState("Decl " + var, var); 
        }
        return 0;
    }

    // ==========================================
    // 4. SENTENCIAS Y EXPRESIONES
    // ==========================================
    
    int visit(AssignStm* stm) override {
        int val = stm->e ? stm->e->accept(this) : 0;
        
        // Verificamos si existe en envValues. 
        // Nota: Si envOffsets se desincronizó, esto lo arregla visualmente si envValues aún lo tiene.
        if (envValues.check(stm->id)) {
            envValues.update(stm->id, val);
            dumpState("Assign " + stm->id, stm->id);
        } else {
             // Fallback opcional: Si no se encuentra, es un error lógico o variable no declarada
             // cout << "TRACE|ERROR|Assign|VarNotFound|" << stm->id << endl;
        }
        return 0;
    }
    int visit(PrintStm* stm) override {
        int val = stm->e->accept(this);
        //cout << "OUTPUT|PRINT|CONSOLE|-|" << val << endl;
        return 0;
    }

    // Evaluador de Expresiones (Calculadora)
    int visit(BinaryExp* exp) override {
        int l = exp->left->accept(this);
        int r = exp->right->accept(this);
        switch(exp->op) {
            case PLUS_OP: return l + r;
            case MINUS_OP: return l - r;
            case MUL_OP: return l * r;
            case DIV_OP: return (r!=0)? l/r : 0;
            case LE_OP: return l < r;
            // Agrega otros si tienes (EQ, GT, etc)
            default: return 0;
        }
    }

    int visit(NumberExp* exp) override { return exp->value; }
    
    int visit(IdExp* exp) override { 
        return envValues.lookup(exp->value); 
    }

    // Estructuras de control (Evaluación real)
    int visit(IfStm* s) override {
        if (s->condition->accept(this)) {
            s->then->accept(this);
        } else if (s->els) {
            s->els->accept(this);
        }
        return 0;
    }

    int visit(WhileStm* s) override {
        while (s->condition->accept(this)) {
            s->b->accept(this);
        }
        return 0;
    }

        // Stubs (Vacíos para que compile, a menos que quieras implementarlos)
    int visit(TernaryExp* exp) override { 
        // condición
        int cond = exp->condition->accept(this);

        // 2. Si la condición es verdadera (distinta de cero), ejecutar el 'then'
        if (cond != 0) {
            return exp->thenExp->accept(this);
        }
        
        // 3. Si la condición es falsa (cero), ejecutar el 'else'
        return exp->elseExp->accept(this);    
    }

    int visit(FcallExp* exp) override { 
        int size = exp->argumentos.size();

        for (int i = 0; i < size; i++) {
            exp->argumentos[i]->accept(this);
        }

        // RAX ya tiene retorno
        return 0;
    }

    int visit(ReturnStm* stm) override { 
        if (stm->e) {
            stm->e->accept(this);
        }
    }

    int visit(ForStm* s) override {
        // 1. Ejecutar Inicialización (ej: int i = 0)
        envOffsets.add_level();
        envValues.add_level();
        if (s->init) s->init->accept(this);

        // 2. Bucle While simulado
        while (true) {
            // Evaluar condición (ej: i < 10)
            int cond = 1; 
            if (s->condition) cond = s->condition->accept(this);

            if (cond == 0) break; // Salir si es falso

            // Ejecutar Cuerpo (ej: x = x + i)
            if (s->b) s->b->accept(this);

            // Ejecutar Step (ej: i++)
            if (s->step) s->step->accept(this);
        }
        // 3. FINALIZAR SCOPE DEL FOR
        dumpState("Exit For Loop"); 
        
        // --- LIMPIEZA CORREGIDA (Eliminar desde el final) ---
        for (const string& var : currentScopeVars) {
            for (int i = visibleVars.size() - 1; i >= 0; --i) {
                if (visibleVars[i] == var) {
                    visibleVars.erase(visibleVars.begin() + i);
                    break; 
                }
            }
        }
        envOffsets.remove_level();
        envValues.remove_level();
        return 0;
    }
};

#endif