#include "pqRand.hpp"
#include <fstream>
#include <iostream>

using namespace pqr;

int main()
{
	uPRNG_64_Seeder<xorshift1024_star> gen;
	
	// 	sudo yum install dieharder
	// Continously output random numbers to std::out, for use like ... 
	// 	./xorshift1024star_tester.x | dieharder -g 200 -a -k 2
	// Preliminary results (i.e. running the above line of code)
	// verify the claims at <http://xoroshiro.di.unimi.it/#shootout> that
	// xorshift1024* is a pretty decent generator
	while(true)
	{
		uint64_t rand = gen();
		for(size_t i = 0; i < 8; ++i)
			std::cout << reinterpret_cast<char*>(&rand)[i];
	}
	
	return 0;
}