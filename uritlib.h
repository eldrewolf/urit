#ifndef URITLIB_H
#define URITLIB_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define URIT_OK						0
#define URIT_FAILURE				1
#define URIT_MALFORMED_EXPRESSION	2
#define URIT_EMPTY_EXPRESSION		3
#define URIT_UNIMPLEMENTED_OPERATOR	4
#define URIT_NONLITERAL_FOUND		5
#define URIT_MALFORMED_LIST			6
#define URIT_MALFORMED_MAP			7
#define URIT_INVALID_VARNAME		8
#define URIT_DUPLICATE_VARIABLE		9

typedef enum { URIT_STRING, URIT_LIST, URIT_MAP } UritValueType;
typedef int UritStatus;
typedef int UritCode;

typedef struct {
	size_t len;
	size_t size;
	char *str;
} UritString;

typedef struct {
	size_t count;
	char **values;
} UritList;

typedef struct {
	char *key;
	char *val;
} UritPair;

typedef struct {
	size_t count;
	UritPair **pairs;
} UritMap;

typedef struct {
	char *name;
	UritValueType type;
	size_t index;
	union {
		char *val_string;
		UritList *val_list;
		UritMap *val_map;
	};
} UritVar;

typedef struct {
	size_t count;
	UritVar **vars;
} UritVars;

typedef struct {
	char op;
	bool first;
	char sep;
	bool named;
	bool ifemp;
	bool allow;
} UritOpRule;

typedef struct {
	UritCode code;
	size_t pos;
	struct UritError *next;
} UritError;

typedef struct {
	UritStatus status;
	UritString *uriref;
	char *uri;
	char *tpl;
	UritError *error;
} UritResult;

UritVars urit_newvars(void);
void urit_printvars(UritVars vars);
void urit_printerrors(UritResult *r);
UritStatus urit_addvariable(UritVars *vars, char *varname, char *varvalue);

UritString *urit_newstring(void);
void urit_addstringvar(UritVars *vars, char *varname, char *varvalue);

UritList *urit_newlist(void);
void urit_addlistvar(UritVars *vars, char *name, size_t count, char **list);
void urit_listadditem(char *str, UritList *list);
void urit_varsaddlist(UritVars *vars, char *name, UritList *list);

UritMap *urit_newmap(void);
void urit_addmapvar(UritVars *vars, char *name, size_t count, char *map[][2]);
void urit_mapaddkeyval(char *key, char *val, UritMap *map);
void urit_varsaddmap(UritVars *vars, char *name, UritMap *map);

UritResult urit_parsetemplate(char *tpl, UritVars vars);
#endif
