// Shuffling algorithms

#include "shuffle.h"

/*!
\class Shuffle shuffle.h
\brief %Shuffling algorithms for generation permutation tables.

\ingroup MathGroup
*/

Xoshiro256 Shuffle::r;

/*!
\brief Compute a permutation table.

This is an implementation of a modified version of the Fisher–Yates algorithm.

\param n Size.
*/
QVector<int> Shuffle::Table(int n)
{
  QVector<int> t(n);
  for (int i = 0; i < n; i++)
  {
    int j = r.Integer(i + 1); // Random integer such that 0 <= j <= i, without modulo bias
    if (j != i)
    {
      t[i] = t[j];
    }
    t[j] = i;
  }
  return t;
}
