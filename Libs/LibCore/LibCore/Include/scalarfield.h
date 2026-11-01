// Fundamentals
#pragma once

#include <functional>

#include <QtGui/QImage>

#include "array.h"
#include "tetrahedra.h"

class Color;

class Polygons2;
class SegmentSet2;
class Segment2;

class Mesh;
class MeshColor;
class Voxel;

class Sphere;
class Segment;

class ScalarField2;
class ScalarField;

class Rectangles;
class Rectangles2;

class Quadrangle;

class AnalyticScalarField
{
protected:
  bool sign = true; //!< Sign convention, used for normal computation.
  static int ncubes; //!< Number of cubes processed by polygonization algorithms.
public:
    virtual double Value(const Vector&) const;
  virtual Vector Gradient(const Vector&) const;
  
  // Normal
  virtual Vector Normal(const Vector&) const;

      
  
  // Color
  virtual Color GetMaterial(const Vector&, const Vector & = Vector::Null) const;

  virtual Box GetBox() const;

  
  
  // Sampling
  virtual bool GetSample(Vector&, const Box&, Random & = Random::R239) const;
  virtual int GetSamples(QVector<Vector>&, const Box&, int, Random & = Random::R239) const;
  virtual QVector<Vector> Poisson(double, int, Random & = Random::R239) const;

  virtual ScalarField2 Sample(const Rectangles&, int, int) const;
  virtual ScalarField2 Sample(const Quadrangle&, int) const;
  virtual ScalarField Sample(const Box&, int) const;

    
  //! Compute the Lipschitz constant of the field function
  virtual double K() const;
  virtual double K(const Box&) const;
  virtual double K(const Sphere&) const;
  virtual double K(const Segment&) const;

  virtual Voxel K(const Box&, int, int, int) const;

  virtual void Polygonize(int, Mesh&, const Box&, const double& = 1e-4) const;
  virtual void PolygonizeOctree(int, Mesh&, const Box&, const double& = 1e-4) const;
  virtual void PolygonizeLucie(const Box&, Mesh&, bool, bool) const;
  virtual void PolygonizeLucie(const double&, Mesh&, bool, bool) const;
  virtual void Dual(int, Mesh&, const Box&) const;

  virtual bool SphereTrace(const Ray&, const double&, const double&, const double&, double&, int&, const double& = 1e-4) const;

  virtual void Polygonize(const Box&, QVector<Triangle>&, const double& = 1e-4, bool = false) const;
  virtual void Polygonize(const Tetrahedra&, QVector<Triangle>&, const double& = 1e-4) const;
  virtual Mesh PolygonizeOctree(const Box&, int, const double& = 1e-4) const;

  virtual void Voxelize(const Box&, int, Box&, QVector<Vector>&) const;
  virtual void Voxelize(int, Voxel&, const Box&) const;

  // Volume
  virtual double Volume(int = 5) const;
  virtual double Volume(const Box&, int) const;
  virtual double Volume(const Box&, int, double&) const;
  virtual double StochasticVolume(const Box&, int) const;

  // Center of gravity
  virtual Sphere Center(int) const;
  virtual Sphere Center(const Box&, int) const;

  virtual void Colorize(const Mesh&, MeshColor&) const;

    virtual void Project(Mesh&, const double& = 1e-3, const double& = 1e-6, int = 16) const;

  protected:
  protected:
  static const double Epsilon; //!< Epsilon value for partial derivatives
};



class AnalyticScalarField2
{
protected:
  bool sign = true; //!< Sign convention, used for normal computation.
  Box2 box = Box2::Infinity; //!< Domain, set as infinite for base class.
public:
  AnalyticScalarField2(bool = true);
  virtual double Value(const Vector2&) const;
  virtual Vector2 Gradient(const Vector2&) const;
  virtual Matrix2 Hessian(const Vector2&) const;
  virtual ScalarField2 Sample(const Array2&) const;

  
  
  virtual Box2 GetBox() const;
protected:
  Matrix Local(const Vector2&) const;
  static const double epsilon; //!< \f$ \varepsilon \f$ value for first order partial derivatives (gradient).
  static const double epsilonhessian; //!< Larger \f$ \varepsilon \f$ value for second order partial derivatives (hessian).
};

/*!
\brief Constructor.

\param s Sign, by default set sign convention to true, i.e., negative values inside and positive outside.
*/
inline AnalyticScalarField2::AnalyticScalarField2(bool s) :sign(s)
{}

/*!
\brief Return the bounding box.
*/
inline Box2 AnalyticScalarField2::GetBox() const
{
  return box;
}


// Forward declare 
class VectorField2;

class ScalarPoint2
{
protected:
  QPoint p; //!< Point.
  double z; //!< Elevation
public:
  //! Empty.
  ScalarPoint2() :p(0, 0), z(0.0) {}
  explicit ScalarPoint2(const QPoint&, const double&);
  friend bool operator<(const ScalarPoint2& a, const ScalarPoint2& b) { return a.z < b.z; }

