#include <stdexcept>
#include <iostream>
#include <string>
#include <typeinfo>
#include "ins.h"

const unsigned argnum[] = {2,2,2,2,1,1,1,1,2,1,2,2,1,1,1,1};

static_assert(sizeof(argnum) == 16 * sizeof(argnum[0]));

Argument::Argument(size_t width)
	: width(width) {}

ArgImm::ArgImm(uint64_t val, size_t sz)
	: Argument(sz)
	, val(val) 
{
	if (sz == 0 || sz > 64) {
		throw std::runtime_error("constructing arg_imm with invalid width");
	}
}

Argument *ArgImm::clone(void) {
	return new ArgImm(val, width);
}

uint64_t ArgImm::get_val(bitvec &st) {
	(void) st;
	return val;
}

uint64_t ArgStk::get_val(bitvec &st) {
	if (st.length() < width) {
		throw std::runtime_error("stack too small to pop arguments");
	}
	return st.split_drop_inplace(width).as_u64();
}

ArgStk::ArgStk(size_t s) 
	: Argument(s) 
{
	if (s == 0 || s > 64) {
		throw std::runtime_error("constructing ArgStk with invalid width");
	}
}

Argument *ArgStk::clone(void) {
	return new ArgStk(width);
}

Ins::Ins(const bitvec& bv) 
	: width(bv.length())
	{
	size_t bl = bv.length();
	// std::cout << "constructing ins (length " << bl << ")" << std::endl;
	if (width < 12 || width > 76) {
		throw std::runtime_error("unable to reconstruct instruction from bitvector: invalid size");
	}
	uint64_t info = bv.drop(width - 12).as_u64();
	nm = static_cast<NEMONIC>(info >> 8);
	bool is_be = (info & 0x40) == 0;
	bool is_imm = (info & 0x80) == 0;
	size_t il = (info & 0x3f) + 1;

	// std::cout << "info:    " << info << std::endl;
	// std::cout << "ins len: " << il << std::endl;
	if (!is_imm && (nm == IMM || nm == FNC)) {
		throw std::runtime_error("inappropriate combination of mnemonic and operand type");
	}
	if (!is_imm && bl != 12) {
		throw std::runtime_error("inappropriate combination of operand type and immediate data");
	}
	if (is_imm && bl != il + 12) {
		throw std::runtime_error("instruction and immediate size mismatch");
	}

	if (is_imm) {
		uint64_t val;
		if (is_be) {
			val = bv.take(bl - 12).reverse().as_u64();
		} else {
			val = bv.take(bl - 12).as_u64();
		}
		// std::cout << "immediate: " << val << std::endl;
		arg1 = std::make_unique<ArgImm>(val, il);
	} else {
		arg1 = std::make_unique<ArgStk>(il);
	}

	if (argnum[nm] == 2) {
		arg2 = std::make_unique<ArgStk>(il);
	}
	// std::cout << "construction finished" << std::endl;
}

Ins::Ins(const Ins& ot)
	: nm(ot.nm)
	, arg1(std::unique_ptr<Argument>(ot.arg1->clone()))
	, width(ot.width)
{
	if (ot.arg2) {
		arg2 = std::unique_ptr<Argument>(ot.arg2->clone());
	}
}

uint64_t Ins::get_arg1(bitvec &bv) {
	return arg1->get_val(bv);
}

uint64_t Ins::get_arg2(bitvec &bv) {
	if (arg2) {
		return arg2->get_val(bv);
	} else {
		throw std::runtime_error("trying to fetch second argument form instruction that does not have one");
	}
}

Ins::NEMONIC Ins::get_nemonic(void) {
	return nm;
}

