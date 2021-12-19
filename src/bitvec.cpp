#include <iterator>
#include <memory>
#include <cassert>
#include <numeric>
//#include <execution>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>

#include "bitvec.h"
using std::uint16_t;

bitvec::bitvec()
	: data()
	, next()
	, len(0)
	, offset(0)
	{}

bitvec::bitvec(uint64_t val, size_t bits)
	: data()
	, next()
	, len(bits & 0x3f)
	, offset(0) {
	for (unsigned i = 0 ; i < bits; i += 8) {
		data.push_back((val >> i) & 0xff);
	}
}

bitvec::bitvec(const uint8_t *d, size_t l)
	: data(d, d+l)
	, next()
	, len(l << 3)
	, offset(0){}

bitvec::bitvec(const bitvec &b){
	b.contract();
	// std::cout << "bitvec copy constructor" << std::endl;
	std::copy(b.data.begin(), b.data.end(), std::back_inserter(this->data));
	this->len = b.len;
	this->offset = b.offset;
	assert(!b.next);
}

bitvec& bitvec::append(const bitvec &ot) {
	if (this->next) {
		this->next->append(ot);
	} else {
		this->next = std::unique_ptr<bitvec>(new bitvec{ot});
	}
	check();
	return *this;
}

bitvec& bitvec::append(bitvec &&ot) {
	if (this->next) {
		this->next->append(std::move(ot));
	} else {
		this->next = std::unique_ptr<bitvec>(new bitvec(std::move(ot)));
	}
	check();
	return *this;
}

size_t bitvec::length() const {
	if (this->next) {
		return this->len + this->next->length();
	} else {
		return this->len;
	}
}

bitvec& bitvec::operator+=(const bitvec &ot) {
	return this->append(ot);
}

bitvec bitvec::operator+(const bitvec& rhs) {
	bitvec ot(*this);
	ot.append(rhs);
	return ot;
}

bitvec bitvec::take(size_t num) const& {
	bitvec ot;

	if (num == 0) {
		return bitvec();
	}

	if (this->len >= num) {
		size_t bc = (num + offset + 7) >> 3;
		std::copy(this->data.cbegin(), this->data.cbegin() + bc, std::back_inserter(ot.data));
		ot.len = num;
		ot.offset = this->offset;
	} else {
		std::copy(this->data.cbegin(), this->data.cend(), std::back_inserter(ot.data));
		ot.len = this->len;
		ot.offset = this->offset;
		if (this->next) {
			ot.next = std::unique_ptr<bitvec>(new bitvec(std::move(this->next->take(num - this->len))));
		}
	}
	ot.check();
	return ot;
}

bitvec bitvec::take(size_t num) && {
	if (num == 0) {
		return bitvec();
	}

	bitvec *cur = this;
	while (cur && cur->len < num) {
		num -= cur->len;
		cur = cur->next.get();
	}

	if (cur) {
		cur->next.reset(nullptr);
		cur->len = num;
		
	}

	check();
	return std::move(*this);
}

void bitvec::check(void) const {
	if (data.size() < ((offset + len + 7) >> 3)) {
		throw std::runtime_error("more bits in vector than memory allocated");
	}
}

bitvec bitvec::drop(size_t num) const& {
	if (this->len <= num) {
		if (this->next) {
			return this->next->drop(num - this->len);
		} else {
			return bitvec();
		}
	} else {
		bitvec ot(*this);
		ot.offset += num;
		ot.len -= num;
		ot.check();
		return ot;
	}
}

bitvec bitvec::drop(size_t num) && {
	if (this->len > num) {
		this->offset += num;
		this->len -= num;
		check();
		return std::move(*this);
	} else {
		bitvec *cur = this;
		num -= this->len;
		while (cur->next && cur->next->len <= num) {
			num -= cur->next->len;
			bitvec *cur = cur->next.get();
		}
		cur = cur->next.release();
		if (cur) {
			cur->offset += num;
			cur->len -= num;
			cur->check();
			return std::move(*cur);
		} else {
			return bitvec();
		}
	}
}

bitvec bitvec::split_take_inplace(size_t num) {
	if (num == 0) {
		bitvec ot;
		swap(ot);
		return ot;
	}
	if (this->len > num) {
		auto begin = this->data.cbegin() + ((this->offset + num) >> 3);
		bitvec ot;
		std::copy(begin, this->data.cend(), std::back_inserter(ot.data));
		ot.len = len - num;
		len = num;
		ot.offset = (this->offset + num) & 7;
		ot.next.swap(this->next);
		ot.check();
		check();
		return ot;
	} else if (this->len < num) {
		if (this->next) {
			return this->next->split_take_inplace(num - this->len);
		} else {
			return bitvec();
		}
	} else {
		if (this->next) {
			return std::move(*next.release());
		} else {
			return bitvec();
		}
	}
}

