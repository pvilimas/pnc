#include "pnc.h"

/*
	TODO
	- variables
	- custom function definitions
	- reading from a file/command line interface and help messages

	language currently supports:

	- builtin types:
		- numbers
		- booleans are 0 and 1
		- lists of numbers

	- constants that start with #
		- #true, #false, #e, #pi
		TODO #ans (requires shared memory)

	- function calls use prefix notation similar to lisp
		(+ (* 5 10) 6)
		= 56

	TODO
	(? x) - print a help message about x (if it's a variable say what type, print any function help message)


	- arithmetic operators
		(+ x y)
		(- x y)
		(* x y)
		(/ x y)
		(% x y)

	- comparison operators
		(= x y)
		(!= x y)
		(< x y)
		(<= x y)
		(> x y)
		(>= x y)

	- boolean logic - anything != 0 is true
		(bool x) - convert a number to 0 or 1

	TODO actually convert to #true or #false
	add bool type to numbers, set only if it's the result of a boolean
	operation, not just if it's 0 or 1

	TODO
		(not x)
		(and x y)
		(or x y)
		(xor x y)

	TODO
		(abs x) - absolute value
		(neg x) - flip the sign of x
		(sign x) - returns -1, 0, or 1
		(sq x) - square x
		(cb x) - cube x
		(pow n x) - x^n power
		(sqrt x) - square root
		(cbrt x) - cube root
		(root n x) - nth root
		(sin x)
		(cos x)
		(tan x)
		rest of the trig functions...
		(min x y)
		(max x y)
		(floor x)
		(ceil x)
		(round x) - to nearest integer
		(rand min max)
		(ln x)
		(log10 x)
		(log base x)
		(exp x) - e^x

	- list operations
		(list ...) - construct a list of numbers
		(len l) - get the length of a list
		(sum l) - sum up a list of numbers
		(range start stop) - construct a list of ints in range [start, stop)

	TODO
		(get l index) - returns l[index]
		(join ...) - join lists into one

	- logic
		(if cond then else)

	- other
		(fib n) - compute nth fibonacci number

*/

// global vars
REPLContext ctx = {0};

RT_FnList RT_BUILTIN_FUNCTIONS = {0};
RT_VarList RT_CONSTANT_VARS = {0};
ErrorString CPRV_ERROR_NAMES[] = {0};

int main(int argc, char** argv) {

	// round floats to nearest number
	// mpfr_set_default_rounding_mode(MPFR_RNDN);

	// init runtime variables
	rt_init();

	// validate args

	bool args_valid = true;
	if (argc != 1
	|| (argc == 3 && strcmp(argv[1], "-s") != 0
		&& strcmp(argv[1], "--string") != 0))
	{
		args_valid = false;
	}

	if (!args_valid) {
		fprintf(stderr,
			"USAGE: \n"
			"\tpnc: enter repl mode\n"
			"\tpnc [-s|--string] \"<program>\"\n");
		repl_quit();
		return 0;
	}

	bool run_once = (argc == 3);
	if (run_once) {
		// if argv == [pnc, -s|--string, "..."], evaluate argv[2]
		repl_once(argv[2]);
	} else {
		// if nothing was passed, start the repl (read from stdin in a loop)
		ctx.is_running = true;
		while (ctx.is_running) {
			repl_once(NULL);
		}
	}

	repl_quit();
	return 0;
}

TokenList tokenize(char* prog) {
	TokenList l = tl_new();
	
	int i = 0;
	int n = strlen(prog);
	while (i < n) {
		char c = prog[i];

		if (c == '(') {
			tl_append(l, (Token){.type=T_OPEN_PAREN});
		} else if (c == ')') {
			tl_append(l, (Token){.type=T_CLOSE_PAREN});
		} else if (c == '\n' || isspace(c)) {
			i++;
			continue;
		} else {
			Token t = { .type=T_ATOM, .atom_str=&prog[i], .atom_len=1 };
			while (prog[i] != '(' && prog[i] != ')' && !isspace(prog[i])) {
				t.atom_len++;
				i++;
			}
			// remove extra character
			t.atom_len -= 1;
			i--;

			// avoid appending Token{""}
			if (t.atom_len > 0) {
				tl_append(l, t);
			}
		}
		
		i++;
	}

	// add () at top-level if necessary
	// this avoids syntax errors for simple expressions like + 1 3
	// or set x 5
	// will not add if top level expression is already valid like
	// (+ 5 6)

	if (l.len >= 2
	&& l.tokens[0].type != T_OPEN_PAREN
	&& l.tokens[l.len - 1].type != T_CLOSE_PAREN) {

		TokenList l_wrapped = tl_new();
		tl_append(l_wrapped, (Token){.type = T_OPEN_PAREN});

		for (int i = 0; i < l.len; i++) {
			tl_append(l_wrapped, l.tokens[i]);
		}

		tl_append(l_wrapped, (Token){.type = T_CLOSE_PAREN});
		return l_wrapped;
	}

	return l;
}

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

