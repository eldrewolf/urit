# URIT

A URI template library written in C conforming to the [RFC 6570](https://tools.ietf.org/html/rfc6570) standards for URI templates.

## Usage
### String Variables
```c
UritVars vars = urit_newvars();
urit_addstringvar(&vars, "username", "mark");

UritResult res = urit_parsetemplate("http://example.com/~{username}/", vars);
//res.uri => http://example.com/~mark/
```

### List Variables
Method 1:
```c
UritVars vars = urit_newvars();
char *list[3] = {"red","green","blue"};
urit_addlistvar(&vars, "mylist", 3, list);
UritResult res = urit_parsetemplate("http://example.com{/mylist*}/", vars);
//res.uri => http://example.com/red/green/blue/
```
Method 2:
```c
UritVars vars = urit_newvars();
char *list[3] = {"one","two","three"};
UritList *l = urit_newlist();
for (int i = 0; i < 3; i++) {
	urit_listadditem(list[i], l);
}
urit_varsaddlist(&vars, "mylist", l);
UritResult res = urit_parsetemplate("http://example.com/{;mylist}", vars);
//res.uri => http://example.com/;mylist=one,two,three
```
Method 3:
```c
UritVars vars = urit_newvars();
urit_addvariable(&vars, "fruits", "(\"apple\",\"orange\",\"banana\")");
UritResult res = urit_parsetemplate("http://example.com/{?fruits}", vars);
//res.uri => http://example.com/?fruits=apple,orange,banana
```
### Map Variables
Method 1:
```c
UritVars vars = urit_newvars();
char *keys[3][2] = {
	{"semi",";"},
	{"dot","."},
	{"comma",","}
};
urit_addmapvar(&vars, "mykeys", 3, keys);
UritResult res = urit_parsetemplate("http://example.com/{?mykeys*}", vars);
//res.uri => http://example.com/?semi=%3B&dot=.&comma=%2C
```
Method 2:
```c
UritVars vars = urit_newvars();
char *keys[3][2] = {
	{"x","0"},
	{"y","1"},
	{"z","2"}
};
UritMap *m = urit_newmap();
for (int i = 0; i < 3; i++) {
	urit_mapaddkeyval(keys[i][0], keys[i][1], m);
}
urit_varsaddmap(&vars, "params", m);
urit_printvars(vars);
UritResult res = urit_parsetemplate("http://example.com?test=true{&params*}", vars);
//res.uri => http://example.com?test=true&x=0&y=1&z=2
```
Method 3:
```c
UritResult res;
UritVars vars = urit_newvars();
urit_addvariable(&vars, "metas", "[(\"foo\",\"bar\"),(\"spam\",\"eggs\")]");
UritResult res = urit_parsetemplate("http://example.com/{#metas*}", vars);
//res.uri => http://example.com/#foo=bar,spam=eggs
```
###Error handling
```c
UritResult res;
UritVars vars = urit_newvars();
urit_addstringvar(&vars, "username", "fred");
UritResult res = urit_parsetemplate("http://example.com/~{username}/", vars);

if (res->status != URIT_OK) {
	UritError *e = res->error;
	while (e) {
		/*e->pos is the character position the error was found
		  e->code is one of:
			URIT_MALFORMED_EXPRESSION
			URIT_EMPTY_EXPRESSION
			URIT_UNIMPLEMENTED_OPERATOR
			URIT_NONLITERAL_FOUND
			URIT_INVALID_VARNAME
		*/
		e = e->next;
	}
}
```
A command-line program is provided for testing purposes
```c
	make
	./urit "http://example.com/{var}/" --var="value"
```
## License

URIT is free software. You are free to redistribute it and/or modify it under the terms of the GNU Lesser General Public License Version 3 (LGPLv3) as published by the Free Software Foundation
See the [LICENSE](LICENSE.md) file for license rights and limitations (LGPLv3).
