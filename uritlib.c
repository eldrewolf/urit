#include <stdint.h>
#include <stdarg.h>
#include "uritlib.h"

static void urit_addvar(UritVars *vars, UritVar *var);
static void urit_adderror(UritResult *res, size_t pos, UritCode code);
static bool urit_isreserved(const char c);
static bool urit_isunreserved(const char c);
static size_t urit_numbytes(unsigned char c);
static unsigned urit_getcodepoint(const char *str);
static bool urit_isucschar(const char *str);
static bool urit_isiprivate(const char *str);
static bool urit_ispct(const char *str);
static bool urit_isliteral(const char *str);
static bool urit_isvarchar(const char *str);
static char *urit_pctencodechar(const unsigned char c);
static UritList *urit_compilelistvar(char *varvalue);
static UritMap *urit_compilemapvar(char *varvalue);
static UritString *urit_appendstring(UritString *des, char *src);
static UritString *urit_appendchar(UritString *des, char src);
static char *urit_encode(char *str, bool allowreserved, size_t max);
static UritOpRule urit_getoprule(char c);
static UritVar *urit_getvar(UritVars *vars, char *name);
static void urit_processexpression(char *expr, UritOpRule oprule, UritVars vars, UritResult *res, size_t pos);

UritVars
urit_newvars(void)
{
	UritVars *vars = malloc(sizeof(UritVars));
	vars->count = 0;
	vars->vars = malloc(sizeof(UritVar *));
	return *vars;
}

void
urit_printvars(UritVars vars)
{
	for (int i = 0; i < vars.count; i++) {
		switch(vars.vars[i]->type) {
			case URIT_STRING:
				printf("%s: %s\n", vars.vars[i]->name, vars.vars[i]->val_string);
				break;
			case URIT_LIST:
				printf("%s: ", vars.vars[i]->name);
				for (int j = 0; j < vars.vars[i]->val_list->count; j++) {
					printf("%s", vars.vars[i]->val_list->values[j]);
					if (j + 1 < vars.vars[i]->val_list->count) {
						printf(", ");
					}
				}
				puts("");
				break;
			case URIT_MAP:
				printf("%s: ", vars.vars[i]->name);
				for (int j = 0; j < vars.vars[i]->val_map->count; j++) {
					printf("%s=%s", vars.vars[i]->val_map->pairs[j]->key, vars.vars[i]->val_map->pairs[j]->val);
					if (j + 1 < vars.vars[i]->val_map->count) {
						printf(", ");
					}
				}
				puts("");
				break;
		}
	}
}

void
urit_printerrors(UritResult *r)
{
	UritError *e = r->error;

	while (e) {
		puts(r->tpl);
		printf("%*c^\n", (int) e->pos, ' ');
		switch (e->code) {
			case URIT_MALFORMED_EXPRESSION:		puts("Urit error: Malformed expression"); break;
			case URIT_EMPTY_EXPRESSION:			puts("Urit error: Empty expression"); break;
			case URIT_UNIMPLEMENTED_OPERATOR:	puts("Urit error: Unimplemented operator"); break;
			case URIT_NONLITERAL_FOUND:			puts("Urit error: Non-literal character found"); break;
			case URIT_INVALID_VARNAME:			puts("Urit error: Invalid variable name"); break;
		}
		puts("");
		e = (UritError *) e->next;
	}
}

UritStatus
urit_addvariable(UritVars *vars, char *varname, char *varvalue)
{
	UritVar *var = malloc(sizeof(UritVar));
	var->name = malloc(sizeof(char) * (strlen(varname) + 1));
	strcpy(var->name, varname);

	if (*varvalue == '(') {
		var->type = URIT_LIST;
		UritList *list = urit_compilelistvar(varvalue);

		if (!list) {
			return URIT_MALFORMED_LIST;
		}
		if (list->count) {
			var->val_list = list;
			urit_addvar(vars, var);
		}
	} else if (*varvalue == '[') {
		var->type = URIT_MAP;
		UritMap *map = urit_compilemapvar(varvalue);

		if (!map) {
			return URIT_MALFORMED_MAP;
		}
		if (map->count) {
			var->val_map = map;
			urit_addvar(vars, var);
		}
	} else {
		char *string = malloc(sizeof(char) * (strlen(varvalue) + 1));
		strcpy(string, varvalue);
		var->val_string = string;
		var->type = URIT_STRING;
		urit_addvar(vars, var);
	}
	return URIT_OK;
}

