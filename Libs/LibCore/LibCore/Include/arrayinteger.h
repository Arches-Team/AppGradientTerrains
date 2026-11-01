// Fundamentals

#pragma once

#include "scalarfield.h"

class Array2I :public Array2
{
protected:
  QVector<int> field; //!< Field samples.
public:
    explicit Array2I(const Array2&, int = 0);
  explicit Array2I(const Box2&, int, int, int = 0);
        
  //! Empty
  ~Array2I();

  
  virtual int Value(int, int) const;

      int& operator()(int, int);
  
  int at(int) const;
  
  
    
  
  
    
  friend std::ostream& operator<<(std::ostream&, const Array2I&);
};




/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline int& Array2I::operator()(int i, int j)
{
  return field[VertexIndex(i, j)];
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline int Array2I::at(int c) const
{
  return field.at(c);
}



