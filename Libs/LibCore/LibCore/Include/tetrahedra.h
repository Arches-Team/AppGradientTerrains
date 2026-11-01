// Tetrahedras

#pragma once

#include "triangle.h"

class Tetrahedra
{
protected:
  Vector p[4]; //!< %Array of vertices.
  Vector n[4]; //!< Normals to the faces.
public:
      
  
        
  constexpr int VertexIndex(int, int) const;

        
    
    
        
      
    
  friend std::ostream& operator<<(std::ostream&, const Tetrahedra&);
    public:
  static constexpr double epsilon = 1e-7; //!< Internal epsilon constant.
protected:
  static constexpr int vertex[4][3] = {
  { 0, 1, 2 },
  { 0, 3, 1 },
  { 0, 2, 3 },
  { 1, 3, 2 }
  }; //!< %Array of indexes defining the faces of the tetrahedron.
  static constexpr int split[6][4] = {
  { 0, 1, 5, 7 },
  { 0, 1, 3, 7 },
  { 0, 3, 7, 2 },
  { 1, 6, 7, 2 },
  { 1, 6, 4, 7 },
  { 1, 4, 7, 5 }
  }; //!< Indexes of the vertexes of the sub-tetrahedron of a cube.
};





/*!
\brief Return the index of the j-th vertex of the i-th triangle.
\param i %Triangle index.
\param j Vertex index.
*/
inline constexpr int Tetrahedra::VertexIndex(int i, int j) const
{
  return vertex[i][j];
}
