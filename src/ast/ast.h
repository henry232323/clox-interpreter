#ifndef CRAFTING_INTERPRETERS_AST_H
#define CRAFTING_INTERPRETERS_AST_H
#include "../scanner.h"
#include "../value.h"
#include "../memory.h"

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
    TYPE_METHOD,
    TYPE_INITIALIZER,
} FunctionType;

#define ALLOCATE_NODE(type, nodeType) (type*) allocateNode(sizeof(type), nodeType)

typedef enum {
    NODE_BINARY,
    NODE_GROUPING,
    NODE_LITERAL,
    NODE_UNARY,
    NODE_VARIABLE,
    NODE_ASSIGN,
    NODE_LOGICAL,
    NODE_CALL,
    NODE_GET,
    NODE_SET,
    NODE_SUPER,
    NODE_THIS,
    NODE_YIELD,
    NODE_LAMBDA,
    NODE_LIST,
    NODE_EXPRESSION,
    NODE_VAR,
    NODE_BLOCK,
    NODE_FUNCTION,
    NODE_CLASS,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_BREAK,
    NODE_RETURN,
    NODE_IMPORT,
} NodeType;

typedef struct {
    NodeType type;
    bool isMarked;
    struct Node *next;
} Node;

Node *allocateNode(size_t size, NodeType type);

typedef struct {
    Node self;
} Expr;

typedef struct {
    int count;
    int capacity;
    Expr** exprs;
} ExprArray;

void initExprArray(ExprArray* exprArray);
void writeExprArray(ExprArray * exprArray, Expr* expr);
void freeExprArray(ExprArray * exprArray);
typedef struct {
    Node self;
} Stmt;

typedef struct {
    int count;
    int capacity;
    Stmt** stmts;
} StmtArray;

void initStmtArray(StmtArray* stmtArray);
void writeStmtArray(StmtArray * stmtArray, Stmt* stmt);
void freeStmtArray(StmtArray * stmtArray);
struct Binary {
    Expr self;
    Expr *left;
    Token operator;
    Expr* right;
};

struct Grouping {
    Expr self;
    Expr* expression;
};

struct Literal {
    Expr self;
    Value value;
};

struct Unary {
    Expr self;
    Token operator;
    Expr* right;
};

struct Variable {
    Expr self;
    Token name;
};

struct Assign {
    Expr self;
    Token name;
    Expr* value;
};

struct Logical {
    Expr self;
    Expr* left;
    Token operator;
    Expr* right;
};

struct Call {
    Expr self;
    Expr* callee;
    Token paren;
    ExprArray arguments;
};

struct Get {
    Expr self;
    Expr* object;
    Token name;
};

struct Set {
    Expr self;
    Expr* object;
    Token name;
    Expr* value;
};

struct Super {
    Expr self;
    Token keyword;
    Token method;
};

struct This {
    Expr self;
    Token keyword;
};

struct Yield {
    Expr self;
    Expr* expression;
};

struct Lambda {
    Expr self;
    TokenArray params;
    StmtArray body;
};

struct List {
    Expr self;
    ExprArray items;
};

struct Expression {
    Stmt self;
    Expr* expression;
};

struct Var {
    Stmt self;
    Token name;
    Expr* initializer;
};

struct Block {
    Stmt self;
    StmtArray statements;
};

struct Function {
    Stmt self;
    Token name;
    TokenArray params;
    StmtArray body;
    FunctionType functionType;
};

struct Class {
    Stmt self;
    Token name;
    struct Variable* superclass;
    StmtArray methods;
};

struct If {
    Stmt self;
    Expr* condition;
    Stmt* thenBranch;
    Stmt* elseBranch;
};

struct While {
    Stmt self;
    Expr* condition;
    Stmt* body;
};

struct For {
    Stmt self;
    Stmt* initializer;
    Expr* condition;
    Expr* increment;
    Stmt* body;
};

struct Break {
    Stmt self;
    Token keyword;
};

struct Return {
    Stmt self;
    Token keyword;
    Expr* value;
};

struct Import {
    Stmt self;
    Expr* expression;
};

#endif //CRAFTING_INTERPRETERS_AST_H