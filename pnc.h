#ifndef PNC_H
#define PNC_H

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

// token

typedef enum {
	T_NONE,
	T_OPEN_PAREN, // 1 char
	T_CLOSE_PAREN, // 1 char
	T_ATOM // <len> chars
} TokenType;

typedef struct {
	TokenType type;
	
	char* atom_str;
	int atom_len;
} Token;

#define token_fmt \
	"Token{%s, '%.*s'}"

#define token_type_str(ttype) \
	((ttype)==T_NONE \
		? "T_NONE" \
	: ((ttype)==T_OPEN_PAREN \
		? "T_OPEN_PAREN" \
	: ((ttype)==T_CLOSE_PAREN \
		? "T_CLOSE_PAREN" \
	: ((ttype)==(T_ATOM) \
		? "T_ATOM" \
	: "T_ERROR_UNKNOWN_TYPE"))))

#define token_arg(t) \
	(token_type_str((t).type)), (t).atom_len, (t).atom_str

typedef struct {
	Token* tokens;
	int len;
} TokenList;

#define tl_new() \
	((TokenList){0})

#define tl_resize(tl, n) \
	do { \
		(tl).len = (n); \
		(tl).tokens = realloc((tl).tokens, sizeof(Token) * (tl).len); \
	} while(0)

#define tl_append(tl, ... ) \
	do { \
		tl_resize((tl), (tl).len + 1); \
		(tl).tokens[(tl).len - 1] = (__VA_ARGS__); \
	} while(0)

#define tl_print(tl) \
	do { \
		for (int i = 0; i < (tl).len; i++) { \
			printf("\t[%d] " token_fmt "\n", i, token_arg((tl).tokens[i])); \
		} \
	} while(0)

// step 1: string to list of tokens 
TokenList tokenize(char* prog);

// ast

typedef enum {
	A_NONE,
	A_ATOM, // singular item
	A_LIST // a list of other ASTNodes
} ASTType;

typedef struct ASTNode {
	ASTType type;
	union {
		// A_ATOM
		struct { char* atom_str; int atom_len; };
		// A_LIST
		struct { struct ASTNode** list_items; int list_len; };
	};
} ASTNode;

#define node_new() \
	(calloc(1, sizeof(ASTNode)))

void node_print_rec(ASTNode* node, int level) {
	for (int i = 0; i < level; i++) {
		putc('\t', stdout);
		putc('|', stdout);
	}
	if (node->type == A_ATOM) {
		printf("ASTNode<type=A_ATOM, \"%.*s\">\n",
			node->atom_len,
			node->atom_str);
	} else if (node->type == A_LIST) {
		printf("ASTNode<type=A_LIST, %d items>:\n",
			node->list_len);
		for (int i = 0; i < node->list_len; i++) {
			node_print_rec(node->list_items[i], level + 1);
		}
	} else {
		printf("ASTNode<type=A_UNIMPLEMENTED: see line %d>\n", __LINE__);
	}
}

#define node_print(node) \
	(node_print_rec((node), 0))

#define list_resize(node, n) \
	do { \
		(node)->list_len = (n); \
		(node)->list_items = realloc((node)->list_items, sizeof(ASTNode*) * (node)->list_len); \
	} while(0)

// arg must be a ASTNode*
#define list_append(node, ...) \
	do { \
		list_resize((node), (node)->list_len + 1); \
		(node)->list_items[(node)->list_len - 1] = (__VA_ARGS__); \
	} while(0)

// takes only a single token
ASTNode* make_ast_single(Token t);

// assumes OPEN_PAREN, ..., CLOSE_PAREN
ASTNode* make_ast_list_simple(TokenList tl);

// step 2: list of tokens to ast tree
ASTNode* make_ast(TokenList tl);

// expr/val/number

// whole numbers should be NUM_INT, the rest are floats
// eventually will add a bignum type with something like `int* digits`
// (round up size to next multiple of 32 bits)
// and maybe a power notation like { int base; int power; } for scientific

// TODO replace int int_value; with double value; 
// set the int flag only if it's a whole number

typedef enum {
	NUM_INT,
	NUM_FLOAT
} NumberType;

