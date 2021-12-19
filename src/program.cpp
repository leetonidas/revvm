#include <stdexcept>

#include "program.h"
#include "ins.h"

Program::Program(uint8_t *data, size_t dlen, uint8_t *code, size_t clen)
	: code(code, clen)
	, global(data, dlen)
{}

std::list<Ins> Program::operator[](size_t pos) {
	auto entry = cache.find(pos);
	if (entry != cache.end()) {
		return entry->second;
	}

	std::list<Ins> res;
	bitvec dat = code.drop(pos).take(76);
	size_t sl = dat.length();
	for (size_t cur = 12; cur <= sl; ++cur) {
		try {
			res.emplace_back(dat.take(cur));
		} catch (const std::runtime_error& e) {
			/* apperantly not a decodeable instruction */
		}
	}
	cache.insert({pos, res});
	return res;
}

std::list<Context> Context::step(Program &p) {
	std::list<Ins> cis = p[ip];
	std::list<Context> ret;

	if (sleepcnt) {
		throw std::runtime_error("sleepwalking is not allowed");
	}

	// std::cout << "----- " << std::dec << ip << " -----" << std::endl;
	for (auto i : cis) {
		try {
			Context nc(*this);
			Ins::exec_res res = i.exec(ip, p.global, nc.stack);
			// std::cout << std::dec << ip << ": " << i << std::endl;
			if (res.fence > 1) {
				size_t fc = p.fences[res.newip] + 1;
				if (fc > res.fence) {
					// other option would be to delete the entry
					// but since this is an unordered map it should
					// not make a difference for other keys and we
					// do not need to create a new entry next time
					p.fences[res.newip] = 0;
				} else {
					p.fences[res.newip] = fc;
					continue;
				}
			}
			sleepcnt = res.skip;
			nc.ip = res.newip;
			ret.push_back(std::move(nc));
		} catch (std::runtime_error &e) {
			/* error during execution -> thread terminates */
		}
	}
	return ret;
}