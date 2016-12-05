#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "uritlib.h"
#include "uritlib.c"

bool test_add_generic(void);
bool test_add_strings(void);
bool test_add_lists(void);
bool test_add_maps(void);
bool test_templates(UritVars vars, size_t count, char templates[][2][100]);

int
main(int argc, char **argv)
{
	bool success;

	puts("test_add_generic()");
	success = test_add_generic();
	if (!success) {
		puts("test_add_generic templates failed");
		return EXIT_SUCCESS;
	} else {
		puts("  success");
	}
	puts("test_add_strings()");
	success = test_add_strings();
	if (!success) {
		puts("test_add_strings templates failed");
		return EXIT_SUCCESS;
	} else {
		puts("  success");
	}
	puts("test_add_lists()");
	success = test_add_lists();
	if (!success) {
		puts("test_add_lists templates failed");
		return EXIT_SUCCESS;
	} else {
		puts("  success");
	}
	puts("test_add_map()");
	success = test_add_maps();
	if (!success) {
		puts("test_add_maps templates failed");
		return EXIT_SUCCESS;
	} else {
		puts("  success");
	}
	puts("All tests have passed");

	return EXIT_SUCCESS;
}

bool
test_templates(UritVars vars, size_t count, char templates[][2][100])
{
	bool success = true;
	char templateUrl[100] = "http://www.example.com/";

	for (int i = 0; i < (int) count; i++) {
		char template[100];
		strcpy(template, templateUrl);
		strcat(template, templates[i][0]);

		char correctExpansion[100];
		strcpy(correctExpansion, templateUrl);
		strcat(correctExpansion, templates[i][1]);

		UritResult res = urit_parsetemplate(template, vars);

		int pos = strcmp(res.uri, correctExpansion);
		if (pos != 0) {
			success = false;
			printf("Expanding '%s' failed, %s should be %s Res: %d\n", template, res.uri, correctExpansion, pos);
		}
	}
	return success;
}

