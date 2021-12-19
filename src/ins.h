#pragma once
#include <cstdint>
#include <memory>
#include <iostream>
#include "bitvec.h"

using std::uint8_t;
using std::size_t;

class Argument {
public:
	const size_t width;
	Argument(size_t);
	virtual uint64_t get_val(bitvec &) = 0;
	virtual Argument *clone(void) = 0;
};

class ArgImm : public Argument {
private:
	uint64_t val;
public:
	ArgImm(uint64_t, size_t);
	uint64_t get_val(bitvec &);
	Argument *clone(void);
};

class ArgStk : public Argument {
public:
	ArgStk(size_t);
	uint64_t get_val(bitvec &);
	Argument *clone(void);
};

class Ins {
private:
	enum NEMONIC : uint8_t {
		ADD = 0,
		SUB,
		MUL,
		DIV,
		IMM,
		POP,
		DUP,
		LDR,
		STR,
		LDG,
		STG,
		BEQ,
		JMP,
		SVC,
		SKP,
		FNC
	} nm;
	std::unique_ptr<Argument> arg1;
	std::unique_ptr<Argument> arg2;
	//friend std::ostream& operator<<(std::ostream &, const Ins &);
public:
	struct exec_res {
		size_t newip;
		size_t skip;
		size_t fence;
	};
	const size_t width;
	Ins(const bitvec&);
	Ins(const Ins&);
	NEMONIC get_nemonic(void);
	uint64_t get_arg1(bitvec &);
	uint64_t get_arg2(bitvec &);
	exec_res exec(size_t, bitvec &, bitvec &);
};