ASTNode* make_ast_single(Token t) {

	ASTNode* root = node_new();
	root->type = A_ATOM;
	root->atom_str = t.atom_str;
	root->atom_len = t.atom_len;
	return root;
}

ASTNode* make_ast_list_simple(TokenList tl) {

	ASTNode* root = node_new();
	root->type = A_LIST;

	// level of nesting 0 ( 1 ( 2 ... ))
	int level = 1;

	for (int i = 1; i < tl.len-1; i++) {

		if (tl.tokens[i].type == T_ATOM && tl.tokens[i].atom_len != 0) {
			list_append(root, make_ast_single(tl.tokens[i]));
		}

		else if (tl.tokens[i].type == T_OPEN_PAREN) {
			// find matching close paren

			int j = i;
			while (level > 0 && j < tl.len-1) {
				if (tl.tokens[j].type == T_OPEN_PAREN) {
					level++;
				} else if (tl.tokens[j].type == T_CLOSE_PAREN) {
					level--;
					if (level == 1) {
						// matching close paren was found, index=j
						break;
					}
				}
				j++;
			}

			// reached end without level=0 again
			if (i >= tl.len - 1) {
				childproc_panic(RV_PARSE_ERROR, "unbalanced parentheses");
			}

			// now i = index of open paren
			// j = index of matching close paren

			TokenList sublist = {
				.tokens = tl.tokens + i,
				.len = j - i + 1
			};
			list_append(root, make_ast_list_simple(sublist));
			// magic
			i = j;
		}
	}

	return root;
}

ASTNode* make_ast(TokenList tl) {
	if (tl.len == 0) {
		return NULL;
	}

	if (tl.len == 1 && tl.tokens[0].type == T_ATOM) {	
		return make_ast_single(tl.tokens[0]);
	}

	else if (tl.tokens[0].type == T_OPEN_PAREN && tl.tokens[tl.len - 1].type == T_CLOSE_PAREN) {
		return make_ast_list_simple(tl);
	}

	else {
		childproc_panic(RV_PARSE_ERROR, "unrecognized expression");
	}
}

// ast_matches_*** should not write to out unless it will also return true

bool ast_matches_number(ASTNode* ast, Number* out) {
	if (ast->type != A_ATOM) {
		return false;
	}

	bool ok = num_from_str(ast->atom_str, ast->atom_len, out);
	return ok;
}

bool ast_matches_ident(ASTNode* ast, E_Ident* out) {
	if (ast->type != A_ATOM) {
		return false;
	}

	out->name = ast->atom_str;
	out->len = ast->atom_len;
	return true;
}

bool ast_matches_funccall(ASTNode* ast, E_FuncCall* out) {
	if (ast->type != A_LIST
	|| ast->list_len == 0
	|| ast->list_items[0]->type != A_ATOM) {
		return false;
	}

	for (int i = 0; i < RT_BUILTIN_FUNCTIONS.num_fns; i++) {
		E_FuncData* fd = &RT_BUILTIN_FUNCTIONS.fns[i];

		if (strncmp(
			ast->list_items[0]->atom_str,
			fd->name,
			fd->name_len) != 0)
		{
			continue;
		}

		// at this point, function name matches

		out->func = *fd;
		out->num_args_passed = ast->list_len - 1;
		out->args = malloc(out->num_args_passed * sizeof(Expr*));
		
		for (int i = 0; i < out->num_args_passed; i++) {
			out->args[i] = parse(ast->list_items[i + 1]);
		}

		return true;
	}

	childproc_panic(RV_NAME_ERROR, "undefined function '%.*s'",
		ast->list_items[0]->atom_len,
		ast->list_items[0]->atom_str);
}