bool
test_add_generic(void)
{
	char values[][2][100] = {
		{"count", "(\"one\",\"two\",\"three\")"},
		{"dom", "(\"example\",\"com\")"},
		{"dub", "me/too"},
		{"hello", "Hello World!"},
		{"half", "50%"},
		{"var", "value"},
		{"who", "fred"},
		{"base", "http://example.com/home/"},
		{"path", "/foo/bar"},
		{"list", "(\"red\",\"green\",\"blue\")"},
		{"keys", "[(\"semi\",\";\"),(\"dot\",\".\"),(\"comma\",\",\")]"},
		{"v", "6"},
		{"x", "1024"},
		{"y", "768"},
		{"empty", ""},
		{"empty_keys", "[]"}
	};
	char templates[][2][100] = {
		{"{count}", "one,two,three"},
		{"{count*}", "one,two,three"},
		{"{/count}", "/one,two,three"},
		{"{/count*}", "/one/two/three"},
		{"{;count}", ";count=one,two,three"},
		{"{;count*}", ";count=one;count=two;count=three"},
		{"{?count}", "?count=one,two,three"},
		{"{?count*}", "?count=one&count=two&count=three"},
		{"{&count*}", "&count=one&count=two&count=three"},
		{"{var}", "value"},
		{"{hello}", "Hello%20World%21"},
		{"{half}", "50%25"},
		{"O{empty}X", "OX"},
		{"O{undef}X", "OX"},
		{"{x,y}", "1024,768"},
		{"{x,hello,y}", "1024,Hello%20World%21,768"},
		{"?{x,empty}", "?1024,"},
		{"?{x,undef}", "?1024"},
		{"?{undef,y}", "?768"},
		{"{var:3}", "val"},
		{"{var:30}", "value"},
		{"{list}", "red,green,blue"},
		{"{list*}", "red,green,blue"},
		{"{keys}", "semi,%3B,dot,.,comma,%2C"},
		{"{keys*}", "semi=%3B,dot=.,comma=%2C"},
		{"{+var}", "value"},
		{"{+hello}", "Hello%20World!"},
		{"{+half}", "50%25"},
		{"{base}index", "http%3A%2F%2Fexample.com%2Fhome%2Findex"},
		{"{+base}index", "http://example.com/home/index"},
		{"O{+empty}X", "OX"},
		{"O{+undef}X", "OX"},
		{"{+path}/here", "/foo/bar/here"},
		{"here?ref={+path}", "here?ref=/foo/bar"},
		{"up{+path}{var}/here", "up/foo/barvalue/here"},
		{"{+x,hello,y}", "1024,Hello%20World!,768"},
		{"{+path,x}/here", "/foo/bar,1024/here"},
		{"{+path:6}/here", "/foo/b/here"},
		{"{+list}", "red,green,blue"},
		{"{+list*}", "red,green,blue"},
		{"{+keys}", "semi,;,dot,.,comma,,"},
		{"{+keys*}", "semi=;,dot=.,comma=,"},
		{"{#var}", "#value"},
		{"{#hello}", "#Hello%20World!"},
		{"{#half}", "#50%25"},
		{"foo{#empty}", "foo#"},
		{"foo{#undef}", "foo"},
		{"{#x,hello,y}", "#1024,Hello%20World!,768"},
		{"{#path,x}/here", "#/foo/bar,1024/here"},
		{"{#path:6}/here", "#/foo/b/here"},
		{"{#list}", "#red,green,blue"},
		{"{#list*}", "#red,green,blue"},
		{"{#keys}", "#semi,;,dot,.,comma,,"},
		{"{#keys*}", "#semi=;,dot=.,comma=,"},
		{"{.who}", ".fred"},
		{"{.who,who}", ".fred.fred"},
		{"{.half,who}", ".50%25.fred"},
		{"www{.dom*}", "www.example.com"},
		{"X{.var}", "X.value"},
		{"X{.empty}", "X."},
		{"X{.undef}", "X"},
		{"X{.var:3}", "X.val"},
		{"X{.list}", "X.red,green,blue"},
		{"X{.list*}", "X.red.green.blue"},
		{"X{.keys}", "X.semi,%3B,dot,.,comma,%2C"},
		{"X{.keys*}", "X.semi=%3B.dot=..comma=%2C"},
		{"X{.empty_keys}", "X"},
		{"X{.empty_keys*}", "X"},
		{"{/who}", "/fred"},
		{"{/who,who}", "/fred/fred"},
		{"{/half,who}", "/50%25/fred"},
		{"{/who,dub}", "/fred/me%2Ftoo"},
		{"{/var}", "/value"},
		{"{/var,empty}", "/value/"},
		{"{/var,undef}", "/value"},
		{"{/var,x}/here", "/value/1024/here"},
		{"{/var:1,var}", "/v/value"},
		{"{/list}", "/red,green,blue"},
		{"{/list*}", "/red/green/blue"},
		{"{/list*,path:4}", "/red/green/blue/%2Ffoo"},
		{"{/keys}", "/semi,%3B,dot,.,comma,%2C"},
		{"{/keys*}", "/semi=%3B/dot=./comma=%2C"},
		{"{;who}", ";who=fred"},
		{"{;half}", ";half=50%25"},
		{"{;empty}", ";empty"},
		{"{;v,empty,who}", ";v=6;empty;who=fred"},
		{"{;v,bar,who}", ";v=6;who=fred"},
		{"{;x,y}", ";x=1024;y=768"},
		{"{;x,y,empty}", ";x=1024;y=768;empty"},
		{"{;x,y,undef}", ";x=1024;y=768"},
		{"{;hello:5}", ";hello=Hello"},
		{"{;list}", ";list=red,green,blue"},
		{"{;list*}", ";list=red;list=green;list=blue"},
		{"{;keys}", ";keys=semi,%3B,dot,.,comma,%2C"},
		{"{;keys*}", ";semi=%3B;dot=.;comma=%2C"},
		{"{?who}", "?who=fred"},
		{"{?half}", "?half=50%25"},
		{"{?x,y}", "?x=1024&y=768"},
		{"{?x,y,empty}", "?x=1024&y=768&empty="},
		{"{?x,y,undef}", "?x=1024&y=768"},
		{"{?var:3}", "?var=val"},
		{"{?list}", "?list=red,green,blue"},
		{"{?list*}", "?list=red&list=green&list=blue"},
		{"{?keys}", "?keys=semi,%3B,dot,.,comma,%2C"},
		{"{?keys*}", "?semi=%3B&dot=.&comma=%2C"},
		{"{&who}", "&who=fred"},
		{"{&half}", "&half=50%25"},
		{"?fixed=yes{&x}", "?fixed=yes&x=1024"},
		{"{&x,y,empty}", "&x=1024&y=768&empty="},
		{"{&x,y,undef}", "&x=1024&y=768"},
		{"{&var:3}", "&var=val"},
		{"{&list}", "&list=red,green,blue"},
		{"{&list*}", "&list=red&list=green&list=blue"},
		{"{&keys}", "&keys=semi,%3B,dot,.,comma,%2C"},
		{"{&keys*}", "&semi=%3B&dot=.&comma=%2C"}
	};

	UritVars vars = urit_newvars();

	for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
		urit_addvariable(&vars, values[i][0], values[i][1]);
	}
	return test_templates(vars, 115, templates);
}

