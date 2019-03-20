#include "inst.hpp"


Inst::Inst(type_t t): suff(NULL), type(t) {
}

Inst::~Inst() {
	if (suff) free(suff);
}


bool Inst::is(type_t t) const {
	return type == t;
}


const char* Inst::suffix() const {
	return suff ?: "";
}


void Inst::comment(const char* c) {
	if (suff) free(suff);
	while (*c == ' ') ++c;
	if (!*c || asprintf(&suff, " ; %s", c) == -1) {
		suff = NULL;
	}
}


Inst* Inst::parse(const char* line) {
	Inst* rv = NULL;

	int n = 0;
	char p;
	char a[512], b[512], c[512], d[512], e[512], f[512];

	try {
		if (sscanf(line, " asm%*[ ] \"%[^\"]%c %n", a, &p, &n) >= 2 && p == '"') {
			rv = new InstAsm(a);
		} else if (sscanf(line, " undef%*[ ] %[a-z_] %n", a, &n) >= 1) {
			rv = new InstUndef(Token::parseVal(a, false, true, false));
		} else if (sscanf(line, " goto%*[ ] %[a-z_] %n", a, &n) >= 1) {
			rv = new InstGoto(Token::parseVal(a, false, true, false));
		} else if (sscanf(line, " ext%*[ ] %[a-z_] %n", a, &n) >= 1) {
			rv = new InstExt(Token::parseVal(a, false, true, false));
		} else if (sscanf(line, " else %c %n", &p, &n) >= 1 && p == '{') {
			rv = new InstElse();
		} else if (sscanf(line, " %[^( ] ( %[^<>= ] %[<>=] %[^) ] ) goto%*[ ] %[a-z_] %n", a, b, c, d, e, &n) >= 5 && (!strcmp(a, "if") || !strcmp(a, "unless"))) {
			Inst* inst = new InstGoto(Token::parseVal(e, false, true, false));
			rv = new InstIf(strcmp(a, "if") != 0, Token::parseVal(b, false, true, true), Token::parseCmp(c), Token::parseVal(d, true, true, true), inst);
		} else if (sscanf(line, " %[^( ] ( %[^<>= ] %[<>=] %[^) ] ) %[][a-z0-9_] := %[][a-z0-9_] %n", a, b, c, d, e, f, &n) >= 6 && (!strcmp(a, "if") || !strcmp(a, "unless"))) {
			Inst* inst = new InstOp(Token::parseVal(e, false, true, true), Token::parseOp(':'), Token::parseVal(f, true, true, true));
			rv = new InstIf(strcmp(a, "if") != 0, Token::parseVal(b, false, true, true), Token::parseCmp(c), Token::parseVal(d, true, true, true), inst);
		} else if (sscanf(line, " %[^( ] ( %[^<>= ] %[<>=] %[^) ] ) %c %n", a, b, c, d, &p, &n) >= 5 && p == '{' && (!strcmp(a, "if") || !strcmp(a, "unless") || !strcmp(a, "while") || !strcmp(a, "until"))) {
			rv = new InstIf(!strcmp(a, "while") || !strcmp(a, "until"), !strcmp(a, "unless") || !strcmp(a, "until"), Token::parseVal(b, false, true, true), Token::parseCmp(c), Token::parseVal(d, true, true, true));
		} else if (sscanf(line, " %[^( ] ( %[^) ] ) goto%*[ ] %[a-z_] %n", a, b, c, &n) >= 3 && (!strcmp(a, "if") || !strcmp(a, "unless"))) {
			Inst* inst = new InstGoto(Token::parseVal(c, false, true, false));
			rv = new InstIf(strcmp(a, "if") != 0, Token::parseVal(b, false, true, true), inst);
		} else if (sscanf(line, " %[^( ] ( %[^) ] ) %[][a-z0-9_] := %[][a-z0-9_] %n", a, b, c, d, &n) >= 4 && (!strcmp(a, "if") || !strcmp(a, "unless"))) {
			Inst* inst = new InstOp(Token::parseVal(c, false, true, true), Token::parseOp(':'), Token::parseVal(d, true, true, true));
			rv = new InstIf(strcmp(a, "if") != 0, Token::parseVal(b, false, true, true), inst);
		} else if (sscanf(line, " %[^( ] ( %[^) ] ) %c %n", a, b, &p, &n) >= 3 && p == '{' && (!strcmp(a, "if") || !strcmp(a, "unless") || !strcmp(a, "while") || !strcmp(a, "until"))) {
			rv = new InstIf(!strcmp(a, "while") || !strcmp(a, "until"), !strcmp(a, "unless") || !strcmp(a, "until"), Token::parseVal(b, false, true, true));
		} else if (sscanf(line, " %[up0-9]%*[ ] %[^( ] ( ) %c %n", a, b, &p, &n) >= 3 && p == '{') {
			InstDecl* inst = new InstDecl(Token::parseType(a), Token::parseVal(b, false, true, false));
			rv = new InstFunc(inst);
		} else if (sscanf(line, " %[up0-9]%*[ ] %[^( ] ( %[a-z0-9_, ] ) %c %n", a, b, c, &p, &n) >= 4 && p == '{') {
			InstDecl* inst = new InstDecl(Token::parseType(a), Token::parseVal(b, false, true, false));
			std::vector<const InstDecl*> args;
			try {
				int nn;
				bool done = false;
				const char* cc = c;
				while (strlen(cc)) {
					if (done) throw __LINE__;
					if (sscanf(cc, " %[up0-9]%*[ ] %[a-z_] %c %n", d, e, &p, &nn) >= 3 && p == ',') {
						args.push_back(new InstDecl(Token::parseType(d), Token::parseVal(e, false, true, false)));
					} else if (sscanf(cc, " %[up0-9]%*[ ] %[a-z_] %n", d, e, &nn) >= 2) {
						args.push_back(new InstDecl(Token::parseType(d), Token::parseVal(e, false, true, false)));
						done = true;
					} else {
						throw __LINE__;
					}
					assert(nn);
					cc += nn;
				}
				if (!done) throw __LINE__;
			} catch (...) {
				delete inst;
				for (std::vector<const InstDecl*>::iterator it = args.begin(); it != args.end(); ++it) delete *it;
				throw __LINE__;
			}
			rv = new InstFunc(inst, args);
		} else if (sscanf(line, " repeat ( %[a-z_] , %[0-9] , %[0-9] ) %c %n", a, b, c, &p, &n) >=4 && p == '{') {
			rv = new InstLoop(Token::parseVal(a, false, true, false), Token::parseVal(b, true, false, false), Token::parseVal(c, true, false, false));
		} else if (sscanf(line, " %[][a-z0-9_] %c= %[][a-z0-9_] %n", a, &p, b, &n) >= 3) {
			rv = new InstOp(Token::parseVal(a, false, true, true), Token::parseOp(p), Token::parseVal(b, true, true, true));
		} else if (sscanf(line, " %[a-z_]%c %n", a, &p, &n) >= 2 && p == ':') {
			rv = new InstLbl(Token::parseVal(a, false, true, false));
		} else if (sscanf(line, " %c %n", &p, &n) >= 1 && p == '}') {
			rv = new InstEnd();
		} else if (sscanf(line, " %[up0-9]%*[ ] %[a-z_] %n", a, b, &n) >= 2) {
			rv = new InstDecl(Token::parseType(a), Token::parseVal(b, false, true, false));
		} else if (sscanf(line, " %[s0-9]%*[ ] %[a-z_] %[^\n] %n", a, b, c, &n) >= 3) {
			rv = new InstStr(Token::parseType(a), Token::parseVal(b, false, true, false), c);
		} else { // trim in case of unknown line, empty line, or comment
			(void)sscanf(line, " %n", &n);
		}
	} catch (int lnum) {
		assert(rv == NULL);
		n = 0;
	}

	line += n;

	if (*line == '\0') {
		if (!rv) {
			rv = new InstNoop();
		}
	} else if (*line == '#') {
		if (!rv) {
			rv = new InstNoop();
		}
		if (line[1] != '#') { // ignore "double" comments
			rv->comment(line+1);
		}
		line += strlen(line);
	} else {
		if (rv) {
			delete rv;
			rv = NULL;
		}
	}

	return rv;
}


