#include "pnc.h"

/*
	TODO
	- make sure parens are balanced and token list is well formed
	- ast edge cases?
	- "or" and "and" functions for list of bools
	- remaining math+comparison functions
	- string types and string literals
	- runtime and variables
	- reading from a file/command line interface and help messages
	- repl mode
	- better error messages

	language currently supports:

	- builtin types:
		- numbers
			- booleans are 0 and 1
		- lists of numbers

	- constants that start with #

	- ability to do function calls with prefix notation similar to lisp
		(+ (* 5 10) 6)
		= 56

	- arithmetic operators, all are (int, int) -> int
		(+ x y)
		(- x y)
		(* x y)
		(% x y)

	- comparison operators, all are (int, int) -> int
		(= x y)
		(!= x y)
		(< x y)
		(<= x y)
		(> x y)
		(>= x y)

	- type conversion
		(bool x) - convert a number to 0 or 1 (equivalent to `!!x` in C)

	- list operations
		(list ...) - construct a list of numbers
		(len l) - get the length of a list
		(sum l) - sum up a list of numbers
		(range start stop) - construct a list of ints in range [start, stop)

	- logic
		(if cond then else)

	- other
		(fib n) - compute nth fibonacci number

*/

// global context
REPLContext ctx;

int main(int argc, char** argv) {

	rt_init();

	ctx.is_running = true;

	if (argc == 3) {
		 if (!strcmp(argv[1], "-s") || !strcmp(argv[1], "--string")) {
			repl_once(argv[2]);
			repl_quit();
			return 0;
		 }
		 fprintf(stderr, "USAGE: \n"
		 				"\tpnc: enter repl mode\n"
		 				"\tpnc [-s|--string] \"<program>\"\n");
		 repl_quit();
		 return 0;
	}

	

	while (ctx.is_running) {
		repl_once(NULL);
	}

	repl_quit();
	return 0;
}

// step 1: program string to list of tokens

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

	return l;
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

bool is_whole_number(double val) {
	return floor(val) == val;
}

bool num_from_str(char* str, int len, Number* out) {

	char* s = strndup(str, len);

	char* endptr = s;
	double d = strtod(s, &endptr);
	if (endptr == s) {
		// failed
		return false;
	}

	free(s);
	
	*out = num(d);
	return true;
}

char* num_to_str(Number n) {

	char* buf;
	int num_bytes;
	if (n.type == NUM_INT) {
		num_bytes = snprintf(NULL, 0, "%d", (int)n.value);
		buf = malloc(num_bytes + 1);
		snprintf(buf, num_bytes, "%d", (int)n.value);
	} else {
		num_bytes = snprintf(NULL, 0, "%lf", n.value);
		buf = malloc(num_bytes + 1);
		snprintf(buf, num_bytes, "%lf", n.value);
	}
	return buf;
}

Number num_add(Number n1, Number n2) {
	return num(n1.value + n2.value);
}

Number num_sub(Number n1, Number n2) {
	return num(n1.value - n2.value);
}

Number num_mul(Number n1, Number n2) {
	return num(n1.value * n2.value);
}

Number num_div(Number n1, Number n2) {
	if (n2.value == 0) {
		childproc_panic(RV_DIVIDE_BY_ZERO_ERROR, "argument #2 of function '/' cannot be 0");
	}

	return num(n1.value / n2.value);
}

Number num_mod(Number n1, Number n2) {

	if (!is_whole_number(n1.value)) {
		childproc_panic(RV_VALUE_ERROR, "argument #1 of function '%' is %lf, expected an integer", n1.value);
	}

	if (!is_whole_number(n2.value)) {
		childproc_panic(RV_VALUE_ERROR, "argument #2 of function '%' is %lf, expected an integer", n2.value);
	}

	if (n2.value == 0) {
		childproc_panic(RV_DIVIDE_BY_ZERO_ERROR, "argument #2 of function '%' cannot be 0");
	}

	return num((int)n1.value % (int)n2.value);
}

Number num_eq(Number n1, Number n2) {
	return num(n1.value == n2.value);
}

Number num_neq(Number n1, Number n2) {
	return num(n1.value != n2.value);
}

Number num_lt(Number n1, Number n2) {
	return num(n1.value < n2.value);
}

Number num_gt(Number n1, Number n2) {
	return num(n1.value > n2.value);
}

Number num_le(Number n1, Number n2) {
	return num(n1.value <= n2.value);
}

Number num_ge(Number n1, Number n2) {
	return num(n1.value >= n2.value);
}

// convert a number to 0 or 1
Number num_to_bool(Number n) {
	return num(!!n.value);
}

void expr_print_rec(Expr* e) {
	if (e->type == E_NUMBER) {
		printf("(num %lf)", e->number.value);
	} else if (e->type == E_FUNCCALL) {
		printf("(%.*s ",
			e->funccall.func.name_len,
			e->funccall.func.name);

		for (int i = 0; i < e->funccall.num_args_passed; i++) {
			expr_print_rec(e->funccall.args[i]);

			if (i != e->funccall.num_args_passed - 1) {
				putc(' ', stdout);
			}

		}

		putc(')', stdout);

	} else if (e->type == E_IDENT) {
		printf("%.*s", e->ident.len, e->ident.name);
	} else {
		putc('?', stdout);
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

char* stringify_value_type(ValueType type) {
	switch (type) {
		case V_NONE: return "none";
		case V_NUM: return "num";
		case V_LIST: return "list of num";
	}
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

Value e_func_add(Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_add(n0, n1)
	};
}

Value e_func_sub(Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_sub(n0, n1)
	};
}

Value e_func_mul(Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_mul(n0, n1)
	};
}