bool
test_add_strings(void)
{
	char values[][2][100] = {
		{"dub", "me/too"},
		{"hello", "Hello World!"},
		{"half", "50%"},
		{"var", "value"},
		{"who", "fred"},
		{"base", "http://example.com/home/"},
		{"path", "/foo/bar"},
		{"v", "6"},
		{"x", "1024"},
		{"y", "768"},
		{"empty", ""}
	};
	char templates[][2][100] = {
		{"{var}", "value"},
		{"{hello}", "Hello%20World%21"},
		{"{half}", "50%25"},
		{"O{empty}X", "OX"},
		{"O{undef}X", "OX"},
		{"{x,y}", "1024,768"},
		{"{x,hello,y}", "1024,Hello%20World%21,768"},
		{"?{x,empty}", "?1024,"},
		{"?{x,undef}", "?1024"},
		{"?{undef,y}", "?768"},
		{"{var:3}", "val"},
		{"{var:30}", "value"},
		{"{+var}", "value"},
		{"{+hello}", "Hello%20World!"},
		{"{+half}", "50%25"},
		{"{base}index", "http%3A%2F%2Fexample.com%2Fhome%2Findex"},
		{"{+base}index", "http://example.com/home/index"},
		{"O{+empty}X", "OX"},
		{"O{+undef}X", "OX"},
		{"{+path}/here", "/foo/bar/here"},
		{"here?ref={+path}", "here?ref=/foo/bar"},
		{"up{+path}{var}/here", "up/foo/barvalue/here"},
		{"{+x,hello,y}", "1024,Hello%20World!,768"},
		{"{+path,x}/here", "/foo/bar,1024/here"},
		{"{+path:6}/here", "/foo/b/here"},
		{"{#var}", "#value"},
		{"{#hello}", "#Hello%20World!"},
		{"{#half}", "#50%25"},
		{"foo{#empty}", "foo#"},
		{"foo{#undef}", "foo"},
		{"{#x,hello,y}", "#1024,Hello%20World!,768"},
		{"{#path,x}/here", "#/foo/bar,1024/here"},
		{"{#path:6}/here", "#/foo/b/here"},
		{"{.who}", ".fred"},
		{"{.who,who}", ".fred.fred"},
		{"{.half,who}", ".50%25.fred"},
		{"X{.var}", "X.value"},
		{"X{.empty}", "X."},
		{"X{.undef}", "X"},
		{"X{.var:3}", "X.val"},
		{"{/who}", "/fred"},
		{"{/who,who}", "/fred/fred"},
		{"{/half,who}", "/50%25/fred"},
		{"{/who,dub}", "/fred/me%2Ftoo"},
		{"{/var}", "/value"},
		{"{/var,empty}", "/value/"},
		{"{/var,undef}", "/value"},
		{"{/var,x}/here", "/value/1024/here"},
		{"{/var:1,var}", "/v/value"},
		{"{;who}", ";who=fred"},
		{"{;half}", ";half=50%25"},
		{"{;empty}", ";empty"},
		{"{;v,empty,who}", ";v=6;empty;who=fred"},
		{"{;v,bar,who}", ";v=6;who=fred"},
		{"{;x,y}", ";x=1024;y=768"},
		{"{;x,y,empty}", ";x=1024;y=768;empty"},
		{"{;x,y,undef}", ";x=1024;y=768"},
		{"{;hello:5}", ";hello=Hello"},
		{"{?who}", "?who=fred"},
		{"{?half}", "?half=50%25"},
		{"{?x,y}", "?x=1024&y=768"},
		{"{?x,y,empty}", "?x=1024&y=768&empty="},
		{"{?x,y,undef}", "?x=1024&y=768"},
		{"{?var:3}", "?var=val"},
		{"{&who}", "&who=fred"},
		{"{&half}", "&half=50%25"},
		{"?fixed=yes{&x}", "?fixed=yes&x=1024"},
		{"{&x,y,empty}", "&x=1024&y=768&empty="},
		{"{&x,y,undef}", "&x=1024&y=768"},
		{"{&var:3}", "&var=val"},
	};

	UritVars vars = urit_newvars();

	for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
		urit_addstringvar(&vars, values[i][0], values[i][1]);
	}
	return test_templates(vars, 70, templates);
}

