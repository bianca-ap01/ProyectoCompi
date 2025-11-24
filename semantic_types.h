#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <iostream>
#include <string>
using namespace std;

// ===========================================================
//  Representación de tipos básicos del lenguaje
// ===========================================================

class Type {
public:
    enum TType { NOTYPE, VOID, INT, FLOAT, UINT, LONG, BOOL, AUTO };
    static const char* type_names[8];

    TType ttype;

    Type() : ttype(NOTYPE) {}
    Type(TType tt) : ttype(tt) {}

    // Comparación de tipos
    bool match(Type* t) const {
        return this->ttype == t->ttype;
    }

    // Asignación de tipo básico desde string
    bool set_basic_type(const string& s) {
        TType tt = string_to_type(s);
        if (tt == NOTYPE) return false;
        ttype = tt;
        return true;
    }

    // Conversión string 
    static TType string_to_type(const string& s) {
        if (s == "int") return INT;
        if (s == "long") return LONG;
        if (s == "long int") return LONG;
        if (s == "float") return FLOAT;
        if (s == "void") return VOID;
        if (s == "unsigned int") return UINT;
        if (s == "bool") return BOOL;
        if (s == "auto") return AUTO;
        return NOTYPE;
    }


};

inline const char* Type::type_names[8] = { "notype", "void", "int", "float", "unsigned int", "long", "bool", "auto"};

#endif // SEMANTIC_TYPES_H
