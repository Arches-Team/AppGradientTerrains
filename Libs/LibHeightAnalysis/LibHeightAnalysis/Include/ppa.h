#pragma once

#include "heightfield.h"

#include <unordered_map>
#include <unordered_set>
#include <set>

/*!
\class PPA ppa.h
\brief Profile-based Peak/Ridge Analysis (PPA).

Detects ridges in a heightfield by analysing cross-profiles in four directions
(E, NE, N, NW). A pixel is a ridge candidate if it dominates at least one side
of each profile it participates in. Candidate pixels are then connected into
segments, reliability-filtered (a segment is reliable if it is higher than both
parallel neighbours), and assembled into a Minimum Spanning Tree (MST) via
Kruskal's algorithm. Short branches are pruned from the MST to produce a clean
ridge network.

Author: Oscar Argudo.
*/
class PPA
{
public:
  typedef std::pair<QPoint, QPoint> RidgeSegment;  //!< A segment between two adjacent ridge pixels.
  typedef std::set<std::pair<int, int> > EdgeSet;  //!< Set of reliable edges (node-id pairs).
  typedef std::vector<std::unordered_set<int> > RidgesTreeMatrix;  //!< Adjacency list of the ridge MST.

protected:
  const HeightField& hf; //!< Reference to heightfield.
  const int nx = 0, ny = 0; //!< Grid dimensions.
  double hmin = 0.0, hmax = 1.0; //!< Elevation range of the heightfield.

  std::vector<bool> ridgeCandidate; //!< True if pixel (i,j) = node i+j*nx is a ridge candidate.
  std::vector<std::vector<bool>>   isSegment;  //!< isSegment[node][dir]: true if a ridge segment connects node to its neighbour in direction dir.
  std::vector<std::vector<double>> segHeight;  //!< segHeight[node][dir]: average elevation of the segment in direction dir.
  std::vector<std::vector<bool>>   isReliable; //!< isReliable[node][dir]: true if the segment is higher than both parallel neighbours.

  EdgeSet reliableEdgesMST;        //!< Subset of MST edges that are reliable.
  RidgesTreeMatrix ridgesMST;      //!< Full ridge MST adjacency list.
  RidgesTreeMatrix ridgesPrunedMST;//!< Ridge MST after small-branch pruning.

public:
  //! Constructor. Stores a reference to the heightfield and caches its dimensions and elevation range.
  PPA(const HeightField& h);
  virtual ~PPA() {}

  /*!
  \brief Run the full ridge detection pipeline.
  \param profileLength Number of pixels on each side of the cross-profile (total profile = 2*profileLength+1).
  */
  void compute(int profileLength = 5);

  
  
  
  
  //! Return the segments that belong to the pruned ridge MST (short branches removed).
  std::vector<RidgeSegment> getSegmentsInPrunedMST() const;

  
  
protected:

  static const int NUM_DIRS = 4;
  const int PROFILE_DIRS[NUM_DIRS][2] = {
    {1, 0}, // E
    {1, 1}, // NE
    {0, 1}, // N
    {-1, 1} // NW
  };
  typedef std::tuple<double, int, int> RidgeEdge;

  //! Convert a flat node id to grid coordinates: \p i = nodeId % nx, \p j = nodeId / nx.
  void nodeToIndices(int nodeId, int& i, int& j) const {
    i = nodeId % nx;
    j = nodeId / nx;
  }

  /*!
  \brief Mark pixels as ridge candidates.

  A pixel is a candidate if, in at least one of the four profile directions, it is
  strictly higher than at least one pixel on each side within the half-profile window.
  \param profileLength Number of pixels on each side of the cross-profile.
  */
  void computeCandidatePoints(int profileLength = 5);

  /*!
  \brief Connect adjacent ridge candidates into segments.

  For each candidate pixel, marks a segment toward each of its four cardinal/diagonal
  neighbours that are also candidates. When two diagonals cross inside the same grid
  square, only the higher one is kept.
  */
  void connectCandidateSegments();

  /*!
  \brief Classify segments as reliable or unreliable.

  Axis-aligned segments are reliable if their average elevation exceeds that of both
  parallel neighbour segments. Diagonal segments are reliable if higher than both
  corner neighbours perpendicular to the segment.
  \param removeParallels If true, remove the lower parallel/corner segments when a
         reliable one is found.
  */
  void checkReliableSegments(bool removeParallels);

  /*!
  \brief Build the weighted edge list used by Kruskal's MST algorithm.

  Edge weight = hmax − segHeight (so higher segments sort first).
  Reliable segments get weight hmax − h; unreliable get 2·hmax − h, making them
  less preferred.
  \param favourReliables If true, reliable segments receive lower weight (higher priority in MST).
  \return Sorted list of (weight, node1, node2) tuples.
  */
  std::vector<RidgeEdge> getRidgeEdges(bool favourReliables);

  /*!
  \brief Build the ridge Minimum Spanning Tree using Kruskal's algorithm.

  Edges are sorted by weight (higher segments first). Union-Find with path compression
  and union by rank (DisjointSets) ensures O(E α(N)) complexity. Reliable edges are
  recorded in \ref reliableEdgesMST for use during pruning.
  */
  void computeRidgesMST();

  //! Add an undirected edge between \p node1 and \p node2 in \p tree.
  static void addEdge(RidgesTreeMatrix& tree, int node1, int node2);

  //! Remove the undirected edge between \p node1 and \p node2 from \p tree.
  static void removeEdge(RidgesTreeMatrix& tree, int node1, int node2);

  /*!
  \brief Perform one iteration of leaf pruning on the MST.

  Removes every degree-1 node whose single edge is not in \p reliable.
  Operates on a copy so that removal order does not affect which nodes become leaves
  within the same iteration.
  \param ridges  Input adjacency matrix.
  \param reliable Set of edge pairs (node1, node2) that must not be pruned.
  \return Pruned adjacency matrix.
  */
  static RidgesTreeMatrix pruneRidgeLeaves(const RidgesTreeMatrix& ridges, const EdgeSet& reliable);

  /*!
  \brief Remove branches shorter than \p minBranchLength nodes from the MST.

  For each leaf, traces the branch toward the first bifurcation (degree ≥ 3).
  If the branch length is below the threshold, all its edges are removed.
  \param ridges         Input adjacency matrix.
  \param minBranchLength Minimum number of nodes a branch must have to be kept.
  \return Pruned adjacency matrix.
  */
  static RidgesTreeMatrix pruneSmallBranches(const RidgesTreeMatrix& ridges, int minBranchLength);

protected:
  /*!
  \brief Convert a direction index (0–7) to a unit offset (dx, dy).

  Indices 0–3 map to PROFILE_DIRS; indices 4–7 map to their opposites.
  \param dir Direction index (modulo 8).
  \param dx,dy Output offset components.
  */
  void getDirection(int dir, int& dx, int& dy) const;
};

