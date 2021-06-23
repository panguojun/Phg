/****************************************************************************
				Phg2.0

****************************************************************************/
namespace PHG
{
	#define INVALIDVAR	0
	#define INVALIDFUN	cd.funcnamemap.end()

	#define var			float
	#define opr			char
	#define varname		string
	#define funcname	string
	#define function	const char*

	#define NAME		0x01FF
	#define NUMBER		0x02FF
	#define OPR			0x03FF
	#define LGOPR		0x04FF

	// ----------------------------------------------------------------------
	extern struct code;
	extern void statement(code& cd);
	extern var callfunc(code& cd);

	//var gvarnamemap[256] = { 0 };
	char rank[256];

	// -----------------------------------------------------------------------
	inline bool checkspace(char c) {
		return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
	}
	inline bool iscalc(char c) {
		return c == '+' || c == '-' || c == '*' || c == '/' || c == '!';
	}
	inline bool islogic(char c) {
		return c == '>' || c == '<' || c == '=' || c == '&' || c == '|';
	}
	inline bool isname(char c) {
		return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
	}
	inline bool isbrackets(char c) {
		return c == '(';
	}
	// stacks define
	struct codestack_t 
	{
		const char* buff[1024];
		int top;
		void push(const char* c) {
			ASSERT(top + 1 < 1024)
				buff[++top] = c;
		}
		const char* pop() {
			ASSERT(top > -1)
				return buff[top--];
		}
		const char* cur() {
			ASSERT(top != -1)
				return buff[top];
		}
		bool empty() {
			return top == -1;
		}
		codestack_t() {
			top = -1;
		}
	};

	struct valstack_t 
	{
		var buff[1024];
		int top;
		void push(var v) {
			PRINT("valstack PUSH " << v);
			ASSERT(top + 1 < 1024);
			buff[++top] = v;
		}
		var pop() {
			PRINT("valstack POP");
			ASSERT(top > -1);
			return buff[top--];
		}
		var cur() {
			ASSERT(top != -1);
			return buff[top];
		}
		valstack_t() {
			top = -1;
		}
	};

	struct oprstack_t
	{
		opr buff[1024];
		int top;
		void push(opr c) {
			ASSERT(top + 1 < 1024);
			buff[++top] = c;
		}
		opr pop() {
			ASSERT(top > -1);
			return buff[top--];
		}
		opr cur() {
			ASSERT(top != -1);
			return buff[top];
		}
		void setcur(opr o) {
			ASSERT(top != -1);
			buff[top] = o;
		}
		bool empty() {
			return top == -1;
		}
		oprstack_t() {
			top = -1;
		}
	};
	struct varmapstack_t
	{
		using varmap_t = std::map<varname, var>;
		std::vector<varmap_t> stack;
		
		void push()
		{
			PRINT("varmapstack PUSH");
			stack.push_back(varmap_t());
		}
		void pop()
		{
			PRINT("varmapstack POP");
			stack.pop_back();
		}
		void addvar(const char* name, var v)
		{
			PRINT("addvar: " << name << " = " << v);
			if (stack.empty())
				push();
			stack.back()[name] = v;
		}
		var getvar(const char* name)
		{
			PRINT("getvar = " << name)
			if (stack.empty())
				return INVALIDVAR;

			for (int i = stack.size() - 1; i >= 0; i--)
			{
				varmap_t& varlist = stack[i];
				if (varlist.find(name) != varlist.end())
					return varlist[name];
			}

			return INVALIDVAR;
		}
		void clear()
		{
			stack.clear();
		}
	};

	// code
	struct code 
	{
		const char* ptr;
		std::map<funcname, function>	funcnamemap;		
		varmapstack_t	varmapstack;

		codestack_t		codestack;
		valstack_t		valstack;
		oprstack_t		oprstack;

		code(const char* buf) {
			ptr = buf;
			varmapstack.clear();
			funcnamemap.clear();
		}
		char next() {
			while (!eoc() && checkspace(*(++ptr)));
			return (*ptr);
		}
		char next2() {
			while (!eoc(++ptr)){
				if (!checkspace(*(ptr)) && !isname(*(ptr)))
					break;
			}
			return (*ptr);
		}
		char getnext() {
			const char* p = ptr;
			while (!eoc(p) && checkspace(*(++p)));
			return (*p);
		}
		char getnext2() {
			const char* p = ptr;
			while (!eoc(++p)){
				if(!checkspace(*(p)) && !isname(*(p)))
					break;
			}
			return (*p);
		}
		bool eoc(const char* p = 0) {
			p == 0 ? p = ptr : 0;
			return (*p) == '\0';
		}
		char cur() {
			return *ptr;
		}
		const char* getname() {
			static char buf[128];
			char* pbuf = buf;
			const char* p = ptr;
			while (!eoc(p) && (isname(*p) || isdigit(*p)))
				*(pbuf++) = *(p++);
			(*pbuf) = '\0';
			return buf;
		}
	};
	
