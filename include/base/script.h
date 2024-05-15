#pragma once

#include <base/math.h>
#include "variable.h"

namespace script {

	/// Script variable context
	class Context {
		public:
		enum Signal { NONE, RETURN, BREAK, CONTINUE, HALT };
		Context();
		Context(const Variable&);
		~Context();
		Variable& get(uint  id);
		Variable& get(uint* id);
		Variable& get(const char* name);
		Variable& get(const VariableName&);
		Variable& getw(uint id);
		Variable& getw(uint* id);
		Variable& set(uint  id, const Variable& value);
		Variable& set(uint* id, const Variable& value);
		Variable& set(const char* id, const Variable& value);
		void push(const Variable&, bool localSet=false);
		void pop();
		Signal getSignal() const;
		const Variable& getReturnValue() const;
		void setSignal(Signal, const Variable&);
		protected:
		Variable* scope;
		int       front;
		Signal    signal;
		uint      writeFlags;	// bitmask, 1=new scope
		uint      writeMask;	// cached bitmask
	};

	class FunctionCall;

	/// Script expression, eg "blah+5", or "a<b", or "6"
	class Expression {
		protected:
		friend class Script;
		enum Operator { NIL, SET, APPEND, OR, AND, EQ, NE, GT, GE, LT, LE, NOT, ADD, SUB, NEG, MUL, DIV, GET,GET2,CALL };
		enum OperandType { NOTHING, NAME, COMPOUNDNAME, CONSTANT, EXPRESSION, FUNCTIONCALL };
		struct Operand { OperandType type; union { uint var; uint* cvar; Variable* con; const Expression* expr; FunctionCall* call; }; };
		Operand  lhs, rhs;	// Operands can be variable, constant, sub-expression
		Operator op;		// Operator
		Expression();
		static Variable  opValue(const Operand&, Context&& context);
		static Variable& opVariableRef(const Operand& v, Context&& context);
		static String    opString(const Operand&);
		static void      opDelete(Operand&);
		static int compare(const Variable& a, const Variable& b);
		public:
		virtual ~Expression();
		virtual Variable evaluate(Context&& context) const;
		virtual String toString() const;
		bool isConst() const { return op!=SET && op!=APPEND && !(lhs.type==EXPRESSION&&!lhs.expr->isConst()) && !(rhs.type==EXPRESSION&&!rhs.expr->isConst()); }
		inline Variable evaluate(Context& context) const { return evaluate(std::forward<Context>(context)); }
		VariableName extractVariableName(Context&&) const; // If a SET expression, or evaluates to a varaible lookup, get the full name lookup
	};
	// Conditional expression ( condition ? trueValue, falseValue )
	class Conditional : public Expression {
		protected:
		friend class Script;
		Operand rhd;
		Conditional();
		public:
		virtual ~Conditional();
		virtual Variable evaluate(Context&&) const override;
		virtual String toString() const override;
	};

	/// Script block
	class Block : public Expression {
		public:
		Block();
		virtual ~Block();
		virtual Variable evaluate(Context&&) const override;
		virtual String toString() const override;
		protected:
		friend class Script;
		Expression** lines;
		int          length;
		protected:
		bool            scoped;
		bool            localSet;	// set operations only use local scope
		Context::Signal signal;
	};
	// Special block type for defining arrays
	class ArrayBlock : public Block {
		public:
		virtual Variable evaluate(Context&&) const override;
		virtual String toString() const override;
	};


	/// Script function - can be stored as a variable
	class Function : protected Block {
		public:
		Function();
		virtual ~Function();
		virtual String toString() const override;
		virtual String toString(bool body) const;
		virtual Variable evaluate(Context&&) const override;
		using Expression::evaluate;

		Variable call() const { return evaluate(Context()); }
		template<typename... Args> Variable call(const Args&... a) const {
			Variable args;
			setArg<0>(args, a...); 
			return evaluate(Context(args));
		}
		
		template<class F> static Function* bind(const F&);
		template<int N, class F> static Function* bind(const char*const(&params)[N], const F&);

		protected:
		void setArgNames(int count, uint* list) { argc=count; argn=list; }
		const uint* getArgs() const { return argn; }

		private:
		template<int N, typename T> void setArg(Variable& c, const T& v) const { if(N<argc) c.set(argn[N], v); }
		template<int N, typename T, typename... Args> void setArg(Variable& c, const T& v, Args&... args ) const {
			if(N<argc) {
				c.set(argn[N], v);
				setArg<N+1>(c, args...);
			}
		}

		private:
		friend class Script;
		friend class Variable;
		friend class FunctionCall;
		int   argc;
		uint* argn;
		int   ref;
	};


	/// Lambda function
	template<int N, class F>
	class CFunction : public Function {
		public:
		CFunction(const char* const (&params)[N], const F& function) : m_function(function) {
			if(N) {
				uint* p = new uint[N];
				for(int i=0; i<N; ++i) p[i] = Variable::lookupName(params[i]);
				setArgNames(N, p);
			}
		}
		virtual Variable evaluate(Context&& context) const override;
		private:
		F m_function;
	};
	template<class F, int I> struct UnpackFunctionCall {
		template<class...Args>
		static Variable call(const F& func, const uint* argn, Context&& c, const Args&...args) {
			return UnpackFunctionCall<F, I-1>::call(func, argn, std::forward<Context>(c), c.get(argn[I-1]), args...); 
		}
	};
	template<class F> struct UnpackFunctionCall<F, 0> {
		template<class...Args> static Variable call(const F& func, const uint* argn, Context&& c, const Args&...args) { return func(args...); }
	};
	template<class F> Function* Function::bind(const F& f) { return new CFunction<0, F>({}, f); }
	template<int N, class F> Function* Function::bind(const char* const (&params)[N], const F& f) { return new CFunction<N, F>(params, f); }
	template<int N, class F> Variable CFunction<N,F>::evaluate(Context&& c) const { 
		return UnpackFunctionCall<F, N>::call(m_function, getArgs(), std::forward<Context>(c));
	}


	/// Entry point
	class Script {
		public:
		Script() = default;
		Script(const char* source) { parse(source); }
		~Script();
		bool        parse(const char* source);
		bool		run(Variable& context);
		public:
		static Function*   parseFunction(const char* src);
		static Function*   parseFunction(const char* src, int argc=0, const char* const argn[]=0);
		static Function*   createFunction(Expression* expr, int argc=0, const char* const argn[]=0);
		static Expression* parseExpression(const char* source, bool set=false);

		template<int N> static Function* parseFunction(const char* src, const char* const (&args)[N]) { return parseFunction(src, N, args); }
		template<int N> static Function* createFunction(Expression* e, const char* const (&args)[N]) { return createFunction(e, N, args); }

		protected:
		static Block*      parseBlock(const char* src, const char*& s, bool set, bool control, bool braces);
		static Expression* parseExpression(const char* src, const char*& s, bool set, bool array);
		static Function*   parseFunction(const char* src, const char*& s);
		static int         parseOperator(const char*&);
		static int         parseInvertor(const char*&);
		static void        compoundExpression(Expression** stack, int& front, Expression* expr, int precidence);
		private:
		Block* m_block = nullptr;
	};

}

