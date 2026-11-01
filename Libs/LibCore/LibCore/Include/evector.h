#pragma once
// Vector  

#include <QtCore/QString>

// Mathematics fundamentals
#include "mathematics.h"

class Vector2;

// Class
class Vector
{
protected:
  double c[3] = { 0.0, 0.0, 0.0 }; //!< Components.
public:
  //! Empty 
  Vector() {}

  explicit Vector(const double&);
  explicit Vector(const double&, const double&, const double&);
  
  // Access members
  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

    Vector operator- () const;

  // Assignment operators
  Vector& operator+= (const Vector&);
        Vector& operator*= (const double&);
  Vector& operator/= (const double&);

  // Binary operators
  friend bool operator> (const Vector&, const Vector&);
  friend bool operator< (const Vector&, const Vector&);

  friend bool operator>= (const Vector&, const Vector&);
  friend bool operator<= (const Vector&, const Vector&);

  // Binary operators
  friend Vector operator+ (const Vector&, const Vector&);
  friend Vector operator- (const Vector&, const Vector&);

  friend constexpr double operator* (const Vector&, const Vector&);

  friend Vector operator* (const Vector&, double);
  friend Vector operator* (const double&, const Vector&);
  friend Vector operator/ (const Vector&, double);

  friend Vector operator/ (const Vector&, const Vector&);

  // Boolean functions
  friend bool operator==(const Vector&, const Vector&);
  friend bool operator!=(const Vector&, const Vector&);

  // Norm
  friend double Norm(const Vector&);
  friend double SquaredNorm(const Vector&);
  friend double NormInfinity(const Vector&);

      
  
  friend void Normalize(Vector&);
  friend Vector Normalized(const Vector&);

  
  
  // High level functions
  friend double Sine(const Vector&, const Vector&);
  friend double Cosine(const Vector&, const Vector&);

  // Compare functions
  static Vector Min(const Vector&, const Vector&);
  static Vector Max(const Vector&, const Vector&);
  
  // Abs
  friend Vector Abs(const Vector&);

  
  // Orthogonal and orthonormal vectors
  Vector Orthogonal() const;
    void Orthonormal(Vector&, Vector&) const;

  // Swap
  friend void Swap(Vector&, Vector&);
  friend void Swap(Vector*&, Vector*&);

  friend bool Aligned(const Vector&, const Vector&);
      friend Vector Clamp(const Vector&, const Vector&, const Vector&);
  friend Vector Lerp(const Vector&, const Vector&, const double&);
    
  
  
  // Scale
  Vector Scaled(const Vector&) const;
  Vector Inverse() const;
  
  friend std::ostream& operator<<(std::ostream&, const Vector&);
    
    
    
  
public:
  static const Vector Null; //!< Null vector.
  static const Vector X; //!< Vector(1,0,0).
  static const Vector Y; //!< Vector(0,1,0).
  static const Vector Z; //!< Vector(0,0,1).
};

/*!
\brief Create a vector with the same coordinates.
\param a Real.
*/
inline Vector::Vector(const double& a) :c{ a,a,a }
{
}

/*!
\brief Create a vector with argument coordinates.
\param a,b,c Coordinates.
*/
inline Vector::Vector(const double& a, const double& b, const double& c) :c{ a,b,c }
{
}



//! Gets the i-th coordinate of vector.
inline constexpr double& Vector::operator[] (int i)
{
  return c[i];
}

//! Returns the i-th coordinate of vector.
inline constexpr double Vector::operator[] (int i) const
{
  return c[i];
}


//! Overloaded.
inline Vector Vector::operator- () const
{
  return Vector(-c[0], -c[1], -c[2]);
}

// Assignment unary operators

//! Destructive addition.
inline Vector& Vector::operator+= (const Vector& u)
{
  c[0] += u.c[0]; c[1] += u.c[1]; c[2] += u.c[2];
  return *this;
}


//! Destructive scalar multiply.
inline Vector& Vector::operator*= (const double& a)
{
  c[0] *= a; c[1] *= a; c[2] *= a;
  return *this;
}

/*!
\brief Scale a vector.
\param a Scaling vector.
*/
inline Vector Vector::Scaled(const Vector& a) const
{
  return Vector(c[0] * a[0], c[1] * a[1], c[2] * a[2]);
}

/*!
\brief Inverse of a vector.

This function inverses the components of the vector. This is the same as:
\code
Vector v=Vector(1.0/u[0],1.0/u[1],1.0/u[2]);
\endcode
*/
inline Vector Vector::Inverse() const
{
  return Vector(1.0 / c[0], 1.0 / c[1], 1.0 / c[2]);
}