InstEnd::InstEnd(): Inst(END) {
}


InstElse::InstElse(): Inst(ELSE) {
}


InstNoop::InstNoop(): Inst(NOOP) {
}


InstAsm::InstAsm(const char* a): Inst(ASM), asmstr(strdup(a?:"")) {
	unless (asmstr && *asmstr) { cln(); throw __LINE__; }
}
InstAsm::~InstAsm() {
	cln();
}
void InstAsm::cln() {
	if (asmstr) free((void*)asmstr);
}


InstDecl::InstDecl(Token* t, Token* name): Inst(DECL), type(t), id(name) { //, init(NULL) {
	unless (type && (type->is(Token::TOK_TYPE_UINT) || type->is(Token::TOK_TYPE_PTR))) { cln(); throw __LINE__; }
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
}
InstDecl::~InstDecl() {
	cln();
}
void InstDecl::cln() {
	if (type) delete type;
	if (id) delete id;
}


InstExt::InstExt(Token* name): Inst(EXT), id(name) {
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
}
InstExt::~InstExt() {
	cln();
}
void InstExt::cln() {
	if (id) delete id;
}


InstStr::InstStr(Token* t, Token* name, const char* s): Inst(STR), type(t), id(name), str(strdup(s?:"")) {
	unless (type && type->is(Token::TOK_TYPE_PTR)) { cln(); throw __LINE__; }
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
	unless (str && *str) { cln(); throw __LINE__; }
}
InstStr::~InstStr() {
	cln();
}
void InstStr::cln() {
	if (type) delete type;
	if (id) delete id;
	if (str) free((void*)str);
}


