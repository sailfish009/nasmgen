#pragma once
#include "common.hpp"


class Token {
	public:
		typedef enum {
			TOK_TYPE_UINT, // val: width_t
			TOK_TYPE_PTR, // val: width_t
			TOK_IDENTIFIER, // val: char*
			TOK_IMMEDIATE, // val: 64 bit int
			TOK_CMP, // val: cmp_t
			TOK_OP  // val: op_t
		} type_t;

		typedef enum {
			TOK_VAR_NONE=0, TOK_VAR_IMM, TOK_VAR_ID, TOK_VAR_PTR
		} var_t;
		typedef enum {
			TOK_CMP_LT, TOK_CMP_LE, TOK_CMP_EQ, TOK_CMP_NE, TOK_CMP_GE, TOK_CMP_GT
		} cmp_t;
		typedef enum {
			TOK_OP_ASSIGN, TOK_OP_ADD, TOK_OP_SUB, TOK_OP_SHL, TOK_OP_SHR, TOK_OP_MUL, TOK_OP_DIV, TOK_OP_MOD // mul and div only for eax
		} op_t;

	private:
		Token(type_t, uint64_t);
		Token(type_t, const char*, Token* = NULL);

		const type_t type;

	public:
		~Token();
		bool is(type_t) const;
		var_t vartype() const;

		static Token* parseType(const char*);
		static Token* parseVal(const char*, bool, bool, bool);
		static Token* parseCmp(const char*);
		static Token* parseOp(char);

		union {
			const uint64_t val_i;
			const width_t val_w;
			const cmp_t val_cmp;
			const op_t val_op;
		};
		const char* const val_s;
		const Token* const offset; // for TOK_IDENTIFIER[TOK_IDENTIFIER|TOK_IMMEDIATE] pointer dereferences
};
