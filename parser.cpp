#include "parser.h"
#include <iostream>
#include <stdexcept>

using namespace std;

// =============================
// Constructor y helpers
// =============================

Parser::Parser(Scanner* sc) : scanner(sc), current(nullptr), previous(nullptr) {
    current = scanner->nextToken();
    if (current->type == Token::ERR) {
        throw runtime_error("Error léxico al iniciar el parser");
    }
}

bool Parser::isAtEnd() const {
    return current->type == Token::END;
}

bool Parser::check(Token::Type ttype) const {
    if (isAtEnd()) return false;
    return current->type == ttype;
}

bool Parser::advance() {
    if (!isAtEnd()) {
        Token* temp = current;
        previous = temp;
        current = scanner->nextToken();
        if (current->type == Token::ERR) {
            throw runtime_error("Error léxico");
        }
        return true;
    }
    return false;
}

bool Parser::match(Token::Type ttype) {
    if (check(ttype)) {
        advance();
        return true;
    }
    return false;
}

void Parser::error(const std::string& msg) {
    cerr << "Error de parseo: " << msg << endl;
    throw runtime_error(msg);
}

// =============================
// Utilidades de tipos
// =============================

bool Parser::isTypeStart() const {
    return check(Token::INT) || check(Token::UNSIGNED) ||
           check(Token::LONG) || check(Token::FLOAT);
}

TypeKind Parser::parseTypeSpec(std::string& outType) {
    if (match(Token::INT)) {
        outType = "int";
        return TYPE_INT;
    } else if (match(Token::FLOAT)) {
        outType = "float";
        return TYPE_FLOAT;
    } else if (match(Token::LONG)) {
        // Permitimos tanto "long" como "long int"
        if (match(Token::INT)) {
            outType = "long int";
        } else {
            outType = "long"; // lo tratamos igual que long int
        }
        return TYPE_LONG;
    } else if (match(Token::UNSIGNED)) {
        if (!match(Token::INT)) {
            error("Se esperaba 'int' después de 'unsigned'");
        }
        outType = "unsigned int";
        return TYPE_UINT;
    } else {
        error("Se esperaba un tipo (int, unsigned int, long, long int, float)");
        return TYPE_INT; // unreachable
    }
}

// =============================
// program -> (declaración)* EOF
// =============================

Program* Parser::parseProgram() {
    Program* prog = new Program();

    while (!isAtEnd()) {
        if (check(Token::END)) break;
        parseTopLevelDeclaration(prog);
    }

    cout << "Parser exitoso" << endl;
    return prog;
}

// =============================
// Declaraciones de nivel superior
// =============================

void Parser::parseTopLevelDeclaration(Program* prog) {
    if (check(Token::AUTO)) {
        VarDec* vd = parseAutoDeclaration();
        prog->vdlist.push_back(vd);
        return;
    }

    if (!isTypeStart()) {
        error("Se esperaba 'auto' o un tipo al inicio de una declaración");
    }

    string typeName;
    TypeKind kind = parseTypeSpec(typeName);

    if (!match(Token::ID)) {
        error("Se esperaba identificador después del tipo");
    }
    string name = previous->text;

    if (check(Token::LPAREN)) {
        // Declaración de función
        FunDec* fd = new FunDec();
        fd->kind   = kind;
        fd->type   = typeName;
        fd->nombre = name;

        match(Token::LPAREN);
        if (!check(Token::RPAREN)) {
            parseParamList(fd);
        }
        if (!match(Token::RPAREN)) {
            error("Se esperaba ')' al final de la lista de parámetros");
        }

        fd->cuerpo = parseBody();
        prog->fdlist.push_back(fd);
    } else {
        // Declaración de variable global: ya consumimos el primer ID
        VarDec* vd = new VarDec();
        vd->kind = kind;
        vd->type = typeName;
        vd->vars.push_back(name);
        vd->initializers.push_back(nullptr);

        // Más variables en la misma línea
        while (match(Token::COMA)) {
            if (!match(Token::ID)) {
                error("Se esperaba identificador después de ',' en una declaración global");
            }
            vd->vars.push_back(previous->text);
            vd->initializers.push_back(nullptr);
        }

        if (!match(Token::SEMICOL)) {
            error("Se esperaba ';' al final de la declaración global");
        }

        prog->vdlist.push_back(vd);
    }
}

// =============================
// Parámetros de función
// =============================

