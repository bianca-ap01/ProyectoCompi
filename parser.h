#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "ast.h"
#include <string>

class Parser {
private:
    Scanner* scanner;
    Token* current;
    Token* previous;

    bool advance();
    bool match(Token::Type ttype);
    bool check(Token::Type ttype) const;
    bool isAtEnd() const;
    void error(const std::string& msg);

    bool isTypeStart() const;
    TypeKind parseTypeSpec(std::string& outType);

    void parseTopLevelDeclaration(Program* prog);

    VarDec* parseTypedVariableDeclaration(); // dentro de un bloque
    VarDec* parseAutoDeclaration();

    void    parseParamList(FunDec* fd);

    Body* parseBody();

    Stm* parseStatement();
    Stm* parseIfStatement();
    Stm* parseWhileStatement();
    Stm* parseReturnStatement();
    Stm* parsePrintStatement();
    Stm* parseAssignOrExprStatement();

    // Expresiones
    Exp* parseExpression();
    Exp* parseComparison();
    Exp* parseAdditive();
    Exp* parseTerm();
    Exp* parseFactor();
    Exp* parsePrimary();

public:
    explicit Parser(Scanner* scanner);
    Program* parseProgram();
};

#endif // PARSER_H