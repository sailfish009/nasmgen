#pragma once
#include "common.hpp"
#include <vector>


class Scope {
	private:
		typedef struct {
			char* name;
			unsigned id;
		} var_t;
		std::vector<var_t> vars;

		static unsigned num; // globally unique
		const Scope* const parent;

	public:
		Scope();
		Scope(const Scope*);
		~Scope();

		unsigned push();
		unsigned push(const char*);
		unsigned push(const char*, size_t);

		void pop(unsigned);
		unsigned pop();

		unsigned find(const char*, bool* = NULL) const;
		unsigned find(const char* v, size_t l, bool* = NULL) const;
};