typedef struct {
	NumberType type;
	union {
		double value;
	};
} Number;

// constructor
#define num(dval) \
	(Number){.type=(is_whole_number(dval)) ? NUM_INT : NUM_FLOAT, .value=dval}

// used to decide if a number should be an int or a float
bool is_whole_number(double val);

#define num_print(n) \
	do { \
		if (n.type == NUM_INT) { \
			printf("%d", (int) n.value); \
		} else { \
			printf("%lf", n.value); \
		} \
	} while(0)

// conversion functions

// returns false if failed, or writes to out
bool num_from_str(char* str, int len, Number* out);

// will never fail
char* num_to_str(Number n);

// arithmetic operators - assume type is correct

Number num_add(Number n1, Number n2);
Number num_sub(Number n1, Number n2);
Number num_mul(Number n1, Number n2);

// panics if n2 is 0
Number num_div(Number n1, Number n2);

// panics if n2 is 0, assumes both are ints
Number num_mod(Number n1, Number n2);

// comparison operators

Number num_eq(Number n1, Number n2);
Number num_neq(Number n1, Number n2);
Number num_lt(Number n1, Number n2);
Number num_gt(Number n1, Number n2);
Number num_le(Number n1, Number n2);
Number num_ge(Number n1, Number n2);

// convert a number to 0 or 1
Number num_to_bool(Number n);

// expr type

struct Expr;

typedef struct {
	char* name;
	int len;
} E_Ident;

// value type

// a value returned from the program
typedef enum {
	V_NONE = 0, // an empty program produces this value
	V_NUM,
	V_LIST // list of numbers, NOT VALUES TODO fix this
} ValueType;

struct Value;

typedef struct {
	Number* nums;
	int num_nums;
} NumberList;

#define nl_new() \
	((NumberList){0})

#define nl_resize(nl, n) \
	do { \
		(nl).num_nums = (n); \
		(nl).nums = realloc((nl).nums, sizeof(Number) * (nl).num_nums); \
	} while(0)

#define nl_append(nl, ... ) \
	do { \
		nl_resize((nl), (nl).num_nums + 1); \
		(nl).nums[(nl).num_nums - 1] = (__VA_ARGS__); \
	} while(0)

typedef struct Value {
	ValueType type;
	union {
		Number number_value;
		NumberList list_value;
	};
} Value;

typedef Value E_Func(struct Expr*);

// pass this to rt_func() to signify that the function takes a variable #
// of arguments
// RTFN = runtime function
// if E_FuncData.num_args = -1, .arg_types must have 1 item and that is the
// type of all arguments
#define RTFN_VARARGS -1

/* 	associative type that holds the name and pointer to a function as well as
	# of arguments */
typedef struct {
	// in (+ 2 3) name is "+"
	char* name;
	int name_len;

	int num_args; // THIS CAN BE -1
	ValueType* arg_types; // NULL if num_args is -1
	ValueType return_type;

	E_Func* actual_function;
} E_FuncData;

typedef struct {
	E_FuncData func;
	struct Expr** args;
	int num_args_passed;
} E_FuncCall;

// declarations of builtin functions and operators
Value e_func_add(struct Expr* e);
Value e_func_sub(struct Expr* e);
Value e_func_mul(struct Expr* e);
Value e_func_div(struct Expr* e);
Value e_func_mod(struct Expr* e);

Value e_func_eq(struct Expr* e);
Value e_func_neq(struct Expr* e);
Value e_func_lt(struct Expr* e);
Value e_func_gt(struct Expr* e);
Value e_func_le(struct Expr* e);
Value e_func_ge(struct Expr* e);

Value e_func_bool(struct Expr* e);

Value e_func_fib(struct Expr* e);

Value e_func_list(struct Expr* e);
Value e_func_len(struct Expr* e);
Value e_func_sum(struct Expr* e);
Value e_func_range(struct Expr* e);

Value e_func_if(struct Expr* e);

// list of functions defined in the runtime
typedef struct {
	E_FuncData* fns;
	int num_fns;
} RT_FnList;

#define rt_fnlist_new() \
	((RT_FnList){0})

