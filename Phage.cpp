#pragma once
#include <windows.h>
#include <direct.h>
#include <sstream>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>

// --------------------------------------------------------------------------------------
#define PRINT(msg){std::stringstream ss; ss << "\n> " << msg; printf(ss.str().c_str());}
#define VAR(nm)		Phg::gvarnamemap[ #@nm ]

#define MYTRACE(msg)//{std::stringstream ss; ss << "\n" << msg; printf(ss.str().c_str()); ::OutputDebugString(ss.str().c_str());}
#define MYASSERT(s) if(!(s)) throw;
#define INVALIDVAR	0
#define INVALIDFUN	0

#define var			float
#define opr			char
#define varname		char
#define funcname	char
#define function	const char*

#define NAME		0x01FF
#define NUMBER		0x02FF
#define OPR			0x03FF
#define LGOPR		0x04FF

//namespace Phg{
// --------------------------------------------------------------------------------------
struct code;
void statement(code& cd);
var callfunc(code& cd);
// --------------------------------------------------------------------------------------
var gvarnamemap[256] = {0};
char rank[256];
// --------------------------------------------------------------------------------------
inline bool checkspace(char c){
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}
inline bool iscalc(char c){
	return c == '+' || c == '-' || c == '*'|| c == '/' || c == '!';
}
inline bool islogic(char c){
	return c == '>' || c == '<' || c == '=' || c == '&' || c == '|';
}
inline bool isname(char c){
	return c >= 'a' && c <= 'z';
}

// --------------------------------------------------------------------------------------
struct codestack_t{
	const char* buff[1024];
	int top;
	void push(const char* c){
		MYASSERT(top + 1 < 1024)
		buff[++ top] = c;
	}
	const char* pop(){
		MYASSERT(top > -1)
		return buff[top --];
	}
	const char* cur(){
		MYASSERT(top != -1)
		return buff[top];
	}
	bool empty(){
		return top == -1;
	}
	codestack_t(){
		top = -1;
	}
};

struct valstack_t{
	var buff[1024];
	int top;
	void push(var v){
		MYASSERT(top + 1 < 1024)
		buff[++ top] = v;
	}
	var pop(){
		MYASSERT(top > -1)
		return buff[top --];
	}
	var cur(){
		MYASSERT(top != -1)
		return buff[top];
	}
	valstack_t(){
		top = -1;
	}
};
struct oprstack_t{
	opr buff[1024];
	int top;
	void push(opr c){
		MYASSERT(top + 1 < 1024)
		buff[++ top] = c;
	}
	opr pop(){
		MYASSERT(top > -1)
		return buff[top --];
	}
	opr cur(){
		MYASSERT(top != -1)
		return buff[top];
	}
	void setcur(opr o){
		MYASSERT(top != -1)
		buff[top] = o;
	}
	bool empty(){
		return top == -1;
	}
	oprstack_t(){
		top = -1;
	}
};
// --------------------------------------------------------------------------------------
struct code{
	const char*		ptr;
	function		funcnamemap[256];
	var*			varnamemap;
	codestack_t		codestack;
	valstack_t		valstack;
	oprstack_t		oprstack;
		