void
urit_addstringvar(UritVars *vars, char *varname, char *varvalue)
{
	UritVar *var = urit_getvar(vars, varname);

	if (var == NULL) {
		var = malloc(sizeof(UritVar));
		var->name = NULL;
		var->val_string = NULL;
	}
	var->name = realloc(var->name, sizeof(char) * (strlen(varname) + 1));
	strcpy(var->name, varname);

	var->val_string = realloc(var->val_string, sizeof(char) * (strlen(varvalue) + 1));
	strcpy(var->val_string, varvalue);
	var->type = URIT_STRING;
	urit_addvar(vars, var);
}

UritList *
urit_newlist(void)
{
	UritList *list = malloc(sizeof(UritList));
	list->count = 0;
	list->values = malloc(sizeof(char *));
	return list;
}

void
urit_addlistvar(UritVars *vars, char *name, size_t count, char **listitems)
{
	UritList *l = urit_newlist();

	for (size_t i = 0; i < count; i++) {
		urit_listadditem(listitems[i], l);
	}
	UritVar *v = urit_getvar(vars, name);
	if (v == NULL) {
		v = malloc(sizeof(UritVar));
		v->type = URIT_LIST;
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_list = l;

		urit_addvar(vars, v);
	} else {
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_list = l;
	}
}

void
urit_listadditem(char *str, UritList *list)
{
	list->values = realloc(list->values, sizeof(char *) * ++list->count);
	list->values[list->count - 1] = str;
}

void
urit_varsaddlist(UritVars *vars, char *name, UritList *list)
{
	UritVar *v = urit_getvar(vars, name);
	if (v == NULL) {
		v = malloc(sizeof(UritVar));
		v->type = URIT_LIST;
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_list = list;

		urit_addvar(vars, v);
	} else {
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_list = list;
	}
}

UritMap *
urit_newmap(void)
{
	UritMap *map = malloc(sizeof(UritMap));
	map->count = 0;
	map->pairs = malloc(sizeof(UritPair *));
	return map;
}

void
urit_mapaddkeyval(char *key, char *val, UritMap *map)
{
	UritPair *p = malloc(sizeof(UritPair));
	p->key = malloc(sizeof(char) * (strlen(key) + 1));
	p->val = malloc(sizeof(char) * (strlen(val) + 1));
	strcpy(p->key, key);
	strcpy(p->val, val);
	
	map->pairs = realloc(map->pairs, sizeof(UritPair *) * ++map->count);
	map->pairs[map->count - 1] = p;
}

void
urit_addmapvar(UritVars *vars, char *name, size_t count, char *map[][2])
{
	UritMap *m = urit_newmap();
	for(size_t i = 0; i < count; i++) {
		urit_mapaddkeyval(map[i][0], map[i][1], m);
	}
	UritVar *v = urit_getvar(vars, name);
	if (v == NULL) {
		v = malloc(sizeof(UritVar));
		v->type = URIT_MAP;
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_map = m;
		urit_addvar(vars, v);
	} else {
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_map = m;
	}
}

void
urit_varsaddmap(UritVars *vars, char *name, UritMap *map)
{
	UritVar *v = urit_getvar(vars, name);
	if (v == NULL) {
		v = malloc(sizeof(UritVar));
		v->type = URIT_MAP;
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_map = map;
		urit_addvar(vars, v);
	} else {
		v->name = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(v->name, name);
		v->val_map = map;
	}
}

UritString *
urit_newstring(void)
{
	UritString *str = malloc(sizeof(UritString));
	str->len = 0;
	str->size = 0;
	str->str = calloc(1, sizeof(char));

	return str;
}

