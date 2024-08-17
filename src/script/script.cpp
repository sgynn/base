#include <base/script.h>
#include <assert.h>
#include <cstring>
#include <cstdio>

#define NAME_LIMIT 64

using namespace script;

Context::Context(const Variable& top) : front(0), signal(NONE), writeFlags(0), writeMask(1) {
	scope = new Variable[16];
	scope[0] = top;
}
Context::Context() : front(0) {
	scope = new Variable[16];
}
Context::~Context() {
	delete [] scope;
}
Variable& Context::get(uint id) {
	for(int i=front; i>=0; --i) {
		Variable& v = scope[i].get_const(id);
		if(v.isValid()) return v;
	}
	return scope[0].get_const(-1); // undefined - this will always return the invalid nullVar
}
Variable& Context::get(uint* id) {
	Variable* v = &get(*id);
	for(++id; *id!=~0u && v->isValid(); ++id) v = &v->get_const(*id);
	return *v;
}
Variable& Context::get(const VariableName& name) {
	for(int i=front; i>=0; --i) {
		Variable& v = scope[i].get_const(name);
		if(v.isValid()) return v;
	}
	return scope[0].get_const(-1); // undefined
}

Variable& Context::getw(uint id) {
	uint mask = writeMask;
	for(int i=front; i>=0; --i, mask>>=1) {
		Variable& v = scope[i].get_const(id);
		if(v.isValid()) return v;
		if(writeFlags&mask) break;
	}
	return scope[0].get_const(-1); // invalid
}
Variable& Context::getw(uint* id) {
	Variable* v = &getw(*id);
	for(++id; *id!=~0u && v->isValid(); ++id) v = &v->get_const(*id);
	return *v;
}

Variable& Context::set(uint id, const Variable& value) {
	Variable* v = &getw(id);
	if(!v->isValid()) v = &scope[front].get(id); // Add to scope
	*v = value;
	return *v;
}
Variable& Context::set(uint* id, const Variable& value) {
	Variable* v = &getw(*id);
	if(!v->isValid()) v = &scope[front].get(*id); // Add to scope
	for(++id; *id!=~0u; ++id) v = &v->get(*id);
	*v = value;
	return *v;
}

Variable& Context::get(const char* name) {
	String tmp;
	const char* dot = strchr(name, '.');
	if(dot) name = tmp = String(name, dot-name);	// name becomes first part
	Variable& r = get( Variable::lookupName(name) );
	if(r.isValid() && dot) return r.find_const(dot+1);
	return r;
}
Variable& Context::set(const char* name, const Variable& value) {
	String tmp;
	const char* first = name;
	const char* dot = strchr(name, '.');
	if(dot) first = tmp = String(name, dot-name);
	Variable* v = &getw( Variable::lookupName(first) );
	if(!v->isValid()) v = &scope[front].find(name); // Add to scope
	else if(dot) v = &v->find(dot+1);
	*v = value;
	return *v;
}

void Context::push(const Variable& v, bool w) {
	assert(front<15); // Throw exception, or resize list ??
	scope[++front] = v;
	writeMask = 1<<front;
	if(w) writeFlags |= writeMask;
}
void Context::pop() {
	if(front) --front;
	writeFlags &= ~writeMask;
	writeMask = 1<<front;
}

const Variable& Context::getReturnValue() const { return scope[15]; }
Context::Signal Context::getSignal() const { return signal; }
void Context::setSignal(Signal s, const Variable& r) {
	if(signal == HALT) return;
	if(s == RETURN) scope[15] = r;
	signal = s;
}

// ================================================================================================== //
/// Function call - private function used only by script
namespace script {
class FunctionCall {
	public:
	FunctionCall(int count, Expression** argv);
	~FunctionCall();
	String toString() const;
	Variable call(Function* func, Context&& context) const;
	private:
	int          argc;
	Expression** argv;
};
}

// ----------------------------------------- //

#define fwd(x) std::forward<Context>(x)

Expression::Expression() : op(NIL) { lhs.type = rhs.type = NOTHING; }
Expression::~Expression() {
	opDelete(lhs);
	opDelete(rhs);
}

void Expression::opDelete(Operand& op) {
	switch(op.type) {
	case NOTHING: break;
	case NAME: break;
	case COMPOUNDNAME: delete [] op.cvar; break;
	case CONSTANT:     delete op.con; break;
	case EXPRESSION:   delete op.expr; break;
	case FUNCTIONCALL: delete op.call; break;
	}
	op.type = NOTHING;
}

