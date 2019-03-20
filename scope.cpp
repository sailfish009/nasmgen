#include "scope.hpp"


unsigned Scope::num = 0;


Scope::Scope(): parent(NULL) {
}


Scope::Scope(const Scope* p): parent(p) {
}


Scope::~Scope() {
	for (std::vector<var_t>::iterator it = vars.begin(); it != vars.end(); ++it) {
		free(it->name);
	}
}

unsigned Scope::push() {
	return ++num;
}


unsigned Scope::push(const char* v) {
	return push(v, strlen(v));
}


unsigned Scope::push(const char* v, size_t l) {
	if (find(v, l)) return 0;
	unsigned id = ++num;
	vars.push_back((var_t){strndup(v, l), id});
	return id;
}


void Scope::pop(unsigned id) {
	for (std::vector<var_t>::iterator it = vars.begin(); it != vars.end(); ++it) {
		if (it->id == id) {
			free(it->name);
			vars.erase(it);
			return;
		}
	}
	assert(false);
}


unsigned Scope::pop() {
	if (!vars.size()) return 0;
	unsigned rv = vars.back().id;
	assert(rv);
	free(vars.back().name);
	vars.pop_back();
	return rv;
}


unsigned Scope::find(const char* v, bool* this_lvl) const {
	return find(v, strlen(v), this_lvl);
}


unsigned Scope::find(const char* v, size_t l, bool* this_lvl) const {
	if (this_lvl) *this_lvl = false;
	for (std::vector<var_t>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
		if (l == strlen(it->name) && !strncmp(it->name, v, l)) {
			if (this_lvl) *this_lvl = true;
			return it->id;
		}
	}
	return parent? parent->find(v, l): 0;
}