#define rt_fnlist_resize(l, n) \
	do { \
		(l).num_fns = (n); \
		(l).fns = realloc((l).fns, sizeof(E_FuncData) * (l).num_fns); \
	} while(0)

#define rt_fnlist_append(l, ...) \
	do { \
		rt_fnlist_resize(l, (l).num_fns + 1); \
		(l).fns[(l).num_fns - 1] = (E_FuncData)__VA_ARGS__; \
	} while(0)

#define rt_fnlist_swap(l, i, j) \
	do { \
		E_FuncData temp = (l).fns[(i)]; \
		(l).fns[(i)] = (l).fns[(j)]; \
		(l).fns[(j)] = temp; \
	} while(0)

#define rt_fnlist_remove(l, index) \
	do { \
		rt_fnlist_swap(l, index, (l).num_fns - 1); \
		rt_fnlist_resize(l, (l).num_fns - 1); \
	} while(0)

// list of functions available in the runtime
// their argument count, argument types, return types are all specified in here
RT_FnList RT_BUILTIN_FUNCTIONS = {0};

typedef enum {
	E_NONE,
	E_NUMBER,
	E_IDENT,
	E_FUNCCALL,
} ExprType;

typedef struct Expr {
	ExprType type;
	union {
		Number number;
		E_Ident ident;
		E_FuncCall funccall;
	};
} Expr;

#define expr_new() \
	(calloc(1, sizeof(Expr)))

#define expr_print(e) \
	do { \
		expr_print_rec(e); \
		putc('\n', stdout); \
	} while(0)

void expr_print_rec(Expr* e);

// step 3: ast tree to expr tree
Expr* parse(ASTNode* ast);

// step 4: collapse expr tree to get a single value
Value eval(Expr* e);

// ast_matches_*** should not write to out unless it will also return true

// contains number parsing code
bool ast_matches_intlit(ASTNode* ast, int* out);
bool ast_matches_ident(ASTNode* ast, E_Ident* out);

// uses RT_BUILTIN_FUNCTIONS
bool ast_matches_funccall(ASTNode* ast, E_FuncCall* out);

// returns a static string literal
char* stringify_value_type(ValueType type);

// allocates data (can be freed on childproc exit like everything else)
char* stringify_value(Value v);

// called outside of each e_func_***
void assert_funccall_arg_count_correct(Expr* e);

// called inside each e_func_***, once per argument
Value try_eval_arg_as_type(Expr* e, int arg_num, ValueType type);

// another associative type
typedef struct {
	char* name;
	Value value;
} VarData;

typedef struct {
	VarData* vars;
	int num_vars;	
} RT_VarList;

#define rt_varlist_new() \
	((RT_VarList){0})

#define rt_varlist_resize(l, n) \
	do { \
		(l).num_vars = (n); \
		(l).vars = realloc((l).vars, sizeof(VarData) * (l).num_vars); \
	} while(0)

#define rt_varlist_append(l, ...) \
	do { \
		rt_varlist_resize(l, (l).num_vars + 1); \
		(l).vars[(l).num_vars - 1] = (VarData)__VA_ARGS__; \
	} while(0)

#define rt_varlist_swap(l, i, j) \
	do { \
		VarData temp = (l).vars[(i)]; \
		(l).vars[(i)] = (l).vars[(j)]; \
		(l).vars[(j)] = temp; \
	} while(0)

#define rt_varlist_remove(l, index) \
	do { \
		rt_fnlist_swap(l, index, (l).num_vars - 1); \
		rt_fnlist_resize(l, (l).num_vars - 1); \
	} while(0)

RT_VarList RT_CONSTANT_VARS = {0};

// variable lookup
// a name that starts with '#'
Value eval_constant(Expr* e);

// runtime stuff

// initialize starting constants and builtin functions
void rt_init();

#define rt_add_constant(name_cstrlit, ...) \
	rt_varlist_append(RT_CONSTANT_VARS, (VarData){ \
		.name=(name_cstrlit), \
		.value=(__VA_ARGS__) \
	})

