/*
 * File: statement.cpp
 * -------------------
 * This file implements the constructor and destructor for
 * the Statement class itself.  Your implementation must do
 * the same for the subclasses you define for each of the
 * BASIC statements.
 */

#include "statement.hpp"

#include <cctype>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "program.hpp"
#include "parser.hpp"
#include "Utils/error.hpp"
#include "Utils/strlib.hpp"

namespace {

bool isDigitsOnly(const std::string &token) {
    if (token.empty()) {
        return false;
    }
    for (char ch: token) {
        if (!isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return true;
}

bool isValidVariableToken(TokenScanner &scanner, const std::string &token) {
    TokenType type = scanner.getTokenType(token);
    if (type == WORD) {
        return true;
    }
    if (type == NUMBER) {
        return isDigitsOnly(token);
    }
    return false;
}

int parseLineNumber(TokenScanner &scanner, const std::string &token) {
    if (scanner.getTokenType(token) != NUMBER) {
        error("SYNTAX ERROR");
    }
    return stringToInteger(token);
}

} // namespace

Statement::Statement() = default;

Statement::~Statement() = default;

void RemStatement::execute(EvalState &state, Program &program) {
    (void) state;
    (void) program;
}

StatementType RemStatement::getType() const {
    return REM_STMT;
}

LetStatement::LetStatement(std::string var, Expression *exp) : var(std::move(var)), exp(exp) {
}

LetStatement::~LetStatement() {
    delete exp;
}

void LetStatement::execute(EvalState &state, Program &program) {
    (void) program;
    state.setValue(var, exp->eval(state));
}

StatementType LetStatement::getType() const {
    return LET_STMT;
}

PrintStatement::PrintStatement(Expression *exp) : exp(exp) {
}

PrintStatement::~PrintStatement() {
    delete exp;
}

void PrintStatement::execute(EvalState &state, Program &program) {
    (void) program;
    std::cout << exp->eval(state) << std::endl;
}

StatementType PrintStatement::getType() const {
    return PRINT_STMT;
}

InputStatement::InputStatement(std::string var) : var(std::move(var)) {
}

void InputStatement::execute(EvalState &state, Program &program) {
    (void) program;
    while (true) {
        std::cout << " ? ";
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input)) {
            input.clear();
        }

        try {
            state.setValue(var, stringToInteger(input));
            return;
        } catch (ErrorException &) {
            std::cout << "INVALID NUMBER" << std::endl;
        }
    }
}

StatementType InputStatement::getType() const {
    return INPUT_STMT;
}

void EndStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.setEndFlag();
}

StatementType EndStatement::getType() const {
    return END_STMT;
}

GotoStatement::GotoStatement(int lineNumber) : lineNumber(lineNumber) {
}

void GotoStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.requestJump(lineNumber);
}

StatementType GotoStatement::getType() const {
    return GOTO_STMT;
}

IfStatement::IfStatement(Expression *lhs, std::string op, Expression *rhs, int lineNumber)
    : lhs(lhs), op(std::move(op)), rhs(rhs), lineNumber(lineNumber) {
}

IfStatement::~IfStatement() {
    delete lhs;
    delete rhs;
}

void IfStatement::execute(EvalState &state, Program &program) {
    int leftValue = lhs->eval(state);
    int rightValue = rhs->eval(state);

    bool jump = false;
    if (op == "=") {
        jump = leftValue == rightValue;
    } else if (op == "<") {
        jump = leftValue < rightValue;
    } else if (op == ">") {
        jump = leftValue > rightValue;
    }

    if (jump) {
        program.requestJump(lineNumber);
    }
}

StatementType IfStatement::getType() const {
    return IF_STMT;
}

bool isKeyword(const std::string &token) {
    static const std::set<std::string> keywords = {
        "REM", "LET", "PRINT", "INPUT", "END", "GOTO", "IF", "THEN", "RUN", "LIST", "CLEAR", "QUIT",
        "HELP"
    };
    return keywords.find(token) != keywords.end();
}