Variable Expression::opValue(const Operand& v, Context&& context) {
	switch(v.type) {
	case NAME:         return context.get(v.var);
	case COMPOUNDNAME: return context.get(v.cvar);
	case CONSTANT:     return *v.con;
	case EXPRESSION:   return v.expr->evaluate(fwd(context));
	default:           return Variable();
	}
}
Variable& Expression::opVariableRef(const Operand& o, Context&& context) {
	Variable* v;
	switch(o.type) {
	case NAME:  // name = ?
		v = &context.getw(o.var);
		if(!v->isValid()) v = &context.set(o.var, Variable());
		return *v;
	case COMPOUNDNAME: // name.name = ?
		v = &context.getw(o.cvar[0]);
		if(!v->isValid()) return context.set(o.cvar, Variable()); // New variable
		for(uint* n=o.cvar+1; *n!=~0u; ++n) {
			if(v->isExplicit() && n[1]!=~0u) v = &v->get_const(*n);
			else v = &v->get(*n);
		}
		if(!v->isValid()) {
			char nameBuffer[1024];
			char* cp = nameBuffer;
			for(uint* n=o.cvar; *n!=~0u; ++n) cp += sprintf(cp, "%s.", Variable::lookupName(*n));
			cp[-1] = 0;
			printf("Error: Cannot implicitly create variable: %s\n", nameBuffer);
		}
		return *v;
	case EXPRESSION:
		if(o.expr->op == GET) {	// GET evaluates rhs for a variable name
			v = &opVariableRef(o.expr->lhs, fwd(context));
			Variable rhs = opValue(o.expr->rhs, fwd(context));
			if(v->isNull() && rhs.isNumber()) v->makeArray(); // make array if created with number subscript
			if(v->isArray() && rhs.isNumber()) return v->get( (int)rhs );
			return v->get( rhs.toString() ); // Does this need to be find ?
		}
		else if(o.expr->op == GET2) { // GET2 does a cvar find on the current object
			v = &opVariableRef(o.expr->lhs, fwd(context));
			for(uint* id=o.expr->rhs.cvar; *id!=~0u; ++id) {
				if(v->isExplicit() && id[1]!=~0u) v = &v->get_const(*id);
				v = &v->get(*id);
			}
			return *v;
		}
	default: break;
	}
	assert(false); // Error: should be valid
	return Variable().get_const(-1); // nullVar
}

template<typename T> inline int compareT(T x, T y) { return x<y? -1: x>y? 1: 0; }
int Expression::compare(const Variable& a, const Variable& b) {
	int t = (a.type&0xf) > (b.type&0xf)? (a.type&0xf): (b.type&0xf);
	switch(t) {
	case 0: return 0; // Both null
	case Variable::BOOL:    return compareT<bool>(a,b);
	case Variable::INT:     return compareT<int>(a,b);
	case Variable::UINT:    return compareT<uint>(a,b);
	case Variable::FLOAT:   return compareT<float>(a,b);
	case Variable::DOUBLE:  return compareT<double>(a,b);
	case Variable::VEC2: { vec2 va=a, vb=b; return va==vb? 0: 2; }
	case Variable::VEC3: { vec3 va=a, vb=b; return va==vb? 0: 2; }
	case Variable::VEC4: { vec4 va=a, vb=b; return va==vb? 0: 2; }
	case Variable::STRING: { String sa=a, sb=b; return strcmp(sa, sb); }
	case Variable::OBJECT: return (a.type&0xf)==(b.type&0xf) && a.obj == b.obj? 0: 2;
	default: return 2;
	}
}

