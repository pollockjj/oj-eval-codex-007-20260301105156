/*
 * File: statement.h
 * -----------------
 * This file defines the Statement abstract type.  In
 * the finished version, this file will also specify subclasses
 * for each of the statement types.  As you design your own
 * version of this class, you should pay careful attention to
 * the exp.h interface, which is an excellent model for
 * the Statement class hierarchy.
 */

#ifndef _statement_h
#define _statement_h

#include <string>

#include "evalstate.hpp"
#include "exp.hpp"
#include "Utils/tokenScanner.hpp"

class Program;

enum StatementType {
    REM_STMT,
    LET_STMT,
    PRINT_STMT,
    INPUT_STMT,
    END_STMT,
    GOTO_STMT,
    IF_STMT
};

/*
 * Class: Statement
 * ----------------
 * This class is used to represent a statement in a program.
 * The model for this class is Expression in the exp.h interface.
 * Like Expression, Statement is an abstract class with subclasses
 * for each of the statement and command types required for the
 * BASIC interpreter.
 */

class Statement {

public:

/*
 * Constructor: Statement
 * ----------------------
 * The base class constructor is empty.  Each subclass must provide
 * its own constructor.
 */

    Statement();

/*
 * Destructor: ~Statement
 * Usage: delete stmt;
 * -------------------
 * The destructor deallocates the storage for this expression.
 * It must be declared virtual to ensure that the correct subclass
 * destructor is called when deleting a statement.
 */

    virtual ~Statement();

/*
 * Method: execute
 * Usage: stmt->execute(state);
 * ----------------------------
 * This method executes a BASIC statement.  Each of the subclasses
 * defines its own execute method that implements the necessary
 * operations.  As was true for the expression evaluator, this
 * method takes an EvalState object for looking up variables or
 * controlling the operation of the interpreter.
 */

    virtual void execute(EvalState &state, Program &program) = 0;

    virtual StatementType getType() const = 0;
};

class RemStatement : public Statement {
public:
    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;
};

class LetStatement : public Statement {
public:
    LetStatement(std::string var, Expression *exp);

    ~LetStatement() override;

    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;

private:
    std::string var;
    Expression *exp;
};

class PrintStatement : public Statement {
public:
    explicit PrintStatement(Expression *exp);

    ~PrintStatement() override;

    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;

private:
    Expression *exp;
};

class InputStatement : public Statement {
public:
    explicit InputStatement(std::string var);

    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;

private:
    std::string var;
};

class EndStatement : public Statement {
public:
    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;
};

class GotoStatement : public Statement {
public:
    explicit GotoStatement(int lineNumber);

    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;

private:
    int lineNumber;
};

class IfStatement : public Statement {
public:
    IfStatement(Expression *lhs, std::string op, Expression *rhs, int lineNumber);

    ~IfStatement() override;

    void execute(EvalState &state, Program &program) override;

    StatementType getType() const override;

private:
    Expression *lhs;
    std::string op;
    Expression *rhs;
    int lineNumber;
};

Statement *parseStatement(TokenScanner &scanner);

bool isKeyword(const std::string &token);

#endif
