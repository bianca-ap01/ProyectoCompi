#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "visitor.h"
#include "environment.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class DebuggerVisitor : public Visitor {
public:
    // SIMULACIÓN DE MEMORIA
    Environment<int> envValues;       // Guarda el VALOR real (ej: 10, 20)
    Environment<string> envOffsets;   // Guarda la DIRECCIÓN simulada (ej: "-8(%rbp)")
    int stepCounter = 0;
    // VARIABLES DE ESTADO (Imitando a GenCodeVisitor)
    int offset = -8;                  // El stack pointer simulado
    bool entornoFuncion = false;      // Para saber si estamos en local o global

    // Helper para imprimir el log que lee Python
    void log(string accion, string var, string mem, int val) {
        cout << "TRACE|" << accion << "|" << var << "|" << mem << "|" << val << endl;
    }
    vector<string> visibleVars;

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
            for (const string& var : visibleVars) {
                string mem = envOffsets.check(var) ? envOffsets.lookup(var) : "?";
                int val = envValues.check(var) ? envValues.lookup(var) : 0;
                
                // Si es la variable que acaba de cambiar, la pintamos de AMARILLO
                string color = (var == highlightVar) ? "BGCOLOR=\"yellow\"" : "";
                
                cout << "    <TR>"
                        << "<TD " << color << ">" << var << "</TD>"
                        << "<TD " << color << ">" << mem << "</TD>"
                        << "<TD " << color << ">" << val << "</TD>"
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

        if (!mainFound) {
            cout << "TRACE|ERROR|SYSTEM|-|No main function found" << endl;
        }

        envValues.remove_level();
        envOffsets.remove_level();
        return 0;
    }

    // ==========================================
    // 2. FUNCIONES (Manejo del Stack Frame)
    // ==========================================
    int visit(FunDec* fd) override {
        // Al entrar a una función, reseteamos el offset local a -8
        // tal como lo hace tu visitor.cpp: "offset = -8;"
        int savedOffset = offset; // Guardamos el offset anterior por si acaso
        offset = -8;
        entornoFuncion = true;

        // Simulamos la entrada al scope de la función
        // NOTA: Tu visitor.cpp hace "env.add_level()"
        envValues.add_level(); 
        envOffsets.add_level(); // Nuevo nivel para variables locales
        // *Aquí deberíamos procesar argumentos si los hubiera, igual que visitor.cpp*
        // Por ahora saltamos directo al cuerpo.
        dumpState("Enter " + fd->nombre);
        fd->cuerpo->accept(this);

        // Al salir, limpiamos
        envValues.remove_level();
        envOffsets.remove_level();
        entornoFuncion = false;
        offset = savedOffset; // Restauramos (aunque al salir del main termina todo)
        return 0;
    }

    int visit(Body* b) override {
        // El Body en tu parser generalmente NO crea nuevo nivel de stack en C simple,
        // pero sí un scope lógico. Lo manejamos transparente aquí.
        
        // Declaraciones locales (VarDec)
        for (auto* dec : b->declarations) {
            dec->accept(this);
        }
        // Sentencias
        for (auto* stm : b->StmList) {
            stm->accept(this);
        }

        
        return 0;
    }

    // ==========================================
    // 3. DECLARACIÓN DE VARIABLES (Asignación de Offsets)
    // ==========================================
    int visit(VarDec* vd) override {
        for (string varName : vd->vars) {
            string memLoc;
            
            if (entornoFuncion) {
                // LÓGICA COPIADA DE visitor.cpp:
                // Asigna offset actual y luego resta 8.
                memLoc = to_string(offset) + "(%rbp)";
                
                // Guardamos dónde vive esta variable
                envOffsets.add_var(varName, memLoc);
                
                // Decrementamos el stack pointer simulado
                offset -= 8;
            } else {
                // Es global
                memLoc = varName + "(%rip)"; // Notación típica para globales
                
            }

            // Guardar datos
            envOffsets.add_var(varName, memLoc);
            envValues.add_var(varName, 0);
            
            // Agregar a la lista visible para dibujar
            visibleVars.push_back(varName);

            dumpState("Decl " + varName, varName);
        }
        return 0;
    }

    // ==========================================
    // 4. SENTENCIAS Y EXPRESIONES
    // ==========================================
    
    int visit(AssignStm* stm) override {
        // 1. Calcular el valor (Interpretación)
        int val = stm->e->accept(this);
        
        // 2. Actualizar memoria simulada
        envValues.update(stm->id, val);
        
        // 3. Recuperar dónde está guardada (para mostrarlo)
        string memLoc = envOffsets.lookup(stm->id);
        
        // 4. Log
        dumpState("Assign " + stm->id, stm->id);
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
        return 0;
        envOffsets.remove_level();
        envValues.remove_level();
    }
};

#endif