Variable Expression::evaluate(Context&& context) const {
	if(op==NIL) return opValue(lhs, fwd(context)); // Trivial case - single value
	if(op==NOT) { Variable r; r=!(bool)opValue(rhs, fwd(context)); return r; } // simple invert
	if(op==NEG) {
		Variable r = opValue(rhs, fwd(context));
		switch(r.type) {
		case Variable::BOOL:
		case Variable::INT:
		case Variable::UINT: r = -(int)r; break;
		case Variable::FLOAT: r = -(float)r; break;
		case Variable::DOUBLE: r = -(double)r; break;
		default: return Variable(); // Error
		}
		return r;
	}

	// The rest require both operands
	Variable a = opValue(lhs, fwd(context));
	Variable b = opValue(rhs, fwd(context));
	Variable r;
	int rt = (a.type&0xf) > (b.type&0xf)? (a.type&0xf): (b.type&0xf);
	switch(op) {
	case NIL: case NOT: case NEG: break;
	case EQ: r = a.isNull()==b.isNull() && compare(a, b)==0; break;
	case NE: r = !(a.isNull()==b.isNull() && compare(a, b)==0); break;
	case GT: r = compare(a, b)==1; break;
	case GE: r = compare(a, b)>=0; break;
	case LT: r = compare(a, b)< 0; break;
	case LE: r = compare(a, b)<=0; break;

	case AND: r = (bool)a && (bool)b; break;
	case OR:  r = (bool)a || (bool)b; break;

	case ADD:
		switch(rt) {
		case Variable::INT:    r = (int)a    + (int)b; break;
		case Variable::UINT:   r = (uint)a   + (uint)b; break;
		case Variable::FLOAT:  r = (float)a  + (float)b; break;
		case Variable::DOUBLE: r = (double)a + (double)b; break;
		case Variable::VEC2:   r = a.operator vec2() + b.operator vec2(); break;
		case Variable::VEC3:   r = a.operator vec3() + b.operator vec3(); break;
		case Variable::VEC4:   r = a.operator vec4() + b.operator vec4(); break;
		case Variable::STRING: r = a.toString() + b.toString(); break;
		case Variable::ARRAY:
			if(a.isArray() && b.isArray()) {
				r.makeArray();
				for(Variable& v: a) r.set(r.size(), v);
				for(Variable& v: b) r.set(r.size(), v);
			}
			break;
		default: break;
		}
		break;
	case SUB:
		switch(rt) {
		case Variable::INT:    r = (int)a    - (int)b; break;
		case Variable::UINT:   r = (uint)a   - (uint)b; break;
		case Variable::FLOAT:  r = (float)a  - (float)b; break;
		case Variable::DOUBLE: r = (double)a - (double)b; break;
		case Variable::VEC2:   r = a.operator vec2() - b.operator vec2(); break;
		case Variable::VEC3:   r = a.operator vec3() - b.operator vec3(); break;
		case Variable::VEC4:   r = a.operator vec4() - b.operator vec4(); break;
		default: break;
		}
		break;
	case MUL:
		switch(rt) {
		case Variable::INT:    r = (int)a    * (int)b; break;
		case Variable::UINT:   r = (uint)a   * (uint)b; break;
		case Variable::FLOAT:  r = (float)a  * (float)b; break;
		case Variable::DOUBLE: r = (double)a * (double)b; break;
		case Variable::VEC2:   r = a.operator vec2() * b.operator vec2(); break;
		case Variable::VEC3:   r = a.operator vec3() * b.operator vec3(); break;
		case Variable::VEC4:   r = a.operator vec4() * b.operator vec4(); break;
		case Variable::ARRAY:
			if(a.isArray() && b.isNumber()) {
				r.makeArray();
				for(int n=b; n>0; --n) for(Variable& v: a) r.set(r.size(), v);
			}
			else if(a.isNumber() && b.isArray()) {
				r.makeArray();
				for(int n=a; n>0; --n) for(Variable& v: b) r.set(r.size(), v);
			}
			break;
		default: break;
		}
		break;
	case DIV:
		switch(rt) {
		case Variable::INT:    r = (int)a    / (int)b; break;
		case Variable::UINT:   r = (uint)a   / (uint)b; break;
		case Variable::FLOAT:  r = (float)a  / (float)b; break;
		case Variable::DOUBLE: r = (double)a / (double)b; break;
		case Variable::VEC2:   r = a.operator vec2() / b.operator vec2(); break;
		case Variable::VEC3:   r = a.operator vec3() / b.operator vec3(); break;
		case Variable::VEC4:   r = a.operator vec4() / b.operator vec4(); break;
		default: break;
		}
		break;
	case SET:
		r = opVariableRef(lhs, fwd(context)) = std::move(b);
		break;
	case APPEND:
		{
		Variable& ref = opVariableRef(lhs, fwd(context));
		if(ref.isArray()) {
			ref.set(ref.size(), b);
		}
		else if(ref.isNull()) {
			r.makeArray();
			r.set(0u, b);
			ref = r;
		}
		else if(!ref.isObject()) {
			r.makeArray();
			r.set(0u, ref);
			r.set(1u, b);
			ref = r;
		}
		else {
			assert(false); // Can't append to objects
		}
		r = std::move(b);
		}
		break;

	case GET: // rhs is subtext of lhs - string, or integer if array : lhs[rhs]
		if(a.isArray() && b.isNumber()) r = a.get_const((int)b);
		else r = a.find_const( b.toString() );
		break;
	case GET2: // rhs must be var or cvar: lhs.rhs
		assert(!a.isArray());
		if(rhs.type==NAME) a = a.get_const(rhs.var);
		else if(rhs.type==COMPOUNDNAME) {
			for(uint* id=rhs.cvar; *id!=~0u; ++id) a = a.get_const(*id);
		}
		else {
			assert(false); //?
		}
		r = a;
		break;
	case CALL:
		if(!a.isFunction()) printf("Error: %s is not a function\n", opString(lhs).str());
		if(rhs.type == FUNCTIONCALL) r = rhs.call->call(a, fwd(context));
		else { assert(false); }
		break;
	}
	return r;
}

VariableName Expression::extractVariableName(Context&& context) const {
	VariableName name;
	if(op == NIL || op == SET || op == APPEND || op == GET || op == GET2) {
		switch(lhs.type) {
		case NAME: name += lhs.var; break;
		case COMPOUNDNAME:
			for(const uint* n = lhs.cvar; *n!=~0u; ++n) name += *n;
			break;
		case EXPRESSION:
			name = lhs.expr->extractVariableName(fwd(context));
		default: break;
		}

		if(op == GET) { // lhs[rhs]
			Variable e = opValue(rhs, fwd(context));
			if(e.isNumber()) name += (uint)e;
			else name += e.toString();
		}
		else if(op == GET2) {  // lhs.rhs
			switch(rhs.type) {
			case NAME: name += rhs.var; break;
			case COMPOUNDNAME:
				for(const uint* n = rhs.cvar; *n!=~0u; ++n) name += *n;
				break;
			case EXPRESSION:
				name += rhs.expr->extractVariableName(fwd(context));
			default: break;
			}
		}
	}
	return name;
}