void Parser::parseParamList(FunDec* fd) {
    while (true) {
        std::string ptype;
        TypeKind pkind = parseTypeSpec(ptype);
        (void)pkind; // no usamos aún el TypeKind del parámetro

        if (!match(Token::ID)) {
            error("Se esperaba nombre de parámetro");
        }

        fd->Ptipos.push_back(ptype);
        fd->Pnombres.push_back(previous->text);

        if (!match(Token::COMA)) break;
    }
}

// =============================
// Declaraciones de variables locales
// =============================

VarDec* Parser::parseTypedVariableDeclaration() {
    VarDec* vd = new VarDec();
    vd->kind = parseTypeSpec(vd->type);

    // primer identificador
    if (!match(Token::ID)) {
        error("Se esperaba identificador en declaración de variable");
    }
    vd->vars.push_back(previous->text);

    if (match(Token::ASSIGN)) {
        vd->initializers.push_back(parseExpression());
    } else {
        vd->initializers.push_back(nullptr);
    }

    // Declaradores adicionales
    while (match(Token::COMA)) {
        if (!match(Token::ID)) {
            error("Se esperaba identificador después de ',' en declaración de variable");
        }
        vd->vars.push_back(previous->text);
        if (match(Token::ASSIGN)) {
            vd->initializers.push_back(parseExpression());
        } else {
            vd->initializers.push_back(nullptr);
        }
    }

    if (!match(Token::SEMICOL)) {
        error("Se esperaba ';' al final de la declaración de variable");
    }

    return vd;
}

VarDec* Parser::parseAutoDeclaration() {
    VarDec* vd = new VarDec();
    vd->kind = TYPE_AUTO;
    vd->type = "auto";

    if (!match(Token::AUTO)) {
        error("Se esperaba 'auto'");
    }

    // auto x = expr, y = expr2;
    if (!match(Token::ID)) {
        error("Se esperaba identificador después de 'auto'");
    }
    vd->vars.push_back(previous->text);

    if (!match(Token::ASSIGN)) {
        error("Se esperaba '=' en declaración con auto");
    }
    vd->initializers.push_back(parseExpression());

    while (match(Token::COMA)) {
        if (!match(Token::ID)) {
            error("Se esperaba identificador después de ',' en declaración con auto");
        }
        vd->vars.push_back(previous->text);
        if (!match(Token::ASSIGN)) {
            error("Se esperaba '=' en declaración con auto");
        }
        vd->initializers.push_back(parseExpression());
    }

    if (!match(Token::SEMICOL)) {
        error("Se esperaba ';' al final de la declaración con auto");
    }

    return vd;
}

// =============================
// Cuerpos / bloques
// =============================

Body* Parser::parseBody() {
    if (!match(Token::LBRACE)) {
        error("Se esperaba '{' al inicio de un bloque");
    }

    Body* body = new Body();

    while (!check(Token::RBRACE) && !isAtEnd()) {
        if (check(Token::AUTO)) {
            VarDec* vd = parseAutoDeclaration();
            body->declarations.push_back(vd);
        } else if (isTypeStart()) {
            VarDec* vd = parseTypedVariableDeclaration();
            body->declarations.push_back(vd);
        } else if (check(Token::FOR)) {
            parseForIntoBody(body);
        } else {
            Stm* s = parseStatement();
            if (s != nullptr) {
                body->StmList.push_back(s);
            }
        }
    }

    if (!match(Token::RBRACE)) {
        error("Se esperaba '}' al final de un bloque");
    }

    return body;
}