Value e_func_div(Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_div(n0, n1)
	};
}

Value e_func_mod(Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_mod(n0, n1)
	};
}

Value e_func_eq(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_eq(n0, n1)
	};
}

Value e_func_neq(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_neq(n0, n1)
	};
}

Value e_func_lt(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_lt(n0, n1)
	};
}

Value e_func_gt(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_gt(n0, n1)
	};
}

Value e_func_le(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_le(n0, n1)
	};
}

Value e_func_ge(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	Number n0 = arg0.number_value;
	Number n1 = arg1.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_ge(n0, n1)
	};
}

Value e_func_bool(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);

	Number n0 = arg0.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num_to_bool(n0)
	};
}

size_t e_func_fib_r(size_t n) {
	if (n < 2)
		return 1;
	else
		return e_func_fib_r(n-1) + e_func_fib_r(n-2);
}

Value e_func_fib(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);

	Number n0 = arg0.number_value;

	return (Value){
		.type = V_NUM,
		.number_value = num(e_func_fib_r(n0.value))
	};
}

Value e_func_list(struct Expr* e) {

	NumberList result = nl_new();

	// empty list
	if (e->funccall.num_args_passed == 0) {
		return (Value){
			.type = V_LIST,
			.list_value = result
		};
	}

	for (int i = 0; i < e->funccall.num_args_passed; i++) {

		// manually doing try_eval_arg_as_type to print a specific msg

		Value list_item = eval(e->funccall.args[i]);

		if (list_item.type == V_LIST) {
			childproc_panic(RV_VALUE_ERROR,
				"a list cannot contain another list");
		}

		nl_append(result, list_item.number_value);
	}
	
	return (Value){
		.type = V_LIST,
		.list_value = result
	};
}

Value e_func_len(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_LIST);

	int result = arg0.list_value.num_nums;

	return (Value){
		.type = V_NUM,
		.number_value = num(result)
	};
}

Value e_func_sum(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_LIST);
	
	Number result = num(0);

	for (int i = 0; i < arg0.list_value.num_nums; i++) {

		Number arg_value_n = arg0.list_value.nums[i];

		result = num_add(result, arg_value_n);
	}

	return (Value){
		.type = V_NUM,
		.number_value = result
	};
}

Value e_func_range(struct Expr* e) {

	// TODO assert start and stop are not floats
	// fix logic pls :)

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);

	int start = arg0.number_value.value;
	int stop = arg1.number_value.value;

	NumberList result = nl_new();

	for (int i = start; i < stop; i++) {
		Number n = num(i);
		nl_append(result, n);
	}

	return (Value){
		.type = V_LIST,
		.list_value = result
	};
}

Value e_func_if(struct Expr* e) {

	Value arg0 = try_eval_arg_as_type(e, 0, V_NUM);
	Value arg1 = try_eval_arg_as_type(e, 1, V_NUM);
	Value arg2 = try_eval_arg_as_type(e, 2, V_NUM);

	Number if_cond = num_to_bool(arg0.number_value);
	Number then_expr = arg1.number_value;
	Number else_expr = arg2.number_value;

	int result = (if_cond.value) ? then_expr.value : else_expr.value;

	return (Value){
		.type = V_NUM,
		.number_value = num(result)
	};
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
}

void rt_init() {

	RT_CONSTANT_VARS = rt_varlist_new();

	rt_add_constant("#false", (Value){.type=V_NUM, .number_value=num(0)});
	rt_add_constant("#true", (Value){.type=V_NUM, .number_value=num(1)});
	rt_add_constant("#pi", (Value){.type=V_NUM, .number_value=num(3.1415926536)});

	RT_BUILTIN_FUNCTIONS = rt_fnlist_new();

	rt_add_func("+", e_func_add, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("-", e_func_sub, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("*", e_func_mul, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("/", e_func_div, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("%", e_func_mod, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("=", e_func_eq, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("!=", e_func_neq, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func(">", e_func_gt, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("<=", e_func_le, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("<", e_func_lt, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func(">=", e_func_ge, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("bool", e_func_bool, 1, V_NUM, {V_NUM});
	rt_add_func("fib", e_func_fib, 1, V_NUM, {V_NUM});
	rt_add_func("list", e_func_list, RTFN_VARARGS, V_LIST, {V_NUM});
	rt_add_func("len", e_func_len, 1, V_NUM, {V_LIST});
	rt_add_func("sum", e_func_sum, 1, V_NUM, {V_LIST});
	rt_add_func("range", e_func_range, 2, V_NUM, {V_NUM, V_NUM});
	rt_add_func("if", e_func_if, 3, V_NUM, {V_NUM, V_NUM, V_NUM});
}

void eval_pnc_expr(char* input, bool spawn_child_proc) {

	if (spawn_child_proc) {

		int pid = fork();
		if (pid > 0) {

			// parent

			wait(NULL);

			// fputs(ctx.result->str, stdout);
			// fputs("\n", stdout);

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