bitvec bitvec::split_drop_inplace(size_t num) {
	if (num == 0) {
		return bitvec();
	}
	bitvec ot = split_take_inplace(num);
	this->swap(ot);
	return ot;
}

void bitvec::prepend_inplace(bitvec &&ot) {
	swap(ot);
	append(std::move(ot));
}

void bitvec::swap(bitvec &ot) {
	this->data.swap(ot.data);
	size_t tmp = this->offset;
	this->offset = ot.offset;
	ot.offset = tmp;
	tmp = this->len;
	this->len = ot.len;
	ot.len = tmp;
	this->next.swap(ot.next);
	check();
	ot.check();
}


#define   init_2(v)   bitrev(v),   bitrev(v+  1)
#define   init_4(v)   init_2(v),   init_2(v+  2)
#define   init_8(v)   init_4(v),   init_4(v+  4)
#define  init_16(v)   init_8(v),   init_8(v+  8)
#define  init_32(v)  init_16(v),  init_16(v+ 16)
#define  init_64(v)  init_32(v),  init_32(v+ 32)
#define init_128(v)  init_64(v),  init_64(v+ 64)
#define init_256(v) init_128(v), init_128(v+128)

constexpr uint8_t bitrev(uint8_t b) {
	return  ((b & 0x80) >> 7) |
			((b & 0x40) >> 5) |
			((b & 0x20) >> 3) |
			((b & 0x10) >> 1) |
			((b & 0x08) << 1) |
			((b & 0x04) << 3) |
			((b & 0x02) << 5) |
			((b & 0x01) << 7);
}

const uint8_t revlut[0x100] = {init_256(0)};

bitvec bitvec::reverse(void) const& {
	this->contract();
	bitvec ot;
	std::copy(this->data.rbegin(), this->data.rend(), std::back_inserter(ot.data));
	std::transform  (/*std::execution::par_unseq
					, */ot.data.begin()
					, ot.data.end()
					, ot.data.begin()
					, [](uint8_t v) -> uint8_t { return revlut[v]; });
	ot.len = this->len;
	ot.offset = (8 - ((ot.offset + ot.len) & 7)) & 7;
	ot.check();
	return ot;
}

bitvec bitvec::reverse(void) && {
	this->contract();
	bitvec ot;
	std::reverse(this->data.begin(), this->data.end());
	std::transform  (/*std::execution::par_unseq
					, */this->data.begin()
					, this->data.end()
					, this->data.begin()
					, [](uint8_t v) -> uint8_t { return revlut[v]; });
	this->data.swap(ot.data);
	ot.len = this->len;
	ot.offset = (8 - ((ot.offset + ot.len) & 7)) & 7;
	ot.check();
	return ot;
}

