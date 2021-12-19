#pragma once

#include <list>
#include <unordered_map>
#include "bitvec.h"

class Ins;

class Program {
private:
	bitvec code;
	std::unordered_map<size_t, std::list<Ins> > cache;
public:
	std::unordered_map<size_t, size_t> fences;
	bitvec global;
	Program(uint8_t *, size_t, uint8_t *, size_t);
	std::list<Ins> operator[](size_t);
};

class Context {
private:
	bitvec stack;
public:
	size_t ip;
	size_t sleepcnt;
	std::list<Context> step(Program &);
};