//! Destructive division by a scalar.
inline Vector& Vector::operator/= (const double& a)
{
  c[0] /= a; c[1] /= a; c[2] /= a;
  return *this;
}



//! Compare two vectors.
inline bool operator> (const Vector& u, const Vector& v)
{
  return ((u.c[0] > v.c[0]) && (u.c[1] > v.c[1]) && (u.c[2] > v.c[2]));
}

//! Compare two vectors.
inline bool operator< (const Vector& u, const Vector& v)
{
  return ((u.c[0] < v.c[0]) && (u.c[1] < v.c[1]) && (u.c[2] < v.c[2]));
}

//! Overloaded
inline bool operator>= (const Vector& u, const Vector& v)
{
  return ((u.c[0] >= v.c[0]) && (u.c[1] >= v.c[1]) && (u.c[2] >= v.c[2]));
}

//! Overloaded
inline bool operator<= (const Vector& u, const Vector& v)
{
  return ((u.c[0] <= v.c[0]) && (u.c[1] <= v.c[1]) && (u.c[2] <= v.c[2]));
}

//! Adds up two vectors.
inline Vector operator+ (const Vector& u, const Vector& v)
{
  return Vector(u.c[0] + v.c[0], u.c[1] + v.c[1], u.c[2] + v.c[2]);
}

//! Difference between two vectors.
inline Vector operator- (const Vector& u, const Vector& v)
{
  return Vector(u.c[0] - v.c[0], u.c[1] - v.c[1], u.c[2] - v.c[2]);
}

//! Scalar product.
inline constexpr double operator* (const Vector& u, const Vector& v)
{
  return (u.c[0] * v.c[0] + u.c[1] * v.c[1] + u.c[2] * v.c[2]);
}

//! Right multiply by a scalar.
inline Vector operator* (const Vector& u, double a)
{
  return Vector(u.c[0] * a, u.c[1] * a, u.c[2] * a);
}

//! Left multiply by a scalar.
inline Vector operator* (const double& a, const Vector& v)
{
  return v * a;
}

//! Cross product.
inline Vector operator/ (const Vector& u, const Vector& v)
{
  return Vector(u.c[1] * v.c[2] - u.c[2] * v.c[1], u.c[2] * v.c[0] - u.c[0] * v.c[2], u.c[0] * v.c[1] - u.c[1] * v.c[0]);
}


// Boolean functions

//! Strong equality test.
inline bool operator== (const Vector& u, const Vector& v)
{
  return ((u.c[0] == v.c[0]) && (u.c[1] == v.c[1]) && (u.c[2] == v.c[2]));
}


/*!
\brief Compute the Euclidean norm of a vector.

This function involves a square root computation, it is in general more efficient to rely on
the squared norm of a vector instead.
\param u %Vector.
\sa SquaredNorm
*/
inline double Norm(const Vector& u)
{
  return sqrt(u.c[0] * u.c[0] + u.c[1] * u.c[1] + u.c[2] * u.c[2]);
}

/*!
\brief Compute the squared Euclidean norm of a vector.
\param u %Vector.
\sa Norm
*/
inline double SquaredNorm(const Vector& u)
{
  return (u.c[0] * u.c[0] + u.c[1] * u.c[1] + u.c[2] * u.c[2]);
}

/*!
\brief Return a normalized vector.

Compute the inverse of its norm and scale the components.

This function does not check if the vector is null.
\param u %Vector.
*/
inline Vector Normalized(const Vector& u)
{
  return u * (1.0 / Norm(u));
}









/*!
\brief Computes the absolute value of a vector.
\param u %Vector.
*/
inline Vector Abs(const Vector& u)
{
  return Vector(u[0] > 0.0 ? u[0] : -u[0], u[1] > 0.0 ? u[1] : -u[1], u[2] > 0.0 ? u[2] : -u[2]);
}

/*!
\brief Return a vector with coordinates set to the minimum coordinates
of the two argument vectors.
*/
inline Vector Vector::Min(const Vector& a, const Vector& b)
{
  return Vector(a[0] < b[0] ? a[0] : b[0], a[1] < b[1] ? a[1] : b[1], a[2] < b[2] ? a[2] : b[2]);
}

/*!
\brief Return a vector with coordinates set to the maximum coordinates
of the two argument vectors.
*/
inline Vector Vector::Max(const Vector& a, const Vector& b)
{
  return Vector(a[0] > b[0] ? a[0] : b[0], a[1] > b[1] ? a[1] : b[1], a[2] > b[2] ? a[2] : b[2]);
}

/*!
\brief Clamp a vector between two bounds.
\param x Input vector
\param a, b %Vector bounds.
*/
inline Vector Clamp(const Vector& x, const Vector& a, const Vector& b)
{
  return Vector(Math::Clamp(x[0], a[0], b[0]), Math::Clamp(x[1], a[1], b[1]), Math::Clamp(x[2], a[2], b[2]));
}

