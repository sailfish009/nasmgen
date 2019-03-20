#pragma once
#include "inst.hpp"
#include "regs.hpp"
#include "scope.hpp"
#include "common.hpp"
#include <vector>


class Program {
	private:
		std::vector<const Inst*> lines;
		std::vector<const char*> pushes; // all register pushes (stack has no iterators)
		Regs regs;

		static width_t min_width(uint64_t);
		static const char* width_decl(width_t);
		static const char* cmp_decl(Token::cmp_t, bool);
		const char* replace_regs(const Scope*, const char*) const;

		bool peek(Token::op_t, std::vector<const Inst*>::const_iterator) const;
		bool min_width(const Scope*, const Token*, width_t&, width_t&) const;
		bool op_width(const Scope*, const Token*, const Token*, bool, bool, width_t&, width_t&) const;
		bool val2str(const Scope*, const Token*, width_t, bool, bool, bool, reg_id_t, char*) const;

		bool interpret_comment(std::vector<const Inst*>::iterator&);
		bool interpret_line(Scope*, std::vector<const Inst*>::iterator&);
		bool interpret_block(Scope*, std::vector<const Inst*>::iterator&);

	public:
		// TODO: destructor/cleanups
		void push(Inst*);
		bool interpret();
};