InstUndef::InstUndef(Token* name): Inst(UNDEF), id(name) {
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
}
InstUndef::~InstUndef() {
	cln();
}
void InstUndef::cln() {
	if (id) delete id;
}


InstLbl::InstLbl(Token* name): Inst(LBL), id(name) {
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
}
InstLbl::~InstLbl() {
	cln();
}
void InstLbl::cln() {
	if (id) delete id;
}


InstGoto::InstGoto(Token* name): Inst(GOTO), id(name) {
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
}
InstGoto::~InstGoto() {
	cln();
}
void InstGoto::cln() {
	if (id) delete id;
}


InstOp::InstOp(Token* a, Token* o, Token* b): Inst(OP), a(a), op(o), b(b) {
	unless (a && a->is(Token::TOK_IDENTIFIER)) { cln(); throw __LINE__; }
	unless (op && op->is(Token::TOK_OP)) { cln(); throw __LINE__; }
	unless (b && b->vartype() != Token::TOK_VAR_NONE) { cln(); throw __LINE__; }
}
InstOp::~InstOp() {
	cln();
}
void InstOp::cln() {
	if (a) delete a;
	if (op) delete op;
	if (b) delete b;
}


InstLoop::InstLoop(Token* i, Token* f, Token* t): Inst(LOOP), id(i), from(f), to(t) {
	unless (id && id->vartype() == Token::TOK_VAR_ID) { cln(); throw __LINE__; }
	unless (from && from->vartype() == Token::TOK_VAR_IMM) { cln(); throw __LINE__; }
	unless (to && to->vartype() == Token::TOK_VAR_IMM) { cln(); throw __LINE__; }
	unless (from->val_i < to->val_i) { cln(); throw __LINE__; }
}
InstLoop::~InstLoop() {
	cln();
}
void InstLoop::cln() {
	if (id) delete id;
	if (from) delete from;
	if (to) delete to;
}


InstIf::InstIf(bool rep, bool neg, Token* a): Inst(IF), repeat(rep), negate(neg), a(a), cmp(NULL), b(NULL), op(NULL) {
	unless (a && a->is(Token::TOK_IDENTIFIER)) { cln(); throw __LINE__; }
}
InstIf::InstIf(bool neg, Token* a, Inst* o): Inst(IF), repeat(false), negate(neg), a(a), cmp(NULL), b(NULL), op(o) {
	unless (a && a->is(Token::TOK_IDENTIFIER)) { cln(); throw __LINE__; }
	unless (!op || op->is(GOTO) || (op->is(OP) && ((InstOp*)op)->op->val_op == Token::TOK_OP_ASSIGN)) { cln(); throw __LINE__; }
}
InstIf::InstIf(bool rep, bool neg, Token* a, Token* c, Token* b): Inst(IF), repeat(rep), negate(neg), a(a), cmp(c), b(b), op(NULL) {
	unless (a && a->is(Token::TOK_IDENTIFIER)) { cln(); throw __LINE__; }
	unless ((!cmp && !b) || (cmp && cmp->is(Token::TOK_CMP) && b && b->vartype() != Token::TOK_VAR_NONE)) { cln(); throw __LINE__; }
}
InstIf::InstIf(bool neg, Token* a, Token* c, Token* b, Inst* o): Inst(IF), repeat(false), negate(neg), a(a), cmp(c), b(b), op(o) {
	unless (a && a->is(Token::TOK_IDENTIFIER)) { cln(); throw __LINE__; }
	unless ((!cmp && !b) || (cmp && cmp->is(Token::TOK_CMP) && b && b->vartype() != Token::TOK_VAR_NONE)) { cln(); throw __LINE__; }
	unless (!op || op->is(GOTO) || (op->is(OP) && ((InstOp*)op)->op->val_op == Token::TOK_OP_ASSIGN)) { cln(); throw __LINE__; }
}
InstIf::~InstIf() {
	cln();
}
void InstIf::cln() {
	if (a) delete a;
	if (cmp) delete cmp;
	if (b) delete b;
	if (op) delete op;
}


InstFunc::InstFunc(InstDecl* d): Inst(FUNC), decl(d) {
	unless (d) { cln(); throw __LINE__; }
}
InstFunc::InstFunc(InstDecl* d, const std::vector<const InstDecl*>& a): Inst(FUNC), decl(d), args(a) {
	unless (d) { cln(); throw __LINE__; }
	unless (args.size()) { cln(); throw __LINE__; }
	for (std::vector<const InstDecl*>::const_iterator it = args.begin(); it != args.end(); ++it) {
		unless (*it) { cln(); throw __LINE__; }
	}
}
InstFunc::~InstFunc() {
	cln();
}
void InstFunc::cln() {
	if (decl) delete decl;
	for (std::vector<const InstDecl*>::const_iterator it = args.begin(); it != args.end(); ++it) {
		if (*it) delete *it;
	}
}
