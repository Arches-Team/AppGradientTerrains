// Fundamentals

#pragma once

#include <math.h>

#include <iostream>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

/*!
\brief Minimum of two integers.
*/
inline int min(int a, int b)
{
  return (a < b ? a : b);
}

/*!
\brief Minimum of two unsigned integers.
*/
inline unsigned int min(unsigned int a, unsigned int b)
{
  return (a < b ? a : b);
}

/*!
\brief Maximum of two integers.
*/
inline int max(int a, int b)
{
  return (a > b ? a : b);
}

/*!
\brief Minimum of two doubles.
*/
inline double min(const double& a, const double& b)
{
  return (a < b ? a : b);
}

/*!
\brief Maximum of two doubles.
*/
inline double max(const double& a, const double& b)
{
  return (a > b ? a : b);
}

/*!
\brief Minimum of two doubles.
*/
inline float min(const float& a, const float& b)
{
  return (a < b ? a : b);
}

/*!
\brief Maximum of two doubles.
*/
inline float max(const float& a, const float& b)
{
  return (a > b ? a : b);
}

/*!
\brief Minimum of three integers.
*/
inline int min(int a, int b, int c)
{
  return min(min(a, b), c);
}

/*!
\brief Maximum of three integers.
*/
inline int max(int a, int b, int c)
{
  return max(max(a, b), c);
}

/*!
\brief Minimum of four integers.
*/
inline int min(int a, int b, int c, int d)
{
  return min(min(a, b), min(c, d));
}

/*!
\brief Minimum of four unsigned integers.
*/
inline unsigned int min(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
  return min(min(a, b), min(c, d));
}

/*!
\brief Maximum of four integers.
*/
inline int max(int a, int b, int c, int d)
{
  return max(max(a, b), max(c, d));
}

//! Clamps an integer value between two bounds.
inline int Clamp(int x, int a, int b)
{
  return (x < a ? a : (x > b ? b : x));
}



//! Clamps a float value between two bounds.
inline float Clamp(float x, float a, float b)
{
  return (x < a ? a : (x > b ? b : x));
}

//! Clamps a double value between two bounds.
inline double Clamp(double x, double a, double b)
{
  return (x < a ? a : (x > b ? b : x));
}

// Math class for constants
class Math
{
public:
  static constexpr double Pi = 3.14159265358979323846; //!< \f$ \pi \f$.
  static constexpr double TwoPi = 6.28318530717958647693; //!< \f$ 2\pi \f$.
  static constexpr double HalfPi = Math::Pi / 2.0; //!< \f$ \pi/2 \f$.
  static constexpr double E = 2.7182818284590452354; //!< \f$ e \f$.
  static constexpr double TwoPiOverThree = 2.0943951023931954923084; //!< \f$ 2/3 \pi \f$.
  static constexpr double FourPiOverThree = 4.1887902047863909846168; //!< \f$ 4/3 \pi \f$.
  static constexpr double Sqrt5 = 2.23606797749978969640917366873127623; //!< Constant \f$ \sqrt{5} \f$.
  static constexpr double Sqrt3 = 1.73205080756887729352744634150587236; //!< Constant \f$ \sqrt{3} \f$.
  static constexpr double Sqrt2 = 1.41421356237309504880168872420969807; //!< Constant \f$ \sqrt{2} \f$.
  static constexpr double InvSqrt2 = 1.0 / Math::Sqrt2; //!< Constant \f$  1/\sqrt{2} \f$.
  static constexpr double Golden = (Math::Sqrt5 + 1.0) / 2.0; //!< Constant \f$  (\sqrt{5}+1)/2 \f$.
  static constexpr double Large = 1.0e20; //!< Large constant \f$ 10^{20} \f$.
  static constexpr double Infinity = 1.7976931348623158e+308; //!< \f$ \infty \f$.
  static constexpr double InfinityEpsilon = 2.2250738585072014e-308; //!< Smallest positive  \f$ \epsilon \f$.

