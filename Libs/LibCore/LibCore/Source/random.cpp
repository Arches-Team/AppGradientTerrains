// Core class

#include <cstdint>

#include "random.h"


/*!
\class Random random.h
\brief This class implements several random number generators wrapped in a single class.

\ingroup MathGroup

*/

constexpr int M = (1 << 31);
#define R   48
#define S   8
constexpr int LOGR = 6;

// Physically-random initial state
static int randomData[48] = {
  1947146662, 255872804, 611859725, 1698768494, 1168331706, 903266320, 156960988,
  2108094856, 1962943837, 190733878, 1611490366, 1064800627, 750133665, 1388252873,
  1292226713, 2024731946, 1741633152, 1048965367, 1242081949, 648944060, 2146570054,
  1918590288, 1154878382, 1659238901, 1517088874, 1343505038, 702442230, 223319626,
  2112415214, 1198445789, 1667134997, 81636584, 1477696870, 1396665379, 296474908,
  221800638, 285530547, 506210295, 322166150, 242833116, 542659480, 1371231536,
  164404762, 580757470, 1564914124, 2004579430, 389459662, 1039937051
};

/*!
\brief This static member implements a simple random number generator with seed 239.

This static member is a convenient way to generate random numbers on the fly wherever needed.
*/
Random Random::R239(239);

/*!
\brief Creates a random number generator.
\param seed Seed argument used for initialization.
*/
Random::Random(int seed)
{
  Seed(seed);
}

/*!
\brief Destroys a random number generator.
*/
Random::~Random()
{
}

/*!
\brief Compute internal parameters given seed.
\param seed The seed.
*/
void Random::Seed(int seed)
{
  int i;
  feed = 0;
  tap = R - S;
  borrow = 0;

  // The seed pseudorandomly perturbes a physically random state

  seed = 1103515245 * seed + 12345;
  seed = 1103515245 * seed + 12345;
  seed = 1103515245 * seed + 12345;
  for (i = 0; i < R; i++)
  {
    seed = 1103515245 * seed + 12345;
    vec[i] = seed ^ randomData[i];
  }
  for (i = 0; i < R * LOGR; i++)
  {
    SimpleInt31();
  }
  for (i = 0; i < 571; i++)
  {
    V[i] = SimpleInt31();
  }
  Y = SimpleInt31();
  for (i = 0; i < R * LOGR; i++)
  {
    Int31();
  }
}

/*!
\brief Generate a random integer.

This is an implementation of Marsaglia's subtract-with-borrow generator.
After <I>The Annals of Applied Probability</I>, <B>1</B>(3), 462-480, 1991.
*/
int Random::Int31()
{
  int tmp = (vec[tap] - vec[feed]) - borrow;
  if (tmp < 0)
  {
    borrow = 1;
    tmp += M;
  }
  else
  {
    borrow = 0;
  }
  vec[feed] = tmp;
  if (++feed >= R)
    feed = 0;
  if (++tap >= R)
    tap = 0;
  // Bays & Durham
  int j = Y & 511;
  Y = V[j];
  V[j] = tmp;
  return Y;
}

/*!
\brief Computes 31 bit integer.
*/
int Random::SimpleInt31()
{
  int tmp = (vec[tap] - vec[feed]) - borrow;
  if (tmp < 0)
  {
    borrow = 1;
    tmp += M;
  }
  else
    borrow = 0;
  vec[feed] = tmp;
  if (++feed >= R)
    feed = 0;
  if (++tap >= R)
    tap = 0;
  return tmp;
}

/*!
\brief Compute a uniform random integer from 0 to n-1.
\param n Integer.
*/
int Random::Integer(int n)
{
  int v;

  int slop = int(0x7fffffffL % n);
  do
  {
    v = Int31();
  } while (v <= slop);
  return v % n;
}class Xoshiro256pp
{
public:
  uint64_t s[4];

  explicit Xoshiro256pp(uint64_t seed = 1)
  {
    uint64_t x = seed;

    for (int i = 0; i < 4; ++i)
      s[i] = splitmix64(x);
  }

  static inline uint64_t rotl(uint64_t x, int k)
  {
    return (x << k) | (x >> (64 - k));
  }

  uint64_t next()
  {
    const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = rotl(s[3], 45);

    return result;
  }

  inline double uniform01()
  {
    return (next() >> 11) * 0x1.0p-53;
  }

  inline void uniform01(double& u,double& v)
  {
    // Pour avoir dun coup 2 double avec un seul tirage
    uint64_t r = next();

    u = uniform32(uint32_t(r));
    v = uniform32(uint32_t(r >> 32));
  }

  private:
   static inline double uniform32(uint32_t x)
  {
    return x * 0x1.0p-32;
  }

private:
  static uint64_t splitmix64(uint64_t& x)
  {
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);

    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;

    return z ^ (z >> 31);
  }
};
