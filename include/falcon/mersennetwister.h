// MersenneTwister.h
// Mersenne Twister random number generator -- a C++ class MTRand
// Based on code by Makoto Matsumoto, Takuji Nishimura, and Shawn Cokus
// Richard J. Wagner  v1.0  15 May 2003  rjwagner@writeme.com

// The Mersenne Twister is an algorithm for generating random numbers.  It
// was designed with consideration of the flaws in various other generators.
// The period, 2^19937-1, and the order of equidistribution, 623 dimensions,
// are far greater.  The generator is also fast; it avoids multiplication and
// division, and it benefits from caches and pipelines.  For more information
// see the inventors' web page at http://www.math.keio.ac.jp/~matumoto/emt.html

// Reference
// M. Matsumoto and T. Nishimura, "Mersenne Twister: A 623-Dimensionally
// Equidistributed Uniform Pseudo-Random Number Generator", ACM Transactions on
// Modeling and Computer Simulation, Vol. 8, No. 1, January 1998, pp 3-30.

// Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
// Copyright (C) 2000 - 2003, Richard J. Wagner
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//   3. The names of its contributors may not be used to endorse or promote
//      products derived from this software without specific prior written
//      permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// The original code included the following notice:
//
//     When you use this, send an email to: matumoto@math.keio.ac.jp
//     with an appropriate reference to your work.
//
// It would be nice to CC: rjwagner@writeme.com and Cokus@math.washington.edu
// when you write.

#ifndef MERSENNETWISTER_H
#define MERSENNETWISTER_H

// Not thread safe (unless auto-initialization is avoided and each thread has
// its own MTRand object)

#include <limits.h>
#include <time.h>
#include <math.h>
#include <falcon/types.h>
#include <falcon/sys.h>

#include <falcon/mt.h>

#if defined(__MINGW32__) || defined(__MINGW64__)
class MTRand {
#else
class FALCON_DYN_CLASS MTRand {
#endif
// Data
public:
    typedef Falcon::uint32 uint32;
    typedef Falcon::int32 int32;
    typedef Falcon::uint64 uint64;
	static const uint32 N = 624;       // length of state vector
	static const uint32 SAVE = N + 1;  // length of array for save()

protected:
	static const int M = 397;  // period parameter

	uint32 state[N];   // internal state
	uint32 *pNext;     // next value to get from state
	int left;          // number of values left before reload needed

	uint32 m_mark;     // FALCON: garbage collector mark (so we don't need a carrier)

//Methods
public:

	MTRand( uint32 oneSeed );  // initialize with a simple uint32
	MTRand( uint32 *const bigSeed, uint32 const seedLength = N );  // or an array
	MTRand();                         // auto-initialize with /dev/urandom or time() and clock()
   MTRand(const MTRand&);            // prevent copy constructor
   MTRand& operator=(const MTRand&); // no-op operator=

   virtual ~MTRand();

	// Do NOT use for CRYPTOGRAPHY without securely hashing several returned
	// values together, otherwise the generator state can be learned after
	// reading 624 consecutive values.

	// Access to 32-bit random numbers
	double rand();                          // real number in [0,1]
	double rand( double n );         // real number in [0,n]
	double randExc();                       // real number in [0,1)
	double randExc( double n );      // real number in [0,n)
	double randDblExc();                    // real number in (0,1)
	double randDblExc( double n );   // real number in (0,n)
	virtual uint32 randInt();                       // integer in [0,2^32-1]
	uint32 randInt( uint32 n );      // integer in [0,n] for n < 2^32
	double operator()() { return rand(); }  // same as rand()

    // uint64 randomness especially for Falcon
    uint64 randInt64();
    uint64 randInt64( uint64 n );      // integer in [0,n] for n < 2^64

	// Access to 53-bit random numbers (capacity of IEEE double precision)
	double rand53();  // real number in [0,1)

	// Access to nonuniform random number distributions
	double randNorm( double mean = 0.0, double variance = 0.0 );

	// Re-seeding functions with same behavior as initializers
	void seed( const uint32 oneSeed );
	void seed( uint32 *const bigSeed, const uint32 seedLength = N );
	void seed();
	void seedWithPid();

	// Saving and loading generator state
	void save( uint32* saveArray ) const;  // to array of size SAVE
	void load( uint32 *const loadArray );  // from such array

	void gcMark( uint32 mark ) { m_mark = mark; }
	uint32 currentMark() const { return m_mark; }
protected:
	void initialize( uint32 oneSeed );

