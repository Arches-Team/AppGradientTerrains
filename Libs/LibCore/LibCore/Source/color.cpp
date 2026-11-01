// Color 

#include "color.h"

/*!
\class Color color.h
\brief %Color with alpha channel in RGB space.

Colors are represented using double.

Several constructors exist, two of them may be confusing:
\code
Color c(127,24,216); // Use constructor with integer parameters in [0,255] range.
Color c(0.5,0.1,0.84); // Use double parameters, in [0.0,1.0] range.
\endcode
\ingroup ColorGroup
*/

const Color Color::Black(0, 0, 0);
const Color Color::White(1.0, 1.0, 1.0);
const Color Color::Red(221, 120, 119);
const Color Color::Blue(97, 130, 234);

const Color Color::Transparent(0.0, 0.0, 0.0, 0.0);/*!
\brief Create a color from Qt representation.
\param c %Color in Qt.
*/
Color::Color(const QRgb& c)
{
  Color::c[0] = qRed(c) / 255.0;
  Color::c[1] = qGreen(c) / 255.0;
  Color::c[2] = qBlue(c) / 255.0;
  Color::c[3] = qAlpha(c) / 255.0;
}