#pragma once

#include <QtCore/QString>

#include "arrayinteger.h"
#include "mesh.h"


class FlowStruct
{
protected:
  int mask = 0; //!< Integer mask of downstream flow topology.
public:
  int i[8] = { -1 }; //!< Indexes to neighboring cells.
  QPoint q[8] = { QPoint(0,0) }; //!< Flow points.
  double h[8] = { 0.0 }; //!< Heights.
  double s[8] = { 0.0 }; //!< Slopes.
  double sn[8] = { 0.0 }; //!< Normalized slopes.
  int num = 0; //GUNDY ADD NUMBER OF DIRECTIONS IN IT
protected:
  int steepest = -1; //!< Steepest slope index, i.e., index in the arrays storing the steepest slope data.
public:
  explicit FlowStruct() {}
  
  
          
  
  friend class HeightField;
  friend class HeightFieldSediment;
protected:
  static Xoshiro256 fast;
};









class HeightField :public ScalarField2
{
protected:
public:
  //! Empty
  HeightField() {}

  HeightField(const ScalarField2&);
    explicit HeightField(const Box2&, int, int, const double& = 0.0);
  
  //! Empty
  ~HeightField() {}

  double GetHeight(const Vector2&) const;

  // Vertices
  Vector Vertex(const Vector2&, bool = true) const;
  Vector Vertex(int, int) const;
  
    Vector Normal(int, int) const;
        
  double K() const;

  
  // Stream area
  ScalarField2 StreamArea(const double& = 1.0) const;
        
  
      
      
                          
      ScalarField2 Light(const Vector&, bool = false) const;

    
  
    int FillDepressions(const double&);
    int FillDepressions(const double&, ScalarField2&) const;
  void CompleteBreach(const double& = -9999.);

    bool Intersect(const Ray&, double&, Vector&, const Box&, const double&, const double& = 1.0e8, const double& = 1.0e-4) const;
    
      
  
      
  // Box
  Box GetBox() const;

  
      
  
    
    
  // Flow
  int CheckFlowSlope(const QPoint&, FlowStruct&, const double& = 1.0) const;
  
  Mesh CreateMesh(bool = true, bool = false, const double& = 0.0) const;
  Mesh CreateMeshSide(const double& = 0.0) const;

  
          
  
  
  
  
      
          
  
    public:
  static const double flat; //!< Small negative epsilon value used in breaching and flow algorithms.
};


/*!
\brief Compute the height at a given position.
\param p Point.
*/
inline double HeightField::GetHeight(const Vector2& p) const
{
  return Value(p);
}









