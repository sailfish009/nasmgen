#include "program.hpp"


width_t Program::min_width(uint64_t v) {
	if (!(v & 0xffffffffffffff00LL)) {
		return WIDTH_8;
	} else if (!(v & 0xffffffffffff0000LL)) {
		return WIDTH_16;
	} else if (!(v & 0xffffffff00000000LL)) {
		return WIDTH_32;
	} else {
		return WIDTH_64;
	}
}


const char* Program::width_decl(width_t w) {
	switch (w) {
		case WIDTH_8:  return "byte";
		case WIDTH_16: return "word";
		case WIDTH_32: return "dword";
		case WIDTH_64: return "qword";
	}
	assert(false);
	return "";
}


const char* Program::cmp_decl(Token::cmp_t op, bool neg) {
	switch (op) {
		case Token::TOK_CMP_LT:
			return neg? "ge": "l";
		case Token::TOK_CMP_LE:
			return neg? "g": "le";
		case Token::TOK_CMP_EQ:
			return neg? "ne": "e";
		case Token::TOK_CMP_NE:
			return neg? "e": "ne";
		case Token::TOK_CMP_GE:
			return neg? "l": "ge";
		case Token::TOK_CMP_GT:
			return neg? "le": "g";
	}
	assert(false);
	return "";
}


bool Program::peek(Token::op_t op, std::vector<const Inst*>::const_iterator it) const {
	while (it != lines.end()) {
		if ((*it)->is(Inst::OP)) {
			if (((InstOp*)*it)->op->val_op == op) {
				return true;
			}
		}
		++it;
	}
	return false;
}


bool Program::min_width(const Scope* scope, const Token* t, width_t& width, width_t& inited) const { // min and max width for reading
	unsigned id;
	switch (t->vartype()) {
		case Token::TOK_VAR_IMM:
			width = min_width(t->val_i);
			inited = WIDTH_64; // will be zero padded if bigger
			return true;
		case Token::TOK_VAR_ID:
		case Token::TOK_VAR_PTR:
			id = scope->find(t->val_s);
			if (!id) {
				LOG("cannot find var '%s'", t->val_s);
				return false;
			}
			if (regs.get(id).get().is_ptr && !t->offset) {
				width = WIDTH_64;
				inited = WIDTH_64;
			} else {
				width = regs.get(id).get().width; // target size for vars and pointers
				inited = regs.get(id).get().init_width; // || Regs::get_ptr(id)
			}
			assert(inited >= width);
			return true;
		default:
			LOG("expected var or immediate");
			return false;
	}
}


bool Program::op_width(const Scope* scope, const Token* src, const Token* dst, bool ro, bool can_extend_src, width_t& src_width, width_t& dst_width) const {
	Token::var_t src_type = src->vartype();
	Token::var_t dst_type = dst->vartype();

	if (src_type == Token::TOK_VAR_NONE || dst_type == Token::TOK_VAR_NONE) {
		LOG("value expected");
		return false;
	}
	if (dst_type == Token::TOK_VAR_IMM) {
		LOG("immediate destination operand probably not supported");
		return false;
	}
	if (src_type == Token::TOK_VAR_PTR && dst_type == Token::TOK_VAR_PTR) {
		LOG("two memory operands probably not supported");
		return false;
	}

	width_t src_inited, dst_inited;
	if (!min_width(scope, src, src_width, src_inited) || !min_width(scope, dst, dst_width, dst_inited)) {
		LOG("cannot determine operand width");
		return false;
	}

	if (src_width == dst_width) {
		// ok in any case
	} else if (src_width > dst_width) { // src is too big
		if (ro && dst_inited >= src_width) { // but dst has enough zeroes for comparison
			dst_width = src_width;
		} else {
			LOG("oparand too small/big or uninitialized (%d:%d-%d <- %d:%d-%d)", dst->vartype(), dst_width, dst_inited, src->vartype(), src_width, src_inited);
			return false;
		}
	} else { // dst is bigger
		if (src_inited >= dst_width) { // but src has enough zeroes
			src_width = dst_width;
		} else if (can_extend_src) { // but can make src to have enough zeroes
			// keep src_width
		} else {
			LOG("oparand too small/big or uninitialized (%d-%d <- %d-%d)", dst_width, dst_inited, src_width, src_inited);
			return false;
		}
	}

	#if 0
		assert(width >= src_width);
		assert(width >= dst_width);
		assert(width <= src_inited || can_extend_src);
		assert(width <= dst_inited);
		if (src_type == Token::TOK_VAR_PTR) assert(width <= src_width);
		if (dst_type == Token::TOK_VAR_PTR) assert(width <= dst_width);
	#endif

	return true;
}