  static constexpr int binomials[16][16] = {
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 4, 6, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 5, 10, 10, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 6, 15, 20, 15, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 7, 21, 35, 35, 21, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 8, 28, 56, 70, 56, 28, 8, 1, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 9, 36, 84, 126, 126, 84, 36, 9, 1, 0, 0, 0, 0, 0, 0 },
  { 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1, 0, 0, 0, 0, 0 },
  { 1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1, 0, 0, 0, 0 },
  { 1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1, 0, 0, 0 },
  { 1, 13, 78, 286, 715, 1287, 1716, 1716, 1287, 715, 286, 78, 13, 1, 0, 0 },
  { 1, 14, 91, 364, 1001, 2002, 3003, 3432, 3003, 2002, 1001, 364, 91, 14, 1, 0 },
  { 1, 15, 105, 455, 1365, 3003, 5005, 6435, 6435, 5005, 3003, 1365, 455, 105, 15, 1 }
  }; //!< %Array of binomial coefficients.

public:
  static double Clamp(const double&, const double& = 0.0, const double& = 1.0);
  static int Clamp(int, int = 0, int = 255);

    static constexpr double Cube(const double&);
      static double Pow(const double&, const double&);
      
    
  // Modulo
  static double Mod(const double&, const double&);

        
  // Minimum and maximum
  static double Min(const double&, const double&);
  static double Max(const double&, const double&);
      static double Min(const double&, const double&, const double&, const double&);
  static double Max(const double&, const double&, const double&, const double&);
  
  // Angles
  static constexpr double DegreeToRadian(const double&);
  static constexpr double RadianToDegree(const double&);

  static constexpr double Angle(int, int);
  static double Angle(const double&);
      
    
  
    
      
  // Linear interpolation
  static double Lerp(const double&, const double&, const double&);
  static double Bilinear(const double&, const double&, const double&, const double&, const double&, const double&);
  
      static double BiCubic(const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double&, const double& = 0.0, const double& = 0.0, const double& = 0.0, const double& = 0.0);
  
    
  
    
    
  static int Integer(const double&);

    static void Sort(double&, double&);
      
  static void Swap(int&, int&);
  static void Swap(double&, double&);

    
          
  
  static constexpr double Unit(int, int);

            
  
  static void Swap(double*&, double*&);
  static void Swap(int*&, int*&);

    
    
  };


/*!
\brief Bi-linear interpolation between four values.

The values are given in trigonometric order.

\image html bilinear.png
\param a00, a10, a11, a01 Interpolated values.
\param u,v Interpolation coefficients.
*/
inline double Math::Bilinear(const double& a00, const double& a10, const double& a11, const double& a01, const double& u, const double& v)
{
  return (1.0 - u) * (1.0 - v) * a00 + (1.0 - u) * (v)*a01 + (u) * (1.0 - v) * a10 + (u) * (v)*a11;
}



/*!
\brief Cubes a double value.
\param x Real value.
*/
inline constexpr double Math::Cube(const double& x)
{
  return x * x * x;
}





/*!
\brief Sort two reals.

\sa Math::Swap(double&,double&)
\param a, b Real arguments, which will be swapped so that a<=b.
*/
inline void Math::Sort(double& a, double& b)
{
  if (a > b)
  {
    double c = a;
    a = b;
    b = c;
  }
}




/*!
\brief Linear interpolation.

Returns (1-t)a+tb.

\param a,b Interpolated values.
\param t Interpolant.
*/
inline double Math::Lerp(const double& a, const double& b, const double& t)
{
  return a + t * (b - a);
}

/*!
\brief Convert degrees to radians.
\param a Angle in degrees.
*/
inline constexpr double Math::DegreeToRadian(const double& a)
{
  return a * Math::Pi / 180.0;
}

/*!
\brief Convert radian to degrees.
\param a Angle in radian.
*/
inline constexpr double Math::RadianToDegree(const double& a)
{
  return a * 180.0 / Math::Pi;
}

/*!
\brief Compute \f$ 2 k \pi / n \f$.
\param k, n Integers.
*/
inline constexpr double Math::Angle(int k, int n)
{
  return (Math::TwoPi * k) / n;
}