Expr* parse(ASTNode* ast) {

	Expr* e = expr_new();

	if (ast == NULL) {
		childproc_panic(RV_OK_EMPTY, "");
	}

	if (ast_matches_number(ast, &e->number)) {
		e->type = E_NUMBER;
		return e;
	}
	
	if (ast_matches_ident(ast, &e->ident)) {
		e->type = E_IDENT;
		return e;
	}

	if (ast_matches_funccall(ast, &e->funccall)) {
		e->type = E_FUNCCALL;
		return e;
	}

	childproc_panic(RV_PARSE_ERROR, "unrecognized expression");
}

// void expr_print_rec(Expr* e) {
// 	if (e->type == E_NUMBER) {
// 		printf("(num %lf)", e->number.value);
// 	} else if (e->type == E_FUNCCALL) {
// 		printf("(%.*s ",
// 			e->funccall.func.name_len,
// 			e->funccall.func.name);
//
// 		for (int i = 0; i < e->funccall.num_args_passed; i++) {
// 			expr_print_rec(e->funccall.args[i]);
//
// 			if (i != e->funccall.num_args_passed - 1) {
// 				putc(' ', stdout);
// 			}
//
// 		}
//
// 		putc(')', stdout);
//
// 	} else if (e->type == E_IDENT) {
// 		printf("%.*s", e->ident.len, e->ident.name);
// 	} else {
// 		putc('?', stdout);
// 	}
// }

void assert_funccall_arg_count_correct(Expr* e) {
	if (e->funccall.func.num_args == RTFN_VARARGS
	|| e->funccall.func.num_args == e->funccall.num_args_passed) {
		return;
	}

	childproc_panic(RV_VALUE_ERROR,
		"function '%.*s' got %d arguments, expected %d",
		e->funccall.func.name_len,
		e->funccall.func.name,
		e->funccall.num_args_passed,
		e->funccall.func.num_args);
}

Value try_eval_arg_as_type(Expr* e, int arg_num, ValueType type) {

	E_FuncData fd = e->funccall.func;
	Value v = eval(e->funccall.args[arg_num]);

	if (v.type != type) {
		childproc_panic(RV_VALUE_ERROR,
			"argument #%d of function '%.*s' is type %s, expected %s",
			arg_num,
			fd.name_len,
			fd.name,
			stringify_value_type(v.type),
			stringify_value_type(type));
	}

	return v;
}

Value eval_constant(Expr* e) {
	for (int i = 0; i < RT_CONSTANT_VARS.num_vars; i++) {
		VarData* vd = &RT_CONSTANT_VARS.vars[i];
		if (!strncmp(
			vd->name,
			e->ident.name,
			e->ident.len))
		{
			// names match
			return vd->value;
		}
	}
	childproc_panic(RV_NAME_ERROR, "undefined constant %.*s",
		e->ident.len,
		e->ident.name);
}

Value eval(Expr* e) {
	if (e->type == E_NUMBER) {
		return (Value){
			.type = V_NUM,
			.number_value = e->number
		};
	}

	if (e->type == E_IDENT) {
		if (e->ident.len == 0) {
			childproc_panic(RV_OTHER_ERROR, "something bad happened on line %d",
				__LINE__);
		}

		if (e->ident.name[0] == '#') {
			// constant
			return eval_constant(e);
		}
		
		childproc_panic(RV_NAME_ERROR, "'%.*s' is unknown",
			e->ident.len,
			e->ident.name);
	}

	if (e->type == E_FUNCCALL) {
		assert_funccall_arg_count_correct(e);
		return e->funccall.func.actual_function(e);
	}

	childproc_panic(RV_OTHER_ERROR,
		"something bad happened on line %d", __LINE__);
}

char* stringify_value(Value v) {
	if (v.type == V_LIST) {
		return strdup("(...)");
	} else if (v.type == V_NUM) {
		return num_to_str(v.number_value);
	} else {
		return strdup("none");
	}
}

char* stringify_value_type(ValueType type) {
	switch (type) {
		case V_NUM: return "number";
		case V_LIST: return "list of numbers";
		default: return "unknown";
	}
}

