#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "uritlib.h"
#include "uritlib.c"

void
printusageandexit(void)
{
	puts("Usage: urit http://example.com/{foo}/ --foo=\"bar\"");
	exit(EXIT_FAILURE);
}

/**
 * Main function
 * Accepts command line arguments
 */
int
main(int argc, char **argv)
{
	char *varname = NULL;
	char *varvalue = NULL;

	UritVars vars = urit_newvars();

	if (argc < 3) {
		printusageandexit();
	}

	for (int i = 2; i < argc; i++) {
		varname = argv[i];

		if (strncmp("--", varname, 2) != 0) {
			printusageandexit();
		}
		varname += 2;

		if (!varname[0]) {
			printusageandexit();
		}
		varvalue = strchr(varname, '=');

		if (!varvalue) {
			printusageandexit();
		}
		varvalue++[0] = '\0';

		if (strlen(varname) == 0) {
			printusageandexit();
		}

		UritStatus status = urit_addvariable(&vars, varname, varvalue);

		if (status != URIT_OK) {
			switch (status) {
				case URIT_INVALID_VARNAME:
					printf("Invalid variable name: %s\n", varname);
					break;
				case URIT_DUPLICATE_VARIABLE:
					printf("variable '%s' already exists\n", varname);
					break;
				case URIT_MALFORMED_LIST:
					printf("Malformed list. Format: (\"val1\",\"val2\",\"val3\")\n");
					break;
				case URIT_MALFORMED_MAP:
					printf("Malformed map. Format: [(\"key1\",\"val1\"),(\"key2\",\"val2\")]\n");
			}
			return EXIT_FAILURE;
		}
	}

	urit_printvars(vars);

	UritResult res = urit_parsetemplate(argv[1], vars);

	if (res.status != URIT_OK) {
		urit_printerrors(&res);
	}
	if (res.uri) {
		puts(res.uri);
	}

	return EXIT_SUCCESS;
}
