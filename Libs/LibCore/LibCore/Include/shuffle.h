// Fundamentals

#pragma once

#include "random.h"

#include <QtCore/QVector>

class Shuffle
{
protected:
public:
    
  static QVector<int> Table(int);

private:
  static Xoshiro256 r;
};
