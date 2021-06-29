/****************************************************************************
							Phg2.0
							脚本是群论的扩展
语法示例:	

'function					
#blend(a, b, alpha)
{
	$ a*(1-alpha) + b*alpha;
}

'call function
ab = blend(2,8, 0.25)
>ab;

'if
?(i = 1){
t = t + 1;
}:{
t = t - 1;
}
>t;

'calc
yy = 1*2+ 4 * 8;
> yy;

'loop
@(yy < 100){
yy = yy + 1;
}
> yy;

****************************************************************************/
#define PHG_VAR(name, defaultval) (PHG::gcode.varmapstack.stack.empty() || PHG::gcode.varmapstack.stack.front().find(#name) == PHG::gcode.varmapstack.stack.front().end() ? defaultval : PHG::gcode.varmapstack.stack.front()[#name])
#define PHG_PARAM(index)	cd.valstack.get(args - index)


#define var			EDGE
#define INVALIDVAR	EDGE(0)
struct EDGE
{
	int val = 0;
	VECLIST vlist;
	EDGE() {}
	EDGE(int _val)
	{
		val = _val;
	}
	operator const int& ()
	{
		return val;
	}
	bool operator==(EDGE v) const
	{
		if (vlist.size() != v.vlist.size()) 
			return false;
		for (int i = 0; i < vlist.size(); i++)
		{
			if (vlist[i].p != v.vlist[i].p)
				return false;
		}
		return true;
	}
	bool operator==(int v) const
	{
		return val == v;
	}
	EDGE operator + (const EDGE& v) const
	{
		if (vlist.empty())
		{
			return v;
		}

		EDGE e;
		e.vlist = vlist;
		vec3 lastp = vec3::ZERO;
		for (auto it : v.vlist)
		{
			vec3 np = e.vlist.back().p + (it.p - lastp);
			lastp = it.p;

			bool bloop = false;
			for(int i = 0; i < e.vlist.size(); i ++)
			{
				if (np == e.vlist[i].p)
				{
					e.vlist.assign(e.vlist.begin(), e.vlist.begin() + i + 1);
					bloop = true;
					break;
				}
			}
			if(!bloop)
				e.vlist.push_back(np);
		}
		return e;
	}
	EDGE operator - (const EDGE& v) const
	{
		return (*this) + (-v);
	}
	EDGE operator - () const
	{
		EDGE e;
		for (int i = 0; i < vlist.size(); i++)
		{
			e.vlist.push_back(-vlist[i].p);
		}
		return e;
	}
	EDGE operator * (const EDGE& v) const
	{
		EDGE e;
		if (v.vlist.size() < 2)
			return e;

		crvec p1 = v.vlist[0].p.normcopy();
		crvec p2 = v.vlist[1].p.normcopy();
		vec3 n = p2.cross(p1); n.norm();
		real ang = acos(p1.dot(p2));

		e.vlist = vlist;
		for (auto& it : e.vlist)
		{
			it.p.rot(ang, n);
		}
		return e;
	}
};

namespace PHG
{
	#define INVALIDFUN	cd.funcnamemap.end()

	#define varname		string
	#define toint(v)	(int)(v)

	#define opr			char
	#define funcname	string
	#define functionptr	const char*

	#define NAME		0x01FF
	#define NUMBER		0x02FF
	#define OPR			0x03FF
	#define LGOPR		0x04FF

	// ----------------------------------------------------------------------
	extern struct code;
	extern void statement(code& cd);
	extern var callfunc(code& cd);

	char rank[256];

	std::vector<var> gtable;
	inline int add2table(const var& v)
	{
		/*for (int i = 0; i < gtable.size(); i++)
		{
			if (gtable[i] == v)
			{
				PRINT("add2table same! i=" << i);
				return i;
			}
		}*/

		gtable.push_back(v);
		return gtable.size() - 1;
	}

	// API
	typedef var (*fun_t)(code& cd, int stackpos);
	struct api_fun_t
	{
		int args = 0;
		fun_t fun;
	};
	
