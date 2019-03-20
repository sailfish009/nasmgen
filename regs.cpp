#include "regs.hpp"


Reg::Reg(const reg_info_t i): reg_info(i), val((reg_val_t){0, false, false, WIDTH_8, WIDTH_8}) {
}


const char* Reg::get_str(width_t w) const {
	switch (w) {
		case WIDTH_8:
			return reg_info.names[0];
		case WIDTH_16:
			return reg_info.names[1];
		case WIDTH_32:
			return reg_info.names[2];
		case WIDTH_64:
			return reg_info.names[3];
	}
	assert(false);
	return "";
}


const char* Reg::get_str() const {
	assert(used());
	return get_str(val.width);
}


const char* Reg::get_c_str(width_t w, bool is_ptr) {
	switch (w) {
		case WIDTH_8:
			return is_ptr? "unsigned char*": "unsigned char";
		case WIDTH_16:
			return is_ptr? "uint16_t*": "uint16_t";
		case WIDTH_32:
			return is_ptr? "uint32_t*": "uint32_t";
		case WIDTH_64:
			return is_ptr? "uint64_t*": "uint64_t";
		default:
			assert(false);
			return "";
	}
}


bool Reg::used() const {
	return val.var_id != 0;
}


const Reg::reg_val_t& Reg::get() const {
	assert(used());
	return val;
}


const Reg::reg_info_t& Reg::info() const {
	return reg_info;
}


void Reg::set() {
	assert(used());
	val.var_id = 0;
}


void Reg::set(unsigned id, width_t w, bool ptr, const char*& needs_push) {
	assert(!used());
	val.var_id = id;
	val.is_ptr = ptr;
	val.width = w;
	val.init_width = w;
	if (w >= WIDTH_32 && !ptr) { // TODO: check if this is really always the case: In 64-bit mode, the default instruction size is still 32 bits. When loading a value into a 32-bit register (but not an 8- or 16-bit register), the upper 32 bits of the corresponding 64-bit register are set to zero.
		val.init_width = WIDTH_64;
	}
	if (reg_info.needs_push && !val.pushed) {
		needs_push = get_str(WIDTH_64);
		val.pushed = true;
	} else {
		needs_push = NULL;
	}
}


void Reg::set(width_t wi) {
	assert(used());
	if (wi > val.width && !val.is_ptr) {
		val.init_width = wi;
	}
}


Regs::Regs(): regs((Reg[]){
		Reg((Reg::reg_info_t){ REG_A,  {"AL",   "AX",   "EAX",  "RAX"}, false, true  }),
		Reg((Reg::reg_info_t){ REG_B,  {"BL",   "BX",   "EBX",  "RBX"}, true , true  }),
		Reg((Reg::reg_info_t){ REG_C,  {"CL",   "CX",   "ECX",  "RCX"}, false, true  }),
		Reg((Reg::reg_info_t){ REG_D,  {"DL",   "DX",   "EDX",  "RDX"}, false, false }),
		Reg((Reg::reg_info_t){ REG_SI, {"SIL",  "SI",   "ESI",  "RSI"}, false, true  }),
		Reg((Reg::reg_info_t){ REG_DI, {"DIL",  "DI",   "EDI",  "RDI"}, false, true  }),
		Reg((Reg::reg_info_t){ REG_BP, {"BPL",  "BP",   "EBP",  "RBP"}, true , true  }),
		Reg((Reg::reg_info_t){ REG_SP, {"SPL",  "SP",   "ESP",  "RSP"}, false, false }),
		Reg((Reg::reg_info_t){ REG_8,  {"R8B",  "R8W",  "R8D",  "R8" }, false, true  }),
		Reg((Reg::reg_info_t){ REG_9,  {"R9B",  "R9W",  "R9D",  "R9" }, false, true  }),
		Reg((Reg::reg_info_t){ REG_10, {"R10B", "R10W", "R10D", "R10"}, false, true  }),
		Reg((Reg::reg_info_t){ REG_11, {"R11B", "R11W", "R11D", "R11"}, false, true  }),
		Reg((Reg::reg_info_t){ REG_12, {"R12B", "R12W", "R12D", "R12"}, true , true  }),
		Reg((Reg::reg_info_t){ REG_13, {"R13B", "R13W", "R13D", "R13"}, true , true  }),
		Reg((Reg::reg_info_t){ REG_14, {"R14B", "R14W", "R14D", "R14"}, true , true  }),
		Reg((Reg::reg_info_t){ REG_15, {"R15B", "R15W", "R15D", "R15"}, true , true  })
	}) {
}