  // Access to members
  QPoint Point() const;
  };

/*!
\class ScalarPoint2
\brief Internal class for sorting points.
*/

/*!
\brief Create a scalar point.
\param p Point.
\param s Elevation.
*/
inline ScalarPoint2::ScalarPoint2(const QPoint& p, const double& s) :p(p), z(s)
{}

/*!
\brief Get point.
*/
inline QPoint ScalarPoint2::Point() const
{
  return p;
}







class FloatArray {
protected:
  QVector<float> a; //!< %Array.
public:
  FloatArray() {}
  FloatArray(int);
    float& operator[](int);
};

inline FloatArray::FloatArray(int n)
{
  a.resize(n);
}


/*!
\brief Access to element.
*/
inline float& FloatArray::operator[](int i)
{
  return a[i];
}

class Mesh2;
class PointCurve2;
class Histogram;
class Array2I;

class ScalarField2 :public Array2
{
protected:
  QVector<double> field; //!< Field samples.
public:
  //! Empty.
  ScalarField2() {}
  ScalarField2(const Array2&, const double& = 0.0);
  explicit ScalarField2(const Box2&, int, int, const double& = 0.0);
            
  //! Empty
  virtual ~ScalarField2();

  Array2 GetArray() const;

  void GetRange(double&, double&) const;
      
              
  virtual Vector2 Gradient(int, int) const;
  virtual Vector2 GradientSmooth(int, int) const;
  virtual Matrix2 Hessian(int, int) const;
  virtual double Value(int, int) const;
  virtual double K() const;

  
  // Access to elements
  double at(int, int) const;
  double at(const QPoint&) const;
  double& operator()(int, int);
  double& operator()(const QPoint&);

  double at(int) const;
  double& operator[](int);

    
  // Create image from the scalar field
  QImage CreateImage(bool = true) const;

    
        QImage CreateImage(const double&, const double&, bool = true) const;

    
        
  
    
  virtual double Value(const Vector2&) const;
  virtual double Triangular(const Vector2&) const;
  virtual double BiCubic(const Vector2&) const;
  virtual double BiCubicValue(const Vector2&) const;
  virtual double Closest(const Vector2&) const;

  
  void Fill(const double&);
  
            
  
                
      
  // Functions
  void Normalize();
  
  
    ScalarField2& operator*= (const double&);
  
    
  ScalarField2& operator+=(const ScalarField2&);
  
  
  friend ScalarField2 operator-(const double&, const ScalarField2&);
  friend ScalarField2 operator-(const ScalarField2&, const double&);
  friend ScalarField2 operator-(const ScalarField2&, const ScalarField2&);

  friend ScalarField2 operator+(const ScalarField2&, const ScalarField2&);
  friend ScalarField2 operator+(const ScalarField2&, const double&);

  friend ScalarField2 operator*(const ScalarField2&, const double&);
  friend ScalarField2 operator*(const double&, const ScalarField2&);
  friend ScalarField2 operator*(const ScalarField2&, const ScalarField2&);

  friend ScalarField2 operator/(const ScalarField2&, const double&);
  friend ScalarField2 operator/(const double&, const ScalarField2&);

      void Step(const double&, const double&);
  void SetRange(const double&, const double&);
        
    
    
    
  
          
    
  // Gradient
  VectorField2 Gradient() const;
  VectorField2 GradientSmooth() const;
    
  
  ScalarField2 Sqrt() const;
      void Sqrted();

      
  // Laplacian
  virtual ScalarField2 Laplacian() const;
  virtual double Laplacian(int, int) const;

  QVector<ScalarPoint2> GetScalarPoints() const;
    
  
  
  ScalarField2 DistanceTransform(const double&) const;
    
    
              
    
    
    
  friend std::ostream& operator<<(std::ostream&, const ScalarField2&);
  
  public:
            


protected:
      Matrix Local(int, int) const;

  };


/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline double ScalarField2::at(int i, int j) const
{
  return field.at(VertexIndex(i, j));
}

/*!
\brief Return the field value at a given array vertex.
\param q Point.
*/
inline double ScalarField2::at(const QPoint& q) const
{
  return field.at(VertexIndex(q.x(), q.y()));
}

/*!
\brief Return the field value at a given array vertex.
\param q Point.
*/
inline double& ScalarField2::operator()(const QPoint& q)
{
  return field[VertexIndex(q.x(), q.y())];
}

/*!
\brief Return the field value at a given array vertex.
\param i,j Integer coordinates of the vertex.
*/
inline double& ScalarField2::operator()(int i, int j)
{
  return field[VertexIndex(i, j)];
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline double ScalarField2::at(int c) const
{
  return field.at(c);
}

/*!
\brief Return the data in the field.
\param c Index.
*/
inline double& ScalarField2::operator[](int c)
{
  return field[c];
}


