	code(const char* buf){
		ptr = buf;
		varnamemap = 0;
		memset(funcnamemap, 0, sizeof(funcnamemap));
	}
	char next(){
		while(!eoc() && checkspace(*(++ptr)));

		MYTRACE("> " << (*ptr))
		return (*ptr);
	}
	char getnext(){
		const char* p = ptr;
		while(!eoc() && checkspace(*(++p)));
		return (*p);
	}
	bool eoc(const char* p = 0){
		p == 0 ? p = ptr : 0;
		return (*p) == '\0';
	}
	char cur(){
		return *ptr;
	}
};
// --------------------------------------------------------------------------------------
int get(code& cd){
	for(;!cd.eoc();cd.next()){
		char c = cd.cur();
		if(checkspace(c))
			continue;
		else if(isdigit(c)){
			return NUMBER;
		}
		else if(iscalc(c)){
			return OPR;
		}
		else if(islogic(c)){
			return LGOPR;
		}
		else if(isname(c)){
			return NAME;
		}				
		else
			return c;
	}
	return 0;
}
// --------------------------------------------------------------------------------------
var act(code & cd, int valcnt){
	opr o = cd.oprstack.pop();
	switch(o){
		case '+':{
			if(valcnt >= 2){
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a + b;
			}
			else{
				return + cd.valstack.pop();
			}	
		}
		case '-':{
			if(valcnt >= 2){
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a - b;
			}
			else{
				return - cd.valstack.pop();
			}			
		}
		case '*':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a * b;
		}
		case '/':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a / b;
		}
		case '>':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a > b;
		}
		case '<':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a < b;
		}
		case '=':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a == b;
		}
		case '&':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a && b;
		}
		case '|':{
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a || b;
		}
		case '!':{
			var a = cd.valstack.pop();
			return !a;
		}
		default: {return 0;}
	}
}
var chars2var(code& cd){
	static char buff[128];
	int i = 0;
	for(; i < 128; i ++){
		char c = cd.cur();
		if(!isdigit(c) && c != '.')
			break;
		buff[i] = c;
		cd.next();
	}
	buff[i] = '\0';
	return atof(buff);
}
// --------------------------------------------------------------------------------------
void getval(code& cd, int type, char c){
	if(type == NUMBER){
			//cd.valstack.push(cd.cur() - 48);
			cd.valstack.push(chars2var(cd));
			if(cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))){
				cd.oprstack.push('.');
			}
			//cd.next();
	}
	else if(type == NAME){
		if(cd.funcnamemap[cd.cur()] != INVALIDFUN){
			cd.valstack.push(callfunc(cd));
		}
		else{
			//MYASSERT(cd.varnamemap[cd.cur()] != INVALIDVAR)
			cd.valstack.push(cd.varnamemap[cd.cur()]);
			cd.next();
		}
		if(cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))){
			cd.oprstack.push('.');
		}
	}
}
// --------------------------------------------------------------------------------------
void finishtrunk(code& cd, int trunkcnt = 0, char sk= '{', char ek= '}'){
	while(!cd.eoc()){
		char c = cd.cur();
		if(c == sk){
			trunkcnt ++;
		}
		else if(c == ek){
			trunkcnt --;
			if(trunkcnt == 0){
				cd.next();
				break;
			}
		}
		cd.next();
	}
}
// --------------------------------------------------------------------------------------
var expr(code& cd){
	int valcnt = 0;
	while(!cd.eoc()){
		int type = get(cd);
		if(type == NAME || type == NUMBER){
			getval(cd, type, cd.cur());
			valcnt ++;
		}
		else if(type == OPR){
			opr o = cd.cur();
			if(!cd.oprstack.empty() && cd.oprstack.cur() == '.')
				cd.oprstack.setcur(o);
			else
				cd.oprstack.push(o);
			cd.next();

			// Ò»ÔªÔËËã·ûÓÅÏÈ¼¶¸ß£¡
			if(iscalc(cd.cur())){
				getval(cd, type, cd.cur());
				cd.valstack.push(expr(cd));
				valcnt ++;
			}
			else{
				char no = cd.getnext();
				if(cd.cur() != '(' && iscalc(no))
				{
					type = get(cd);
					if(rank[o] >= rank[no]){
						getval(cd, type, cd.cur());
						cd.valstack.push(act(cd, valcnt));
						valcnt ++;
					}
					else{
						getval(cd, type, cd.cur());
						cd.valstack.push(expr(cd));
						valcnt ++;
					}
				}
			}
		}
		else if(type == LGOPR){
			if(cd.oprstack.cur() == '.')
				cd.oprstack.setcur(cd.cur());
			else
				cd.oprstack.push(cd.cur());
			cd.next();
		}		
		else{
			char c = cd.cur();
			if(c == '('){
				cd.next();
				var v = expr(cd);
				cd.next();
				cd.valstack.push(v);
				valcnt ++;
			}
			else if(c == ')' || c == ';' || c == ',' || c == '{'){
					
				if(!cd.oprstack.empty() && 
					(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur())))
					return act(cd, valcnt);
				else{
					return cd.valstack.pop();
				}
			}
		}
	}
	MYASSERT(false)
	return INVALIDVAR;
}
// --------------------------------------------------------------------------------------
void singvar(code& cd){
	varname name = cd.cur();
	cd.next();
	MYASSERT(cd.cur() == '=')
	cd.next();
	
	var v = expr(cd);
	cd.next();
	cd.varnamemap[name] = v;
	
	MYTRACE(name << " = " << v)
}
// --------------------------------------------------------------------------------------
void statement(code& cd){
	int type = get(cd);
	if(type == NAME){
		char nc = cd.getnext();
		if(nc == '='){
			singvar(cd);
		}
		else if(nc == '('){
			callfunc(cd);
			cd.next();
		}
	}
	else if(cd.cur() == '>'){
		cd.next();
		PRINT(expr(cd))
		cd.next();
	}

}
// --------------------------------------------------------------------------------------
int subtrunk(code& cd, var& ret){
	MYTRACE("subtrunk{")
	while(!cd.eoc()){
		int type = get(cd);
		
		if(cd.cur() == '{'){
			cd.next();
			int rettype = subtrunk(cd, ret);
			MYTRACE("subtrunk " << rettype << "}")
			if (rettype > 0)
				return rettype;
		}
		else if(cd.cur() == '}'){
			cd.next();
			break;			
		}		
		else if(type == '?'){
			MYASSERT(cd.next() == '(')
			cd.next();
			var e = expr(cd);
			cd.next();

			if(e == 0){
				finishtrunk(cd, 0, '{', '}');
				if(cd.cur() == ':')
					cd.next();
				else
					continue; 
			}
			cd.next();

			int rettype = subtrunk(cd, ret);
			if(rettype == 1)
			{	
				finishtrunk(cd, 1);
				MYTRACE("subtrunk " << rettype << "}")
				return rettype;
			}
			if(rettype == 2){
				MYTRACE("subtrunk " << rettype << "}")
				return rettype;
			}
			
		}
		else if(type == '@'){
			if(cd.next() == '('){
				cd.next();
			
				const char* cp = cd.ptr;
	codepos1:				
				var e = expr(cd);
				cd.next();
				if(e != 0){
					cd.next();
					int rettype = subtrunk(cd, ret);
					if(rettype == 1)
					{	
						finishtrunk(cd, 1);
						break;
					}
					if(rettype == 2){
						MYTRACE("subtrunk " << rettype << "}")
						return rettype;
					}

					cd.ptr = cp;
					MYTRACE("loop")
	goto codepos1;							
				}
				else{
					finishtrunk(cd, 0);
				}	
			}
			else{				
				int loopcnt = expr(cd);
				cd.next();
				const char* cp = cd.ptr;
				for(int i = 0; i < loopcnt; i ++){
					int rettype = subtrunk(cd, ret);
					if(rettype == 1)
					{	
						finishtrunk(cd, 1);
						break;
					}
					if(rettype == 2){
						MYTRACE("subtrunk " << rettype << "}")
						return rettype;
					}

					cd.ptr = cp;
					MYTRACE("loop")
				}
			}
		}
		else if(type == '$'){			
			cd.next();
			if(cd.cur() == '@'){
				MYTRACE("subtrunk " << 1 << "}")
				return 1;
			}
			else{
				MYTRACE("subtrunk " << 2 << "}")
				ret = expr(cd);
				return 2;
			}
		}
		else
			statement(cd);
	}
	MYTRACE("subtrunk 0}")
	return 0;
}

