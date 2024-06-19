#include "pnc.h"
#include "number.h"

// implementation of all builtin functions available during runtime

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
/*
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

	Number result = (if_cond.value) ? then_expr : else_expr;

	return (Value){
		.type = V_NUM,
		.number_value = result
	};
}*/