	void reload();
	uint32 hiBit( const uint32& u ) const { return u & 0x80000000UL; }
	uint32 loBit( const uint32& u ) const { return u & 0x00000001UL; }
	uint32 loBits( const uint32& u ) const { return u & 0x7fffffffUL; }
	uint32 mixBits( const uint32& u, const uint32& v ) const
		{ return hiBit(u) | loBits(v); }
	uint32 twist( const uint32& m, const uint32& s0, const uint32& s1 ) const
		{ return m ^ (mixBits(s0,s1)>>1) ^ uint32(-(int32)(loBit(s1) & 0x9908b0dfUL)); }
	static uint32 hash( const time_t& t, const clock_t& c );
};

#if defined(__MINGW32__) || defined(__MINGW64__)
class MTRand_interlocked: public MTRand
#else
class FALCON_DYN_CLASS MTRand_interlocked: public MTRand
#endif
{

//Methods
public:

   MTRand_interlocked( uint32 oneSeed );  // initialize with a simple uint32
   MTRand_interlocked( uint32 *const bigSeed, uint32 const seedLength = N );  // or an array
   MTRand_interlocked();                         // auto-initialize with /dev/urandom or time() and clock()
   MTRand_interlocked(const MTRand_interlocked&);            // prevent copy constructor
   MTRand_interlocked& operator=(const MTRand&); // no-op operator=
   MTRand_interlocked& operator=(const MTRand_interlocked&); // no-op operator=

   virtual ~MTRand_interlocked();

   virtual uint32 randInt();                       // integer in [0,2^32-1]
   uint32 randInt( uint32 n ) { return MTRand::randInt(n); }

   // Re-seeding functions with same behavior as initializers
   virtual void seed( const uint32 oneSeed );
   virtual void seed( uint32 *const bigSeed, const uint32 seedLength = N );

   // Saving and loading generator state
   void save( uint32* saveArray ) const;  // to array of size SAVE
   void load( uint32 *const loadArray );  // from such array

private:
   mutable Falcon::Mutex m_mtx;
};


inline MTRand::MTRand(const MTRand&):
         m_mark(0)
    { seed(); }

inline MTRand& MTRand::operator=(const MTRand&)
    { return *this; }

inline MTRand::MTRand( uint32 oneSeed ):
                  m_mark(0)
	{ seed(oneSeed); }

inline MTRand::MTRand( uint32 *const bigSeed, const uint32 seedLength ):
                  m_mark(0)
	{ seed(bigSeed,seedLength); }

inline MTRand::MTRand():
                  m_mark(0)
	{ seed(); }

inline MTRand::~MTRand() {}

inline double MTRand::rand()
	{ return double(randInt()) * (1.0/4294967295.0); }

inline double MTRand::rand( double n )
	{ return rand() * n; }

inline double MTRand::randExc()
	{ return double(randInt()) * (1.0/4294967296.0); }

inline double MTRand::randExc( double n )
	{ return randExc() * n; }

inline double MTRand::randDblExc()
	{ return ( double(randInt()) + 0.5 ) * (1.0/4294967296.0); }

inline double MTRand::randDblExc( double n )
	{ return randDblExc() * n; }

inline double MTRand::rand53()
{
	uint32 a = randInt() >> 5, b = randInt() >> 6;
	return ( a * 67108864.0 + b ) * (1.0/9007199254740992.0);  // by Isaku Wada
}

inline MTRand::uint64 MTRand::randInt64()
{
    return (uint64(randInt()) << 32) | randInt();
}

inline double MTRand::randNorm( double mean, double variance )
{
	// Return a real number from a normal (Gaussian) distribution with given
	// mean and variance by Box-Muller method
	double r = sqrt( -2.0 * log( 1.0-randDblExc()) ) * variance;
	double phi = 2.0 * 3.14159265358979323846264338328 * randExc();
	return mean + r * cos(phi);
}

inline MTRand::uint32 MTRand::randInt()
{
	// Pull a 32-bit integer from the generator state
	// Every other access function simply transforms the numbers extracted here
   if( left == 0 ) reload();
	--left;

	register uint32 s1;
	s1 = *pNext++;

	s1 ^= (s1 >> 11);
	s1 ^= (s1 <<  7) & 0x9d2c5680UL;
	s1 ^= (s1 << 15) & 0xefc60000UL;
	return ( s1 ^ (s1 >> 18) );
}

inline MTRand::uint32 MTRand::randInt( uint32 n )
{
	// Find which bits are used in n
	// Optimized by Magnus Jonsson (magnus@smartelectronix.com)
	uint32 used = n;
	used |= used >> 1;
	used |= used >> 2;
	used |= used >> 4;
	used |= used >> 8;
	used |= used >> 16;

	// Draw numbers until one is found in [0,n]
	uint32 i;
	do
		i = randInt() & used;  // toss unused bits to shorten search
	while( i > n );
	return i;
}

inline MTRand::uint64 MTRand::randInt64( uint64 n )
{
    // Find which bits are used in n
    // Optimized by Magnus Jonsson (magnus@smartelectronix.com)
    uint64 used = n;
    used |= used >> 1;
    used |= used >> 2;
    used |= used >> 4;
    used |= used >> 8;
    used |= used >> 16;
    used |= used >> 32;

    // Draw numbers until one is found in [0,n]
    uint64 i;
    do
    i = randInt64() & used;  // toss unused bits to shorten search
    while( i > n );
    return i;
}


inline void MTRand::seed( uint32 oneSeed )
{
	// Seed the generator with a simple uint32
   initialize(oneSeed);
	reload();

}


inline void MTRand::seed( uint32 *const bigSeed, uint32 seedLength )
{
	// Seed the generator with an array of uint32's
	// There are 2^19937-1 possible initial states.  This function allows
	// all of those to be accessed by providing at least 19937 bits (with a
	// default seed length of N = 624 uint32's).  Any bits above the lower 32
	// in each element are discarded.
	// Just call seed() if you want to get array from /dev/urandom
	initialize(19650218UL);
	register uint32 i = 1;
	register uint32 j = 0;
	register uint32 k = seedLength;
	if( N > seedLength )
	{
	   k = N;
	}

	for( ; k; --k )
	{
		state[i] =
			state[i] ^ ( (state[i-1] ^ (state[i-1] >> 30)) * 1664525UL );
		state[i] += ( bigSeed[j] & 0xffffffffUL ) + j;
		state[i] &= 0xffffffffUL;
		++i;  ++j;
		if( i >= N ) { state[0] = state[N-1];  i = 1; }
		if( j >= seedLength ) j = 0;
	}
	for( k = N - 1; k; --k )
	{
		state[i] =
			state[i] ^ ( (state[i-1] ^ (state[i-1] >> 30)) * 1566083941UL );
		state[i] -= i;
		state[i] &= 0xffffffffUL;
		++i;
		if( i >= N ) { state[0] = state[N-1];  i = 1; }
	}
	state[0] = 0x80000000UL;  // MSB is 1, assuring non-zero initial array
	reload();
}


inline void MTRand::seed()
{
	// Seed the generator with hash of time() and clock() values
	seed( hash( time(NULL), clock() ) );
}

inline void MTRand::seedWithPid()
{
   // Seed the generator with hash of time() and clock() values
   seed( hash( time_t(Falcon::Sys::_getpid()), clock() ) );
}


inline void MTRand::initialize( uint32 seed )
{
   // Initialize generator state with seed
   // See Knuth TAOCP Vol 2, 3rd Ed, p.106 for multiplier.
   // In previous versions, most significant bits (MSBs) of the seed affect
   // only MSBs of the state array.  Modified 9 Jan 2002 by Makoto Matsumoto.

   register uint32 *s = state;
   register uint32 *r = state;
   register uint32 i = 1;
   *s++ = seed & 0xffffffffUL;
   for( ; i < N; ++i )
   {
      *s++ = ( 1812433253UL * ( *r ^ (*r >> 30) ) + i ) & 0xffffffffUL;
      r++;
   }
}


inline void MTRand::reload()
{
   // FALCON: Not locked --- must always be called inside a lock

	// Generate N new values in state
	// Made clearer and faster by Matthew Bellew (matthew.bellew@home.com)
	register uint32 *p = state;
	register int i;
	for( i = N - M; i--; ++p )
		*p = twist( p[M], p[0], p[1] );
	for( i = M; --i; ++p )
		*p = twist( *(p+M-N), p[0], p[1] );
	*p = twist( *(p+M-N), p[0], state[0] );

	left = N, pNext = state;
}


inline MTRand::uint32 MTRand::hash( const time_t& t, const clock_t& c )
{
	// Get a uint32 from t and c
	// Better than uint32(x) in case x is floating point in [0,1]
	// Based on code by Lawrence Kirby (fred@genesis.demon.co.uk)

	static uint32 differ = 0;  // guarantee time-based seeds will change

	uint32 h1 = 0;
	unsigned char *p = (unsigned char *) &t;
	for( size_t i = 0; i < sizeof(t); ++i )
	{
		h1 *= UCHAR_MAX + 2U;
		h1 += p[i];
	}
	uint32 h2 = 0;
	p = (unsigned char *) &c;
	for( size_t j = 0; j < sizeof(c); ++j )
	{
		h2 *= UCHAR_MAX + 2U;
		h2 += p[j];
	}
	return ( h1 + differ++ ) ^ h2;
}


inline void MTRand::save( uint32* saveArray ) const
{
	register uint32 *sa = saveArray;
	register const uint32 *s = state;
	register int i = N;

   for( ; i--; *sa++ = *s++ ) {}
	*sa = left;
}


inline void MTRand::load( uint32 *const loadArray )
{
	register uint32 *s = state;
	register uint32 *la = loadArray;
	register int i = N;

	for( ; i--; *s++ = *la++ ) {}
	left = *la;
	pNext = &state[N-left];
}

/* Avoid dependency from STL for Falcon
inline std::ostream& operator<<( std::ostream& os, const MTRand& mtrand )
{
	register const MTRand::uint32 *s = mtrand.state;
	register int i = mtrand.N;
	for( ; i--; os << *s++ << "\t" ) {}
	return os << mtrand.left;
}

inline std::istream& operator>>( std::istream& is, MTRand& mtrand )
{
	register MTRand::uint32 *s = mtrand.state;
	register int i = mtrand.N;
	for( ; i--; is >> *s++ ) {}
	is >> mtrand.left;
	mtrand.pNext = &mtrand.state[mtrand.N-mtrand.left];
	return is;
}
*/

//===========================================================
// Interlocked version
//===========================================================

inline MTRand_interlocked::MTRand_interlocked(const MTRand_interlocked& other):
         MTRand(other)
{}

inline MTRand_interlocked& MTRand_interlocked::operator=(const MTRand&)
    { return *this; }

inline MTRand_interlocked& MTRand_interlocked::operator=(const MTRand_interlocked&)
    { return *this; }

inline MTRand_interlocked::MTRand_interlocked( uint32 oneSeed ):
         MTRand(oneSeed)
{}


inline MTRand_interlocked::MTRand_interlocked( uint32 *const bigSeed, const uint32 seedLength ):
         MTRand(bigSeed, seedLength)
{}

inline MTRand_interlocked::MTRand_interlocked():
         MTRand()
{}

inline MTRand_interlocked::~MTRand_interlocked()
{}

inline MTRand::uint32 MTRand_interlocked::randInt()
{
   // Pull a 32-bit integer from the generator state
   // Every other access function simply transforms the numbers extracted here
   m_mtx.lock();
   if( left == 0 ) reload();
   --left;

   register uint32 s1;
   s1 = *pNext++;
   m_mtx.unlock();

   s1 ^= (s1 >> 11);
   s1 ^= (s1 <<  7) & 0x9d2c5680UL;
   s1 ^= (s1 << 15) & 0xefc60000UL;
   return ( s1 ^ (s1 >> 18) );
}


// Re-seeding functions with same behavior as initializers
inline void MTRand_interlocked::seed( uint32 oneSeed )
{
   m_mtx.lock();
   initialize(oneSeed);
   reload();
   m_mtx.unlock();
}

inline void MTRand_interlocked::seed( uint32 *const bigSeed, const uint32 seedLength )
{
   m_mtx.lock();
   MTRand::seed(bigSeed, seedLength);
   m_mtx.unlock();
}

inline void MTRand_interlocked::save( uint32* saveArray ) const
{
   m_mtx.lock();
   MTRand::save( saveArray );
   m_mtx.unlock();
}

inline void MTRand_interlocked::load( uint32 *const loadArray )
{
   m_mtx.lock();
   MTRand::load( loadArray );
   m_mtx.unlock();
}


#endif  // MERSENNETWISTER_H

// Change log:
//
// v0.1 - First release on 15 May 2000
//      - Based on code by Makoto Matsumoto, Takuji Nishimura, and Shawn Cokus
//      - Translated from C to C++
//      - Made completely ANSI compliant
//      - Designed convenient interface for initialization, seeding, and
//        obtaining numbers in default or user-defined ranges
//      - Added automatic seeding from /dev/urandom or time() and clock()
//      - Provided functions for saving and loading generator state
//
// v0.2 - Fixed bug which reloaded generator one step too late
//
// v0.3 - Switched to clearer, faster reload() code from Matthew Bellew
//
// v0.4 - Removed trailing newline in saved generator format to be consistent
//        with output format of built-in types
//
// v0.5 - Improved portability by replacing static const int's with enum's and
//        clarifying return values in seed(); suggested by Eric Heimburg
//      - Removed MAXINT constant; use 0xffffffffUL instead
//
// v0.6 - Eliminated seed overflow when uint32 is larger than 32 bits
//      - Changed integer [0,n] generator to give better uniformity
//
// v0.7 - Fixed operator precedence ambiguity in reload()
//      - Added access for real numbers in (0,1) and (0,n)
//
// v0.8 - Included time.h header to properly support time_t and clock_t
//
// v1.0 - Revised seeding to match 26 Jan 2002 update of Nishimura and Matsumoto
//      - Allowed for seeding with arrays of any length
//      - Added access for real numbers in [0,1) with 53-bit resolution
//      - Added access for real numbers from normal (Gaussian) distributions
//      - Increased overall speed by optimizing twist()
//      - Doubled speed of integer [0,n] generation
//      - Fixed out-of-range number generation on 64-bit machines
//      - Improved portability by substituting literal constants for long enum's
//      - Changed license from GNU LGPL to BSD
