// Fundamentals

#pragma once

#include <QtGui/QImage>

#include "scalarfield.h"

class ScalarField2;
class PointCurve2;


class VectorField2 :public Array2
{
protected:
  QVector<Vector2> field; //!< Field samples.
public:
  //! Empty.
  VectorField2() {}
  explicit VectorField2(const Box2&, int, int, const Vector2 & = Vector2::Null);
  
  //! Empty
  ~VectorField2() {}

  
  // Access to elements
  Vector2 at(int, int) const;
    Vector2& operator()(int, int);
  
    Vector2& operator[](int);

    
  
  
      
  
            
  virtual Vector2 Value(const Vector2&) const;
  virtual Vector2 Value(int, int) const;

  
  
  // Divergence
  virtual ScalarField2 Divergence() const;
  virtual double Divergence(int, int) const;

      void Mul(const double&);

  // Curl
  virtual ScalarField2 Curl() const;
  virtual double Curl(int, int) const;

  
  
  // modify the VectorField
  void Multiply(const Vector2&, const double&, const double&);
    void Clone(const Vector2&, const Vector2&, const Vector2&, const double&);
  void CloneZone(const Vector2&, const QPolygon&, int);
  void CloneBrokenLine(const QVector<Vector2>&, const QVector<Vector2>&, int);

            
        
    };

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline Vector2 VectorField2::at(int i, int j) const
{
  return field.at(VertexIndex(i, j));
}


/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline Vector2& VectorField2::operator()(int i, int j)
{
  return field[VertexIndex(i, j)];
}



/*!
\brief Return the data in the field.
\param c Index.
*/
inline Vector2& VectorField2::operator[](int c)
{
  return field[c];
}


class PointCurve;





