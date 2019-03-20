#pragma once
#include "common.hpp"
#include <vector>


typedef enum {
	REG_A, REG_B, REG_C, REG_D, //used by idiv so will be our tmp-register
	REG_SI, REG_DI, REG_BP, REG_SP, //won't use SP as well
	REG_8, REG_9, REG_10, REG_11, REG_12, REG_13, REG_14, REG_15,
	REG_NONE=REG_15+1
} reg_id_t;


class Reg {
	public:
		typedef struct {
			reg_id_t reg_id;
			const char* names[4]; // 8, 16, 32, 64
			bool needs_push;
			bool auto_assign; // must be explicitly requested
		} reg_info_t;

		typedef struct {
			unsigned var_id; // whether its used
			bool is_ptr;
			bool pushed;
			width_t width; // actual declared size
			width_t init_width; // zero-initialized size (>= width, as for this the user should be responsible)
		} reg_val_t;

	private:
		const reg_info_t reg_info;
		reg_val_t val;

	public:
		Reg(const reg_info_t);

		const char* get_str(width_t) const;
		const char* get_str() const;
		static const char* get_c_str(width_t, bool);

		bool used() const;
		const reg_val_t& get() const;
		const reg_info_t& info() const;

		void set();
		void set(unsigned, width_t, bool, const char*&);
		void set(width_t);
};


class Regs {
	private:
		Reg regs[REG_NONE];
		std::vector<Reg*> bufs;

	public:
		Regs();
		~Regs();

		bool push(unsigned, width_t, bool);
		bool push(unsigned, width_t, bool, const char*&);
		bool push(unsigned, width_t, reg_id_t, bool); // for rv and args
		bool push(unsigned, width_t, reg_id_t, bool, const char*&);
		bool push_lbl(unsigned, width_t, bool, const char*);

		void pop(unsigned);

		const Reg& get(reg_id_t) const;
		const Reg& get(unsigned) const;

		void inc_init(unsigned, width_t);
};
