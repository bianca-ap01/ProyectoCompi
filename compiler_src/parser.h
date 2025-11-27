#ifndef PARSER_H
#define PARSER_H
// parser descendente recursivo sobre tokens del scanner

#include "scanner.h"
#include "ast.h"
#include <string>

class Parser {
private:
    Scanner* scanner;
    Token* current;
    Token* previous;

    // Helpers básicos
    bool advance();
    bool match(Token::Type ttype);
    bool check(Token::Type ttype) const;
    bool isAtEnd() const;
    void error(const string& msg);

    // Tipos
    bool     isTypeStart() const;
    TypeKind parseTypeSpec(string& outType);

    // Nivel programa / top-level
    void     parseTopLevelDeclaration(Program* prog);

    // Declaraciones de variables
    VarDec*  parseTypedVariableDeclaration(); // dentro de un bloque
    VarDec*  parseAutoDeclaration();

    // Parámetros de funciones
    void     parseParamList(FunDec* fd);

    // Bloques / cuerpos
    Body*    parseBody();
    void     parseForIntoBody(Body* body);

    // Sentencias
    Stm*     parseStatement();
    Stm*     parseIfStatement();
    Stm*     parseWhileStatement();
    Stm*     parseReturnStatement();
    Stm*     parsePrintStatement();
    Stm*     parseAssignOrExprStatement();

    // Expresiones
    Exp*     parseExpression();
    Exp*     parseTernary();
    Exp*     parseComparison();
    Exp*     parseAdditive();
    Exp*     parseTerm();
    Exp*     parseFactor();
    Exp*     parsePrimary();

public:
    explicit Parser(Scanner* scanner);
    Program* parseProgram();
};

#endif // PARSER_H