/*!
\brief %Linear interpolation between two vectors.
\param a,b Interpolated points.
\param t Interpolant.
*/
inline Vector Lerp(const Vector& a, const Vector& b, const double& t)
{
  return a + t * (b - a);
}






// Class
class Vector2
{
protected:
  double c[2] = { 0.0, 0.0 }; //!< Components.
public:
  //! Empty.
  Vector2() {}

  explicit Vector2(const double&);
  explicit Vector2(const double&, const double&);
  Vector2(const Vector&);

  double& operator[] (int);
  constexpr double operator[] (int) const;

  Vector2 Orthogonal() const;

    Vector2 operator- () const;

  // Assignment operators
  Vector2& operator+= (const Vector2&);
        Vector2& operator*= (double);
  
  // Binary operators
  friend bool operator> (const Vector2&, const Vector2&);
  friend bool operator< (const Vector2&, const Vector2&);

  friend bool operator>= (const Vector2&, const Vector2&);
  friend bool operator<= (const Vector2&, const Vector2&);

  // Binary operators
  friend Vector2 operator+ (const Vector2&, const Vector2&);
  friend Vector2 operator- (const Vector2&, const Vector2&);

  friend double operator* (const Vector2&, const Vector2&);

  friend Vector2 operator* (const Vector2&, double);
  friend Vector2 operator* (double, const Vector2&);
  friend Vector2 operator/ (const Vector2&, double);

  friend double operator/ (const Vector2&, const Vector2&);

  // Boolean functions
  friend bool operator==(const Vector2&, const Vector2&);
  friend bool operator!=(const Vector2&, const Vector2&);

  /*!
 \brief Weak equality test.
  */
  friend bool nearEqual(const Vector2& u, const Vector2& v, const double& eps = 1e-4)
  {
      return std::fabs(u[0] - v[0]) < eps && std::fabs(u[1] - v[1]) < eps;
  }


  // Norm
  friend double Norm(const Vector2&);
  friend double SquaredNorm(const Vector2&);
  friend double NormInfinity(const Vector2&);

      
  friend void Normalize(Vector2&);
  friend Vector2 Normalized(const Vector2&);

  
  
  // Conversion
  Vector ToVector(const double& = 0.0) const;

  // High level functions
  friend double Sine(const Vector2&, const Vector2&);
  friend double Cosine(const Vector2&, const Vector2&);
  Vector2 Inverse() const;

    
  
  // Abs
  friend Vector2 Abs(const Vector2&);

  
  
  // Swap
  friend void Swap(Vector2&, Vector2&);

  friend Vector2 Clamp(const Vector2&, const Vector2&, const Vector2&);
    static Vector2 Bilinear(const Vector2&, const Vector2&, const Vector2&, const Vector2&, const double&, const double&);

  // Position of a point
  friend double WhichSide(const Vector2&, const Vector2&, const Vector2&);

  friend std::ostream& operator<<(std::ostream&, const Vector2&);
    
      
  static Vector2 Polar(const double&);

  double Angle(const Vector2&) const;
  double Angle() const;

        
    public:
  static const Vector2 Null; //!< Null vector.
  static const Vector2 X; //!< Vector2(1,0).
  static const Vector2 Y; //!< Vector2(0,1).
};

/*!
\brief Create a vector with the same real coordinates.
\param a Real.
*/
inline Vector2::Vector2(const double& a) :c{ a,a }
{
}

//! Create a vector with argument coordinates.
inline Vector2::Vector2(const double& a, const double& b) : c{ a,b }
{
}


//! Create a two dimension vector from another three dimension vector.
inline Vector2::Vector2(const Vector& v) : c{ v[0],v[1] }
{
}

//! Gets the i-th coordinate of vector.
inline double& Vector2::operator[] (int i)
{
  return c[i];
}

//! Returns the i-th coordinate of vector.
inline constexpr double Vector2::operator[] (int i) const
{
  return c[i];
}

/*!
\brief Convert a Vector2 to a Vector.
\param z Extra coordinate.
*/
inline Vector Vector2::ToVector(const double& z) const
{
  return Vector(c[0], c[1], z);
}



//! Overloaded.
inline Vector2 Vector2::operator- () const
{
  return Vector2(-c[0], -c[1]);
}

// Assignment unary operators

//! Destructive addition.
inline Vector2& Vector2::operator+= (const Vector2& u)
{
  c[0] += u.c[0]; c[1] += u.c[1];
  return *this;
}


//! Destructive scalar multiply.
inline Vector2& Vector2::operator*= (double a)
{
  c[0] *= a; c[1] *= a;
  return *this;
}




