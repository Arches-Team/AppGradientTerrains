// Core class

#pragma once

#include <cstdint>

// Random number generator
class Random {
protected:
  int vec[48];
  int feed, tap, borrow;
  int V[571];
  int Y;
public:
  Random(int = 239);
  ~Random();

  void Seed(int);
  int Integer(int);
  
      
  
      
public:
  static Random R239;
protected:
  int Int31();
  int SimpleInt31();
};

/*!
\class Xoshiro256 random.h
\brief A fast, high quality random number generator.

This class implements the xoshiro256+ generator of Blackman and Vigna.
It has a period of 2^256-1, a state of 256 bits (four 64-bit words), and generates a 64-bit word
in a few instructions. Its member functions follow those of Random, so that both classes
can be used in the same way.

The lowest bits of the generator are weak: the lowest bit is an LFSR sequence, which fails linearity tests.
All the functions of the class therefore use the 53 or 32 highest bits of the 64-bit words;
only Xoshiro256::Next() gives access to the raw word, and its lowest bits should not be used on their own.

The generator is not suitable for cryptography.

A default generator seeded with 512 is available as Xoshiro256::X512, in the same way as Random::R239.

Reference: D. Blackman, S. Vigna, <I>Scrambled linear pseudorandom number generators</I>,
ACM Transactions on Mathematical Software, <B>47</B>(4), 2021.

\ingroup MathGroup
*/
class Xoshiro256
{
protected:
  uint64_t s[4]; //!< State of the generator, which should not be null.
public:
  explicit Xoshiro256(uint64_t = 512);
  //! Empty.
  ~Xoshiro256() {}

  void Seed(uint64_t);

  uint64_t Next();
    int Integer(int);
  
      
  
        
public:
  static Xoshiro256 X512;
};

/*!
\brief Generate a random 64-bit word.

This is the xoshiro256+ generator. The highest bits of the word are of good quality,
the lowest bits are not: the lowest bit is an LFSR sequence.
\return A random integer in [0,2^64[.
*/
inline uint64_t Xoshiro256::Next()
{
  const uint64_t result = s[0] + s[3];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = (s[3] << 45) | (s[3] >> 19);

  return result;
}