void Parser::parseForIntoBody(Body* body) {
    // current == FOR
    match(Token::FOR);
    if (!match(Token::LPAREN)) {
        error("Se esperaba '(' después de 'for'");
    }

    // 1) Inicialización: for (int i = expr; ...; ...)
    if (!isTypeStart()) {
        error("Por ahora solo se soporta 'for' con inicialización 'int id = expr;'");
    }

    std::string itype;
    TypeKind ikind = parseTypeSpec(itype);

    if (!match(Token::ID)) {
        error("Se esperaba identificador en la inicialización del for");
    }
    std::string varName = previous->text;

    if (!match(Token::ASSIGN)) {
        error("Se esperaba '=' en la inicialización del for");
    }
    Exp* initExp = parseExpression();

    if (!match(Token::SEMICOL)) {
        error("Se esperaba ';' después de la inicialización del for");
    }

    // Añadimos 'int i;' a las declaraciones del bloque (para reservar espacio)
    VarDec* vd = new VarDec();
    vd->kind = ikind;
    vd->type = itype;
    vd->vars.push_back(varName);
    vd->initializers.push_back(nullptr);
    body->declarations.push_back(vd);

    // Sentencia de inicialización (no la metemos directo al Body, la guardamos en el ForStm)
    Stm* initStm = new AssignStm(varName, initExp);

    // 2) Condición: for (int i = ...; i < expr; ...)
    if (!match(Token::ID)) {
        error("Se esperaba identificador en la condición del for");
    }
    std::string condVar = previous->text;

    if (!match(Token::LE)) {
        error("Por ahora solo se soporta condición 'var < expr' en for");
    }

    Exp* condRight = parseExpression();

    if (!match(Token::SEMICOL)) {
        error("Se esperaba ';' después de la condición del for");
    }

    // 3) Paso: for (...; ...; i++)
    if (!match(Token::ID)) {
        error("Se esperaba identificador en el incremento del for");
    }
    std::string stepVar = previous->text;

    if (!match(Token::PLUS) || !match(Token::PLUS)) {
        error("Por ahora solo se soporta incremento 'var++' en for");
    }

    if (!match(Token::RPAREN)) {
        error("Se esperaba ')' al final del encabezado del for");
    }

    // 4) Cuerpo del for: bloque o sentencia simple
    Body* loopBody;
    if (check(Token::LBRACE)) {
        loopBody = parseBody();
    } else {
        Stm* only = parseStatement();
        loopBody = new Body();
        if (only) loopBody->StmList.push_back(only);
    }

    // Construimos la sentencia de paso: i = i + 1;
    Exp* stepLeft = new IdExp(stepVar);
    Exp* one      = new NumberExp(1);
    Exp* plusExpr = new BinaryExp(stepLeft, one, PLUS_OP);
    Stm* stepStm  = new AssignStm(stepVar, plusExpr);

    // Condición del for: i < expr  => BinaryExp(IdExp(condVar), condRight, LE_OP)
    Exp* condLeft = new IdExp(condVar);
    Exp* condExpr = new BinaryExp(condLeft, condRight, LE_OP);

    // Creamos el nodo ForStm y lo agregamos como sentencia del bloque
    ForStm* f = new ForStm(initStm, condExpr, stepStm, loopBody);
    body->StmList.push_back(f);
}

// =============================
// Sentencias
// =============================

Stm* Parser::parseStatement() {
    if (check(Token::IF))    return parseIfStatement();
    if (check(Token::WHILE)) return parseWhileStatement();
    if (check(Token::RETURN))return parseReturnStatement();
    if (check(Token::PRINT)) return parsePrintStatement();

    // Sentencia vacía: ;
    if (check(Token::SEMICOL)) {
        match(Token::SEMICOL);
        return nullptr;
    }

    // Por defecto, asumimos una sentencia de asignación tipo 'id = expr;'
    return parseAssignOrExprStatement();
}

Stm* Parser::parseIfStatement() {
    match(Token::IF);
    if (!match(Token::LPAREN)) {
        error("Se esperaba '(' después de 'if'");
    }
    Exp* cond = parseExpression();
    if (!match(Token::RPAREN)) {
        error("Se esperaba ')' después de la condición de if");
    }

    // Soportar if (cond) stmt; y if (cond) { ... }
    Body* thenBody;
    if (check(Token::LBRACE)) {
        thenBody = parseBody();
    } else {
        Stm* thenStm = parseStatement();
        thenBody = new Body();
        if (thenStm) thenBody->StmList.push_back(thenStm);
    }

    Body* elseBody = nullptr;
    if (match(Token::ELSE)) {
        if (check(Token::LBRACE)) {
            elseBody = parseBody();
        } else {
            Stm* elseStm = parseStatement();
            elseBody = new Body();
            if (elseStm) elseBody->StmList.push_back(elseStm);
        }
    }

    return new IfStm(cond, thenBody, elseBody);
}

Stm* Parser::parseWhileStatement() {
    match(Token::WHILE);
    if (!match(Token::LPAREN)) {
        error("Se esperaba '(' después de 'while'");
    }
    Exp* cond = parseExpression();
    if (!match(Token::RPAREN)) {
        error("Se esperaba ')' después de la condición de while");
    }

    Body* b;
    if (check(Token::LBRACE)) {
        b = parseBody();
    } else {
        Stm* s = parseStatement();
        b = new Body();
        if (s) b->StmList.push_back(s);
    }

    return new WhileStm(cond, b);
}

Stm* Parser::parseReturnStatement() {
    match(Token::RETURN);
    if (check(Token::SEMICOL)) {
        match(Token::SEMICOL);
        return new ReturnStm();
    } else {
        Exp* e = parseExpression();
        if (!match(Token::SEMICOL)) {
            error("Se esperaba ';' después de return");
        }
        return new ReturnStm(e);
    }
}