// --------------------------------------------------------------------------------------
void trunk(code& cd, var& ret){
	var* omp = cd.varnamemap;	
	cd.varnamemap = gvarnamemap;

	subtrunk(cd, ret);

	cd.varnamemap = omp;
}
// --------------------------------------------------------------------------------------
var callfunc(code& cd){
	funcname fnm = cd.cur();
	MYASSERT(cd.funcnamemap[fnm] != 0)
	MYASSERT(cd.next() == '(')
	
	cd.next();
	while(!cd.eoc()){
		char c = cd.cur();
		if(c == ')'){
			cd.next();
			break;
		}
		else if(c == ','){
			cd.next();
			continue;
		}
		else{
			var e = expr(cd);			
			cd.valstack.push(e);
		}
	}

	cd.codestack.push(cd.ptr);
	cd.ptr = cd.funcnamemap[fnm];
	cd.next();
	MYASSERT(cd.cur() == '(');	

	var* omp = cd.varnamemap;
	var varnamemap[256];
	cd.varnamemap = varnamemap;

	while(!cd.eoc()){
		char c = cd.next();
		if(c == ')'){
			cd.next();
			break;
		}
		else if(c == ','){
		}
		else{
			//MYASSERT(cd.varnamemap[c] == 0)
			cd.varnamemap[c] = cd.valstack.pop();
		}
	}
	cd.next();

	var ret = INVALIDVAR;
	MYASSERT(subtrunk(cd, ret) == 2)
	cd.varnamemap = omp;

	cd.ptr = cd.codestack.pop();
	MYTRACE("func = " << ret)
	return ret;
}

// --------------------------------------------------------------------------------------
static void func(code &cd){
	funcname fnm = cd.cur();
	MYASSERT(cd.funcnamemap[fnm] == 0);
	cd.funcnamemap[fnm] = cd.ptr;
	cd.next();
	finishtrunk(cd, 0);
}
// --------------------------------------------------------------------------------------
void parser(code & cd){
	MYTRACE("-------phg--------")
	MYTRACE(cd.ptr)
	MYTRACE("------------------")
	
	rank['+'] = 1;
	rank['-'] = 1;
	rank['*'] = 2;
	rank['/'] = 2;
	rank['!'] = 3;

	while(!cd.eoc()){
		int type = get(cd);
		if(type == '#'){
			cd.next();
			func(cd);
		}
		else {
			var ret = INVALIDVAR;
			trunk(cd, ret);
		}
	}
}
// --------------------------------------------------------------------------------------
void phg_dofile(const char* filename){
	FILE* f = fopen(filename, "rb");
	MYASSERT(f != 0);
	char buf[1024];
	int sp = ftell(f);
	fseek(f, 0, SEEK_END);
	int sz = ftell(f) - sp;
	fseek(f, 0, SEEK_SET);
	fread(buf, 1, sz, f);
	buf[sz] = '\0';
	fclose(f);
	code cd(buf);
	parser(cd);
	PRINT("\n")
}
// --------------------------------------------------------------------------------------
 int main(int nargs, char* args[]){
	 if(nargs < 2)
		 return 0;
	 MYTRACE("phg_dofile:> " << args[1])
	 phg_dofile(args[1]);
	 getchar();
	 return 0;
 }

//}