bool Program::val2str(const Scope* scope, const Token* token, width_t width, bool imm_ok, bool id_ok, bool mem_ok, reg_id_t must_reg_id, char* buf) const {
	if (token->is(Token::TOK_IMMEDIATE)) {
		if (!imm_ok) {
			LOG("immediates not supported here");
			return false;
		}
		sprintf(buf, "%s 0d%" PRIu64, width_decl(width), token->val_i);
	} else if (token->is(Token::TOK_IDENTIFIER)) {
		if (!id_ok) {
			LOG("vars not supported here: '%s'", token->val_s);
			return false;
		}
		unsigned id = scope->find(token->val_s);
		if (!id) {
			LOG("cannot find var '%s'", token->val_s);
			return false;
		}
		if (!regs.get(id).get().is_ptr || !token->offset) {
			reg_id_t reg_id = regs.get(id).info().reg_id;
			if (must_reg_id != REG_NONE && must_reg_id != reg_id) {
				LOG("only %s allowed for '%s'", regs.get(must_reg_id).get_str(width), token->val_s);
				return false;
			}
			if (false && reg_id == REG_NONE) { // label, loop-var, ...?
				strcpy(buf, token->val_s);
			} else {
				strcpy(buf, regs.get(reg_id).get_str(width));
			}
		} else if (!mem_ok) {
			LOG("memory locations not supported here: '%s'", token->val_s);
			return false;
		} else {
			if (token->offset->is(Token::TOK_IMMEDIATE)) {
				if (token->offset->val_i == 0) {
					sprintf(buf, "%s[%s]", width_decl(width), regs.get(id).get_str(WIDTH_64));
				} else if (regs.get(id).get().width == WIDTH_8) {
					sprintf(buf, "%s[%s+%" PRIu64 "]", width_decl(width), regs.get(id).get_str(WIDTH_64), token->offset->val_i);
				} else {
					sprintf(buf, "%s[%s+%" PRIu64 "*%d]", width_decl(width), regs.get(id).get_str(WIDTH_64), token->offset->val_i, (int)regs.get(id).get().width/8);
				}
			} else {
				unsigned oid = scope->find(token->offset->val_s);
				if (!oid) {
					LOG("cannot find offset var '%s'", token->offset->val_s);
					return false;
				}
				if (regs.get(oid).get().init_width != WIDTH_64) {
					LOG("pointer offset size mismatch (or not fully initialized) for '%s'", token->offset->val_s); // impossible combination of address sizes, invalid effective address
					return false;
				}
				if (regs.get(id).get().width == WIDTH_8) {
					sprintf(buf, "%s[%s+%s]", width_decl(width), regs.get(id).get_str(WIDTH_64), regs.get(oid).get_str(WIDTH_64));
				} else {
					sprintf(buf, "%s[%s+%s*%d]", width_decl(width), regs.get(id).get_str(WIDTH_64), regs.get(oid).get_str(WIDTH_64), (int)regs.get(id).get().width/8);
				}
			}
		}
	} else {
		LOG("identifier or immediate expected");
		return false;
	}
	return true;
}


bool Program::interpret_comment(std::vector<const Inst*>::iterator& it) {
	while (it != lines.end()) {
		if ((*it)->is(Inst::NOOP)) {
			const char* suff = (*it)->suffix();
			while (*suff == ' ') ++suff;
			if (*suff) {
				STDOUT("\t%s #%zu", suff, it-lines.begin()+1);
			} else {
				STDOUT("");
			}
		} else {
			break;
		}
		++it;
	}
	return true;
}


const char* Program::replace_regs(const Scope* scope, const char* str) const {
	static char rv[1024];
	strcpy(rv, str);

	char* p = rv;
	while ((p = strchr(p, '$')) != NULL) {
		char* q = strchr(p+1, '$');
		if (!q) break;

		unsigned id = scope->find(p+1, q-p-1);
		if (!id) continue;

		const char* rep = regs.get(id).get_str();
		memmove(p+strlen(rep)-1, q, strlen(q)+1);
		memcpy(p, rep, strlen(rep));
	}
	return rv;
}