Stm* Parser::parsePrintStatement() {
    match(Token::PRINT);
    if (!match(Token::LPAREN)) {
        error("Se esperaba '(' después de printf");
    }

    // printf("formato", expr);
    if (check(Token::STRING)) {
        advance(); // ignorar cadena de formato
        if (!match(Token::COMA)) {
            error("Se esperaba ',' después de la cadena en printf");
        }
    }

    Exp* arg = parseExpression();

    if (!match(Token::RPAREN)) {
        error("Se esperaba ')' al final de printf");
    }
    if (!match(Token::SEMICOL)) {
        error("Se esperaba ';' después de printf");
    }

    return new PrintStm(arg);
}

Stm* Parser::parseAssignOrExprStatement() {
    if (!match(Token::ID)) {
        error("Se esperaba identificador al inicio de la sentencia");
    }
    std::string name = previous->text;

    if (!match(Token::ASSIGN)) {
        error("Por ahora solo se soportan sentencias de asignación tipo 'id = expr;'");
    }

    Exp* rhs = parseExpression();

    if (!match(Token::SEMICOL)) {
        error("Se esperaba ';' al final de la sentencia de asignación");
    }

    return new AssignStm(name, rhs);
}

// =============================
// Expresiones
// =============================

Exp* Parser::parseExpression() {
    return parseTernary();
}

Exp* Parser::parseTernary() {
    // condición:  lo que ya soporta comparison (<, >, +, -, *, /, etc.)
    Exp* condition = parseComparison();

    // ¿hay operador ternario?
    if (match(Token::QMARK)) {
        // parte "then"
        Exp* thenExp = parseComparison();

        if (!match(Token::COL)) {
            error("Se esperaba ':' en la expresión condicional ternaria");
        }

        // parte "else"
        Exp* elseExp = parseComparison();

        return new TernaryExp(condition, thenExp, elseExp);
    }

    // si no hay '?', simplemente devolvemos la comparison
    return condition;
}

// comparison: additive ( '<' additive | '>' additive )*
Exp* Parser::parseComparison() {
    Exp* left = parseAdditive();
    while (true) {
        if (match(Token::LE)) {
            // left < right
            Exp* right = parseAdditive();
            left = new BinaryExp(left, right, LE_OP);
        } else if (match(Token::GT)) {
            // left > right  ≈  right < left
            Exp* right = parseAdditive();
            left = new BinaryExp(right, left, LE_OP);
        } else {
            break;
        }
    }
    return left;
}

Exp* Parser::parseAdditive() {
    Exp* left = parseTerm();
    while (true) {
        if (match(Token::PLUS)) {
            Exp* right = parseTerm();
            left = new BinaryExp(left, right, PLUS_OP);
        } else if (match(Token::MINUS)) {
            Exp* right = parseTerm();
            left = new BinaryExp(left, right, MINUS_OP);
        } else {
            break;
        }
    }
    return left;
}

Exp* Parser::parseTerm() {
    Exp* left = parseFactor();
    while (true) {
        if (match(Token::MUL)) {
            Exp* right = parseFactor();
            left = new BinaryExp(left, right, MUL_OP);
        } else if (match(Token::DIV)) {
            Exp* right = parseFactor();
            left = new BinaryExp(left, right, DIV_OP);
        } else {
            break;
        }
    }
    return left;
}

Exp* Parser::parseFactor() {
    // Soportar unario '-'
    if (match(Token::MINUS)) {
        Exp* inner = parseFactor();
        return new BinaryExp(new NumberExp(0), inner, MINUS_OP);
    }

    return parsePrimary();
}

Exp* Parser::parsePrimary() {
    if (match(Token::NUM)) {
        int val = std::stoi(previous->text);
        return new NumberExp(val);
    }

    if (match(Token::ID)) {
        std::string name = previous->text;

        // Posible llamada a función: id '(' args ')'
        if (match(Token::LPAREN)) {
            std::vector<Exp*> args;
            if (!check(Token::RPAREN)) {
                // parsear 1 o más argumentos
                args.push_back(parseExpression());
                while (match(Token::COMA)) {
                    args.push_back(parseExpression());
                }
            }
            if (!match(Token::RPAREN)) {
                error("Se esperaba ')' al final de la llamada a función");
            }
            return new FcallExp(name, args);
        }

        return new IdExp(name);
    }

    if (match(Token::LPAREN)) {
        Exp* e = parseExpression();
        if (!match(Token::RPAREN)) {
            error("Se esperaba ')' después de la expresión");
        }
        return e;
    }

    error("Expresión primaria inválida");
    return nullptr;
}