UritResult
urit_parsetemplate(char *tpl, UritVars vars)
{
	UritResult res;
	res.uriref = urit_newstring();
	res.error = NULL;
	res.status = URIT_OK;
	res.tpl = tpl;
	char *exprend = NULL;
	char *pctenc = NULL;
	char *exprstr = NULL;
	size_t exprlen = 0;
	bool expr = false;
	uint8_t numbytes;
	unsigned char c;
	char curr;

	for (size_t i = 0, j = 0; (curr = tpl[i]); i++, j++) {
		if (!expr) {
			if (curr == '{') {
				expr = true;
				exprend = strchr(&tpl[i], '}');
				if (exprend == NULL) {
					urit_appendstring(res.uriref, tpl + i);
					urit_adderror(&res, j, URIT_MALFORMED_EXPRESSION);
					return res;
				} else {
					exprlen = exprend - (tpl + i) - 1;
					if (exprlen == 0) {
						urit_appendstring(res.uriref, "{}");
						urit_adderror(&res, j, URIT_EMPTY_EXPRESSION);
						expr = false;
						i++;
						j++;
					} else {
						exprstr = malloc(sizeof(char) * (exprlen + 1));
						snprintf(exprstr, exprlen + 1, "%s", &tpl[i] + 1);
						UritOpRule oprule = urit_getoprule(exprstr[0]);

						if (!oprule.op && !urit_isvarchar(exprstr)) {
							urit_appendchar(res.uriref, '{');
							urit_appendstring(res.uriref, exprstr);
							urit_appendchar(res.uriref, '}');
							urit_adderror(&res, ++j, URIT_UNIMPLEMENTED_OPERATOR);
							i += exprlen + 1;
							j += exprlen;
							expr = false;
						} else {
							if (oprule.op) {
								exprstr++;
							}
							urit_processexpression(exprstr, oprule, vars, &res, j);
						}
					}
				}
			} else if (urit_isreserved(curr) || urit_isunreserved(curr)) {
				urit_appendchar(res.uriref, curr);

			} else if (urit_isliteral(tpl)) {
				numbytes = urit_numbytes(curr);

				for (size_t k = 0; k < numbytes; k++, i++) {
					c = tpl[i];
					pctenc = urit_pctencodechar(c);
					urit_appendstring(res.uriref, pctenc);
				}
			} else {
				urit_adderror(&res, j, URIT_NONLITERAL_FOUND);
				return res;
			}
		} else {
			if (curr == '}') {
				expr = false;
			}
		}
	}
	res.uri = malloc(sizeof(char) * (strlen(res.uriref->str) + 1));
	strcpy(res.uri, res.uriref->str);
	return res;
}

static void
urit_addvar(UritVars *vars, UritVar *var)
{
	vars->vars = realloc(vars->vars, sizeof(UritVar *) * ++vars->count);
	vars->vars[vars->count - 1] = var;
}

static void
urit_adderror(UritResult *res, size_t pos, UritCode code)
{
	UritError *e = malloc(sizeof(UritError));
	e->pos = pos;
	e->code = code;
	e->next = NULL;

	if (res->error == NULL) {
		res->error = e;
	} else {
		UritError *err = res->error;
		while (err->next != NULL) {
			err = (UritError *) err->next;
		}
		err->next = (struct UritError *) e;
	}
	res->status = URIT_FAILURE;
}

static bool
urit_isreserved(const char c)
{
	switch (c) {
		case ':':
		case '/':
		case '?':
		case '#':
		case '[':
		case ']':
		case '@':
		case '!':
		case '$':
		case '&':
		case '\'':
		case '(':
		case ')':
		case '*':
		case '+':
		case ',':
		case ';':
		case '=':
		case '|':
			return true;
	}
	return false;
}

static bool
urit_isunreserved(const char c)
{
	if (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~') {
		return true;
	}
	return false;
}

static size_t
urit_numbytes(unsigned char c)
{
	if (c < 0xC0) {
		return 1;
	} else if (c < 0xE0) {
		return 2;
	} else if (c < 0xF0) {
		return 3;
	} else if (c < 0xF8) {
		return 4;
	} 
	return 0;
}

static unsigned
urit_getcodepoint(const char *str)
{
	uint8_t numbytes = urit_numbytes(*str);
	unsigned mask = (1 << (7 - numbytes)) - 1;
	unsigned submask = (1 << 6) - 1;
	unsigned last = *str & mask;

	switch (numbytes) {
		case 1:
			return *str;
		case 2:
			return (last << 6) + (str[1] & submask);
		case 3:
			return (last << 12) + ((str[1] & submask) << 6) + (str[2] & submask);
		case 4:
			return (last << 18) + ((str[1] & submask) << 12) + ((str[2] & submask) << 6) + (str[3] & submask);
	}
	return 0;
}