Conditional::Conditional() { rhd.type = NOTHING; }
Conditional::~Conditional() { opDelete(rhd); }
Variable Conditional::evaluate(Context&& context) const {
	bool c = opValue(lhs, fwd(context));
	if(c) return opValue(rhs, fwd(context));
	else return opValue(rhd, fwd(context));
}


// ================================================================================================== //

Block::Block() : lines(0), length(0), scoped(true), localSet(false), signal(Context::NONE) {}
Block::~Block() {
	for(int i=0; i<length; ++i) delete lines[i];
	delete [] lines;
}
Variable Block::evaluate(Context&& context) const {
	Variable local, result;
	if(scoped) local.makeObject(), context.push(local, localSet);
	for(int i=0; i<length; ++i) {
		result = lines[i]->evaluate(fwd(context));
		if(context.getSignal()) break;
	}
	if(scoped) context.pop();
	if(signal && !context.getSignal()) context.setSignal(signal, result);
	return local;
}

// ----------------------------------------- //

Variable ArrayBlock::evaluate(Context&& context) const {
	Variable array;
	array.makeArray();
	for(int i=0; i<length; ++i) {
		array.set(i, lines[i]->evaluate(fwd(context)));
	}
	return array;
}

// ----------------------------------------- //

Function::Function() : argc(0), argn(0), ref(0) { scoped=false; }
Function::~Function() { delete [] argn; }
String Function::toString() const { return toString(true); }
String Function::toString(bool body) const {
	String s("function(");
	for(int i=0; i<argc; ++i) { 
		if(i) s = s + ", ";
		s = s + Variable::lookupName(argn[i]);
	}
	s = s + ")";
	if(!body) return s;
	return s + Block::toString();
}

// To make a function from an expression
Function* Script::createFunction(Expression* expr, int argc, const char* const argn[]) {
	Function* f = new Function();
	f->argc = argc;
	if(argc>0) {
		f->argn = new uint[argc];
		for(int i=0; i<argc; ++i) f->argn[i] = Variable::lookupName(argn[i]);
	}
	f->length = 1;
	f->lines = new Expression*[1];
	f->lines[0] = expr;
	f->signal = Context::RETURN;
	return f;
}
Variable Function::evaluate(Context&& c) const {
	Block::evaluate(fwd(c));
	Variable ret;
	if(c.getSignal() == Context::RETURN) ret = c.getReturnValue();
	c.setSignal(Context::NONE, ret);
	return ret;
}

// ----------------------------------------- //

FunctionCall::FunctionCall(int count, Expression** params) : argc(count), argv(0) {
	if(argc) argv = new Expression*[argc];
	for(int i=0; i<argc; ++i) argv[i] = params[i];
}
FunctionCall::~FunctionCall() {
	for(int i=0;i<argc;++i) delete argv[i];
	delete [] argv;
}
String FunctionCall::toString() const {
	String s="(";
	for(int i=0; i<argc; ++i) {
		if(i) s = s + ",";
		s = s + argv[i]->toString();
	}
	return s + ")";
}
Variable FunctionCall::call(Function* func, Context&& context) const {
	if(!func) return Variable().get_const(-1); // return invalid if not a function
	Variable params;
	for(int i=0; i<argc && i<func->argc; ++i) params.set( func->argn[i], argv[i]->evaluate(fwd(context)) );
	context.push(params);
	Variable r = func->evaluate(fwd(context));
	context.pop();
	return r;
}

// ================================================================================================== //

inline String addQuotes(const Variable* v) {
	if(v->isString()) return String("\"") + v->toString() + "\"";
	else return v->toString();
}

String Expression::opString(const Operand& op) {
	switch(op.type) {
	case NAME: return Variable::lookupName(op.var);
	case CONSTANT: return addQuotes( op.con );
	case EXPRESSION: return op.expr->toString();
	case COMPOUNDNAME: 
	{
		String s = Variable::lookupName(*op.cvar);
		for(uint* n=op.cvar+1; *n!=~0u; ++n) s = s + "." + Variable::lookupName(*n);
		return s;
	}
	default: return String();
	}
}

String Expression::toString() const {
	// ToDo : Operator precidence parentheses
	static const char* sop[] = { "", "=", "[] =",  "||", "&&", "==", "!=", ">", ">=", "<", "<=", "!", "+", "-", "-", "*", "/", "[", "." };
	static int paren[] = { 0,1,1, 2,3,4,5,6,7,8,9,10, 11,12,13,14,15,16,16 };
	String s = opString(lhs);
	if(op==NOT) s = String(sop[NOT]) + s;
	else if(op==GET) s = s + "[" + opString(rhs) + "]";
	else if(op==CALL) s = s + rhs.call->toString();
	else if(op) {
		String r = opString(rhs);
		if(lhs.type==EXPRESSION && paren[lhs.expr->op]<paren[op]) s = String("(") + s + ")";
		if(rhs.type==EXPRESSION && paren[rhs.expr->op]<paren[op]) r = String("(") + r + ")";
		s = s + sop[op] + r;
	}
	return s;
}
String Conditional::toString() const {
	return String("(") + opString(lhs) + " ? " + opString(rhs) + " : " + opString(rhd) + ")";
}
String Block::toString() const {
	String s = "{\n";
	for(int i=0; i<length; ++i) {
		s = s + lines[i]->toString() + "\n";
	}
	s = s + "}";
	return s;
}
String ArrayBlock::toString() const {
	String s = "[";
	for(int i=0; i<length; ++i) {
		if(i) s = s + ", ";
		s = s + lines[i]->toString();
	}
	s = s + "]";
	return s;
}


