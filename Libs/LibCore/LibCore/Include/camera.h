// Camera

#pragma once

#include "box.h"

// Core camera class
class Camera
{
protected:
  Vector eye = -Vector::X;       //!< Eye.
  Vector at = Vector::Null;        //!< Look at point.
  Vector up = Vector::Z;        //!< Up vector.
  Vector view = Vector::X;      //!< View unit vector.

  double cah = 0.980;       //!< Camera aperture horizontal. 
  double cav = 0.735;       //!< Camera aperture vertical.
  double fl = 35.0;        //!< Focal length.

  double nearplane = 1.0; //!< Near plane.
  double farplane = 100000.0;  //!< Far plane.
public:
  Camera();
  explicit Camera(const Vector&, const Vector & = Vector::Null, const Vector & = Vector::Z, const double& = 1.0, const double& = 1.0, const double& = 1.0, const double& = 100000.0);
  
        
  Vector At() const;
  Vector Eye() const;
  Vector Up() const;
  Vector View() const;

    double GetNear() const;
  double GetFar() const;
        double GetAngleOfViewH() const;
  double GetAngleOfViewV(double, double) const;

  
    
        void SideWay(const double&);
  void BackForth(const double&, bool = false);
  void LeftRightRound(const double&);
  void UpDownRound(const double&);

    
    void SetAt(const Vector&);
  void SetEye(const Vector&);
  void SetPlanes(const double&, const double&);
      
  // Move camera in a plane
  void UpDownVertical(const double&);
  void LeftRightHorizontal(const double&);

  QString ToString(int = 6) const;

  friend std::ostream& operator<<(std::ostream&, const Camera&);

  // Pixel and sub-pixel sampling
  Ray PixelToRay(int, int, int, int) const;
    public:
  static Camera View(const Box&);
};

//! Returns the look-at point.
inline Vector Camera::At() const
{
  return at;
}

//! Returns the eye point.
inline Vector Camera::Eye() const
{
  return eye;
}

//! Returns the up vector.
inline Vector Camera::Up() const
{
  return up;
}

/*!
\brief Returns the viewing direction (not normalized).
*/
inline Vector Camera::View() const
{
  return at - eye;
}





