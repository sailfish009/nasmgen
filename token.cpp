#include "token.hpp"


Token::Token(type_t t, uint64_t v): type(t), val_i(v), val_s(NULL), offset(NULL) {
	assert(t != TOK_IDENTIFIER); // && t != TOK_COMMENT);
}


Token::Token(type_t t, const char* c, Token* off): type(t), val_i(0), val_s(strdup(c)), offset(off) {
	assert(t == TOK_IDENTIFIER); // || t == TOK_COMMENT);
	if (offset) {
		assert(t == TOK_IDENTIFIER);
		assert(offset->type == TOK_IDENTIFIER || offset->type == TOK_IMMEDIATE);
		assert(!offset->offset);
	}
}


Token::~Token() {
	if (val_s) free((void*)val_s);
	if (offset) delete (Token*)offset;
}


bool Token::is(type_t t) const {
	return type == t;
}


Token::var_t Token::vartype() const {
	switch (type) {
		case TOK_IMMEDIATE:
			return TOK_VAR_IMM;
		case TOK_IDENTIFIER:
			return offset? TOK_VAR_PTR: TOK_VAR_ID;
		default:
			return TOK_VAR_NONE;
	}
}


Token* Token::parseType(const char* line) {
	int n;
	char c;
	unsigned u;
	if (sscanf(line, " %c%u %n", &c, &u, &n) == 2 && n == (int)strlen(line)) {
		width_t w;
		switch (u) {
			case 8:  w = WIDTH_8;  break;
			case 16: w = WIDTH_16; break;
			case 32: w = WIDTH_32; break;
			case 64: w = WIDTH_64; break;
			default: return NULL;
		}
		switch (c) {
			case 'u':
				return new Token(TOK_TYPE_UINT, w);
			case 'p':
			case 's':
				return new Token(TOK_TYPE_PTR, w);
		}
	}
	return NULL;
}


Token* Token::parseVal(const char* line, bool imm_ok, bool id_ok, bool mem_ok) {
	int n;
	uint64_t i;
	unsigned u;
	char a[64], b[64];
	if (sscanf(line, " %[a-z][%u] %n", a, &u, &n) == 2 && n == (int)strlen(line)) {
		return (id_ok && mem_ok)? new Token(TOK_IDENTIFIER, a, new Token(TOK_IMMEDIATE, u)): NULL;
	} else if (sscanf(line, " %[a-z][%[a-z]] %n", a, b, &n) == 2 && n == (int)strlen(line)) {
		return (id_ok && mem_ok)? new Token(TOK_IDENTIFIER, a, new Token(TOK_IDENTIFIER, b)): NULL;
	} else if (sscanf(line, " %[a-z] %n", a, &n) == 1 && n == (int)strlen(line)) {
		return (id_ok)? new Token(TOK_IDENTIFIER, a): NULL;
	} else if (sscanf(line, " %" SCNu64 " %n", &i, &n) == 1 && n == (int)strlen(line)) {
		return (imm_ok)? new Token(TOK_IMMEDIATE, i): NULL;
	}
	return NULL;
}


Token* Token::parseCmp(const char* line) {
	int n;
	char a[64];
	if (sscanf(line, " %[<>=] %n", a, &n) == 1 && n == (int)strlen(line)) {
		if (!strcmp(a, "==")) {
			return new Token(TOK_CMP, TOK_CMP_EQ);
		} else if (!strcmp(a, "<>")) {
			return new Token(TOK_CMP, TOK_CMP_NE);
		} else if (!strcmp(a, "<=")) {
			return new Token(TOK_CMP, TOK_CMP_LE);
		} else if (!strcmp(a, ">=")) {
			return new Token(TOK_CMP, TOK_CMP_GE);
		} else if (!strcmp(a, "<")) {
			return new Token(TOK_CMP, TOK_CMP_LT);
		} else if (!strcmp(a, ">")) {
			return new Token(TOK_CMP, TOK_CMP_GT);
		}
	}
	return NULL;
}


Token* Token::parseOp(char c) {
	switch (c) {
		case ':':
			return new Token(TOK_OP, TOK_OP_ASSIGN);
		case '+':
			return new Token(TOK_OP, TOK_OP_ADD);
		case '-':
			return new Token(TOK_OP, TOK_OP_SUB);
		case '<':
			return new Token(TOK_OP, TOK_OP_SHL);
		case '>':
			return new Token(TOK_OP, TOK_OP_SHR);
		case '*':
			return new Token(TOK_OP, TOK_OP_MUL);
		case '/':
			return new Token(TOK_OP, TOK_OP_DIV);
		case '%':
			return new Token(TOK_OP, TOK_OP_MOD);
		default:
			return NULL;
	}
}