Regs::~Regs() {
	while (!bufs.empty()) {
		pop(bufs.front()->get().var_id);
	}
}


bool Regs::push_lbl(unsigned id, width_t size, bool needs_deref, const char* lbl) { // buf
	char* dlbl = strdup(lbl);
	Reg* r = new Reg((Reg::reg_info_t){ REG_NONE, {dlbl, dlbl, dlbl, dlbl}, false, false });
	const char* needs_push = NULL;
	r->set(id, size, needs_deref, needs_push);
	assert(!needs_push);
	bufs.push_back(r);
	return true;
}


bool Regs::push(unsigned id, width_t size, bool needs_deref) {
	const char* needs_push = NULL;
	bool rv = push(id, size, needs_deref, needs_push);
	assert(!needs_push);
	return rv;
}


bool Regs::push(unsigned id, width_t size, bool needs_deref, const char*& needs_push) {
	for (unsigned i=0; i<REG_NONE; ++i) {
		if (!regs[i].info().needs_push && regs[i].info().auto_assign) {
			if (push(id, size, (reg_id_t)i, needs_deref, needs_push)) return true;
		}
	}
	for (unsigned i=0; i<REG_NONE; ++i) {
		if (regs[i].info().needs_push && regs[i].info().auto_assign) {
			if (push(id, size, (reg_id_t)i, needs_deref, needs_push)) return true;
		}
	}
	return false;
}


bool Regs::push(unsigned id, width_t size, reg_id_t reg, bool needs_deref) { // for rv and args
	const char* needs_push = NULL;
	bool rv = push(id, size, reg, needs_deref, needs_push);
	assert(!needs_push);
	return rv;
}


bool Regs::push(unsigned id, width_t size, reg_id_t reg, bool needs_deref, const char*& needs_push) {
	assert(id);
	assert(reg != REG_NONE);
	if (regs[reg].used()) return false;
	regs[reg].set(id, size, needs_deref, needs_push);
	return true;
}


void Regs::pop(unsigned id) {
	assert(id);
	for (unsigned i=0; i<REG_NONE; ++i) {
		if (regs[i].used() && regs[i].get().var_id == id) {
			regs[i].set();
			return;
		}
	}
	for (std::vector<Reg*>::iterator it=bufs.begin(); it!=bufs.end(); ++it) {
		if ((*it)->used() && (*it)->get().var_id == id) {
			free((void*)((*it)->get_str()));
			delete *it;
			bufs.erase(it);
			return;
		}
	}
	assert(false);
}


const Reg& Regs::get(reg_id_t id) const {
	assert(id != REG_NONE);
	return regs[id];
}


const Reg& Regs::get(unsigned id) const {
	assert(id);
	for (unsigned i=0; i<REG_NONE; ++i) {
		if (regs[i].used() && regs[i].get().var_id == id) {
			return regs[i];
		}
	}
	for (std::vector<Reg*>::const_iterator it=bufs.begin(); it!=bufs.end(); ++it) {
		if ((*it)->used() && (*it)->get().var_id == id) {
			return **it;
		}
	}
	assert(false);
	return regs[0];
}


void Regs::inc_init(unsigned id, width_t w) {
	assert(id);
	for (unsigned i=0; i<REG_NONE; ++i) {
		if (regs[i].used() && regs[i].get().var_id == id) {
			regs[i].set(w);
			return;
		}
	}
	for (std::vector<Reg*>::iterator it=bufs.begin(); it!=bufs.end(); ++it) {
		if ((*it)->used() && (*it)->get().var_id == id) {
			(*it)->set(w);
			return;
		}
	}
	assert(false);
}