void rt_init() {

	RT_CONSTANT_VARS = rt_varlist_new();

	rt_add_constant("#false", (Value){
		.type=V_NUM,
		.number_value=number_integer_from_u32(0)
	});

	rt_add_constant("#true", (Value){
		.type=V_NUM,
		.number_value=number_integer_from_u32(1)
	});

	// rt_add_constant("#pi", (Value){
	// 	.type=V_NUM,
	// 	.number_value=num_from_u32(3.1415926536)
	// });

	RT_BUILTIN_FUNCTIONS = rt_fnlist_new();

	rt_add_func("+", e_func_add, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("-", e_func_sub, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("*", e_func_mul, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("/", e_func_div, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("%", e_func_mod, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("=", e_func_eq, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("!=", e_func_neq, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func(">", e_func_gt, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("<=", e_func_le, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("<", e_func_lt, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func(">=", e_func_ge, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("bool", e_func_bool, 1, V_NUM, {V_NUM});
	// rt_add_func("fib", e_func_fib, 1, V_NUM, {V_NUM});
	// rt_add_func("list", e_func_list, RTFN_VARARGS, V_LIST, {V_NUM});
	// rt_add_func("len", e_func_len, 1, V_NUM, {V_LIST});
	// rt_add_func("sum", e_func_sum, 1, V_NUM, {V_LIST});
	// rt_add_func("range", e_func_range, 2, V_NUM, {V_NUM, V_NUM});
	// rt_add_func("if", e_func_if, 3, V_NUM, {V_NUM, V_NUM, V_NUM});

	// not needed, print the result instead
	CPRV_ERROR_NAMES[RV_OK] = "";

	// not needed, print nothing
	CPRV_ERROR_NAMES[RV_OK_EMPTY] = "";

	CPRV_ERROR_NAMES[RV_PARSE_ERROR] = "parse error";
	// : unbalanced parentheses
	// : unrecognized expression
	// : illegal number literal <lit>

	CPRV_ERROR_NAMES[RV_VALUE_ERROR] = "value error";
	// : argument #1 of function '+' is type list, expected num
	// : function '+' got 3 arguments, expected 2
	// : argument #2 of function '%' is %lf, expected an integer
	// : a list can only contain numbers

	CPRV_ERROR_NAMES[RV_DIVIDE_BY_ZERO_ERROR] = "divide by zero error";
	// : argument #2 of function '%' cannot be 0
	// : argument #2 of function '/' cannot be 0

	CPRV_ERROR_NAMES[RV_NAME_ERROR] = "name error";
	// : 'x' is unknown
	// : undefined variable 'x'
	// : undefined constant '#x'
	// : undefined function '+'
	// : '5' is not a function, maybe you meant '(list 5 6 7)'?

	CPRV_ERROR_NAMES[RV_MEMORY_ERROR] = "memory error";
	// : memory allocation failed, try again
	// : process creation failed, try again

	CPRV_ERROR_NAMES[RV_OTHER_ERROR] = "internal error";
	// : something bad happened on line %d

	// the fact that this is even being printed is an error
	// so it can be treated like an internal error
	CPRV_ERROR_NAMES[RV_NONE] = "internal error";
	// : something REALLY bad happened on line %d

}

void eval_pnc_expr(char* input, bool spawn_child_proc) {

	if (spawn_child_proc) {

		int pid = fork();
		if (pid > 0) {

			// parent waits for child to finish

			wait(NULL);
			return;

		} else if (pid < 0) {

			// fork error

			fputs("= ", stdout);
			fputs(CPRV_ERROR_NAMES[RV_MEMORY_ERROR], stdout);
			fputs(": process creation failed, try again\n", stdout);

			return;
		}

	}

	// if fork was called, only the child got to this point
	// if not, the main process is doing this stuff

	if (input == NULL) {
		return;
	}
	
	TokenList tl = tokenize(input);

	if (tl.len == 0) {
		return;
	}
	
	ASTNode* ast = make_ast(tl);

	if (ast == NULL) {
		return;
	}

	Expr* expr = parse(ast);

	if (expr == NULL) {
		return;
	}

	Value v = eval(expr);

	if (spawn_child_proc) {
		// child exits, freeing all memory
		childproc_return(v);
	} else {
		// or main process prints the result
		char* str = stringify_value(v);
		printf("= %s\n", str);
		free(str);
	}
}

void repl_once(char* prog) {

	// read program from input string
	if (prog != NULL) {

		// do not spawn child process, only run it once
		eval_pnc_expr(prog, false);
	}
	
	// or read from stdin
	else {

		char* buffer = NULL;
		size_t size = 0;

		getline(&buffer, &size, stdin);
		if (buffer == NULL || size == 0) {
			// empty line
			return;
		}

		// spawn child proc, compute result, and kill it
		// the child proc will print the result
		eval_pnc_expr(buffer, true);
		free(buffer);
	}
}

// main process exit
void repl_quit() {
	free(RT_CONSTANT_VARS.vars);
	free(RT_BUILTIN_FUNCTIONS.fns);
	exit(RV_OK);
}
