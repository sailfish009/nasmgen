#pragma once
#include "token.hpp"
#include "common.hpp"
#include <vector>


class Inst {
	public:
		typedef enum { FUNC, DECL, STR, UNDEF, EXT, IF, ELSE, LOOP, OP, GOTO, LBL, END, ASM, NOOP } type_t;

	protected:
		Inst(type_t);

	private:
		char* suff;
		const type_t type;

	public:
		~Inst();

		static Inst* parse(const char*);
		bool is(type_t) const;

		void comment(const char*);
		const char* suffix() const;
};


class InstEnd: public Inst {
	public:
		InstEnd();
};


class InstElse: public Inst {
	public:
		InstElse();
};


class InstNoop: public Inst {
	public:
		InstNoop();
};


class InstAsm: public Inst {
	private:
		void cln();

	public:
		InstAsm(const char*);
		~InstAsm();

		const char* const asmstr;
};


class InstDecl: public Inst {
	private:
		void cln();

	public:
		InstDecl(Token*, Token*);
		~InstDecl();

		const Token* const type;
		const Token* const id;
};


class InstExt: public Inst {
	private:
		void cln();

	public:
		InstExt(Token*);
		~InstExt();

		const Token* const id;
};


class InstStr: public Inst {
	private:
		void cln();

	public:
		InstStr(Token*, Token*, const char*);
		~InstStr();

		const Token* const type;
		const Token* const id;
		const char* const str;
};


class InstUndef: public Inst {
	private:
		void cln();

	public:
		InstUndef(Token*);
		~InstUndef();

		const Token* const id;
};

class InstLbl: public Inst {
	private:
		void cln();

	public:
		InstLbl(Token*);
		~InstLbl();

		const Token* const id;
};

class InstGoto: public Inst {
	private:
		void cln();

	public:
		InstGoto(Token*);
		~InstGoto();

		const Token* const id;
};

class InstOp: public Inst {
	private:
		void cln();

	public:
		InstOp(Token*, Token*, Token*);
		~InstOp();

		const Token* const a;
		const Token* const op;
		const Token* const b;
};


class InstLoop: public Inst {
	private:
		void cln();

	public:
		InstLoop(Token*, Token*, Token*);
		~InstLoop();

		const Token* const id;
		const Token* const from;
		const Token* const to;
};


class InstIf: public Inst {
	private:
		void cln();

	public:
		InstIf(bool, bool, Token*);
		InstIf(bool, Token*, Inst*);
		InstIf(bool, bool, Token*, Token*, Token*);
		InstIf(bool, Token*, Token*, Token*, Inst*);
		~InstIf();

		const bool repeat;
		const bool negate;
		const Token* const a;
		const Token* const cmp;
		const Token* const b;
		const Inst* op;
};


class InstFunc: public Inst {
	private:
		void cln();

	public:
		InstFunc(InstDecl*);
		InstFunc(InstDecl*, const std::vector<const InstDecl*>&);
		~InstFunc();

		const InstDecl* const decl;
		const std::vector<const InstDecl*> args;
};
