/*
 * File: Basic.cpp
 * ---------------
 * This file is the starter project for the BASIC interpreter.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "program.hpp"
#include "statement.hpp"
#include "Utils/error.hpp"
#include "Utils/strlib.hpp"
#include "Utils/tokenScanner.hpp"

/* Function prototypes */

void processLine(std::string line, Program &program, EvalState &state);

namespace {

void ensureNoTrailingTokens(TokenScanner &scanner) {
    if (scanner.hasMoreTokens()) {
        error("SYNTAX ERROR");
    }
}

void runProgram(Program &program, EvalState &state) {
    int currentLine = program.getFirstLineNumber();

    while (currentLine != -1) {
        Statement *stmt = program.getParsedStatement(currentLine);
        if (stmt == nullptr) {
            error("LINE NUMBER ERROR");
        }

        int nextLine = program.getNextLineNumber(currentLine);
        program.resetRunState();
        stmt->execute(state, program);

        if (program.shouldEnd()) {
            break;
        }

        if (program.hasJump()) {
            const int jumpTarget = program.getJumpTarget();
            if (!program.containsLine(jumpTarget)) {
                error("LINE NUMBER ERROR");
            }
            currentLine = jumpTarget;
        } else {
            currentLine = nextLine;
        }
    }

    program.resetRunState();
}

} // namespace

/* Main program */

int main() {
    EvalState state;
    Program program;

    while (true) {
        try {
            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input.empty()) {
                continue;
            }
            processLine(input, program, state);
        } catch (ErrorException &ex) {
            std::cout << ex.getMessage() << std::endl;
        }
    }
    return 0;
}

/*
 * Function: processLine
 * Usage: processLine(line, program, state);
 * -----------------------------------------
 * Processes a single line entered by the user.  In this version of
 * implementation, the program reads a line, parses it as an expression,
 * and then prints the result.  In your implementation, you will
 * need to replace this method with one that can respond correctly
 * when the user enters a program line (which begins with a number)
 * or one of the BASIC commands, such as LIST or RUN.
 */

void processLine(std::string line, Program &program, EvalState &state) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(line);

    if (!scanner.hasMoreTokens()) {
        return;
    }

    std::string token = scanner.nextToken();

    if (scanner.getTokenType(token) == NUMBER) {
        int lineNumber;
        try {
            lineNumber = stringToInteger(token);
        } catch (ErrorException &) {
            error("SYNTAX ERROR");
        }

        if (!scanner.hasMoreTokens()) {
            program.removeSourceLine(lineNumber);
            return;
        }

        Statement *stmt = parseStatement(scanner);
        program.addSourceLine(lineNumber, line);
        program.setParsedStatement(lineNumber, stmt);
        return;
    }

    if (token == "RUN") {
        ensureNoTrailingTokens(scanner);
        runProgram(program, state);
        return;
    }

    if (token == "LIST") {
        ensureNoTrailingTokens(scanner);

        for (int lineNumber = program.getFirstLineNumber(); lineNumber != -1; lineNumber = program.getNextLineNumber(
            lineNumber)) {
            std::cout << program.getSourceLine(lineNumber) << std::endl;
        }
        return;
    }

    if (token == "CLEAR") {
        ensureNoTrailingTokens(scanner);
        program.clear();
        state.Clear();
        return;
    }

    if (token == "QUIT") {
        ensureNoTrailingTokens(scanner);
        std::exit(0);
    }

    if (token == "LET" || token == "PRINT" || token == "INPUT") {
        scanner.saveToken(token);

        std::unique_ptr<Statement> stmt(parseStatement(scanner));
        StatementType type = stmt->getType();
        if (type != LET_STMT && type != PRINT_STMT && type != INPUT_STMT) {
            error("SYNTAX ERROR");
        }

        stmt->execute(state, program);
        return;
    }

    error("SYNTAX ERROR");
}