uint8_t masks[9] = {0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

void bitvec::contract(void) const {
	uint16_t acc = 0;
	uint8_t acc_bits = 0;
	size_t totlen = 0;

	check();
	std::vector<uint8_t> newdat;
	bitvec *thmod = const_cast<bitvec*>(this);
	bitvec *cur = thmod;

	do {
		if (cur->len == 0) {
			// nothing to do, nothing to copy
		} else if (cur->len <= 8) {
			// uses one or two uint8_ts
			auto begin = cur->data.cbegin() + (cur->offset >> 3);
			uint16_t tmp = *begin >> (cur->offset & 7);
			uint8_t cb = 8 - (cur->offset & 7);
			if (cb < cur->len) {
				tmp |= *std::next(begin) << cb;
			}
			acc |= tmp << acc_bits;
			acc_bits += cur->len;
		} else {
			// uses at least two uint8_ts
			// fuse the first one with acc
			// points to the first element containing valid bits
			auto begin = cur->data.cbegin() + (cur->offset >> 3);
			// points past the last element containing valid bits
			auto end   = cur->data.cbegin() + ((cur->offset + cur->len + 7) >> 3);
			acc = ((*begin >> (cur->offset & 7)) << acc_bits) | acc;
			acc_bits += (8 - (cur->offset & 7));
			if (acc_bits >= 8) {
				newdat.push_back((uint8_t) acc);
				acc >>= 8;
				acc_bits -= 8;
			}

			// loop over the middle ones
			std::transform  ( std::next(begin)
							, std::prev(end)
							, std::back_inserter(newdat)
							, [&acc, acc_bits] (uint8_t c) -> uint8_t
				{
					uint16_t tmp = (((uint16_t) c) << acc_bits) | acc;
					acc = tmp >> 8;
					return (uint8_t) tmp;
				});

			// store the overflow in acc
			acc |= *std::prev(end) << acc_bits;

			// prev(end) will always hold atleast one bit
			/*
			uint8_t tmp = (cur->offset + cur->len) & 7;
			acc_bits += tmp ? tmp : 8;
			*/
			acc_bits += ((cur->offset + cur->len - 1) & 7) + 1;
		}

		if (acc_bits >= 8) {
			newdat.push_back((uint8_t) acc);
			acc >>= 8;
			acc_bits -= 8;
		}

		// there may be bits set in the last bytes we need to clear.
		acc &= masks[acc_bits];

		totlen += cur->len;
		bitvec *nxt = cur->next.release();
		if (cur != thmod) {
			delete cur;
		}
		cur = nxt;
	} while(cur);

	if (acc_bits) {
		newdat.push_back((uint8_t) acc);
	}

	thmod->data.swap(newdat);
	thmod->offset = 0;
	thmod->len = totlen;
}

/*
void bitvec::contract(void) const {
	uint8_t val = 0;
	uint8_t bits = 0;
	size_t totlen = 0;
	check();
	std::vector<uint8_t> newdat;
	bitvec *thmod = const_cast<bitvec*>(this);
	bitvec *cur = thmod;
	do {
		uint16_t last = val;
		uint8_t lastbits = bits;
		auto begin = cur->data.cbegin() + (cur->offset >> 3);
		cur->offset = cur->offset & 7;
		if (cur->len == 0) {
			// the data vector is potentially empty
			// so begin may not be dereferencable
		} else if (cur->len <= 8) {
			uint16_t tmp = *begin >> cur->offset;
			uint8_t cb = 8 - cur->offset;
			if (cb < cur->len) {
				tmp |= *(std::next(begin)) << cb;
			}
			last = (tmp << bits) | val;
			lastbits = bits + cur->len;
		} else {
			uint16_t tmp = ((*begin >> cur->offset) << bits) | val;
			uint8_t cb = bits + 8 - cur->offset;
			auto end = begin + ((cur->offset + cur->len + 7) >> 3);
			if (cb >= 8) {
				newdat.push_back(tmp & 0xff);
				cb -= 8;
				val = tmp >> 8;
			} else {
				val = tmp;
			}
			std::transform	( std::next(begin)
							, std::prev(end)
							, std::back_inserter(newdat)
							, [&val,cb](uint8_t c) -> uint8_t {
				uint16_t tmp = (((uint16_t) c) << cb) | val;
				val = tmp >> 8;
				return tmp & 0xff;
			});
			last = ((uint16_t) (*std::prev(cur->data.cend())) << cb) | val;
			*/
			/* add number of bits spilled into the last byte */
			/*lastbits = cb;

			cb = (cur->len + cur->offset) & 7;
			if (cb) {
				lastbits += cb;
			} else {
				lastbits += 8;
			}

			//lastbits = cb + ((cur->len + cur->offset) & 7);
			//lastbits = cb + (8 - ((cur->len + cur->offset) & 7));
		}

		if (lastbits >= 8) {
			newdat.push_back(last & 0xff);
			bits = lastbits - 8;
			val = (last >> 8) & masks[bits];
		} else {
			bits = lastbits;
			val = last & masks[bits];
		}

		totlen += cur->len;
		bitvec *nxt = cur->next.release();
		if (cur != thmod) {
			delete cur;
		}
		cur = nxt;
	} while (cur);
	if (bits) {
		newdat.push_back(val);
	}
	thmod->data.swap(newdat);
	thmod->offset = 0;
	thmod->len = totlen;


	check();
	*/
	/*
	if (data.size() * 8 < len) {
		std::cerr << "something went wrong during contraction" << std::endl;
		throw std::runtime_error("something went wrong during contraction");
	}*/
	/* next has already been released */
/*
}
*/

uint64_t bitvec::as_u64(void) const {
	contract();
	if (len > 64) {
		throw std::runtime_error("not convertable with 64 bits");
	}
	uint64_t val = 0;
	unsigned idx = 0;
	for (uint64_t v : data) {
		val |= ((uint64_t) v) << (8 * idx++);
	}
	if (len < 64) {
		val &= (1ul << len) - 1;
	}
	return val;
}

char *bitvec::c_str(void) const {
	contract();
	char *ret = new char[data.size() + 1];
	std::copy(data.begin(), data.end(), ret);
	ret[data.size()] = 0;
	return ret;
}

std::ostream& operator<<(std::ostream &os, const bitvec &bv) {
	bv.contract();
	os << std::dec << "(" << bv.len << ") ";
	os << std::hex << std::setfill('0') ;
	for (uint8_t d : bv.data) {
		os << std::setw(2) << (uint16_t) d;
	}
	return os;
}