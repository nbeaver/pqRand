/* pqRand: The precise quantile random package
 * Copyright (C) 2017 Keith Pedersen (Keith.David.Pedersen@gmail.com)
 * 
 * This package is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the COPYRIGHT_NOTICE for more details.
 * 
 * Under Section 7 of GPL version 3, you are granted additional
 * permissions described in the GCC Runtime Library Exception, version
 * 3.1, as published by the Free Software Foundation.
 * 
 * You should have received a copy of the GNU General Public License and
 * a copy of the GCC Runtime Library Exception along with this package;
 * see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
 * <http://www.gnu.org/licenses/>.
*/

#include "../include/pqRand.hpp"
#include <string>
#include <fstream>
#include <stdexcept>
#include <assert.h>
#include <sstream>

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

template<class prng_t>
void pqRand::seeded_uPRNG<prng_t>::Seed()
{
	std::stringstream ss;
	{
		std::random_device randDev;
		
		// Insert one word for every state_size, space-separated
		switch(seeded_uPRNG::word_size)
		{
			case 32:
				for(size_t i = 0; i < prng_t::state_size; ++i)
				{
					ss << randDev() << " ";
				}
			break;
			
			case 64:
				for(size_t i = 0; i < prng_t::state_size; ++i)
				{
					// Unfortunately, random_device outputs 32 bits. Make a 64-bit uint.
					ss << ((uint64_t(randDev()) << 32) bitor uint64_t(randDev())) << " ";
				}
			break;
		}
	}
	
	ss << prng_t::state_size; // Terminate with state_size
	this->Seed_FromStream(ss);
}

////////////////////////////////////////////////////////////////////////

template<class prng_t>
void pqRand::seeded_uPRNG<prng_t>::Seed_FromFile(std::string const& fileName)
{
	std::ifstream file(fileName.c_str(), std::ios::in);
	
	if(not file.is_open())
	{
		throw std::runtime_error("pqRand::seeded_uPRNG::Seed ... <"
			+ fileName + "> ... file not found!");
	}
	
	this->Seed_FromStream(file);
	file.close();
}

////////////////////////////////////////////////////////////////////////

template<class prng_t>
void pqRand::seeded_uPRNG<prng_t>::Seed_FromString(std::string const& seedString)
{
	std::stringstream stream(seedString);
	this->Seed_FromStream(stream);
}

////////////////////////////////////////////////////////////////////////

template<class prng_t>
void pqRand::seeded_uPRNG<prng_t>::Seed_FromStream(std::istream& stream)
{
	stream >> *this; // seeded_uPRNG (as a wrapper) has no state to seed
}

////////////////////////////////////////////////////////////////////////

template<class prng_t>
void pqRand::seeded_uPRNG<prng_t>::WriteState(std::string const& fileName)
{
	// CAUTION: overwrite existing file without warning (ios::trunc)
	std::ofstream file(fileName.c_str(), std::ios::out | std::ios::trunc);
	
	if(not file.is_open())
	{
		throw std::runtime_error("pqRand::seeded_uPRNG::WriteState ... <"
			+ fileName + "> ... file cannot be created!");
	}
	
	this->WriteState_ToStream(file);
	file.close();
}

////////////////////////////////////////////////////////////////////////

template<class prng_t>
std::string pqRand::seeded_uPRNG<prng_t>::GetState()
{
	std::stringstream string;
	this->WriteState_ToStream(string);
	return string.str();
}

////////////////////////////////////////////////////////////////////////

template<class prng_t>
void pqRand::seeded_uPRNG<prng_t>::WriteState_ToStream(std::ostream& stream)
{
	stream << *this; // seeded_uPRNG (as a wrapper) has no state to write
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// BEGIN deep magic, do not touch!
uint64_t const pqRand::xorshift1024_star::JUMP[pqRand::xorshift1024_star::state_size] =  
{ 0x84242f96eca9c41d, 0xa3c65b8776f96855, 0x5b34a39f070b5837, 0x4489affce4f31a1e,
	0x2ffeeb0a48316f40, 0xdc2d9891fe68c022, 0x3659132bb12fea70, 0xaac17d8efa43cab8,
	0xc4cb815590989b13, 0x5ee975283d71c93b, 0x691548c86c1bd540, 0x7910c41d10a1e6a5, 
	0x0b5fc64563b3e2a8, 0x047f7684e9fc949d, 0xb99181f2d8f685ca, 0x284600e3f30e38c3};

uint64_t pqRand::xorshift1024_star::operator()() 
{
	uint64_t s1;
	{
		uint64_t const s0 = state[p];
		s1 = state[p = (p + 1) & 15];
		s1 ^= s1 << 31; // a
		s1 = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30); // b,c
	}
	state[p] = s1;
	return s1 * UINT64_C(1181783497276652981);
}