// ================================================================================================== //

// Some basic text parsing functions
namespace script {
	static int whitespace(const char* s) {
		int t = 0;
		while(s[t]==' ' || s[t]=='\t' || s[t]=='\n' || s[t]=='\r') ++t;
		return t;
	}
	static int comment(const char* s) {
		int t=whitespace(s);
		while(s[t] == '/' && (s[t+1] == '/' || s[t+1] == '*')) {
			t+=2;
			if(s[t-1]=='*') {
				while(s[t] && (s[t]!='*' || s[t+1]!='/')) ++t;
				if(s[t]) t+=2;
			}
			else {
				while(s[t] && s[t]!='\n') ++t;
			}
			t += whitespace(s+t);
		}
		return t;
	}
	//static int number(const char* s, double& v) {
	//	char* e; v = strtod(s, &e); return e-s;
	//}
	static int name(const char* s, char* v=0) {
		if((*s>='a' && *s<='z') || (*s>='A' && *s<='Z') || *s=='_') {
			int t = 0;
			while((*s>='a' && *s<='z') || (*s>='A' && *s<='Z') || (*s>='0' && *s<='9') || *s=='_') {
				if(v) v[t] = *s;
				++s; ++t;
			}
			return t;

		} else return 0;
	}
	static bool keyword(const char*& s, const char* key) {
		const char* start = s;
		while(*s && *s==*key) ++s, ++key;
		if(*key) return s=start,false;
		if(*s>='a' && *s<='z') return s=start,false;
		if(*s>='A' && *s<='Z') return s=start,false;
		if(*s>='0' && *s<='9') return s=start,false;
		if(*s=='_') return s=start,false;
		return true;
	}
	// Get line number for error
	static int getLineNumber(const char* src, const char* s) {
		int line = 0;
		while(--s > src) {
			if(*s=='\n') ++line;
		}
		return line;
	}
}

// --------------------------------------------------------- //

template<typename T> inline Variable* createConstant(T value) {
	Variable* v = new Variable();
	*v = value;
	return v;
}

int Script::parseOperator(const char*& s) {
	switch(*s) {
	case '!': if(s[1]=='=') return s+=2,Expression::NE; else return 0;
	case '=': if(s[1]=='=') return s+=2,Expression::EQ; else return ++s,Expression::SET;
	case '<': if(s[1]=='=') return s+=2,Expression::LE; else return ++s,Expression::LT;
	case '>': if(s[1]=='=') return s+=2,Expression::GE; else return ++s,Expression::GT;

	case '&': if(s[1]=='&') return s+=2,Expression::AND; else return 0;
	case '|': if(s[1]=='|') return s+=2,Expression::OR;  else return 0;

	case '+': return ++s,Expression::ADD;
	case '-': return ++s,Expression::SUB;
	case '*': return ++s,Expression::MUL;
	case '/': return ++s,Expression::DIV;
	default:  return 0;
	}
}
int Script::parseInvertor(const char*& s) {
	if(*s=='-') return ++s, Expression::NEG;
	if(*s=='!' && s[1]!='=') return ++s, Expression::NOT;
	return 0;
}
Variable* parseConstant(const char*& s) {
	// String constant
	if(*s == '"' || *s == '\'') {
		const char* e = strchr(s+1, *s); // TODO: Escaped characters and escaped quotes
		if(e==s+1) {
			s = e + 1;
			return createConstant( String() ); // Empty string
		}
		else if(e>s+1) {
			const char* str = s + 1;
			s = e + 1;
			String value(str, e-str);
			//escape characters
			char* scan = &value[0];
			char* end = scan + (e-str);
			while(char* c=strchr(scan, '\\')) {
				if(c[1] == 'n') { c[0] = '\n'; memmove(c+1, c+2, end-c-1); }
				else if(c[1] == 't') { c[0] = '\t'; memmove(c+1, c+2, end-c-1); }
				else if(c[1] == '\\') { memmove(c+1, c+2, end-c-1); }
				scan = c + 1;
			}
			return createConstant(value);
		}
	}
	// Vector constant
	else if(*s == '(') {
		const char* start = s;
		char* e = 0;
		++s;
		float val[4] = {0,0,0,0};
		int count = 0;
		while(*s && count<4) {
			s += whitespace(s);
			val[count] = strtod(s, &e);
			if(s>=e) break;
			++count;
			s = e;
			s += whitespace(s);
			if(*s==',') ++s;
			else break;
		}
		if(*s==')') {
			++s;
			if(count==2) return createConstant( vec2(val) );
			if(count==3) return createConstant( vec3(val) );
			if(count==4) return createConstant( vec4(val) );
		}
		s = start;
		return 0;
	}
	// Hex value
	else if(*s=='#' || (*s=='0' && s[1]=='x')) {
		char* e;
		const char* start = *s=='#'? s+1: s+2;
		uint value = strtoll(start, &e, 16);
		if(e>start) {
			s = e;
			return createConstant(value);
		}
	}
	// Numeric constant
	else if(*s=='-' || (*s>='0' && *s<='9')) {
		char* e;
		double f = strtod(s, &e);
		if(e>s) {
			s = e;
			return createConstant( (float)f );
		}
	}
	// Boolean constants
	else if(keyword(s, "true") || keyword(s, "True")) {
		return createConstant(true);
	}
	else if(keyword(s, "false") || keyword(s, "False")) {
		return createConstant(false);
	}
	return 0;
}
uint* parseName(const char*& s) {
	int l = name(s);
	if(!l) return 0; // not a name
	int parts = 1;
	while(s[l]=='.') ++parts, l += 1 + name(s+l+1); // Get full variable length

	// Convert to tokens
	uint* out = new uint[parts+1];
	char* tmp = new char[l+1];
	memcpy(tmp, s, l);
	tmp[l]=0;
	out[parts] = ~0u;

	if(parts==1) {
		out[0] = Variable::lookupName(tmp);
	}
	else {
		char* t = strtok(tmp, ".");
		for(int i=0; t; ++i) {
			out[i] = Variable::lookupName(t);
			t = strtok(NULL, ".");
		}
	}
	s += l;
	delete [] tmp;
	return out;
}

