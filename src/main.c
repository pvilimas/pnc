#include "pnc.h"

// global vars
REPLContext ctx = {0};

RT_FnList RT_BUILTIN_FUNCTIONS = {0};
RT_VarList RT_CONSTANT_VARS = {0};
ErrorString CPRV_ERROR_NAMES[] = {0};

int main(int argc, char** argv) {

	// round floats to nearest number
	mpfr_set_default_rounding_mode(MPFR_RNDN);

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
