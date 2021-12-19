#include <iostream>
#include <fstream>
#include <string>

#include "program.h"
#include "bitvec.h"
#include "ins.h"

int main(int argc, char *argv[]) {
	/*
	const uint8_t buf[4] = {0x31, 0x33, 0x33, 0x37};
	bitvec b = bitvec(buf, 4).drop(4).reverse();
	b.append(bitvec(27, 6));
	b.append(bitvec(1ul, 2));
	b.append(bitvec(0ul, 4));

	std::cout << b.length() << std::endl;
	std::cout << b.reverse() << std::endl;

	Ins i(b);
	uint64_t val = i.get_arg1(b);
	std::cout << "arg1: " << std::hex << val << std::endl;
	*/
	if (argc == 2) {
		std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
		if (!input) {
			std::cout << "unable to open file \"" << argv[1] << "\" for reading" << std::endl;
			return 1;
		}
		size_t fs = input.tellg();
		if (fs <= 8) {
			std::cout << "not a valid program" << std::endl;
			return 1;
		}
		input.seekg(0, std::ios::beg);
		uint8_t *buf = new uint8_t[fs];
		input.read((char*) buf, fs);
		uint64_t pstart = *reinterpret_cast<uint64_t*>(buf);
		Program p(buf + 8, pstart, buf + pstart + 8, fs - pstart - 8);
		delete[] buf;

		std::list<Context> ctxs;
		std::string inp;
		ctxs.push_back(Context());

		while (!ctxs.empty()) {
			Context cur = std::move(ctxs.front());
			ctxs.pop_front();
			if (cur.sleepcnt) {
				--cur.sleepcnt;
				ctxs.push_back(std::move(cur));
				continue;
			}
			//if (cur.ip == 0) {
			//	std::cout << "step? ";
			//	std::getline(std::cin, inp);
			//	if (inp.size() && (inp[0] == 'n' || inp[0] != 'N')) {
			//	 	break;
			//	}
			//}
			ctxs.splice(ctxs.cend(), cur.step(p));
		}
	}
	return 0;
}