/**
 * From RFC:
 * ucschar	= %xA0-D7FF / %xF900-FDCF / %xFDF0-FFEF
 *			/ %x10000-1FFFD / %x20000-2FFFD / %x30000-3FFFD
 *			/ %x40000-4FFFD / %x50000-5FFFD / %x60000-6FFFD
 *			/ %x70000-7FFFD / %x80000-8FFFD / %x90000-9FFFD
 *			/ %xA0000-AFFFD / %xB0000-BFFFD / %xC0000-CFFFD
 *			/ %xD0000-DFFFD / %xE1000-EFFFD
 */

static bool
urit_isucschar(const char *str)
{
	unsigned c = urit_getcodepoint(str);

	if ((0xA0 <= c && c <= 0xD7FF) || (0xF900 <= c && c <= 0xFDCF) || (0xFDF0 <= c && c <= 0xFFEF) ||
		(0x10000 <= c && c <= 0x1FFFD) || (0x20000 <= c && c <= 0x2FFFD) || (0x30000 <= c && c <= 0x3FFFD) ||
		(0x40000 <= c && c <= 0x4FFFD) || (0x50000 <= c && c <= 0x5FFFD) || (0x60000 <= c && c <= 0x6FFFD) ||
		(0x70000 <= c && c <= 0x7FFFD) || (0x80000 <= c && c <= 0x8FFFD) || (0x90000 <= c && c <= 0x9FFFD) ||
		(0xA0000 <= c && c <= 0xAFFFD) || (0xB0000 <= c && c <= 0xBFFFD) || (0xC0000 <= c && c <= 0xCFFFD) ||
		(0xD0000 <= c && c <= 0xDFFFD) || (0xE0000 <= c && c <= 0xEFFFD)) {
		return true;
	}
	return false;
}

/**
 * From RFC:
 * iprivate	= %xE000-F8FF / %xF0000-FFFFD / %x100000-10FFFD
 */
static bool
urit_isiprivate(const char *str)
{
	unsigned c = urit_getcodepoint(str);

	if ((0xE000 <= c && c <= 0xF8FF) || (0xF0000 <= c && c <= 0xFFFFD) || (0x100000 <= c && c <= 0x10FFFD)) {
		return true;
	}
	return false;
}

static bool
urit_ispct(const char *str)
{
	uint8_t i = 0;

	if (strlen(str) < 3) {
		return false;
	}
	if (str[i++] != '%') {
		return false;
	}
	if (!isxdigit(str[i++])) {
		return false;
	}
	if (!isxdigit(str[i])) {
		return false;
	}
	return true;
}

/**
 * From RFC:
 * literals	= %x21 / %x23-24 / %x26 / %x28-3B / %x3D / %x3F-5B / %x5D / %x5F
 *			/ %x61-7A / %x7E / ucschar / iprivate / pct-encoded
 *				; any Unicode character except: CTL, SP, DQUOTE, "'",
 *				; "%" (aside from pct-encoded), "<", ">", "/", "^", `", "{"
 *				; "|", "}"
 */
static bool
urit_isliteral(const char *str)
{
	char c = *str;

	if (c == 0x21 || c == 0x23 || c == 0x24 || c == 0x26 || (0x28 <= c && c <= 0x3B) ||
		c == 0x3D || (0x3F <= c && c <= 0x5B) || c == 0x5D || c == 0x5F || (0x61 <= c && c <= 0x7A) ||
		c == 0x7E || urit_ispct(str) || urit_isucschar(str) || urit_isiprivate(str)) {
		return true;
	}
	return false;
}

static bool
urit_isvarchar(const char *str)
{
	if (urit_ispct(str) || isalnum(str[0]) || str[0] == '_') {
		return true;
	}
	return false;
}

static char *
urit_pctencodechar(const unsigned char c)
{
	char *pct = malloc(4);

	pct[0] = '%';
	sprintf(pct + 1, "%02X", c);
	pct[3] = '\0';

	return pct;
}