/*!
\brief Swap two reals.
\sa Sort(double&,double&)
\param a, b Arguments.
*/
inline void Math::Swap(double& a, double& b)
{
  double t = a;
  a = b;
  b = t;
}

/*!
\brief Swap two integers.
\sa Sort(int&,int&)
\param a, b Arguments.
*/
inline void Math::Swap(int& a, int& b)
{
  int t = a;
  a = b;
  b = t;
}

/*!
\brief Swap two integers.
\sa Swap(double&,double&)
\param a, b Arguments.
*/
inline void Swap(int& a, int& b)
{
  int t = a;
  a = b;
  b = t;
}

/*!
\brief Sort two integers.
\sa Swap(int&,int&)
\param a, b Arguments.
*/
inline void Sort(int& a, int& b)
{
  if (a > b)
  {
    Swap(a, b);
  }
}

/*!
\brief Compute the integer part of a real.

This function handles negative values differently by subtracting 1 from the result.
\param x %Real.
*/
inline int Math::Integer(const double& x)
{
  return (x > 0.0 ? int(x) : int(x) - 1);
}

/*!
\brief Modulus for reals with negative values handled properly.
\param x %Real.
\param a Modulo.
*/
inline double Math::Mod(const double& x, const double& a)
{
  return fmod(fmod(x, a) + a, a);
}

/*!
\brief Angle strictly in [0,2\pi[.
\param a %Angle.
*/
inline double Math::Angle(const double& a)
{
  //return a>0.0? fmod(a, Math::TwoPi):Math::TwoPi-fmod(-a, Math::TwoPi);
  return Math::Mod(a, Math::TwoPi);
}


/*!
\brief Clamp a double value between two bounds.
\param x Input value.
\param a, b Lower and upper bounds.
*/
inline double Math::Clamp(const double& x, const double& a, const double& b)
{
  return (x < a ? a : (x > b ? b : x));
}

/*!
\brief Clamp an integer value between two bounds.
\param x Input value.
\param a, b Lower and upper bounds.
*/
inline int Math::Clamp(int x, int a, int b)
{
  return (x < a ? a : (x > b ? b : x));
}


/*!
\brief Minimum of two reals.
\param a, b Real values.
*/
inline double Math::Min(const double& a, const double& b)
{
  return (a < b ? a : b);
}

/*!
\brief Maximum of two reals.
\param a, b Real values.
*/
inline double Math::Max(const double& a, const double& b)
{
  return (a > b ? a : b);
}



/*!
\brief Maximum of four reals.
\param a, b, c, d Real values.
*/
inline double Math::Max(const double& a, const double& b, const double& c, const double& d)
{
  return Math::Max(Math::Max(a, b), Math::Max(c, d));
}

/*!
\brief Minimum of four reals.
\param a, b, c, d Real values.
*/
inline double Math::Min(const double& a, const double& b, const double& c, const double& d)
{
  return Math::Min(Math::Min(a, b), Math::Min(c, d));
}





/*!
\brief Power.

Returns base x raised to the power exponent e if x>0, and the opposite of -x raised to the power exponent e otherwise
(odd extension, the sign of x is kept).

\param x Base.
\param ee Exponent.
*/
inline double Math::Pow(const double& x, const double& ee)
{
  if (x == 0.0)
  {
    return 0.0;
  }
  else if (x > 0.0)
  {
    return pow(x, ee);
  }
  else
  {
    return -pow(-x, ee);
  }
}


/*!
\brief Unit real value in [0,1] from two integers.

Usefull for loops, same as:
\code
for (int i=0;i<n;i++)
{
double t=double(i)/double(n-1); // t=Math::Unit(i,n);
}
\endcode
\param i, n Integers.
*/
inline constexpr double Math::Unit(int i, int n)
{
  return double(i) / double(n - 1);
}














/*!
\brief Swap two reals.
\param a,b Arguments.
*/
inline void Math::Swap(double*& a, double*& b)
{
  double* t = a;
  a = b;
  b = t;
}

/*!
\brief Swap two integers.
\param a,b Arguments.
*/
inline void Math::Swap(int*& a, int*& b)
{
  int* t = a;
  a = b;
  b = t;
}









