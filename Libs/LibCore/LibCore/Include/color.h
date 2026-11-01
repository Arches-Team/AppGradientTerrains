// Color

#pragma once

// Qt Color
#include <QtGui/QColor>
#include <QtCore/QVector>

// Mathematics fundamentals
#include "evector.h"

class Color
{
protected:
  double c[4] = { 0.0,0.0,0.0,1.0 }; //!< %Array of color components, with an alpha channel.
public:
  explicit Color(const double& = 0.0);
    explicit Color(const double&, const double&, const double&, const double& = 1.0);
    explicit Color(int, int, int, int = 255);
    explicit Color(const QRgb&);
  
  //! Empty
  ~Color() {}

  
  
  
  friend Color operator+(const Color&, const Color&);
  friend Color operator-(const Color&, const Color&);

  friend Color operator*(const Color&, const double&);
  friend Color operator*(const double&, const Color&);
  friend Color operator/(const Color&, const double&);

  
    
  static Color Lerp(const double&, const Color & = Color::Black, const Color & = Color::White);
  Color Clamp(const Color & = Color::Black, const Color & = Color::White) const;
  
  
  constexpr double& operator[] (int);
  constexpr double operator[] (int) const;

  QColor GetQt() const;

      
  // Stream
  friend std::ostream& operator<<(std::ostream&, const Color&);

  
  
    
    static const Color Black; //!< %Black.
  static const Color White; //!< %White.
  static const Color Red; //!< %Red.
  static const Color Blue; //!< %Blue.

  static const Color Transparent; };

/*!
\brief Returns the i<sup>th</sup> channel of the spectrum
\param i nummber of the channel queried (default=last)
*/
inline constexpr double& Color::operator[] (int i)
{
  return c[i];
}

/*!
\brief Returns a copy of the i<sup>th</sup> channel of the spectrum
\param i nummber of the channel queried (default=last)
*/
inline constexpr double Color::operator[] (int i) const
{
  return c[i];
}


/*!
\brief %Linear interpolation between two colors.

Interpolation is performed in RGB space.

Note that the following lines are equivalent:
\code
Color a,b;
double t;
Color c=(1.0-t)*a+t*b;
Color c=Lerp(t,a,b);
\endcode
*/
inline Color Color::Lerp(const double& t, const Color& a, const Color& b)
{
  return Color((1.0 - t) * a[0] + t * b[0], (1.0 - t) * a[1] + t * b[1], (1.0 - t) * a[2] + t * b[2], (1.0 - t) * a[3] + t * b[3]);
}

/*!
\brief Creates a greyscale color.

Initializes all the components to the given value except the opacity coefficient wich is set to 1.0.
\param v Grey value.
*/
inline Color::Color(const double& v) :Color(v, v, v, 1.0)
{
}


/*!
\brief Creates a color given each of these components
\param r,g,b Red, green and blue components.
\param a Alpha channel, set to 1.0 (opaque) as default.
*/
inline Color::Color(const double& r, const double& g, const double& b, const double& a)
{
  c[0] = r;
  c[1] = g;
  c[2] = b;
  c[3] = a;
}


/*!
\brief Create a color given integer components
\param r, g, b Red, green and blue components.
\param a Alpha channel, set to 255 (opaque) as default.
*/
inline Color::Color(int r, int g, int b, int a) :Color(r / 255.0, g / 255.0, b / 255.0, a / 255.0)
{
}





/*!
\brief Overloaded product by a scalar operator.
*/
inline Color operator*(const Color& u, const double& a)
{
  return Color(u[0] * a, u[1] * a, u[2] * a, u[3] * a);
}



/*!
\brief Clamp a color between two bounds.
\param a, b %Color bounds.
*/
inline Color Color::Clamp(const Color& a, const Color& b) const
{
  return Color(Math::Clamp(c[0], a[0], b[0]), Math::Clamp(c[1], a[1], b[1]), Math::Clamp(c[2], a[2], b[2]), Math::Clamp(c[3], a[3], b[3]));
}

/*!
\brief Create a Qt color from a color.

Example how to write a pixel with a given color in an image:
\code
QImage image;
Color c;
image.setPixel(i, j, c.GetQt().rgb());
\endcode
*/
inline QColor Color::GetQt() const
{
  return QColor(int(255.0 * Math::Clamp(c[0])), int(255.0 * Math::Clamp(c[1])), int(255.0 * Math::Clamp(c[2])), int(255.0 * Math::Clamp(c[3])));
}