bool parseAppend(const char*& s) {
	if(*s != '[') return false;
	const char* start = s;
	++s;
	while(*s==' '||*s=='\t') ++s;
	if(*s != ']') return s=start, false;
	++s;
	s += whitespace(s);
	if(*s != '=') return s=start, false;
	++s;
	return true;
}

void Script::compoundExpression(Expression** stack, int& front, Expression* expr, int precidence) {
	if(stack[front]->op == Expression::NIL) {
		expr->lhs = stack[front]->lhs;
		stack[front]->lhs.type = Expression::NOTHING;
		delete stack[front];
		stack[front] = expr;
	}
	else {
		// Precidence rules for 'a = b = c' need to be inverted, as we need a=(b=c)
		while(front>=0 && stack[front]->op >= precidence && !(stack[front]->op==Expression::SET && precidence==Expression::SET)) --front;
		if(front<0) {
			expr->lhs.expr = stack[0];
			expr->lhs.type = Expression::EXPRESSION;
			stack[++front] = expr;
		}
		else {
			expr->lhs = stack[front]->rhs;
			stack[front]->rhs.expr = expr;
			stack[front]->rhs.type = Expression::EXPRESSION;
		}
	}
	stack[++front] = expr;
}

Expression* Script::parseExpression(const char* src, const char*& s, bool allowSet, bool allowArray) {
	typedef Expression::Operand Operand;
	const char* start = s;
	Expression* stack[32];
	Operand out; out.type = Expression::NOTHING;
	int front = -1;
	int oper = 0;
	Variable* var = 0;
	Block* blk = 0;
	Function* func = 0;
	uint* name = 0;
	#define Error(desc) printf("Error: %s on line %d\n", desc, getLineNumber(src,s)),s=start,(Expression*)0
	#define Valid(on, off) valid = (valid & ~(off)) | (on)
	enum { OP=0x1, INV=0x2, VAL=0x4, BLK=0x8, SET=0x10, FUNC=0x20, SUBTEXT=0x40, ARRAY=0x80, ALL=0xff };
	char valid = INV | VAL | BLK | (allowSet? SET: 0);
	if(allowArray) valid |= ARRAY;

	while(*s) {
		s += whitespace(s);
		// End of statement
		if(*s == ';' || *s == ',' || *s == ')' || *s=='}' || *s==':' || *s==']') break;
		if(*s == '/' && (s[1] == '/' || s[1]=='*')) break; // start of a comment
		// Constant
		if((valid&VAL) && (var = parseConstant(s))) {
			out.type = Expression::CONSTANT;
			out.con = var;
			Valid(OP,ALL);
		}
		// Function
		else if((valid&FUNC) && (func = parseFunction(src, s))) {
			var = new Variable;
			*var = func;
			out.type = Expression::CONSTANT;
			out.con = var;
			Valid(0,ALL);
		}
		// Variable name
		else if((valid&VAL) && (name = parseName(s))) {
			if(name[1]==~0u) {
				out.type = Expression::NAME;
				out.var = name[0];
				Valid(OP|SUBTEXT, INV|VAL|BLK|FUNC);
				delete [] name;
			} else {
				out.type = Expression::COMPOUNDNAME;
				out.cvar = name;
				Valid(OP|SUBTEXT, INV|VAL|BLK|FUNC);
			}
		}
		else if((valid&SET) && parseAppend(s)) {
			Expression* e = new Expression;
			e->op = Expression::APPEND;
			compoundExpression(stack, front, e, e->op);
			Valid(BLK|VAL|INV|FUNC|ARRAY, OP|SUBTEXT);
		}
		else if((valid&SUBTEXT) && *s=='.') {
			name = parseName(++s);
			if(!name) return Error("Expected variable name");
			Expression* e = new Expression;
			e->op = Expression::GET2;
			e->rhs.cvar = name;
			e->rhs.type = Expression::COMPOUNDNAME;
			compoundExpression(stack, front, e, Expression::GET);
		}
		else if((valid&SUBTEXT) && *s=='[') {
			Expression* subtext = parseExpression(src, ++s, false, false);
			if(!subtext) return Error("Invalid subtext");
			if(*s!=']') return Error("Expected ']'");
			++s;

			Expression* e = new Expression;
			e->op = Expression::GET;
			e->rhs.expr = subtext;
			e->rhs.type = Expression::EXPRESSION;
			compoundExpression(stack, front, e, e->op);
		}
		// Function call
		else if((valid&SUBTEXT) && *s=='(') {
			// rhs is the function call
			++s; s+=whitespace(s);
			Expression* params[8];
			int count = 0;
			while(count<8 && *s!=')') {
				params[count] = parseExpression(src, s, false, true);
				if(!params[count]) return Error("-");
				if(*s==',') ++s;
				++count;
			}
			if(*s==')') ++s;
			Expression* e = new Expression;
			e->op = Expression::CALL;
			e->rhs.type = Expression::FUNCTIONCALL;
			e->rhs.call = new FunctionCall(count, params);
			compoundExpression(stack, front, e, e->op);
			Valid(OP|SUBTEXT, INV|VAL|BLK|FUNC);
		}
		// Block
		else if((valid&BLK) && (blk = parseBlock(src, s, true, false, true))) {
			out.type = Expression::EXPRESSION;
			out.expr = blk;
			if(front>=0 && stack[front]->op==Expression::SET) blk->localSet = true; //Fix for: v=0;a={v=1}
			Valid(0,ALL); // Nothing
		}
		// Array - FIXME: syntax conflicts with subtext  ???
		else if((valid&ARRAY) && *s=='[') {
			++s;
			std::vector<Expression*> lines;
			while(*s && *s!=']') {
				Expression* e = parseExpression(src, s, false, true);
				if(!e) return Error("Expected expression");
				lines.push_back(e);
				if(*s==',') ++s;
				else if(*s!=']') return Error("Expected ]");
			}
			++s;

			// build array block
			Block* array = new ArrayBlock();
			array->length = lines.size();
			if(!lines.empty()) array->lines = new Expression*[lines.size()];
			for(size_t i=0; i<lines.size(); ++i) array->lines[i] = lines[i];
			out.type = Expression::EXPRESSION;
			out.expr = array;
			Valid(OP,ALL); // FIXME only + and * operators are valid
		}
		// Parentheses
		else if(*s=='(') {
			++s;
			out.type = Expression::EXPRESSION;
			out.expr = parseExpression(src, s, allowSet, false);
			if(!out.expr) return Error("-"); // Error - already reported
			if(*s != ')') return Error("Missing ')'");
			Valid(OP,ALL);
			++s;
		}
		// Conditional
		else if((valid&OP) && *s=='?') {
			++s;
			Expression* trueValue = parseExpression(src, s, false, true);
			if(!trueValue) return Error("Unexpected ?");
			if(*s!=':') return Error("Expected a ':'");
			++s;
			Expression* falseValue = parseExpression(src, s, false, true);
			if(!falseValue) return Error("Expected expression");
			Conditional* cnd = new Conditional();
			cnd->rhs.type = cnd->rhd.type = Expression::EXPRESSION;
			cnd->rhs.expr = trueValue;
			cnd->rhd.expr = falseValue;
			compoundExpression(stack, front, cnd, Expression::SET);
			Valid(0,ALL);
		}
		// Negative and not operators are a little different
		else if((valid&INV) && (oper = parseInvertor(s))) {
			Expression* e = new Expression();
			e->op = (Expression::Operator)oper;
			if(front>=0) {
				stack[front]->rhs.expr = e;
				stack[front]->rhs.type = Expression::EXPRESSION;
			}
			stack[++front] = e;
			Valid(VAL, ALL);
		}
		// Operators - the complicated one
		else if((oper = parseOperator(s))) {
			if(oper == Expression::SET && !(valid&SET)) return Error("Unexpected '='");
			else if(~valid&OP) return Error("Unexpected operator");
			
			Expression* e = new Expression();
			e->op = (Expression::Operator) oper;
			compoundExpression(stack, front, e, oper);

			// Valid next token
			if(oper==Expression::SET) Valid(BLK|VAL|INV|FUNC|ARRAY, OP|SUBTEXT);
			else if(oper==Expression::ADD || oper==Expression::MUL) Valid(VAL|INV|ARRAY,ALL);
			else Valid(VAL|INV,ALL);
		}
		else break;

		// Add to expression
		if(front<0) {
			stack[0] = new Expression;
			stack[0]->op = Expression::NIL;
			stack[0]->lhs = out;
			front = 0;
		}
		else if(out.type) {
			stack[front]->rhs = out;
		}
		out.type = Expression::NOTHING;
	}
	if(front<0) return 0;
	return stack[0];
}