bool
test_add_lists(void)
{
	char *var_count[3] = {"one","two","three"};
	char *var_dom[2] = {"example","com"};
	char *var_list[3] = {"red","green","blue"};

	char **values[3] = {var_count, var_dom, var_list};
	char *valnames[3] = {"count","dom","list"};

	char templates[][2][100] = {
		{"{count}", "one,two,three"},
		{"{count*}", "one,two,three"},
		{"{/count}", "/one,two,three"},
		{"{/count*}", "/one/two/three"},
		{"{;count}", ";count=one,two,three"},
		{"{;count*}", ";count=one;count=two;count=three"},
		{"{?count}", "?count=one,two,three"},
		{"{?count*}", "?count=one&count=two&count=three"},
		{"{&count*}", "&count=one&count=two&count=three"},
		{"{list}", "red,green,blue"},
		{"{list*}", "red,green,blue"},
		{"{+list}", "red,green,blue"},
		{"{+list*}", "red,green,blue"},
		{"{#list}", "#red,green,blue"},
		{"{#list*}", "#red,green,blue"},
		{"www{.dom*}", "www.example.com"},
		{"X{.list}", "X.red,green,blue"},
		{"X{.list*}", "X.red.green.blue"},
		{"{/list}", "/red,green,blue"},
		{"{/list*}", "/red/green/blue"},
		{"{;list}", ";list=red,green,blue"},
		{"{;list*}", ";list=red;list=green;list=blue"},
		{"{?list}", "?list=red,green,blue"},
		{"{?list*}", "?list=red&list=green&list=blue"},
		{"{&list}", "&list=red,green,blue"},
		{"{&list*}", "&list=red&list=green&list=blue"}
	};

	UritVars vars = urit_newvars();
	bool success = true;
	size_t s[3] = {3,2,3};
	for (int i = 0; i < 3; i++) {
		urit_addlistvar(&vars, valnames[i],s[i], values[i]);
	}
	if (!test_templates(vars, 26, templates)) {
		success = false;
		puts("test_add_lists:arr failed");
	}
	vars = urit_newvars();
	for (int i = 0; i < 3; i++) {
		UritList *l = urit_newlist();
		for (int j = 0; j < s[i]; j++) {
			urit_listadditem(values[i][j], l);
		}
		urit_varsaddlist(&vars, valnames[i], l);
	}
	if (!test_templates(vars, 26, templates)) {
		success = false;
		puts("test_add_generic:str failed");
	}
	return success;
}

bool test_add_maps(void)
{
	char templates[][2][100] = {
		{"{keys}", "semi,%3B,dot,.,comma,%2C"},
		{"{keys*}", "semi=%3B,dot=.,comma=%2C"},
		{"{+keys}", "semi,;,dot,.,comma,,"},
		{"{+keys*}", "semi=;,dot=.,comma=,"},
		{"{#keys}", "#semi,;,dot,.,comma,,"},
		{"{#keys*}", "#semi=;,dot=.,comma=,"},
		{"X{.keys}", "X.semi,%3B,dot,.,comma,%2C"},
		{"X{.keys*}", "X.semi=%3B.dot=..comma=%2C"},
		{"X{.empty_keys}", "X"},
		{"X{.empty_keys*}", "X"},
		{"{/keys}", "/semi,%3B,dot,.,comma,%2C"},
		{"{/keys*}", "/semi=%3B/dot=./comma=%2C"},
		{"{;keys}", ";keys=semi,%3B,dot,.,comma,%2C"},
		{"{;keys*}", ";semi=%3B;dot=.;comma=%2C"},
		{"{?keys}", "?keys=semi,%3B,dot,.,comma,%2C"},
		{"{?keys*}", "?semi=%3B&dot=.&comma=%2C"},
		{"{&keys}", "&keys=semi,%3B,dot,.,comma,%2C"},
		{"{&keys*}", "&semi=%3B&dot=.&comma=%2C"}
	};
	char *keys[3][2] = {
		{"semi",";"},
		{"dot","."},
		{"comma",","}
	};
	bool success = true;
	UritVars vars = urit_newvars();

	urit_addmapvar(&vars, "keys", 3, keys);
	if (!test_templates(vars, 18, templates)) {
		success = false;
		puts("test_add_map:arrarr failed");
	}

	vars = urit_newvars();
	UritMap *m = urit_newmap();
	for (int i = 0; i < 3; i++) {
		urit_mapaddkeyval(keys[i][0], keys[i][1], m);
	}
	urit_varsaddmap(&vars, "keys", m);
	if (!test_templates(vars, 18, templates)) {
		success = false;
		puts("test_add_map:arr failed");
	}
	return success;
}

