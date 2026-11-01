// Fundamentals
#pragma once

#include "circle.h"

class Triangle2;

class Array;

class Array2 :protected Box2
{
public:
  int nx = 0, ny = 0; //!< Sizes.
  Vector2 celldiagonal = Vector2::Null; //!< Cell diagonal.
  Vector2 inversecelldiagonal = Vector2::Null; //!< Inverse cell diagonal.
public:
  Array2();
  explicit Array2(const Box2&, int, int);
    
    
  
  
  int VertexSize() const;
  int GetSizeX() const;
  int GetSizeY() const;

  
        QSize GetQtSize() const;

      
  
    
  Box2 GetBox() const;

  
  Vector2 CellDiagonal() const;
  
  Vector2 ArrayVertex(int, int) const;
  Vector2 ArrayVertex(const QPoint&) const;
  
  
  QRect AreaInteger() const;

    
  
  
    
  // Vertex queries
  void VertexInteger(const Vector2&, int&, int&) const;
  QPoint VertexInteger(const Vector2&) const;
    QRect VertexIntegerArea(const Box2&) const;
  QRect VertexIntegerArea(const Circle2&) const;
    
    void CellInteger(const Vector2&, int&, int&, double&, double&) const;
    
    
  friend std::ostream& operator<<(std::ostream&, const Array2&);

    
  // Domain queries
  constexpr bool InsideVertexIndex(int, int) const;
    constexpr bool InsideVertexIndex(const QPoint&) const;
  constexpr bool InsideVertexIndex(int, int, int) const;

  bool Inside(const Vector2&) const;

    
  // Indexes for storing elements at vertices
  constexpr int VertexIndex(int, int) const;
  constexpr int VertexIndex(const QPoint&) const;

  
  
  protected:
    
    
  constexpr bool InsideCellIndex(int, int) const;
  constexpr bool InsideCellIndex(const QPoint&) const;
  public:
    protected:
  static const QPoint next[8]; //!< %Array of points in the 1-ring neighborhood.
  static const double length[8]; //!< Length to the i-th neighbor.
  static const double inverselength[8]; //!< Inverse length.
};





/*!
\brief Get the vertex size of the array for x axis.
*/
inline int Array2::GetSizeX() const
{
  return nx;
}

/*!
\brief Get the vertex size of the array for y axis.
*/
inline int Array2::GetSizeY() const
{
  return ny;
}



/*!
\brief Return the size of the vertex array.
*/
inline int Array2::VertexSize() const
{
  return nx * ny;
}

/*!
\brief Return the Qt size of the vertex array.
*/
inline QSize Array2::GetQtSize() const
{
  return QSize(nx, ny);
}


/*!
\brief Compute the coordinates of a point on the lattice.
\param i,j Integer coordinates.
*/
inline Vector2 Array2::ArrayVertex(int i, int j) const
{
  return Vector2(a[0] + i * celldiagonal[0], a[1] + j * celldiagonal[1]);
}

/*!
\brief Compute the coordinates of a point on the lattice.
\param p Point.
*/
inline Vector2 Array2::ArrayVertex(const QPoint& p) const
{
  return Vector2(a[0] + p.x() * celldiagonal[0], a[1] + p.y() * celldiagonal[1]);
}

/*!
\brief Get the box of the array.
*/
inline Box2 Array2::GetBox() const
{
  return Box2(a, b);
}

/*!
\brief Compute the index of a given cell.
\param i,j Integer coordinates of the cell.
*/
inline constexpr int Array2::VertexIndex(int i, int j) const
{
  return i + nx * j;
}
/*!
\brief Compute the index of a given cell.
\param p Point.
*/
inline constexpr int Array2::VertexIndex(const QPoint& p) const
{
  return p.x() + nx * p.y();
}



/*!
\brief Check if the indexes are within range.
\param i,j Integer coordinates of the vertex.
*/
inline constexpr bool Array2::InsideCellIndex(int i, int j) const
{
  return (i >= 0) && (i < nx - 1) && (j >= 0) && (j < ny - 1);
}

/*!
\brief Check if the indexes are within range.
\param p Point.
*/
inline constexpr bool Array2::InsideCellIndex(const QPoint& p) const
{
  return (p.x() >= 0) && (p.x() < nx - 1) && (p.y() >= 0) && (p.y() < ny - 1);
}