bool Program::interpret_line(Scope* scope, std::vector<const Inst*>::iterator& it) {
	const Inst* t = *it;
	bool do_inc = true;

	if (t->is(Inst::GOTO)) {
		// TODO: unchecked atm (not in scope as we want to allow crossing scope boundaries - nasm will check inexistent labels)
		STDOUT("\tjmp .%s%s", ((InstGoto*)t)->id->val_s, t->suffix());
	} else if (t->is(Inst::LBL)) {
		STDOUT("\t.%s:%s", ((InstLbl*)t)->id->val_s, t->suffix());
	} else if (t->is(Inst::ASM)) {
		const char* str = replace_regs(scope, ((InstAsm*)t)->asmstr);
		STDOUT("\t%s%s", str, t->suffix());
	} else if (t->is(Inst::DECL)) {
		const Token* type = ((InstDecl*)t)->type;
		const Token* name = ((InstDecl*)t)->id;

		unsigned id = scope->push(name->val_s);
		if (!id) {
			LOG("'%s' already declared in this scope", name->val_s);
			return false;
		}

		const char* push = NULL;
		if (!regs.push(id, type->val_w, type->is(Token::TOK_TYPE_PTR), push)) {
			LOG("cannot declare '%s'", name->val_s);
			return false;
		}
		if (push) {
			STDOUT("\tpush %s%s", push, t->suffix());
			pushes.push_back(push);
		}

		STDOUT("\t; var '%s': %s (%d)%s", name->val_s, regs.get(id).get_str(), (int)type->val_w, t->suffix());
	} else if (t->is(Inst::STR)) {
		const Token* type = ((InstStr*)t)->type;
		const Token* name = ((InstStr*)t)->id;

		unsigned id = scope->push(name->val_s);
		if (!id) {
			LOG("'%s' already declared in this scope", name->val_s);
			return false;
		}
		if (!regs.push_lbl(id, type->val_w, true, name->val_s)) {
			LOG("cannot declare '%s'", name->val_s);
			return false;
		}

		STDOUT("\t; str '%s' (%d)%s", regs.get(id).get_str(), (int)type->val_w, t->suffix());
	} else if (t->is(Inst::EXT)) {
#if 1
		LOG("TODO:");
		return false;
#else
		const Token* name = ((InstExt*)t)->id;

		bool in_main_scope = false;
		unsigned id = scope->find(name->val_s, &in_main_scope);
		if (!id) {
			LOG("cannot find '%s'", name->val_s);
			return false;
		}
		if (!in_main_scope) {
			LOG("won't extend '%s' in sub-scope", name->val_s);
			return false;
		}
		if (regs.get(id).get().init_width == WIDTH_64) {
			STDOUT("\t;movzx %s omitted%s", regs.get(id).get_str(WIDTH_64), t->suffix());
		} else {
			// TODO: no 64bit immediate AND?
			switch (regs.get(id).get().width) {
				case WIDTH_8:
				case WIDTH_16:
					STDOUT("\tmovzx %s, %s%s", regs.get(id).get_str(WIDTH_64), regs.get(id).get_str(), t->suffix());
					break;
				case WIDTH_32:
					// In 64-bit mode, the default instruction size is still 32 bits. When loading a value into a 32-bit register (but not an 8- or 16-bit register), the upper 32 bits of the corresponding 64-bit register are set to zero.
					break;
				default:
					assert(false);
					break;
			}
			regs.inc_init(id, WIDTH_64);
		}
#endif
	} else if (t->is(Inst::UNDEF)) {
		const Token* name = ((InstUndef*)t)->id;
		unsigned id = scope->find(name->val_s);
		if (!id) {
			LOG("cannot unset '%s'", name->val_s);
			return false;
		}

		STDOUT("\t; del '%s': %s%s", name->val_s, regs.get(id).get_str(), t->suffix());
		regs.pop(id);
		scope->pop(id);
	} else if (t->is(Inst::LOOP)) {
		const Token* name = ((InstLoop*)t)->id;
		const Token* from = ((InstLoop*)t)->from;
		const Token* to = ((InstLoop*)t)->to;

		unsigned id = scope->push(name->val_s);
		if (!id) {
			LOG("'%s' already declared in this scope", name->val_s);
			return false;
		}
		width_t width = MAX(min_width(from->val_i), min_width(to->val_i));
		if (!regs.push_lbl(id, width, false, name->val_s)) {
			LOG("cannot declare '%s'", name->val_s);
			return false;
		}
		regs.inc_init(id, WIDTH_64);
		STDOUT("\t; rep '%s' (%d)%s", regs.get(id).get_str(), (int)regs.get(id).get().width, t->suffix());

		STDOUT("\t%%assign %s %" PRIu64 " ; to %" PRIu64, name->val_s, from->val_i, to->val_i);
		STDOUT("\t%%rep %" PRIu64 "%s", to->val_i - from->val_i + 1, t->suffix());

		++it;
		if (!interpret_block(new Scope(scope), it)) {
			return false;
		}
		do_inc = false;

		STDOUT("\t%%assign %s %s+1%s", name->val_s, name->val_s, t->suffix());
		STDOUT("\t%%endrep%s", t->suffix());
	} else if (t->is(Inst::OP)) {
		const Token* a = ((InstOp*)t)->a;
		const Token::op_t op = ((InstOp*)t)->op->val_op;
		const Token* b = ((InstOp*)t)->b;

		const bool is_mul = (op == Token::TOK_OP_MOD || op == Token::TOK_OP_DIV || op == Token::TOK_OP_MUL);
		const bool is_shift = (op == Token::TOK_OP_SHL || op == Token::TOK_OP_SHR);

		width_t src_width, dst_width;
		if (!op_width(scope, b, a, false, (op == Token::TOK_OP_ASSIGN), src_width, dst_width)) {
			return false;
		}

		char dst_buf[64];
		if (!val2str(scope, a, dst_width, false, true, !is_mul, is_mul? REG_A: REG_NONE, dst_buf)) {
			return false;
		}

		char src_buf[64];
		if (!val2str(scope, b, is_shift? WIDTH_8: src_width, !is_mul, true, !is_shift, is_shift? REG_C: REG_NONE, src_buf)) {
			return false;
		}

		if (a->is(Token::TOK_IDENTIFIER) && op == Token::TOK_OP_ASSIGN) {
			bool dst_in_main_scope;
			unsigned dst_id = scope->find(a->val_s, &dst_in_main_scope);
			assert(dst_id);
			if (dst_in_main_scope) {
				regs.inc_init(dst_id, dst_width); // so following/descendant ops can assume its inited this far
			}
		}

		switch (op) {
			case Token::TOK_OP_ASSIGN:
				// if (t[2]->tok == TOK_IMMEDIATE && t[2]->val_i == 0) STDOUT("\txor %s, %s", dst_buf, dst_buf);
				if (src_width == dst_width) {
					STDOUT("\tmov %s, %s%s", dst_buf, src_buf, t->suffix());
				} else {
					STDOUT("\tmovzx %s, %s%s", dst_buf, src_buf, t->suffix());
				}
				break;
			case Token::TOK_OP_ADD:
				if (b->is(Token::TOK_IMMEDIATE) && b->val_i == 1) {
					STDOUT("\tinc %s%s", dst_buf, t->suffix());
				} else {
					STDOUT("\tadd %s, %s%s", dst_buf, src_buf, t->suffix());
				}
				break;
			case Token::TOK_OP_SUB:
				if (b->is(Token::TOK_IMMEDIATE) && b->val_i == 1) {
					STDOUT("\tdec %s%s", dst_buf, t->suffix());
				} else {
					STDOUT("\tsub %s, %s%s", dst_buf, src_buf, t->suffix());
				}
				break;
			case Token::TOK_OP_SHL:
			case Token::TOK_OP_SHR:
				STDOUT("\tsh%c %s, %s%s", (op == Token::TOK_OP_SHL)? 'l': 'r', dst_buf, src_buf, t->suffix());
				break;
			case Token::TOK_OP_MUL:
				STDOUT("\tmul %s%s", src_buf, t->suffix()); // TODO: LEA or SHL for 1,2,4,8 immediates?
				break;
			case Token::TOK_OP_MOD:
			case Token::TOK_OP_DIV:
				// https://software.intel.com/sites/default/files/managed/39/c5/325462-sdm-vol-1-2abcd-3abcd.pdf 587/4684pp
				assert(src_width == dst_width);
				if (src_width != WIDTH_8) {
					STDOUT("\txor RDX, RDX%s", t->suffix());
				}
				STDOUT("\tdiv %s%s", src_buf, t->suffix());
				if (op == Token::TOK_OP_MOD) {
					if (src_width == WIDTH_8) {
						STDOUT("\tmov AL, AH%s", t->suffix());
					} else {
						STDOUT("\tmov RDX, RAX%s", t->suffix());
					}
				}
				break;
		}
	} else if (t->is(Inst::IF)) {
		const InstIf* i = (InstIf*)t;

		const unsigned lbl = it-lines.begin()+1; //uniq();
		if (i->repeat) {
			STDOUT("\t.if_%u:%s", lbl, t->suffix());
		}

		const char* if_jump;
		const char* else_jump;
		if (!i->cmp) {
			unsigned id = scope->find(i->a->val_s);
			if (!id) {
				LOG("cannot find var '%s'", i->a->val_s);
				return false;
			}
			char buf[64];
			if (!val2str(scope, i->a, regs.get(id).get().width, false, true, false, REG_NONE, buf)) { // XXX: can be mem only if other side is not, for test this is not possible
				return false;
			}

			STDOUT("\ttest %s, %s%s", buf, buf, t->suffix());
			if_jump = cmp_decl(Token::TOK_CMP_NE, i->negate);
			else_jump = cmp_decl(Token::TOK_CMP_NE, !i->negate);
		} else {
			// op_width allows src to be extended and dst to be pre-initialized
			width_t src_width, dst_width;
			if (!op_width(scope, i->b, i->a, true, false, src_width, dst_width)) {
				return false;
			}
			assert(src_width == dst_width);

			char a_buf[64];
			if (!val2str(scope, i->a, dst_width, false, true, true, REG_NONE, a_buf)) {
				return false;
			}

			char b_buf[64];
			if (!val2str(scope, i->b, src_width, true, true, true, REG_NONE, b_buf)) {
				return false;
			}

			STDOUT("\tcmp %s, %s%s", a_buf, b_buf, t->suffix());
			if_jump = cmp_decl(i->cmp->val_cmp, i->negate);
			else_jump = cmp_decl(i->cmp->val_cmp, !i->negate);
		}

		if (i->op) { // if only one command we might use cmov jump/goto or sth? CMOVcc, Jcc(, SETcc)
			if (i->repeat) {
				LOG("loops must have a body");
				return false;
			} else if (i->op->is(Inst::GOTO)) {
				STDOUT("\tj%s .%s%s", if_jump, ((InstGoto*)i->op)->id->val_s, t->suffix()); // pretty unchecked
			} else if (i->op->is(Inst::OP) && ((InstOp*)i->op)->op->val_op == Token::TOK_OP_ASSIGN) {
				InstOp* op = (InstOp*)i->op;

				width_t src_width, dst_width;
				if (!op_width(scope, op->b, op->a, false, false, src_width, dst_width)) {
					return false;
				}

				char dst_buf[64];
				if (!val2str(scope, op->a, dst_width, false, true, true, REG_NONE, dst_buf)) {
					return false;
				}

				char src_buf[64];
				if (!val2str(scope, op->b, src_width, true, true, true, REG_NONE, src_buf)) {
					return false;
				}

				STDOUT("\tcmov%s %s, %s%s", if_jump, dst_buf, src_buf, t->suffix());
			} else {
				LOG("malformed or unsupported single-block if");
				return false;
			}
		} else {
			STDOUT("\tj%s .else_%u%s", else_jump, lbl, t->suffix()); // TODO: must ensure its in the same segment? (farlabels w/ jmp)

			++it;
			if (!interpret_block(new Scope(scope), it)) {
				return false;
			}
			do_inc = false;

			if (i->repeat) {
				STDOUT("\tjmp .if_%u%s", lbl, t->suffix());
				STDOUT("\t.else_%u: ; endif%s", lbl, t->suffix());
			} else if (it != lines.end() && (*it)->is(Inst::ELSE)) { // TODO: allow single op here, too?
				STDOUT("\tjmp .endif_%u%s", lbl, t->suffix());
				STDOUT("\t.else_%u:%s", lbl, t->suffix());
				++it;
				if (!interpret_block(new Scope(scope), it)) {
					return false;
				}
				do_inc = false;
				STDOUT("\t.endif_%u:%s", lbl, t->suffix());
			} else {
				STDOUT("\t.else_%u: ; endif%s", lbl, t->suffix());
			}
		}
	} else {
		return false;
	}

	if (do_inc) ++it;
	return true;
}