Statement *parseStatement(TokenScanner &scanner) {
    try {
        if (!scanner.hasMoreTokens()) {
            error("SYNTAX ERROR");
        }

        const std::string keyword = scanner.nextToken();

        if (keyword == "REM") {
            return new RemStatement();
        }

        if (keyword == "LET") {
            if (!scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }
            const std::string var = scanner.nextToken();
            if (!isValidVariableToken(scanner, var) || isKeyword(var)) {
                error("SYNTAX ERROR");
            }

            if (scanner.nextToken() != "=") {
                error("SYNTAX ERROR");
            }
            if (!scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }

            std::unique_ptr<Expression> exp(parseExp(scanner));
            return new LetStatement(var, exp.release());
        }

        if (keyword == "PRINT") {
            if (!scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }

            std::unique_ptr<Expression> exp(parseExp(scanner));
            return new PrintStatement(exp.release());
        }

        if (keyword == "INPUT") {
            if (!scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }
            const std::string var = scanner.nextToken();
            if (!isValidVariableToken(scanner, var) || isKeyword(var)) {
                error("SYNTAX ERROR");
            }
            if (scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }
            return new InputStatement(var);
        }

        if (keyword == "END") {
            if (scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }
            return new EndStatement();
        }

        if (keyword == "GOTO") {
            if (!scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }
            const std::string token = scanner.nextToken();
            const int lineNumber = parseLineNumber(scanner, token);
            if (scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }
            return new GotoStatement(lineNumber);
        }

        if (keyword == "IF") {
            if (!scanner.hasMoreTokens()) {
                error("SYNTAX ERROR");
            }

            std::vector<std::string> tokens;
            while (scanner.hasMoreTokens()) {
                tokens.push_back(scanner.nextToken());
            }
            if (tokens.size() < 4) {
                error("SYNTAX ERROR");
            }

            int thenIndex = -1;
            for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
                if (tokens[i] == "THEN") {
                    thenIndex = i;
                    break;
                }
            }
            if (thenIndex <= 1 || thenIndex + 2 != static_cast<int>(tokens.size())) {
                error("SYNTAX ERROR");
            }

            int depth = 0;
            int opIndex = -1;
            for (int i = 0; i < thenIndex; ++i) {
                const std::string &current = tokens[i];
                if (current == "(") {
                    depth++;
                    continue;
                }
                if (current == ")") {
                    depth--;
                    continue;
                }
                if (depth == 0 && (current == "=" || current == "<" || current == ">")) {
                    if (opIndex != -1) {
                        error("SYNTAX ERROR");
                    }
                    opIndex = i;
                }
            }

            if (depth != 0 || opIndex <= 0 || opIndex >= thenIndex - 1) {
                error("SYNTAX ERROR");
            }

            std::string lhsText;
            for (int i = 0; i < opIndex; ++i) {
                if (!lhsText.empty()) {
                    lhsText += ' ';
                }
                lhsText += tokens[i];
            }

            std::string rhsText;
            for (int i = opIndex + 1; i < thenIndex; ++i) {
                if (!rhsText.empty()) {
                    rhsText += ' ';
                }
                rhsText += tokens[i];
            }

            TokenScanner lhsScanner(lhsText);
            lhsScanner.ignoreWhitespace();
            lhsScanner.scanNumbers();
            TokenScanner rhsScanner(rhsText);
            rhsScanner.ignoreWhitespace();
            rhsScanner.scanNumbers();

            std::unique_ptr<Expression> lhs(parseExp(lhsScanner));
            std::unique_ptr<Expression> rhs(parseExp(rhsScanner));

            const int lineNumber = parseLineNumber(scanner, tokens[thenIndex + 1]);
            return new IfStatement(lhs.release(), tokens[opIndex], rhs.release(), lineNumber);
        }

        error("SYNTAX ERROR");
    } catch (ErrorException &) {
        error("SYNTAX ERROR");
    }

    return nullptr;
}