/*!
\brief Check if the indexes are within range.
\param i,j Integer coordinates of the vertex.
*/
inline constexpr bool Array2::InsideVertexIndex(int i, int j) const
{
  return (i >= 0) && (i < nx) && (j >= 0) && (j < ny);
}


/*!
\brief Check if the indexes are within range.
\param p Point.
*/
inline constexpr bool Array2::InsideVertexIndex(const QPoint& p) const
{
  return (p.x() >= 0) && (p.x() < nx) && (p.y() >= 0) && (p.y() < ny);
}



/*!
\brief Check if a point is in the rectangular domain.
\param p Point.
*/
inline bool Array2::Inside(const Vector2& p) const
{
  return Box2::Inside(p);
}

/*!
\brief Check if the indexes are within k-range.
\param i,j Integer coordinates of the vertex.
\param k Thickness of the boundary around the domain
*/
inline constexpr bool Array2::InsideVertexIndex(int i, int j, int k) const
{
  return (i >= 0 + k) && (i < nx - k) && (j >= 0 + k) && (j < ny - k);
}


/*!
\brief Compute the point on the lattice given an input point.
\param p Point.
\param i,j Integer coordinates of the cell.
\param u,v Coordinates of the point in the corresponding cell.
*/
inline void Array2::CellInteger(const Vector2& p, int& i, int& j, double& u, double& v) const
{
  Vector2 q = p - a;

  /*
    Vector2 d = b - a;

    u = q[0] / d[0];
    v = q[1] / d[1];

    // Scale
    u *= (nx - 1);
    v *= (ny - 1);
  */
  u = q[0] * inversecelldiagonal[0];
  v = q[1] * inversecelldiagonal[1];

  // Integer coordinates
  i = int(u);
  j = int(v);

  // Local coordinates within cell
  u -= i;
  v -= j;
}





/*!
\brief Compute the coordinates of a vertex inside a cell.
\param p Point.
\param i,j Integer coordinates of the cell.
*/
inline void Array2::VertexInteger(const Vector2& p, int& i, int& j) const
{
  Vector2 q = p - a;
  /*
  Vector2 d = b - a;

  double u = q[0] / d[0];
  double v = q[1] / d[1];

  i = int(u * (nx - 1));
  j = int(v * (ny - 1));
  */
  i = int(q[0] * inversecelldiagonal[0]);
  j = int(q[1] * inversecelldiagonal[1]);
}

/*!
\brief Compute the coordinates of a vertex inside a cell.
\param p Point.
*/
inline QPoint Array2::VertexInteger(const Vector2& p) const
{
  int i, j;
  VertexInteger(p, i, j);
  return QPoint(i, j);
}


/*!
\brief Return the range of integer values for the domain.
*/
inline QRect Array2::AreaInteger() const
{
  return QRect(0, 0, nx - 1, ny - 1);
}



class Array :protected Box
{
protected:
  int nx, ny, nz; //!< Sizes
  Vector celldiagonal; //!< Cell diagonal.
  Vector inversecelldiagonal; //!< Inverse cell diagonal.
public:
      
  
  
    
      
    
  
    
  
      
        
  
  
  
    
      
  friend std::ostream& operator<<(std::ostream&, const Array&);
    
    
public:
  constexpr int VertexIndex(int, int, int) const;
  constexpr bool InsideVertexIndex(int, int, int) const;
  constexpr bool InsideCellIndex(int, int, int) const;
  
    };






/*!
\brief Compute the index of a given vertex.
\param i,j,k Integer coordinates of the vertex.
*/
inline constexpr int Array::VertexIndex(int i, int j, int k) const
{
  return (i * ny + j) * nz + k;
}

/*!
\brief Check if the indexes are within range.
\param i,j,k Integer coordinates of the vertex.
*/
inline constexpr bool Array::InsideCellIndex(int i, int j, int k) const
{
  return (i >= 0) && (i < nx - 1) && (j >= 0) && (j < ny - 1) && (k >= 0) && (k < nz - 1);
}

/*!
\brief Check if the indexes are within range.
\param i,j,k Integer coordinates of the vertex.
*/
inline constexpr bool Array::InsideVertexIndex(int i, int j, int k) const
{
  return (i >= 0) && (i < nx) && (j >= 0) && (j < ny) && (k >= 0) && (k < nz);
}
















