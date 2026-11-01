// Vector sets

#pragma once

#include <QtCore/QVector> 

#include "box.h"

class VectorSet2;








// Set of 2D vectors
class VectorSet2
{
protected:
  QVector<Vector2> v; //!< Vectors.
public:
    VectorSet2(const QVector<Vector2>&);

  //! Empty.
  ~VectorSet2() {}

      
  
  Vector2 At(int) const;
    
        
  
    
          
        
    
    
        
  friend std::ostream& operator<<(std::ostream&, const VectorSet2&);

  
    public:
            };

/*!
\brief Access to element.
*/
inline Vector2 VectorSet2::At(int i) const
{
  return v.at(i);
}