	std::map<string, api_fun_t> api_list;

	// 运算
	var(*act)(code& cd, int args);

	// -----------------------------------------------------------------------
	inline bool checkline(char c) {
		return (c == '\n');
	}
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
	static struct codestack_t
	{
		const char* buff[1024];
		int top;
		void push(const char* c) {
			ASSERT(top + 1 < 1024);
			buff[++top] = c;
		}
		const char* pop() {
			ASSERT(top > -1);
			return buff[top--];
		}
		const char* cur() {
			ASSERT(top != -1);
			return buff[top];
		}
		bool empty() {
			return top == -1;
		}
		codestack_t() {
			top = -1;
		}
	};

	static struct valstack_t
	{
		var buff[1024];
		int top;
		void push(const var& v) {
			//PRINT("valstack PUSH " << v.vlist.size());
			ASSERT(top + 1 < 1024);
			buff[++top] = v;
		}
		var pop() {
			//PRINT("valstack POP");
			ASSERT(top > -1);
			return buff[top--];
		}
		var cur() {
			ASSERT(top != -1);
			return buff[top];
		}
		var get(int pos) {
			ASSERT(top != -1);
			ASSERT(top - pos >= 0);
			return buff[top - pos];
		}
		void reset()
		{
			top = -1;
		}
		valstack_t() {
			top = -1;
		}
	};

	static struct oprstack_t
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