//! Compare two vectors.
inline bool operator> (const Vector2& u, const Vector2& v)
{
  return ((u.c[0] > v.c[0]) && (u.c[1] > v.c[1]));
}

//! Compare two vectors.
inline bool operator< (const Vector2& u, const Vector2& v)
{
  return ((u.c[0] < v.c[0]) && (u.c[1] < v.c[1]));
}

//! Overloaded
inline bool operator>= (const Vector2& u, const Vector2& v)
{
  return ((u.c[0] >= v.c[0]) && (u.c[1] >= v.c[1]));
}

//! Overloaded
inline bool operator<= (const Vector2& u, const Vector2& v)
{
  return ((u.c[0] <= v.c[0]) && (u.c[1] <= v.c[1]));
}

//! Adds up two vectors.
inline Vector2 operator+ (const Vector2& u, const Vector2& v)
{
  return Vector2(u.c[0] + v.c[0], u.c[1] + v.c[1]);
}

//! Difference between two vectors.
inline Vector2 operator- (const Vector2& u, const Vector2& v)
{
  return Vector2(u.c[0] - v.c[0], u.c[1] - v.c[1]);
}

/*!
\brief Dot product between two vectors.
\param u,v Argument vectors.
*/
inline double operator* (const Vector2& u, const Vector2& v)
{
  return u.c[0] * v.c[0] + u.c[1] * v.c[1];
}

//! Right multiply by a scalar.
inline Vector2 operator* (const Vector2& u, double a)
{
  return Vector2(u.c[0] * a, u.c[1] * a);
}

//! Left multiply by a scalar.
inline Vector2 operator* (double a, const Vector2& v)
{
  return v * a;
}

/*!
\brief Cross product of two vectors.
Note that the determinant of a 2×2 matrix is the cross product of its two column vectors.
*/
inline double operator/ (const Vector2& u, const Vector2& v)
{
  return u.c[0] * v.c[1] - u.c[1] * v.c[0];
}

//! Left divide by a scalar
inline Vector2 operator/ (const Vector2& u, double a)
{
  return Vector2(u.c[0] / a, u.c[1] / a);
}



/*!
\brief Compute the Euclidean norm of a vector.

This function involves a square root computation, it is often more efficient to rely on
the squared norm of a vector instead. \sa SquaredNorm
*/
inline double Norm(const Vector2& u)
{
  return sqrt(u.c[0] * u.c[0] + u.c[1] * u.c[1]);
}

/*!
\brief Compute the squared Euclidean norm of a vector.
\sa Norm
*/
inline double SquaredNorm(const Vector2& u)
{
  return (u.c[0] * u.c[0] + u.c[1] * u.c[1]);
}

/*!
\brief Return a normalized vector, computing the inverse of its norm and scaling the components.

This function does not check if
the vector is null, which might result in errors.
*/
inline Vector2 Normalized(const Vector2& u)
{
  return u * (1.0 / Norm(u));
}


/*!
\brief Computes the absolute value of a vector.
*/
inline Vector2 Abs(const Vector2& u)
{
  return Vector2(u[0] > 0.0 ? u[0] : -u[0], u[1] > 0.0 ? u[1] : -u[1]);
}







/*!
\brief Clamp a Vector2 between two bounds.
\param x Input vector
\param a, b Vector bounds.
*/
inline Vector2 Clamp(const Vector2& x, const Vector2& a, const Vector2& b)
{
  return Vector2(Math::Clamp(x[0], a[0], b[0]), Math::Clamp(x[1], a[1], b[1]));
}

/*!
\brief Creates a vector given polar coordinates.
\param t Theta.
*/
inline Vector2 Vector2::Polar(const double& t)
{
  return Vector2(cos(t), sin(t));
}




/*!
\brief Returns a direct orthogonal vector.
*/
inline Vector2 Vector2::Orthogonal() const
{
  return Vector2(-c[1], c[0]);
}







/*!
\brief Bi-linear interpolation between four vectors.

The vectors are given in trigonometric order.

\param a00,a10,a11,a01 Interpolated vectors.
\param u,v Interpolation coefficients.

\sa Math::Bilinear
*/
inline Vector2 Vector2::Bilinear(const Vector2& a00, const Vector2& a10, const Vector2& a11, const Vector2& a01, const double& u, const double& v)
{
  return (1.0 - u) * (1.0 - v) * a00 + (1.0 - u) * v * a01 + (u) * (1.0 - v) * a10 + u * v * a11;
}


/*!
\brief Inverse of a vector.

This function inverses the components of the vector.
\sa Vector::Inverse()
*/
inline Vector2 Vector2::Inverse() const
{
  return Vector2(1.0 / c[0], 1.0 / c[1]);
}



