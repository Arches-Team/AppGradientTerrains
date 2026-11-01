// Core class

#include <cmath>

#include "random.h"


/*!
\brief Default generator, seeded with 512.

This static member is a convenient way to generate random numbers on the fly wherever needed,
in the same way as Random::R239.
\sa Random::R239
*/
Xoshiro256 Xoshiro256::X512(512);

/*!
\brief Creates a random number generator.

The default seed is 512, so that two generators created without argument generate the same sequence.
\param seed Seed argument used for initialization.
\sa Xoshiro256::Seed
*/
Xoshiro256::Xoshiro256(uint64_t seed)
{
  Seed(seed);
}

/*!
\brief Compute the state of the generator given a seed.

The 256 bits of the state are generated from the 64-bit seed with the splitmix64 generator,
which is the initialization recommended for xoshiro generators. Two different seeds give
unrelated sequences, and the state is never null.
\param seed The seed.
*/
void Xoshiro256::Seed(uint64_t seed)
{
  uint64_t x = seed;

  for (int i = 0; i < 4; i++)
  {
    // splitmix64
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    s[i] = z ^ (z >> 31);
  }

  // The null state is the only state which should not be used
  if ((s[0] | s[1] | s[2] | s[3]) == 0)
  {
    s[0] = 1;
  }
}/*!
\brief Compute a uniform random integer from 0 to n-1.

The 32 highest bits of the generator are used, and the distribution is exactly uniform:
the draws which would introduce a bias are rejected (Lemire's method).
\param n Number of values, which should be strictly positive. The function returns 0 otherwise.
\return An integer in [0,n[.
*/
int Xoshiro256::Integer(int n)
{
  if (n <= 0)
  {
    return 0;
  }

  const uint32_t range = uint32_t(n);

  uint64_t m = uint64_t(uint32_t(Next() >> 32)) * range;
  uint32_t l = uint32_t(m);

  if (l < range)
  {
    // Smallest value of l which is not biased
    const uint32_t threshold = (0u - range) % range;
    while (l < threshold)
    {
      m = uint64_t(uint32_t(Next() >> 32)) * range;
      l = uint32_t(m);
    }
  }
  return int(m >> 32);
}