static UritList *
urit_compilelistvar(char *varvalue)
{
	UritList *list = malloc(sizeof(UritList));
	list->count = 0;
	list->values = malloc(sizeof(char *));
	UritString *buff = urit_newstring();
	char curr;
	bool esc = false;
	bool quot = false;
	bool delim = true;

	while ((curr = (++varvalue)[0])) {
		if (quot) {
			if (esc && (curr == '"' || curr == '\\')) {
				buff = urit_appendchar(buff, curr);
				esc = false;
			} else if (curr == '"') {
				list->values[list->count] = malloc(sizeof(char) * (buff->len + 1));
				strcpy(list->values[list->count], buff->str);
				list->count++;
				buff = urit_newstring();
				quot = false;
			} else if(curr == '\\') {
				esc = true;
			} else {
				buff = urit_appendchar(buff, curr);
				esc = false;
			}
		} else if (curr == '"') {
			if (delim) {
				quot = true;
				delim = false;
			} else {
				return NULL;
			}
		} else if (curr == ',') {
			if (delim) {
				return NULL;
			}
			delim = true;
		} else if (curr == ')') {
			if ((varvalue + 1)[0] != '\0' || delim) {
				return NULL;
			}
		} else if (curr != ' ') {
			return NULL;
		}
	}
	return list;
}

static UritMap *
urit_compilemapvar(char *varvalue)
{
	UritMap *map = malloc(sizeof(UritMap));
	map->count = 0;
	map->pairs = malloc(sizeof(UritPair *));

	UritString *buff = urit_newstring();
	bool esc = false;
	bool keyval = false;
	bool keyvals = false;
	bool quot = false;
	bool item = false;
	int8_t delim = -1;
	char curr;
	UritPair *pair;
	
	while ((curr = (++varvalue)[0])) {
		if (quot) {
			if (esc && (curr == '"' || curr == '\\')) {
				buff = urit_appendchar(buff, curr);
				esc = false;
			} else if (curr == '"') {
				if (!item) {
					pair = malloc(sizeof(UritPair));
					pair->key = malloc(sizeof(char) * (buff->len + 1));
					pair->val = NULL;
					strcpy(pair->key, buff->str);
				} else {
					pair->val = malloc(sizeof(char) * (buff->len + 1));
					strcpy(pair->val, buff->str);
					map->pairs[map->count] = pair;
					map->count++;
					keyvals = true;
				}
				buff = urit_newstring();
				quot = false;
				item = !item;
			} else if (curr == '\\') {
				esc = true;
			} else {
				buff = urit_appendchar(buff, curr);
				esc = false;
			}
		} else if (curr == '(') {
			if (keyval || delim == 0) {
				return NULL;
			}
			if (delim == 1) {
				delim = 0;
			}
			keyval = true;
			keyvals = false;
		} else if (curr == '"') {
			if (!keyval || (delim == 0 && item) || keyvals) {
				return NULL;
			}
			quot = true;
			delim = 0;
		} else if (curr == ',') {
			if (delim != 0 || (keyval && !item)) {
				return NULL;
			}
			delim = 1;
		} else if (curr == ')') {
			if (delim == 1 || item || !keyval) {
				return NULL;
			}
			keyval = false;
		} else if (curr == ']') {
			if ((varvalue + 1)[0] != '\0' || delim == 1 || keyval) {
				return NULL;
			}
		} else if (curr != ' ') {
			return NULL;
		}
	}
	return map;
}

static UritString *
urit_appendstring(UritString *des, char *src)
{
	size_t rem = des->size - (sizeof(char) * (des->len + 1));
	size_t len = strlen(src);
	size_t min = sizeof(char) * len;
	size_t inc;

	if (rem < min) {
		inc = (min / 10 + 1) * 10;
		des->size += inc;
		des->str = realloc(des->str, des->size);
	}
	strcat(des->str, src);
	des->len += len;

	return des;
}

static UritString *
urit_appendchar(UritString *des, char src)
{
	int rem = des->size - (sizeof(char) * (des->len + 1));

	if (rem < 1) {
		des->size += 10;
		des->str = realloc(des->str, des->size);
	}
	strncat(des->str, &src, 1);
	des->len++;

	return des;
}

