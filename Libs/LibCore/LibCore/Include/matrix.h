// Matrix 

#pragma once

// Include Vector class
#include "evector.h"

#include "cubic.h"

class Matrix2 {
protected:
  double r[4] = { 1.0,0.0,0.0,1.0 };  //!< The array storing the coefficients of the matrix.
public:
  //! Empty.
  Matrix2() {}
  explicit Matrix2(const double&);
      explicit Matrix2(const double&, const double&, const double&, const double&);
  explicit Matrix2(const double&, const double&, const double&);

  static const Matrix2 Null;
  static const Matrix2 Identity;

  // Functions to access class components  
  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  constexpr double& operator() (int, int);
  constexpr double operator() (int, int) const;

  
    
          
  
  // Binary operators
  friend Matrix2 operator+ (const Matrix2&, const Matrix2&);
  friend Matrix2 operator- (const Matrix2&, const Matrix2&);
  friend Matrix2 operator* (const Matrix2&, const Matrix2&);

  Vector2 operator*(const Vector2&) const;

  friend Matrix2 operator* (const Matrix2&, const double&);
  friend Matrix2 operator* (const double&, const Matrix2&);

  friend Matrix2 operator/ (const Matrix2&, const double&);
  friend Matrix2 operator/ (const double&, const Matrix2&);

  static Matrix2 Rotation(const double&);
    static Matrix2 Tensor2(const double&);
  
  friend std::ostream& operator<<(std::ostream&, const Matrix2&);

  
  
  
    
        
    
  static const Matrix2 RotationHalfPi; //!< Half-pi rotation.
  static const Matrix2 RotationOneThirdPi; //!< One third pi rotation (60°).
private:
  static const double epsilon; //!< Epsilon value used to check angles of a rotation matrix, and in the implementation of some algebra methods.
};

//! Direct access to the array of the matrix.
inline constexpr double& Matrix2::operator[] (int i)
{
  return r[i];
}

//! Overloaded.
inline constexpr double Matrix2::operator[] (int i) const
{
  return r[i];
}

/*!
\brief Get element (i,j) of the matrix.
\param i Row.
\param j Column.
*/
inline constexpr double& Matrix2::operator() (int i, int j)
{
  return r[i + j + j];
}

//! Overloaded.
inline constexpr double Matrix2::operator() (int i, int j) const
{
  return r[i + j + j];
}

/*!
\brief Creates a matrix with a set of double values.
*/
inline Matrix2::Matrix2(const double& a00, const double& a01, const double& a10, const double& a11)
{
  r[0] = a00;
  r[1] = a01;

  r[2] = a10;
  r[3] = a11;
}

/*!
\brief Create a symmetric matrix.
\param a,b Diagonal terms.
\param c Last term.
*/
inline Matrix2::Matrix2(const double& a, const double& b, const double& c) :Matrix2(a, c, c, b)
{
}

/*!
\brief Creates a matrix with the same diagonal value.
\param a The diagonal value.
*/
inline Matrix2::Matrix2(const double& a)
{
  r[1] = r[2] = 0.0;
  r[0] = r[3] = a;
}











/*!
\brief Right multiply by a vector.
*/
inline Vector2 Matrix2::operator*(const Vector2& v) const
{
  return Vector2(v[0] * r[0] + v[1] * r[2], v[0] * r[1] + v[1] * r[3]);
}


//! Overloaded. 
inline Matrix2 operator+(const Matrix2& u, const Matrix2& v)
{
  return Matrix2(u[0] + v[0], u[1] + v[1], u[2] + v[2], u[3] + v[3]);
}


//! Left multiply by a double. 
inline Matrix2 operator*(const double& a, const Matrix2& u)
{
  return Matrix2(a * u[0], a * u[1], a * u[2], a * u[3]);
}


class Matrix {
protected:
  double r[9] = { 1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0 };  //!< The array storing the coefficients of the matrix.
public:
  //! Empty.
  Matrix() {}
  explicit Matrix(const double&);
    explicit Matrix(const Vector&);
    explicit Matrix(const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&);

  static const Matrix Null;
  static const Matrix Identity;

  // Functions to access Matrix class components  
  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  constexpr double& operator() (int, int);
  constexpr double operator() (int, int) const;

    
    
          
  // Binary operators
  friend Matrix operator+ (const Matrix&, const Matrix&);
  friend Matrix operator- (const Matrix&, const Matrix&);
  friend Matrix operator* (const Matrix&, const Matrix&);

  Vector operator*(const Vector&) const;

  friend Matrix operator* (const Matrix&, const double&);
  friend Matrix operator* (const double&, const Matrix&);

  friend Matrix operator/ (const Matrix&, const double&);
  friend Matrix operator/ (const double&, const Matrix&);

  
  
      
      
  
  Vector GetRotationAngles() const;
  
  
  
  friend std::ostream& operator<<(std::ostream&, const Matrix&);

  
  
  
  
  
    
        
  
  
  
    
    
      
  static const Matrix RotationHalfPiZ;   //!< Half-pi rotation around vertical axis.
private:
    private:
  static const double epsilon; //!< Epsilon value used to check angles of a rotation matrix, and in the implementation of some algebra methods.
};

//! Direct access to the array of the matrix.
inline constexpr double& Matrix::operator[] (int i)
{
  return r[i];
}

//! Overloaded.
inline constexpr double Matrix::operator[] (int i) const
{
  return r[i];
}

/*!
\brief Get element (i,j) of the matrix.
\param i Row.
\param j Column.
*/
inline constexpr double& Matrix::operator() (int i, int j)
{
  return r[i + j + j + j];
}

//! Overloaded.
inline constexpr double Matrix::operator() (int i, int j) const
{
  return r[i + j + j + j];
}

/*!
\brief Creates a matrix with a set of double values.

Coefficients are given in column order.
*/
inline Matrix::Matrix(const double& a00, const double& a01, const double& a02, const double& a10, const double& a11, const double& a12, const double& a20, const double& a21, const double& a22)
{
  r[0] = a00;
  r[1] = a01;
  r[2] = a02;

  r[3] = a10;
  r[4] = a11;
  r[5] = a12;

  r[6] = a20;
  r[7] = a21;
  r[8] = a22;
}

/*!
\brief Creates a diagonal matrix.
\param a The diagonal value.
*/
inline Matrix::Matrix(const double& a)
{
  r[1] = r[2] = r[3] = r[5] = r[6] = r[7] = 0.0;
  r[0] = r[4] = r[8] = a;
}


/*!
\brief Create a diagonal matrix with diagonal terms set to the vector entries.
\param a %Vector of diagonal values.
*/
inline Matrix::Matrix(const Vector& a)
{
  r[1] = r[2] = r[3] = r[5] = r[6] = r[7] = 0.0;
  r[0] = a[0];
  r[4] = a[1];
  r[8] = a[2];
}









/*!
\brief Right multiply by a vector.

Right multiplication by a vector is a member function and not a friend to avoid left/right
multiplication ambiguities since matrix multiplication is not commutative.

\param v %Vector.
\code
Vector v=Matrix::Rotation(Vector(0.0,Math::Pi/3.0,Math::Pi/4))*Vector(1.0,1.0,1.0);
\endcode
*/
inline Vector Matrix::operator*(const Vector& v) const
{
  return Vector(v[0] * r[0] + v[1] * r[3] + v[2] * r[6], v[0] * r[1] + v[1] * r[4] + v[2] * r[7], v[0] * r[2] + v[1] * r[5] + v[2] * r[8]);
}











