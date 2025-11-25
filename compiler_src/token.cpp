#include "token.h"
#include <iostream>

Token::Token(Type type)
    : type(type), text("") {}

Token::Token(Type type, char c)
    : type(type), text(1, c) {}

Token::Token(Type type, const std::string& source, int first, int last)
    : type(type), text(source.substr(first, last - first)) {}

std::ostream& operator<<(std::ostream& outs, const Token& tok) {
    switch (tok.type) {
        case Token::PLUS:      outs << "TOKEN(PLUS, \""      << tok.text << "\")"; break;
        case Token::MINUS:     outs << "TOKEN(MINUS, \""     << tok.text << "\")"; break;
        case Token::MUL:       outs << "TOKEN(MUL, \""       << tok.text << "\")"; break;
        case Token::DIV:       outs << "TOKEN(DIV, \""       << tok.text << "\")"; break;
        case Token::POW:       outs << "TOKEN(POW, \""       << tok.text << "\")"; break;
        case Token::MOD:       outs << "TOKEN(MOD, \""       << tok.text << "\")"; break;
        case Token::LPAREN:    outs << "TOKEN(LPAREN, \""    << tok.text << "\")"; break;
        case Token::RPAREN:    outs << "TOKEN(RPAREN, \""    << tok.text << "\")"; break;
        case Token::LBRACE:    outs << "TOKEN(LBRACE, \""    << tok.text << "\")"; break;
        case Token::RBRACE:    outs << "TOKEN(RBRACE, \""    << tok.text << "\")"; break;
        case Token::SEMICOL:   outs << "TOKEN(SEMICOL, \""   << tok.text << "\")"; break;
        case Token::COMA:      outs << "TOKEN(COMA, \""      << tok.text << "\")"; break;
        case Token::LE:        outs << "TOKEN(LE, \""        << tok.text << "\")"; break;
        case Token::GT:        outs << "TOKEN(GT, \""        << tok.text << "\")"; break;
        case Token::ASSIGN:    outs << "TOKEN(ASSIGN, \""    << tok.text << "\")"; break;
        case Token::QMARK:     outs << "TOKEN(QMARK, \""     << tok.text << "\")"; break;
        case Token::COL:       outs << "TOKEN(COL, \""       << tok.text << "\")"; break;
        case Token::BACKSLASH: outs << "TOKEN(BACKSLASH, \"" << tok.text << "\")"; break;

        case Token::RETURN:    outs << "TOKEN(RETURN, \""    << tok.text << "\")"; break;
        case Token::IF:        outs << "TOKEN(IF, \""        << tok.text << "\")"; break;
        case Token::ELSE:      outs << "TOKEN(ELSE, \""      << tok.text << "\")"; break;
        case Token::WHILE:     outs << "TOKEN(WHILE, \""     << tok.text << "\")"; break;
        case Token::FOR:       outs << "TOKEN(FOR, \""       << tok.text << "\")"; break;
        case Token::DO:        outs << "TOKEN(DO, \""        << tok.text << "\")"; break;
        case Token::PRINT:     outs << "TOKEN(PRINT, \""     << tok.text << "\")"; break;
        case Token::TRUE:      outs << "TOKEN(TRUE, \""      << tok.text << "\")"; break;
        case Token::FALSE:     outs << "TOKEN(FALSE, \""     << tok.text << "\")"; break;
        case Token::UNSIGNED:  outs << "TOKEN(UNSIGNED, \""  << tok.text << "\")"; break;
        case Token::INT:       outs << "TOKEN(INT, \""       << tok.text << "\")"; break;
        case Token::FLOAT:     outs << "TOKEN(FLOAT, \""     << tok.text << "\")"; break;
        case Token::LONG:      outs << "TOKEN(LONG, \""      << tok.text << "\")"; break;
        case Token::AUTO:      outs << "TOKEN(AUTO, \""      << tok.text << "\")"; break;

        case Token::NUM:       outs << "TOKEN(NUM, \""       << tok.text << "\")"; break;
        case Token::ID:        outs << "TOKEN(ID, \""        << tok.text << "\")"; break;
        case Token::STRING:    outs << "TOKEN(STRING, \""    << tok.text << "\")"; break;

        case Token::HASHTAG:   outs << "TOKEN(HASHTAG, \""   << tok.text << "\")"; break;
        case Token::DOT:       outs << "TOKEN(DOT, \""       << tok.text << "\")"; break;
        case Token::END:       outs << "TOKEN(END, \""       << tok.text << "\")"; break;
        case Token::ERR:       outs << "TOKEN(ERR, \""       << tok.text << "\")"; break;
    }
    return outs;
}

std::ostream& operator<<(std::ostream& outs, const Token* tok) {
    if (!tok) return outs << "TOKEN(NULL)";
    return outs << *tok;
}