static char *
urit_encode(char *str, bool allowreserved, size_t max)
{
	UritString *enc = urit_newstring();
	size_t count = 0;
	uint8_t numbytes;
	unsigned char curr;

	while ((curr = *str++)) {
		numbytes = urit_numbytes(curr);
		if (numbytes == 1) {
			if (urit_ispct(str - 1)) {
				urit_appendchar(enc, *(str - 1));
				urit_appendchar(enc, *str);
				urit_appendchar(enc, *(str + 1));
				str += 2;
			} else if (urit_isunreserved(curr) || (allowreserved && urit_isreserved(curr))) {
				urit_appendchar(enc, curr);
			} else {
				urit_appendstring(enc, urit_pctencodechar(curr));
			}
		} else if (urit_isliteral(str - 1)) {
			str--;
			for (size_t i = 0; i < numbytes; i++, str++) {
				urit_appendstring(enc, urit_pctencodechar(*str));
			}
		}
		count++;
		if (count == max) {
			return enc->str;
		}
	}
	return enc->str;
}

static UritOpRule
urit_getoprule(char c)
{
	UritOpRule oprule = {'\0', '\0', ',', false, '\0', false};

	switch (c) {
		case '#':
			oprule.first = true;
		case '+':
			oprule.allow = true;
			oprule.op = c;
			break;
		case ';':
			oprule.named = true;
		case '.':
		case '/':
			oprule.first = true;
			oprule.sep = c;
			oprule.op = c;
			break;
		case '?':
		case '&':
			oprule.first = true;
			oprule.sep = '&';
			oprule.named = true;
			oprule.ifemp = true;
			oprule.op = c;
			break;
	}
	return oprule;
}

static UritVar *
urit_getvar(UritVars *vars, char *name)
{
	for (size_t i = 0; i < (size_t) vars->count; i++) {
		if (strcmp(vars->vars[i]->name, name) == 0) {
			return vars->vars[i];
		}
	}
	return NULL;
}