// declare a runtime function (without having to specify name len separately)
// the ... is the list of return types so you can pass like {V_INT, V_LIST, V_INT, ...}
// for varargs all of the varargs will be evaluated as the last type in the list
// and num_args should be the number of REQUIRED (aka non-vararg) arguments, 
// which can be 0
#define rt_add_func(name_cstrlit, actual_func_ptr, \
func_arg_count, func_ret_type, ...) \
	rt_fnlist_append(RT_BUILTIN_FUNCTIONS, (E_FuncData){ \
		.name = (name_cstrlit), \
		.name_len = strlen(name_cstrlit), \
		.num_args = (func_arg_count), \
		.arg_types = ((ValueType[]) __VA_ARGS__), \
		.return_type = (func_ret_type), \
		.actual_function = (actual_func_ptr) \
	})

// the function that does everything
// starts child proc (if true), writes value to shared memory, kills child
void eval_pnc_expr(char* input, bool spawn_child_proc);

// result of a child process
typedef struct {
	int retval;
	char str[1000];
} ChildProcResult;

// repl stuff - manages everything else

typedef struct {

	// should always be true
	bool is_running;

	// shared memory using mmap
	ChildProcResult* result;

} REPLContext;

// global context
extern REPLContext ctx;

// read user input, eval it, print the result
// pass NULL to read a line from stdin instead
void repl_once(char* prog);

// exit the main program in a "good" way
void repl_quit();

typedef enum {
	// default value
	RV_NONE,

	// executed result, print the result
	RV_OK,

	// the result was empty, so print nothing
	RV_OK_EMPTY,

	// parse error in the result,like mismatched parens
	RV_PARSE_ERROR,

	// mismatched type, nested list, wrong type argument for function,
	// wrong number of arguments for function
	RV_VALUE_ERROR,

	// only happens in mod and div for now
	RV_DIVIDE_BY_ZERO_ERROR,

	// unknown function name or variable name
	RV_NAME_ERROR,

	// malloc failed, fork failed, etc
	RV_MEMORY_ERROR,

	// something else happened
	RV_OTHER_ERROR,

	RV_N

} ChildProcRetval;

typedef char* ErrorString;

// these are used as prefixes for the actual error message
// which should have the format "<errtype>: <description>"
// "value error: divide by zero"
ErrorString CPRV_ERROR_NAMES[RV_N] = (ErrorString[]){
	[RV_OK] = "", // not needed, print the result instead
	[RV_OK_EMPTY] = "", // not needed, print nothing

	[RV_PARSE_ERROR] = "parse error",
	// : unbalanced parentheses
	// : unrecognized expression

	[RV_VALUE_ERROR] = "value error",
	// : argument #1 of function '+' is type list, expected num
	// : function '+' got 3 arguments, expected 2
	// : argument #2 of function '%' is %lf, expected an integer

	[RV_DIVIDE_BY_ZERO_ERROR] = "divide by zero error",
	// : argument #2 of function '%' cannot be 0
	// : argument #2 of function '/' cannot be 0

	[RV_NAME_ERROR] = "name error",
	// : 'x' is unknown
	// : undefined variable 'x'
	// : undefined constant '#maybe'
	// : undefined function '+'
	// : '5' is not a function, maybe you meant '(list 5 6 7)'?

	[RV_MEMORY_ERROR] = "memory error",
	// : memory allocation failed, try again
	// : process creation failed, try again

	[RV_OTHER_ERROR] = "internal error",
	// : something bad happened on line %d

	[RV_NONE] = "internal error",
	// the fact that this is even being printed is an error
	// so it can be treated like an internal error
	// : something REALLY bad happened on line %d
};

// print value and exit
#define childproc_return(v) \
	do { \
		printf("%s\n", stringify_value(v)); \
		ctx.result->retval = RV_OK; \
		exit(RV_OK); \
	} while(0)

// print error message and exit
#define childproc_panic(rv, fmt, ...) \
	do { \
		printf("= %s: " fmt "\n", \
			CPRV_ERROR_NAMES[(rv)] \
			__VA_OPT__(,) __VA_ARGS__); \
		ctx.result->retval = (rv); \
		exit(rv); \
	} while(0)

#endif // PNC_H