bool Program::interpret_block(Scope* my_scope, std::vector<const Inst*>::iterator& it) {
	while (it != lines.end()) {
		if (!interpret_comment(it)) return false;
		if (it != lines.end()) {
			if ((*it)->is(Inst::END)) {
				++it;
				break;
			}
			if (!interpret_line(my_scope, it)) {
				LOG("cannot interpret line #%zu", it-lines.begin()+1);
				delete my_scope;
				return false;
			}
		}
	}

	unsigned id;
	while ((id = my_scope->pop()) != 0) {
		STDOUT("\t; eol %s", regs.get(id).get_str());
		regs.pop(id);
	}
	delete my_scope;

	return true;
}


void Program::push(Inst* i) {
	lines.push_back(i);
}


bool Program::interpret() {
	STDOUT("BITS 64");

	bool data_sect = false;
	for (std::vector<const Inst*>::const_iterator datait=lines.begin(); datait!=lines.end(); ++datait) {
		if ((*datait)->is(Inst::STR)) {
			if (!data_sect) {
				data_sect = true;
				STDOUT("");
				STDOUT("section .data");
			}
			const InstStr* i = (InstStr*)*datait;
			STDOUT("\t%s: d%c %s%s", i->id->val_s, width_decl(i->type->val_w)[0], i->str, i->suffix());
		}
	}
	if (data_sect) STDOUT("");

	std::vector<const Inst*>::iterator it = lines.begin();
	Scope* scope = new Scope(); // TODO: cleanup

	// TODO: over-engineer logic for which registers must be clean by a struct with handlers
	if (peek(Token::TOK_OP_DIV, it) || peek(Token::TOK_OP_MOD, it) || peek(Token::TOK_OP_MUL, it)) {
		if (!regs.push(scope->push(), WIDTH_64, REG_D, false)) return false;
	}

	if (!interpret_comment(it)) return false;
	if (!(*it)->is(Inst::FUNC)) return false;
	const InstFunc* inst = (const InstFunc*)*it;

	STDOUT_R("; extern %s %s(", Reg::get_c_str(inst->decl->type->val_w, inst->decl->type->is(Token::TOK_TYPE_PTR)), inst->decl->id->val_s);
	for (std::vector<const InstDecl*>::const_iterator ait = inst->args.begin(); ait != inst->args.end(); ++ait) {
		const InstDecl* decl = *ait;
		STDOUT_R("%s%s", (ait == inst->args.begin())? "": ", ", Reg::get_c_str(decl->type->val_w, decl->type->is(Token::TOK_TYPE_PTR)));
	}
	STDOUT(");");

	STDOUT("section .text");
	STDOUT("global %s", inst->decl->id->val_s);
	STDOUT("%s:%s", inst->decl->id->val_s, inst->suffix());

	unsigned rv = scope->push("rv");
	if (!rv || !regs.push(rv, inst->decl->type->val_w, REG_A, inst->decl->type->is(Token::TOK_TYPE_PTR))) return false;
	STDOUT("\t; ret 'rv': %s (%d)", regs.get(rv).get_str(), (int)regs.get(rv).get().width);

	static const reg_id_t regargs[6] = { REG_DI, REG_SI, REG_D, REG_C, REG_8, REG_9 };
	if (inst->args.size() > 6) return false;
	unsigned argnum = 0;
	for (std::vector<const InstDecl*>::const_iterator ait = inst->args.begin(); ait != inst->args.end(); ++ait) {
		const InstDecl* decl = *ait;

		unsigned id = scope->push(decl->id->val_s);
		if (!id) return false;

		reg_id_t r = regargs[argnum++];
		if (!regs.push(id, decl->type->val_w, r, decl->type->is(Token::TOK_TYPE_PTR))) {
			if (!regs.push(id, decl->type->val_w, decl->type->is(Token::TOK_TYPE_PTR))) return false;
			STDOUT("\tmov %s, %s", regs.get(id).get_str(WIDTH_64), regs.get(r).get_str(WIDTH_64));
			r = regs.get(id).info().reg_id;
		}

		STDOUT("\t; arg '%s': %s (%s%d)", decl->id->val_s, regs.get(id).get_str(), regs.get(id).get().is_ptr? "*": "", (int)regs.get(id).get().width);
	}

	++it;
	assert(pushes.empty());
	if (!interpret_block(scope, it)) return false; // clears scope
	while (pushes.size()) {
		STDOUT("\tpop %s", pushes.back());
		pushes.pop_back();
	}

	STDOUT("");
	STDOUT("\tret");
	return true;
}