// ========================================================================================== //

#include <vector>
Block* Script::parseBlock(const char* src, const char*& s, bool allowSet, bool allowControl, bool braces) {
	#undef Error
	#define Error(desc) printf("Error: %s on line %d\n",desc,getLineNumber(src,s)),s=start,(Block*)0
	std::vector<Expression*> lines;
	const char* start = s;
	s += comment(s);
	if(braces) {
		if(*s == '{') ++s;
		else return 0;
	}
	bool noMoreLines = false;
	Context::Signal signal = Context::NONE;
	while(*s && *s!='}') {
		s += comment(s);
		while(*s==';') ++s, s += comment(s);
		
		// Parse control signals
		if(allowControl && !signal) {
			if(keyword(s, "continue")) signal = Context::CONTINUE, noMoreLines=true;
			if(keyword(s, "return")) signal = Context::RETURN; // Need return value
			if(keyword(s, "break")) signal = Context::BREAK, noMoreLines=true;
			if(keyword(s, "halt")) signal = Context::HALT, noMoreLines=true;
			if(signal) continue;
		}
		
		Expression* e = parseExpression(src, s, allowSet, false);
		if(noMoreLines) delete e;
		else if(e) lines.push_back(e);
		else break; // Some error
		if(signal) noMoreLines = true; // Ignore any expressions after control signal
	}
	s += comment(s);
	if(braces) {
		if(*s=='}') ++s;
		else {
			for(size_t i=0; i<lines.size(); ++i) delete lines[i];
			return Error("Missing '}'");
		}
	}
	// Create block
	Block* b = new Block();
	if(!lines.empty()) {
		b->length = lines.size();
		b->lines = new Expression*[lines.size()];
		for(size_t i=0; i<lines.size(); ++i) b->lines[i] = lines[i];
	}
	b->signal = signal;
	return b;
}