	// get char
	short get(code& cd)
	{
		for (; !cd.eoc(); cd.next()) {
			char c = cd.cur();
			if (checkspace(c))
				continue;
			else if (isdigit(c)) {
				return NUMBER;
			}
			else if (iscalc(c)) {
				return OPR;
			}
			else if (islogic(c)) {
				return LGOPR;
			}
			else if (isname(c)) {
				return NAME;
			}
			else
				return c;
		}
		return 0;
	}

	// 运算
	var act(code& cd, int valcnt) 
	{
		opr o = cd.oprstack.pop();
		switch (o) {
		case '+': {
			if (valcnt == 2) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a + b;
			}
			else {
				return +cd.valstack.pop();
			}
		}
		case '-': {
			if (valcnt == 2) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a - b;
			}
			else {
				return -cd.valstack.pop();
			}
		}
		case '*': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a * b;
		}
		case '/': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a / b;
		}
		case '>': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a > b;
		}
		case '<': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a < b;
		}
		case '=': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a == b;
		}
		case '&': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a && b;
		}
		case '|': {
			var b = cd.valstack.pop();
			var a = cd.valstack.pop();
			return a || b;
		}
		case '!': {
			var a = cd.valstack.pop();
			return !a;
		}
		default: {return 0; }
		}
	}

	inline var chars2var(code& cd) {
		static char buff[128];
		int i = 0;
		for (; i < 128; i++) {
			char c = cd.cur();
			if (!isdigit(c) && c != '.')
				break;
			buff[i] = c;
			cd.next();
		}
		buff[i] = '\0';
		return atof(buff);
	}

	// get value
	void getval(code& cd, short type) {
		if (type == NUMBER) {
			cd.valstack.push(chars2var(cd));
			if (cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))) {
				cd.oprstack.push('.');
			}
		}
		else if (type == NAME) {
			if (cd.funcnamemap.find(cd.getname()) != INVALIDFUN) {
				cd.valstack.push(callfunc(cd));
			}
			else {
				cd.valstack.push(cd.varmapstack.getvar(cd.getname()));
				cd.next2();
			}
			if (cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))) {
				cd.oprstack.push('.');
			}
		}
	}
	// finished trunk
	void finishtrunk(code& cd, int trunkcnt = 0, char sk = '{', char ek = '}') 
	{
		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == sk) {
				cd.varmapstack.push();
				trunkcnt++;
			}
			else if (c == ek) {
				trunkcnt--;
				cd.varmapstack.pop();

				if (trunkcnt == 0) {
					cd.next();
					break;
				}
			}
			cd.next();
		}
	}

	// 表达式 for example: x=a+b, v = fun(x), x>2 ||x < 5
	var expr(code& cd)
	{
		int valcnt = 0;
		while (!cd.eoc()) {
			short type = get(cd);
			if (type == NAME || type == NUMBER) {
				getval(cd, type);
				valcnt++;
			}
			else if (type == OPR) {
				opr o = cd.cur();
				if (!cd.oprstack.empty() && cd.oprstack.cur() == '.')
					cd.oprstack.setcur(o);
				else
					cd.oprstack.push(o);
				cd.next();

				if (iscalc(cd.cur())) {
					getval(cd, type);
					cd.valstack.push(expr(cd));
					valcnt++;
				}
				else {
					char no = cd.getnext();
					if (cd.cur() != '(' && iscalc(no))
					{
						type = get(cd);
						if (rank[o] >= rank[no]) {
							getval(cd, type);
							cd.valstack.push(act(cd, valcnt));
							valcnt++;
						}
						else {
							getval(cd, type);
							cd.valstack.push(expr(cd));
							valcnt++;
						}
					}
				}
			}
			else if (type == LGOPR) {
				if (cd.oprstack.cur() == '.')
					cd.oprstack.setcur(cd.cur());
				else
					cd.oprstack.push(cd.cur());
				cd.next();
			}
			else {
				char c = cd.cur();
				if (c == '(') {
					cd.next();
					var v = expr(cd);
					cd.next();
					cd.valstack.push(v);
					valcnt++;
				}
				else if (c == ')' || c == ';' || c == ',' || c == '{') {

					if (!cd.oprstack.empty() &&
						(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur())))
						return act(cd, valcnt);
					else {
						return cd.valstack.pop();
					}
				}
			}
		}
		ERRORMSG("';' is missing?");
		return INVALIDVAR;
	}

	// single var
	void singvar(code& cd) {
		const char* name = cd.getname();
		PRINT("singvar: " << name);
		cd.next2();
		ASSERT(cd.cur() == '=');
		cd.next();

		var v = expr(cd);
		cd.next();
		cd.varmapstack.addvar(name, v);

		PRINT(name << " = " << v);
		//PRINTV(cd.cur());
	}

	// statement
	void statement(code& cd) {
		
		short type = get(cd);
		if (type == NAME) {
			char nc = cd.getnext2();
			
			if (nc == '=') {
				singvar(cd);
			}
			else if (nc == '(') {
				callfunc(cd);
				cd.next();
			}
		}
		else if (cd.cur() == '>') {
			cd.next();
			PRINT(expr(cd));
			cd.next();
		}
	}

	// subtrunk
	int subtrunk(code& cd, var& ret)
	{
		while (!cd.eoc()) {
			short type = get(cd);
			//PRINTV(type);
			if (cd.cur() == '{') 
			{
				cd.next();
				int rettype = subtrunk(cd, ret);
				if (rettype > 0)
					return rettype;
			}
			else if (cd.cur() == '}')
			{
				cd.next();
				break;
			}
			else if (type == '?') 
			{
				ASSERT(cd.next() == '(')
					cd.next();
				var e = expr(cd);
				cd.next();

				if (e == 0) {
					finishtrunk(cd, 0, '{', '}');
					if (cd.cur() == ':')
						cd.next();
					else
						continue;
				}
				cd.next();

				int rettype = subtrunk(cd, ret);
				if (rettype == 1)
				{
					finishtrunk(cd, 1);
					return rettype;
				}
				if (rettype == 2) {
					return rettype;
				}

			}
			else if (type == '@')
			{
				if (cd.next() == '(') {
					cd.next();

					const char* cp = cd.ptr;
				codepos1:
					var e = expr(cd);
					cd.next();
					if (e != 0) {
						cd.next();
						int rettype = subtrunk(cd, ret);
						if (rettype == 1)
						{
							finishtrunk(cd, 1);
							break;
						}
						if (rettype == 2) {
							return rettype;
						}

						cd.ptr = cp;
						PRINT("loop");
						goto codepos1;
					}
					else {
						finishtrunk(cd, 0);
					}
				}
				else 
				{
					int loopcnt = expr(cd);
					cd.next();
					const char* cp = cd.ptr;
					for (int i = 0; i < loopcnt; i++) {
						int rettype = subtrunk(cd, ret);
						if (rettype == 1)
						{
							finishtrunk(cd, 1);
							break;
						}
						if (rettype == 2) {
							return rettype;
						}

						cd.ptr = cp;
						PRINT("loop " << i);
					}
				}
			}
			else if (type == '$')
			{
				cd.next();
				if (cd.cur() == '@') {
					return 1;
				}
				else {
					ret = expr(cd);
					return 2;
				}
			}
			else
			{
				statement(cd);
			}
		}
		return 0;
	}

	// trunk
	void trunk(code& cd, var& ret) {
		subtrunk(cd, ret);
	}
	// ----------------------------------------------------------------------
	var callfunc(code& cd) {
		funcname fnm = cd.getname();
		PRINT("callfunc: " << fnm << "()");
		PRINT("{");
		ASSERT(cd.next2() == '(');

		cd.next();
		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == ')') {
				cd.next();
				break;
			}
			else if (c == ',') {
				cd.next();
				continue;
			}
			else {
				var e = expr(cd);
				//PRINTV(e);
				cd.valstack.push(e);
			}
		}

		cd.codestack.push(cd.ptr);
		if (cd.funcnamemap.find(fnm) == INVALIDFUN)
		{
			ERRORMSG("No function named: '" << fnm << "'")
			return INVALIDVAR;
		}
		cd.ptr = cd.funcnamemap[fnm];

		cd.next2();
		ASSERT(cd.cur() == '(');

		cd.varmapstack.push();
		cd.next();
		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == ')') {
				cd.next();
				break;
			}
			else if (c == ',') {
				cd.next();
			}
			else {
				cd.varmapstack.addvar(cd.getname(), cd.valstack.pop());
				cd.next2();
			}
		}

		var ret = INVALIDVAR;
		ASSERT(subtrunk(cd, ret) == 2);
		cd.varmapstack.pop();

		cd.ptr = cd.codestack.pop();
		PRINT("return " << ret);
		PRINT("}");
		return ret;
	}

	// func
	static void func(code& cd) {
		funcname fnm = cd.getname();
		PRINT("define func: " << fnm);
		ASSERT(cd.funcnamemap[fnm] == 0);
		cd.funcnamemap[fnm] = cd.ptr;
		cd.next();
		finishtrunk(cd, 0);
	}
	void parser(code& cd) {
		PRINT("-------script--------");
		PRINT(cd.ptr);
		PRINT("---------------------");
		
		rank['+'] = 1;
		rank['-'] = 1;
		rank['*'] = 2;
		rank['/'] = 2;
		rank['!'] = 3;
		getchar();

		cd.varmapstack.push();
		while (!cd.eoc()) {
			short type = get(cd);
			PRINTV(type);
			if (type == '#') {
				cd.next();
				func(cd);
			}
			else {
				var ret = INVALIDVAR;
				trunk(cd, ret);
			}
		}
	}

	void dofile(const char* filename) {
		FILE* f = fopen(filename, "rb");
		ASSERT(f != 0);
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

	void dostring(const char* str) {
		code cd(str);
		parser(cd);
		PRINT("\n")
	}
}

void test()
{
	//PHG::dostring("aa = 2;BBB = 3;");
	PHG::dofile("main.phg");
}