static void
urit_processexpression(char *expr, UritOpRule oprule, UritVars vars, UritResult *res, size_t pos)
{
	UritVar *var;
	UritString *varname;
	UritString *prefixstr;
	bool expl;
	bool colon;
	bool namestate;
	bool dot;
	bool firstdig;
	int prefix;
	char curr;
	bool firstappend = true;
	char *varspec = strtok(expr, ",");

	while (varspec != NULL) {
		var = malloc(sizeof(UritVar));
		varname = urit_newstring();
		prefixstr = urit_newstring();
		prefix = 0;
		firstdig = false;
		expl = false;
		colon = false;
		dot = true;
		namestate = true;

		while ((curr = *varspec++)) {
			pos++;
			if (namestate) {
				if (curr == '.') {
					if (dot) {
						urit_adderror(res, pos, URIT_INVALID_VARNAME);
						return;
					}
					dot = true;
				} else if (curr == '*') {
					if (dot || !varname->len) {
						urit_adderror(res, pos, URIT_INVALID_VARNAME);
						return;
					}
					expl = true;
					namestate = false;
				} else if (curr == ':') {
					if (dot || !varname->len) {
						urit_adderror(res, pos, URIT_INVALID_VARNAME);
						return;
					}
					colon = true;
					firstdig = true;
					namestate = false;
				} else if (!urit_isvarchar(varspec - 1)) {
					urit_adderror(res, pos, URIT_INVALID_VARNAME);
					return;
				} else {
					varname = urit_appendchar(varname, curr);
					dot = false;
				}
			} else {
				if (curr == ':') {
					if (colon) {
						urit_adderror(res, pos, URIT_MALFORMED_EXPRESSION);
						return;
					}
					colon = true;
				} else if (colon) {
					if (!isdigit(curr) || (firstdig && curr == '0') || prefixstr->len == 4) {
						urit_adderror(res, pos, URIT_MALFORMED_EXPRESSION);
						return;
					}
					prefixstr = urit_appendchar(prefixstr, curr);
					firstdig = false;
				} else {
					urit_adderror(res, pos, URIT_MALFORMED_EXPRESSION);
					return;
				}
			}
		}

		if (prefixstr->len) {
			prefix = atoi(prefixstr->str);
		}

		var = urit_getvar(&vars, varname->str);
		if (!var) {
			varspec = strtok(NULL, ",");
			continue;
		}

		if (firstappend) {
			if (oprule.first) {
				urit_appendchar(res->uriref, oprule.op);
			}
			firstappend = false;
		} else {
			urit_appendchar(res->uriref, oprule.sep);
		}

		if (var->type == URIT_STRING) {
			
			char *val = urit_encode(var->val_string, oprule.allow, prefix);

			if (val == NULL) {
				puts("unable to encode");
				exit(EXIT_FAILURE);
			}
			if (oprule.named) {
				urit_appendstring(res->uriref, var->name);
				
				if (strlen(val)) {
					urit_appendchar(res->uriref, '=');
					urit_appendstring(res->uriref, val);
				} else {
					if (oprule.ifemp) {
						urit_appendchar(res->uriref, '=');
					}
					varspec = strtok(NULL, ",");
					continue;
				}
			} else {
				if (strlen(val)) {
					urit_appendstring(res->uriref, val);
				}
			}
		} else if (!expl) {
			if (var->type == URIT_LIST) {
				UritList *list = var->val_list;

				if (oprule.named) {
					urit_appendstring(res->uriref, var->name);

					if (list->count) {
						urit_appendchar(res->uriref, '=');
					} else {
						if (oprule.ifemp) {
							urit_appendchar(res->uriref, '=');
						}
						varspec = strtok(NULL, ",");
						continue;
					}
				}
				for (size_t i = 0; i < list->count; i++) {
					urit_appendstring(res->uriref, urit_encode(list->values[i], oprule.allow, prefix));
					if (i + 1 < list->count) {
						urit_appendchar(res->uriref, ',');
					}
				}
			} else {
				UritMap *map = var->val_map;

				if (oprule.named) {
					urit_appendstring(res->uriref, var->name);

					if (map->count) {
						urit_appendchar(res->uriref, '=');
					} else {
						if (oprule.ifemp) {
							urit_appendchar(res->uriref, '=');
						}
						varspec = strtok(NULL, ",");
						continue;
					}
				}
				for (size_t i = 0; i < map->count; i++) {
					urit_appendstring(res->uriref, urit_encode(map->pairs[i]->key, oprule.allow, prefix));
					urit_appendchar(res->uriref, ',');
					urit_appendstring(res->uriref, urit_encode(map->pairs[i]->val, oprule.allow, prefix));
					if (i + 1 < map->count) {
						urit_appendchar(res->uriref, ',');
					}
				}
			}
		} else {
			if (oprule.named) {
				if (var->type == URIT_LIST) {
					UritList *list = var->val_list;
					for (size_t i = 0; i < list->count; i++) {
						urit_appendstring(res->uriref, var->name);
						if (strlen(list->values[i])) {
							urit_appendchar(res->uriref, '=');
							urit_appendstring(res->uriref, urit_encode(list->values[i], oprule.allow, prefix));
						} else if (oprule.ifemp) {
							urit_appendchar(res->uriref, '=');
						}
						if (i + 1 < list->count) {
							urit_appendchar(res->uriref, oprule.sep);
						}
					}
				} else {
					UritMap *map = var->val_map;
					for (size_t i = 0; i < map->count; i++) {
						urit_appendstring(res->uriref, urit_encode(map->pairs[i]->key, oprule.allow, prefix));
						if (strlen(map->pairs[i]->val)) {
							urit_appendchar(res->uriref, '=');
							urit_appendstring(res->uriref, urit_encode(map->pairs[i]->val, oprule.allow, prefix));
						} else if (oprule.ifemp) {
							urit_appendchar(res->uriref, '=');
						}
						if (i + 1 < map->count) {
							urit_appendchar(res->uriref, oprule.sep);
						}
					}
				}
			} else {
				if (var->type == URIT_LIST) {
					UritList *list = var->val_list;
					for (size_t i = 0; i < list->count; i++) {
						urit_appendstring(res->uriref, urit_encode(list->values[i], oprule.allow, prefix));
						if (i + 1 < list->count) {
							urit_appendchar(res->uriref, oprule.sep);
						}
					}
				} else {
					UritMap *map = var->val_map;
					for (size_t i = 0; i < map->count; i++) {
						urit_appendstring(res->uriref, urit_encode(map->pairs[i]->key, oprule.allow, prefix));
						urit_appendchar(res->uriref, '=');
						urit_appendstring(res->uriref, urit_encode(map->pairs[i]->val, oprule.allow, prefix));
						if (i + 1 < map->count) {
							urit_appendchar(res->uriref, oprule.sep);
						}
					}
				}
			}
		}
		varspec = strtok(NULL, ",");
	}
}