Function* Script::parseFunction(const char* src, const char*& s) {
	if(!keyword(s, "function")) return 0;
	const char* start = s;
	s += strspn(s, " \t"); // basic whitespace - no comments or line breaaks
	// Parameters
	#undef Error
	#define Error(desc) printf(desc),s=start,(Function*)0
	if(*s != '(') return Error("Error: Function expects parameter list\n");
	++s;
	int count = 0;
	const char* argn[20];
	int argl[20];
	while(*s && count<20) {
		s += whitespace(s);
		if(*s==')') break;
		int len = name(s);
		if(len==0) return Error("Error: Expected parameter name\n");
		argn[count] = s;
		argl[count] = len;
		++count;
		s += len;
		s += whitespace(s);
		if(*s==')') break;
		else if(*s==',') ++s;
		else return Error("Error: Expected ')'\n");
	}
	if(count==20) return Error("Error: unction has too many arguments");
	if(*s==0) return Error("Error: Expected ')'\n");
	++s;
	// Function body
	s+=whitespace(s);
	Block* b = parseBlock(src, s, true, true, true);
	if(!b) return Error("Error: Expected block\n");
	// Done
	Function* func = new Function();
	func->argc = count;
	func->argn = new uint[count];
	for(int i=0; i<count; ++i) func->argn[i] = Variable::lookupName(String(argn[i], argl[i]));
	func->length = b->length;
	func->lines = b->lines;
	func->signal = b->signal;
	b->lines = 0;
	b->length = 0;
	delete b;
	return func;
}

// ========================================================================================== //

Expression* Script::parseExpression(const char* source, bool set) {
	const char* s = source;
	return parseExpression(source, s, set, false);
}

Function* Script::parseFunction(const char* source) {
	const char* s = source;
	return parseFunction(source, s);
}

Function* Script::parseFunction(const char* source, int argc, const char* const argv[]) {
	if(!source || !source[0]) return nullptr;
	const char* s = source;
	Block* block = parseBlock(source, s, true, false, false);
	if(block) {
		Function* func = new Function();
		func->argc = argc;
		func->argn = new uint[argc];
		for(int i=0; i<argc; ++i) func->argn[i] = Variable::lookupName(argv[i]);
		func->length = block->length;
		func->lines = block->lines;
		func->signal = Context::RETURN;
		block->lines = 0;
		block->length = 0;
		delete block;
		return func;
	}
	return nullptr;
}

bool Script::parse(const char* source) {
	if(m_block) delete m_block;
	if(source) {
		const char* s = source;
		Block* b = parseBlock(source, s, true, false, false);
		m_block = b;
		m_block->scoped = false;
	}
	else m_block = nullptr;
	return m_block;
}
bool Script::run(Variable& v) {
	if(!m_block) return false;
	// Make sure context is an object variable
	if(!v.isValid() || !v.makeObject()) return printf("Script Error: Invalid context"), false;
	Context context(v);
	m_block->evaluate(fwd(context));
	return true;
}
Script::~Script() { delete m_block; }