void pqRand::xorshift1024_star::Jump()
{
	uint64_t t[16] = { 0 };
	for(size_t i = 0; i < (sizeof(JUMP) / sizeof(*JUMP)); i++)
	{
		for(size_t b = 0; b < 64; b++) 
		{
			if (JUMP[i] & 1ULL << b) // What is the order of operations here?
			{
				for(size_t j = 0; j < 16; j++)
					t[j] ^= state[(j + p) & 15];
			}
			(*this)();
		}
	}

	for(size_t j = 0; j < 16; j++)
		state[(j + p) & 15] = t[j];
}
// END deep magic

////////////////////////////////////////////////////////////////////////

void pqRand::xorshift1024_star::Jump(size_t nTimes)
{
	for(size_t n = 0; n < nTimes; ++n)
		Jump();
}

////////////////////////////////////////////////////////////////////////

std::ostream& pqRand::operator << (std::ostream& stream, xorshift1024_star const& gen)
{
	for(size_t i = 0; i < xorshift1024_star::state_size; ++i)
		stream << gen.state[i] << " ";
	
	stream << xorshift1024_star::state_size << " ";
	
	// Store p as well (it defines the internal state too).
	stream << gen.p;
	
	return stream;
}

////////////////////////////////////////////////////////////////////////

// Seed the generator from the stream. Two formats expected (N = state_size)
// s_1 s_2 ... s_N N    --> p not specified, set to zero
// s_1 s_2 ... s_N N p  --> p specified
std::istream& pqRand::operator >> (std::istream& stream, xorshift1024_star& gen)
{
	uint64_t word;
	
	for(size_t i = 0; i < xorshift1024_star::state_size; ++i)
	{	
		if(not (stream >> word))
			throw std::runtime_error("pqRand::xorshift1024_star: seed stream malformed -- not enough words to fill state.");
		
		gen.state[i] = word;
	}
	
	if(not (stream >> word))
		throw std::runtime_error("pqRand::xorshift1024_star: seed stream malformed -- state size not supplied.");
	else if(word not_eq xorshift1024_star::state_size)
		throw std::runtime_error("pqRand::xorshift1024_star: seed stream malformed -- wrong state size.");
	
	// Read p, which exists in [0, 16). If p is not stored, then use p = 0
	if(stream >> word)
	{
		if(word >= xorshift1024_star::state_size)
			throw std::runtime_error("pqRand::xorshift1024_star: seed stream malformed -- p is larger than state_size");
		gen.p = word;
	}
	else gen.p = 0;
	
	return stream;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// Need to instantiate the template class for the object file or shared library
template class pqRand::seeded_uPRNG<pqRand::PRNG_t>;

////////////////////////////////////////////////////////////////////////

typename pqRand::real_t pqRand::engine::RandomMantissa_Quasiuniform()
{
	result_type randUint = (*this)();
	
	if(randUint < minEntropy) // Add entropy to randUint until it has enough
	{
		real_t downScale = 1.; // Downscale will be set at the end
		
		{
			// We need to shift left at least once, so let's start with that
			int shiftLeft = 1; // Must use signed type, for negative exponent in pow()
			randUint <<= 1;
			
			if(randUint == 0) // Exceedingly rare, but need to handle
			{
				shiftLeft = 0; // Undo the initial left shift because ...
				
				// ... every time we draw a zero, do a 64-bit leftward shift.
				// It's like we have an infinite bit stream which we keep shifting left
				do // We already drew one zero, so we have to downscale at least once
					downScale *= scaleToU_Quasiuniform;
				while((randUint = (*this)()) == 0);
			}
			
			// Keep shifting left until the mantissa's most significant bit is
			// in the correct position
			while(randUint < minEntropy)
			{
				randUint <<= 1;
				++shiftLeft;
			}
			
			assert(shiftLeft < numBitsOfEntropyRequired);
			downScale *= real_t(std::exp2(-shiftLeft));
						
			// Insert new bits into the gap filled by the shift left
			// Usually quite wasteful, but generally quite rare
			randUint or_eq ((*this)() >> (numBitsPRNG - shiftLeft));
		}
		
		// Set the sticky bit, to defeat round-to-even,
		// then downScale, to maintain the coarse location
		return real_t(randUint bitor 1) * downScale;
	}
	else
		return real_t(randUint bitor 1);
}

////////////////////////////////////////////////////////////////////////

//~ typename pqRand::real_t pqRand::engine::RandomMantissa_Superuniform()
//~ {
	//~ result_type randUint;
	
	//~ // Yes, this is a goto, but they are not always bad.
	//~ // This jump spans only a small number of lines, so it's easy to read.
	//~ generate:
		//~ // Better to shift right than use a bit-mask, due to bad bits
		//~ randUint = ((*this)() >> bitShiftRight_Superuniform); 
		//~ if(randUint == 0)
		//~ {
			//~ // Map 0 to the max mantissa, but only half the time
			//~ // Should not cause an endless loop, unless PRNG is broken
			//~ if(RandBool())
				//~ randUint = maxMantissa_Superuniform;
			//~ else
				//~ goto generate;
		//~ }
	
	// Equivalent code, without goto, but every call has an extra addition
	// (hence the virtue of the goto is speed).
	// do
		// randUint = (((*this)() & bitMask_Superuniform) + 1);
	// while((randUint == maxMantissa_Superuniform) and RandBool());
	
	//~ return real_t(randUint);
//~ }

////////////////////////////////////////////////////////////////////////

typename pqRand::real_t pqRand::engine::RandomMantissa_Superuniform()
{
	result_type randUint;
	
	while((randUint = ((*this)() >> bitShiftRight_Superuniform)) == 0);
	
	return real_t(randUint);
}

////////////////////////////////////////////////////////////////////////

void pqRand::engine::Seed_FromStream(std::istream& stream)
{
	// Seed the base class
	seeded_uPRNG<PRNG_t>::Seed_FromStream(stream);
		
	// The internal state of pqRand_engine contains the bitCache,
	// which should be appended to the seed stream after PRNG details
	result_type word;
	if(stream >> word)
	{
		bitCache = word;
		
		// The state of bitCache can be missing. But if the bitCache is there, 
		// the cacheMask cannot be missing
		if(not (stream >> word))
			throw std::runtime_error("pqRand::engine::Seed: bitCache stored in seed, but not cacheMask");
		else
			cacheMask = word;
			
		// WARNING! What if badBits changes? What if the PRNG itself changes?
		// We assume that seeds will not be shared among different builds!
	}
	else
		DefaultInitializeBitCache();
}

////////////////////////////////////////////////////////////////////////

void pqRand::engine::WriteState_ToStream(std::ostream& stream)
{
	seeded_uPRNG<PRNG_t>::WriteState_ToStream(stream);
	
	// Now write out the state of the bitCache and the cacheMask
	stream  << " " <<  bitCache << " " << cacheMask;
}

////////////////////////////////////////////////////////////////////////

void pqRand::engine::DefaultInitializeBitCache()
{
	// Defer initialization until next call to RandBool()
	cacheMask = replenishBitCache;
}

////////////////////////////////////////////////////////////////////////

bool pqRand::engine::RandBool()
{
	// The cacheMask starts at the leftmost bit and moves right
	if(cacheMask == replenishBitCache)
	{
		// When the cacheMask has moved too far right ... 
		bitCache = (*this)(); // Get a new set of random bits
		cacheMask = (result_type(1) << (numBitsPRNG - 1)); // Reset the cacheMask
	}
	
	bool const decision = bool(cacheMask bitand bitCache);
	cacheMask >>= 1;
	return decision;
}

////////////////////////////////////////////////////////////////////////

void pqRand::engine::ApplyRandomSign(real_t& victim)
{
	bool const decision = RandBool();
	victim = std::copysign(victim, decision ? real_t(1.) : real_t(-1.));
}