Ins::exec_res Ins::exec(size_t ip, bitvec &glob, bitvec &stack) {
	bitvec prestack(stack);
	uint64_t x = arg1->get_val(stack);
	uint64_t y = 0;
	exec_res result = {ip + width, 0, false};
	__uint128_t tmp;
	bitvec btmp;
	std::string line;
	char *outstr;

	if (arg2) {
		y = arg2->get_val(stack);
	}

	switch (nm) {
	case ADD:
		stack.prepend_inplace(bitvec(x + y, arg1->width));
		break;
	case SUB:
		stack.prepend_inplace(bitvec(y - x, arg1->width));
		break;
	case MUL:
		if (arg1->width <= 32) {
			stack.prepend_inplace(bitvec(x * y, 2 * arg1->width));
		} else {
			tmp = ((__uint128_t) x) * y;
			stack.prepend_inplace(std::move(bitvec((uint64_t) tmp, 64)
					.append(std::move(bitvec((uint64_t)(tmp >> 64), 2 * arg1->width - 64)))));
		}
		break;
	case DIV:
		if (x == 0) {
			throw std::runtime_error("division by zero");
		}
		stack.prepend_inplace(std::move(bitvec(y / x, arg1->width).append(std::move(bitvec(y % x, arg1->width)))));
		break;
	case IMM:
		stack.prepend_inplace(bitvec(x, arg1->width));
		break;
	case POP:
		if (stack.split_drop_inplace(x).length() != x) {
			throw std::runtime_error("not enough space on the stack to pop");
		}
		break;
	case DUP:
		btmp = stack.take(x);
		if (btmp.length() != x) {
			throw std::runtime_error("out of bounds: duplicating past end of stack");
		}
		stack.prepend_inplace(std::move(btmp));
		break;
	case LDR:
		btmp = stack.drop(x).take(arg1->width);
		if (btmp.length() != arg1->width) {
			throw std::runtime_error("out of bounds: reading past end of stack");
		}
		stack.prepend_inplace(std::move(btmp));
		break;
	case STR:
		btmp = stack.split_drop_inplace(x + arg1->width);
		if (btmp.length() != x + arg1->width) {
			throw std::runtime_error("out of bounds: writing past end of stack");
		}
		stack.prepend_inplace(std::move(btmp.take(x).append(bitvec(y, arg1->width))));
		//stack.append(bitvec(y, arg1->width).append(std::move(btmp).drop(arg1->width)));
		break;
	case LDG:
		//std::cout << "LDG " << x << std::endl;
		//std::cout << "len glob: " << glob.length() << std::endl;
		btmp = glob.drop(x).take(arg1->width);
		//std::cout << btmp << std::endl;
		if (btmp.length() != arg1->width) {
			throw std::runtime_error("out of bounds: reading past end of global data");
		}
		stack.prepend_inplace(std::move(btmp));
		break;
	case STG:
		btmp = glob.take(x + arg1->width);
		if (btmp.length() != x + arg1->width) {
			throw std::runtime_error("out of bounds: writing past end of global data");
		}
		btmp = std::move(btmp.take(x).append(bitvec(y, arg1->width)).append(glob.drop(x + arg1->width)));
		glob.swap(btmp);
		//glob.prepend_inplace(std::move(btmp.take(x).append(bitvec(y, arg1->width))));
		//glob.append(bitvec(y, arg1->width).append(std::move(btmp).drop(arg1->width)));
		break;
	case BEQ:
		if (y == 0) {
			result.newip += x;
		}
		break;
	case JMP:
		result.newip = x;
		break;
	case SVC:
		switch (x) {
		case 0:
			//std::cout << "SVC 0" << std::endl;
			btmp = stack.split_drop_inplace(arg1->width);
			if (btmp.length() < arg1->width) {
				throw std::runtime_error("out of bounds: reading past end of stack");
			}
			y = btmp.as_u64();
			btmp = stack.split_drop_inplace(y);
			if (btmp.length() < y) {
				throw std::runtime_error("out of bounds: reading past end of stack");
			}
			outstr = btmp.c_str();
			std::cout << outstr << std::flush;
			delete[] outstr;
			break;
		case 1:
			//std::cout << "SVC 1" << std::endl;
			std::getline(std::cin, line);
			if (line.size() > 42) {
				line.resize(42);
			}
			stack.prepend_inplace(bitvec((const uint8_t*)line.c_str(), line.size()));
			break;
		case 2:
			//std::cout << "SVC 2" << std::endl;
			btmp = stack.split_drop_inplace(arg1->width);
			if (btmp.length() < arg1->width) {
				throw std::runtime_error("out of bounds: reading past end of stack");
			}
			std::cout << btmp.as_u64();
			break;
		case 3:
			//std::cout << "SVC 3" << std::endl;
			std::cin >> y;
			stack.prepend_inplace(bitvec(y, arg1->width));
			break;
		default:
			throw std::runtime_error("undefined SVC number");
		}
		break;
	case SKP:
		result.skip = x;
		break;
	case FNC:
		result.fence = x;
		break;
	default:
		throw std::runtime_error("instruction not part of ISA");
	}

	
	//std::cout << "................................" << std::endl;
	//std::cout << prestack.reverse() << std::endl;
	//std::cout << ip << ":" << *this << " (" << std::hex << x;
	//if (arg2) {
	//	std::cout << ", " << y;
	//}
	//std::cout << ") -> " << result.newip << std::endl;
	//std::cout << stack.reverse() << std::endl;
	//std::cout << "********************************" << std::endl;
	
	return result;
}

//char mnemonics[][16] = {
//	"ADD\0",
//	"SUB\0",
//	"MUL\0",
//	"DIV\0",
//	"IMM\0",
//	"POP\0",
//	"DUP\0",
//	"LDR\0",
//	"STR\0",
//	"LDG\0",
//	"\x1b[31;1mSTG\x1b[0m",
//	"BEQ\0",
//	"\033[31;1mJMP\033[0m",
//	"SVC\0",
//	"SKP\0",
//	"FNC\0"
//};

//std::ostream& operator<<(std::ostream &os, const Ins &i) {
//	if (i.nm <= Ins::NEMONIC::FNC) {
//		os << mnemonics[i.nm];
//		if (typeid(*i.arg1) == typeid(ArgImm)) {
//			os << "D";
//		}
//		os << " " << i.arg1->width;
//		if (typeid(*i.arg1) == typeid(ArgImm)) {
//			bitvec b;
//			os << " " << i.arg1->get_val(b);
//		}
//	}
//	return os;
//}
