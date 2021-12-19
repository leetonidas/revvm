#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <iterator>
#include <iostream>

using std::uint8_t;
using std::uint64_t;
using std::size_t;

class bitvec {
private:
	std::vector<uint8_t> data;
	std::unique_ptr<bitvec> next;
	size_t len;
	size_t offset;
public:
	bitvec(void);
	bitvec(bitvec&&) = default;
	bitvec(uint64_t, size_t);
	bitvec(const uint8_t *, size_t);
	bitvec(const bitvec&);
	bitvec& operator=(const bitvec&) = delete;
	bitvec& operator=(bitvec&&) = default;
	static bitvec from_int(uint64_t, size_t);
	static bitvec from_bytes(const uint8_t *data, size_t len);
	bitvec& append(const bitvec&);
	bitvec& append(bitvec&&);
	size_t length(void) const;
	bitvec& operator+=(const bitvec&);
	bitvec operator+(const bitvec&);
	bitvec take(size_t) const&;
	bitvec take(size_t) &&;
	bitvec drop(size_t) const&;
	bitvec drop(size_t) &&;
	bitvec reverse(void) const&;
	bitvec reverse(void) &&;
	bitvec split_take_inplace(size_t);
	bitvec split_drop_inplace(size_t);
	char* c_str(void) const;
	void prepend_inplace(bitvec &&);
	void swap(bitvec &);
	uint64_t as_u64(void) const;
private:
	void check(void) const;
	void contract(void) const;
	friend std::ostream& operator<<(std::ostream &, const bitvec&);
};