	static struct varmapstack_t
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
		void addvar(const char* name, const var& v)
		{
			PRINT("addvar: " << name);// << " = " << v);
			add2table(v);
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
	static struct code
	{
		const char* ptr;
		std::map<funcname, functionptr>	funcnamemap;		
		varmapstack_t	varmapstack;

		codestack_t		codestack;
		valstack_t		valstack;
		oprstack_t		oprstack;

		code() {}
		code(const char* buf) {
			ptr = buf;
			varmapstack.clear();
			funcnamemap.clear();
		}
		char next() {
			while (!eoc(++ptr) && checkspace(*(ptr)));
			return (*ptr);
		}
		char next2() {
			while (!eoc(++ptr)){
				if (!checkspace(*(ptr)) && !isname(*(ptr)))
					break;
			}
			return (*ptr);
		}
		char nextline() {
			while (!eoc(++ptr) && !checkline(*(ptr)));
			return (*++ptr);
		}
		char getnext() {
			const char* p = ptr;
			while (!eoc(++p) && checkspace(*(p)));
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
	static short get(code& cd)
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
	/*
	// 运算
	var act_default(code& cd, int args)
	{
		opr o = cd.oprstack.pop();

		PHGPRINT("act:" << o << " args = " << args)

		switch (o) {
		case '+': {
			if (args > 1) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a + b;
			}
			else {
				return cd.valstack.pop();
			}
		}
		case '-': {
			if (args > 1) {
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
	*/
	static inline var chars2var(code& cd) {
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

		int number = atoi(buff)-1;
		if (number < 0 || number >= gtable.size())
		{
			ERRORMSG("chars2var error! number=" << number);
			return INVALIDVAR;
		}
		//PRINTV(number);
		return gtable[number];
	}

	// get value
	static void getval(code& cd, short type) {
		
		if (type == NUMBER) {
			cd.valstack.push(chars2var(cd));
			if (cd.oprstack.empty() || !(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur()))) {
				cd.oprstack.push('.');
			}
		}
		else if (type == NAME) {
			if (api_list.find(cd.getname()) != api_list.end() ||
				cd.funcnamemap.find(cd.getname()) != INVALIDFUN) {
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
	static void finishtrunk(code& cd, int trunkcnt = 0, char sk = '{', char ek = '}')
	{
		while (!cd.eoc()) {
			char c = cd.cur();
			if (c == sk) {
				trunkcnt++;
			}
			else if (c == ek) {
				trunkcnt--;

				if (trunkcnt == 0) {
					cd.next();
					break;
				}
			}
			cd.next();
		}
	}

	// 表达式 for example: x=a+b, v = fun(x), x > 2 || x < 5
	static var expr(code& cd)
	{
		//PRINT("expr(");
		int args = 0;
		while (!cd.eoc()) {
			short type = get(cd);

			if (type == NAME || type == NUMBER) {
				getval(cd, type);
				args++;
			}
			else if (type == OPR) {
				opr o = cd.cur();
				if (!cd.oprstack.empty() && cd.oprstack.cur() == '.')
					cd.oprstack.setcur(o);
				else
					cd.oprstack.push(o);

				if (iscalc(cd.cur())) {
					args++;
				}

				cd.next();

				if (iscalc(cd.cur())) {
					cd.valstack.push(expr(cd));
					args++;
				}
				else {
					if (cd.cur() == '(')
					{
						cd.next();
						var v = expr(cd);
						cd.valstack.push(v);
						args++;
					}
					char no = cd.getnext();
					if (cd.cur() != '(' && 
						iscalc(no))
					{
						if (cd.cur() == ')')
							cd.next();

						type = get(cd);
						if (rank[o] >= rank[no]) {
							getval(cd, type);
							cd.valstack.push(act(cd, args));
							args++;
						}
						else {
							getval(cd, type);
							cd.valstack.push(expr(cd));
							args++;
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
					args++;
				}
				else if (c == ')' || c == ']' || c == ';' || c == ',' || c == '{' || c == '\n') {

					if (!cd.oprstack.empty() &&
						(iscalc(cd.oprstack.cur()) || islogic(cd.oprstack.cur())))
					{
						//const var& ret = act(cd, args);
						//PRINT(")");
						return act(cd, args);
					}
					else {
						//const var& ret = cd.valstack.pop();
						//PRINT(")");
						return cd.valstack.pop();
					}
				}
			}
		}
		ERRORMSG("';' is missing?");
		return INVALIDVAR;
	}
	// single var
	static void singvar(code& cd) {
		string name = cd.getname();
		//PHGPRINT("singvar: " << name);
		cd.next2();
		ASSERT(cd.cur() == '=');
		cd.next();

		var v = expr(cd);
		cd.next();
		cd.varmapstack.addvar(name.c_str(), v);
	}

	// statement
	static void statement(code& cd) {
		
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
			const var& v = expr(cd);
			//PHGPRINT(v);
			cd.next();
		}
	}

	// subtrunk
	static int subtrunk(code& cd, var& ret)
	{
		while (!cd.eoc()) {
			short type = get(cd);
			
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
			else if (type == '\'')
			{
				//PHGPRINT("注解");
				cd.nextline();
			}
			else if (type == '?') 
			{
				ASSERT(cd.next() == '(')
					cd.next();
				const var& e = expr(cd);
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
						PRINT("loop ");
						goto codepos1;
					}
					else {
						finishtrunk(cd, 0);
					}
				}
				else 
				{
					int loopcnt = toint(expr(cd));
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
	
	// 函数
	static var callfunc_phg(code& cd) {
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

		if (api_list.find(cd.getname()) != api_list.end() || 
			cd.funcnamemap.find(fnm) == INVALIDFUN)
		{
			ERRORMSG("No function named: '" << fnm << "'")
			return INVALIDVAR;
		}
		cd.ptr = cd.funcnamemap[fnm];

		cd.next2();
		ASSERT(cd.cur() == '(');

		cd.varmapstack.push();
		cd.next();
		std::vector<std::string> paramnamelist;
		std::vector<var> paramvallist;
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
				paramnamelist.push_back(cd.getname());
				paramvallist.push_back(cd.valstack.pop());
				cd.next2();
			}
		}

		for(int i = 0; i < paramnamelist.size(); i ++)
		{
			cd.varmapstack.addvar(paramnamelist[i].c_str(), paramvallist[paramvallist.size() - 1 - i]);
		}

		var ret = INVALIDVAR;
		ASSERT(subtrunk(cd, ret) == 2);
		cd.varmapstack.pop();

		cd.ptr = cd.codestack.pop();
		PRINT("return " << ret);
		PRINT("}");
		return ret;
	}

	static var callfunc(code& cd) {
		funcname fnm = cd.getname();
		if (api_list.find(fnm) != api_list.end())
		{
			PRINT("API:" << fnm);
			api_fun_t& apifun = api_list[fnm];
			apifun.args = 0;

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
					apifun.args++;
				}
			}
			
			var ret = apifun.fun(cd, apifun.args);
			return ret;
		}
		else
			return callfunc_phg(cd);
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

	// table
	static void table(code& cd)
	{
		PRINT("table: ");

		cd.next();
		while (!cd.eoc()) {
			char c = cd.cur();
			//PHGPRINT(c);
			if (c == ']') {
				cd.next();
				break;
			}
			else if (c == ',') {
				cd.next();
				continue;
			}
			else {
				/*var e = expr(cd);
				PRINTV(e);
				gtable.push_back(e);*/
			}
		}
	}

	// parser
	static void parser(code& cd) {
		PRINT("--------PHG---------");
		PRINT(cd.ptr);
		PRINT("--------------------");
		
		rank['+'] = 1;
		rank['-'] = 1;
		rank['*'] = 2;
		rank['/'] = 2;
		rank['!'] = 3;

		//getchar();

		cd.varmapstack.push();
		while (!cd.eoc()) {
			short type = get(cd);
			
			if (type == '#') {
				cd.next();
				func(cd);
			}
			else if (type == '[') {
				table(cd);
			}
			else {
				var ret = INVALIDVAR;
				subtrunk(cd, ret);
			}
		}
	}

	code gcode;

	// Init
	void init()
	{
		if (api_list.empty())
			initphg();
		//act = act_default;
	}

	// dofile
	void dofile(const char* filename)
	{
		PHG::init();

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
		parser(gcode = code(buf));
		PRINT("\n");
	}

	// dostring
	void dostring(const char* str) 
	{
		PHG::init();

		parser(gcode = code(str));
		PRINT("\n");
	}

	// API
	void register_api(crstr name, fun_t fun)
	{
		api_list[name].fun = fun;
	}
}

namespace PMHG
{
	inline void PHGPRINT(const EDGE& e)
	{
		for (auto& i : e.vlist)
			PRINTVEC3(i.p)
	}

	var act(PHG::code& cd, int args)
	{
		opr o = cd.oprstack.pop();

		PRINT("act: " << o << "(" << args << ")");

		switch (o) {
		case '+': {
			if (args > 1) {
				var b = cd.valstack.pop();
				var a = cd.valstack.pop();
				return a + b;
			}
			else {
				return cd.valstack.pop();
			}
		}
		case '-': {
			if (args > 1) {
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
		default: {return 0; }
		}
	}

	void setup()
	{
		PHG::act = act;
		{
			EDGE e;
			e.vlist.push_back(vec3(0, 0, 0));
			e.vlist.push_back(vec3(1, 1, 0));
			e.vlist.push_back(vec3(0, 2, 0));
			PHG::gtable.push_back(e);
		}
		{
			EDGE e;
			e.vlist.push_back(vec3(0, 1, 0));
			e.vlist.push_back(vec3(1, 1, 0));
			e.vlist.push_back(vec3(2, 1, 0));
			e.vlist.push_back(vec3(1, 4, 0));
			PHG::gtable.push_back(e);
		}
		PHG::register_api("render",
			[](PHG::code& cd, int args)->var {
				estack.clear();
				for (auto& it : PHG::gtable)
				{
					vec3 lastp;
					for (int i = 0; i < it.vlist.size(); i ++)
					{
						auto& vit = it.vlist[i];
						pointi(point_t(vit.p.x * 50, vit.p.y * 50), 8, 0xFF0000FF);
						if(i != 0)
							drawlinei(lastp.xy() * 50, vit.p.xy() * 50, 0xFF0000FF);
						lastp = vit.p;
					}
					estack.push_back(it.vlist);
				}
				return INVALIDVAR;
			});
	}
}

void test()
{
	PMHG::setup();

	PHG::dofile("main.phg");
}
