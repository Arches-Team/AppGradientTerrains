#include "ppa.h"

#include <algorithm>

/*!
\class PPA ppa.h
\brief Profile-based Peak/Ridge Analysis — see class description in ppa.h.
\author Oscar Argoud.
*/

/*!
\brief Construct a PPA analyser for the given heightfield.
\param h Heightfield to analyse. The reference must remain valid for the lifetime of this object.
*/
PPA::PPA(const HeightField& h) : hf(h), nx(hf.GetSizeX()), ny(hf.GetSizeY())
{
  hf.GetRange(hmin, hmax);
}

/*!
\brief Run the full ridge detection pipeline.

Allocates internal arrays, then calls in sequence:
-# computeCandidatePoints()
-# connectCandidateSegments()
-# checkReliableSegments()
-# computeRidgesMST()
-# \p profileLength iterations of pruneRidgeLeaves()
-# pruneSmallBranches()

\param profileLength Number of pixels on each side of the cross-profile
       (total profile width = 2·profileLength + 1). Controls both candidate
       detection sensitivity and the number of leaf-pruning iterations.
*/
void PPA::compute(int profileLength)
{
  ridgeCandidate = std::vector<bool>(nx * ny, false);
  isSegment = std::vector<std::vector<bool>>(nx * ny, std::vector<bool>(NUM_DIRS, false));
  segHeight = std::vector<std::vector<double>>(nx * ny, std::vector<double>(NUM_DIRS, 0.0));
  isReliable = std::vector<std::vector<bool>>(nx * ny, std::vector<bool>(NUM_DIRS, false));

  computeCandidatePoints(profileLength);
  connectCandidateSegments();
  checkReliableSegments(false);

  computeRidgesMST();

  const int pruneLeavesIterations = profileLength;
  const int minLeafBranchSize = profileLength;
  ridgesPrunedMST = RidgesTreeMatrix(ridgesMST);
  for (int i = 0; i < pruneLeavesIterations; i++)
  { // half profile length iterations (recommended by authors)
    //ridgesPrunedMST = pruneRidgeLeaves(ridgesPrunedMST, reliableEdgesMST);
    ridgesPrunedMST = pruneRidgeLeaves(ridgesPrunedMST, EdgeSet()); // do not account for reliables
  }
  ridgesPrunedMST = pruneSmallBranches(ridgesPrunedMST, minLeafBranchSize);
}/*!
\brief Return the segments that belong to the pruned ridge MST.
\return Vector of (start, end) pixel pairs. Each undirected edge appears once.
*/
std::vector<PPA::RidgeSegment> PPA::getSegmentsInPrunedMST() const
{

  std::vector<PPA::RidgeSegment> segs;
  const int n = nx * ny;
  for (int n1 = 0; n1 < n ; n1++)
  {
    for (int n2 : ridgesPrunedMST[n1])
    {
      if (n2 < n1) continue; // avoid duplicates

      int i1, i2, j1, j2;
      nodeToIndices(n1, i1, j1);
      nodeToIndices(n2, i2, j2);
      segs.push_back(RidgeSegment(QPoint(i1, j1), QPoint(i2, j2)));
    }
  }
  return segs;
}

/*!
\brief Convert a direction index (0–7) to a unit offset (dx, dy).

Indices 0–3 map directly to PROFILE_DIRS; indices 4–7 map to their opposites.
\param dir Direction index. Taken modulo 8; negative values are wrapped.
\param dx,dy Output offset components.
*/
inline void PPA::getDirection(int dir, int& dx, int& dy) const
{
  int i = dir % 8;
  if (i < 0) i += 8;

  if (i < 4)
  {
    dx = PROFILE_DIRS[i][0];
    dy = PROFILE_DIRS[i][1];
  }
  else
  {
    dx = -PROFILE_DIRS[i - 4][0];
    dy = -PROFILE_DIRS[i - 4][1];
  }
}

/*!
\brief Mark pixels as ridge candidates by cross-profile analysis.

For each pixel, examines up to four profile directions. A pixel becomes a candidate
if, in at least one direction, there exists a strictly lower pixel within
\p profileLength/2 steps on each side.
\param profileLength Half-profile search radius (pixels on each side).
*/
void PPA::computeCandidatePoints(int profileLength)
{
  int profileHalf = profileLength / 2;

  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      double ctrHeight = hf.at(i, j);

      // check if we have a peak in any of the profile directions
      for (int d = 0; d < NUM_DIRS; d++)
      {

        // a peak implies finding a lower point along either profile half
        bool lowerLeft = false;
        bool lowerRight = false;
        for (int k = 1; k <= profileHalf; k++)
        {
          int ileft = i - PROFILE_DIRS[d][0] * k;
          int jleft = j - PROFILE_DIRS[d][1] * k;
          if (hf.InsideVertexIndex(ileft, jleft) && hf.at(ileft, jleft) < ctrHeight)
            lowerLeft = true;

          int iright = i + PROFILE_DIRS[d][0] * k;
          int jright = j + PROFILE_DIRS[d][1] * k;
          if (hf.InsideVertexIndex(iright, jright) && hf.at(iright, jright) < ctrHeight)
            lowerRight = true;
        }

        // if lower point on both sides, mark as ridge candidate (we can skip other dirs)
        if (lowerLeft && lowerRight)
        {
          ridgeCandidate[hf.VertexIndex(i, j)] = true;
          break;
        }
      }
    }
  }
}

/*!
\brief Connect adjacent ridge candidate pixels into directed segments.

Iterates all candidates and marks a segment in each of the four PROFILE_DIRS
toward any neighbouring candidate. When two diagonal segments cross inside the
same 2×2 square, the lower one is discarded to avoid ambiguity.
*/
void PPA::connectCandidateSegments()
{
  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      int idx = hf.VertexIndex(i, j);
      // if ridge candidate, look for neighbors
      if (ridgeCandidate[idx])
      {

        for (int d = 0; d < NUM_DIRS; d++)
        {
          int in = i + PROFILE_DIRS[d][0];
          int jn = j + PROFILE_DIRS[d][1];
          if (!hf.InsideVertexIndex(in, jn)) continue;

          int idn = hf.VertexIndex(in, jn);
          if (ridgeCandidate[idn])
          {
            isSegment[idx][d] = true;
            segHeight[idx][d] = 0.5 * (hf.at(idx) + hf.at(in, jn));

            // if both diagonals in a square are marked, remove lower one
            int idd = hf.VertexIndex(i - 1, j);
            if (d == 3 && i > 0 && isSegment[idd][1])
            {
              if (segHeight[idx][3] > segHeight[idd][1])
                isSegment[idd][1] = false;
              else
                isSegment[idx][3] = false;
            }
          }
        }
      }
    }
  }
}

/*!
\brief Classify segments as reliable or unreliable.

For axis-aligned segments (dir 0 and 2): reliable if the segment's average elevation
exceeds both left-hand and right-hand parallel neighbours.
For diagonal segments (dir 1 and 3): reliable if higher than both perpendicular corner
neighbours at each endpoint.
\param removeParallels If true, unreliable parallel/corner segments are immediately
       removed from \ref isSegment (aggressive filtering).
*/
void PPA::checkReliableSegments(bool removeParallels)
{
  for (int i = 1; i < nx - 1; i++)
  {
    for (int j = 1; j < ny - 1; j++)
    {
      int idx = hf.VertexIndex(i, j);
      //double h = hf.at(idx);
      for (int d = 0; d < NUM_DIRS; d++)
      {

        if (!isSegment[idx][d]) continue;

        int dx, dy;
        getDirection(d, dx, dy);
        double h = segHeight[idx][d];

        // NS or EW axis
        if (d % 2 == 0)
        {
          // left-hand parallel segment height
          int dx1, dy1;
          getDirection(d + 2, dx1, dy1);
          int i1 = i + dx1, j1 = j + dy1;
          double h1 = 0.5 * (hf.at(i1, j1) + hf.at(i1 + dx, j1 + dy));

          // right-hand parallel segment height
          int dx2, dy2;
          getDirection(d - 2, dx2, dy2);
          int i2 = i + dx2, j2 = j + dy2;
          double h2 = 0.5 * (hf.at(i2, j2) + hf.at(i2 + dx, j2 + dy));

          // reliable if higher than parallel neighbors
          if (h > h1 && h > h2) {
            isReliable[idx][d] = true;
            if (removeParallels) {
              isSegment[hf.VertexIndex(i1, j1)][d] = false;
              isSegment[hf.VertexIndex(i2, j2)][d] = false;
            }
          }
        }
        // diagonal axis
        else
        {
          // left-hand corner height
          int dx1, dy1;
          getDirection(d + 1, dx1, dy1);
          int i1 = i + dx1, j1 = j + dy1;
          double h1 = hf.at(i1, j1);

          // right-hand corner height
          int dx2, dy2;
          getDirection(d - 1, dx2, dy2);
          int i2 = i + dx2, j2 = j + dy2;
          double h2 = hf.at(i2, j2);

          // reliable if higher than opposite corners at either side
          if (h > h1 && h > h2) {
            isReliable[idx][d] = true;
            if (removeParallels) {
              isSegment[hf.VertexIndex(i1, j1)][d] = false;
              isSegment[hf.VertexIndex(i2, j2)][d] = false;

              // opposite direction segment as well
              // note that [(i,j)][d+4] is encoded in [(i,j)+(dxo,dyo)][d]
              int dxo, dyo;
              getDirection(d + 4, dxo, dyo);
              isSegment[hf.VertexIndex(i1 + dxo, j1 + dyo)][d] = false;
              isSegment[hf.VertexIndex(i2 + dxo, j2 + dyo)][d] = false;
            }
          }
        }
      }
    }
  }
}

/*!
\brief Build a weighted edge list for Kruskal's MST.

Weight = hmax − segHeight for reliable segments (lower weight = higher priority),
       = 2·hmax − segHeight for unreliable ones.
Only one directed copy (from lower to higher node id) is needed; the MST builder
handles both directions.
\param favourReliables If true, reliable segments get the lower-cost weight formula.
\return Unsorted vector of (weight, node1, node2) tuples.
*/
std::vector<PPA::RidgeEdge> PPA::getRidgeEdges(bool favourReliables)
{
  // create edges with cost
  std::vector<std::tuple<double, int, int> > edges;

  for (int i = 0; i < nx; i++)
  {
    for (int j = 0; j < ny; j++)
    {
      int id1 = hf.VertexIndex(i, j);
      for (int d = 0; d < NUM_DIRS; d++)
      {
        if (isSegment[id1][d])
        {
          int id2 = hf.VertexIndex(i + PROFILE_DIRS[d][0], j + PROFILE_DIRS[d][1]);
          double w = segHeight[id1][d];

          // reliable: less cost for this segment
          if (favourReliables && isReliable[id1][d])
          {
            w = hmax - w;
          }
          else {
            w = 2 * hmax - w;
          }

          edges.push_back(std::make_tuple(w, id1, id2));
        }
      }
    }
  }

  return edges;
}

// To represent Disjoint Sets
class DisjointSets
{
public:
  std::vector<int> parent;
  std::vector<int> rank;

  DisjointSets(int n) {
    parent = std::vector<int>(n + 1);
    rank = std::vector<int>(n + 1);
    // Initially, all vertices are in different sets and have rank 0
    for (int i = 0; i <= n; i++)
    {
      rank[i] = 0;
      parent[i] = i;
    }
  }

  // Find the parent of a node 'u' and do path compression
  int findParent(int u) {
    // Make the parent of the nodes in the path from u--> parent[u] point to parent[u]
    if (u != parent[u])
      parent[u] = findParent(parent[u]);
    return parent[u];
  }

  // Union by rank
  void merge(int x, int y) {
    x = findParent(x);
    y = findParent(y);

    // Make tree with smaller height a subtree of the other tree
    if (rank[x] > rank[y])
      parent[y] = x;
    else
      parent[x] = y;

    if (rank[x] == rank[y])
      rank[y]++;
  }
};

/*!
\brief Build the ridge Minimum Spanning Tree using Kruskal's algorithm.

Edges are sorted by ascending weight (high-elevation, reliable segments first).
Union-Find with path compression and union by rank gives near-linear complexity.
Edges whose weight is below hmax (i.e. reliable) are also recorded in
\ref reliableEdgesMST for use during pruning.
*/
void PPA::computeRidgesMST()
{
  // get edges
  std::vector<RidgeEdge> edges = getRidgeEdges(true);

  // MST using Kruskal algorithm
  std::sort(edges.begin(), edges.end());
  DisjointSets ds(nx * ny);
  std::vector<RidgeEdge> mstEdges;

  for (auto e : edges) {

    int p1 = std::get<1>(e);
    int p2 = std::get<2>(e);
    int s1 = ds.findParent(p1);
    int s2 = ds.findParent(p2);

    // Check if the selected edge is creating a cycle or not
    // A cycle is created if u and v belong to same set
    if (s1 != s2) {
      // if no cycles, then keep this edge
      mstEdges.push_back(e);

      // merge sets
      ds.merge(s1, s2);
    }
  }

  // build sparse matrix
  ridgesMST.clear();
  ridgesMST.resize(nx * ny);
  for (auto e : mstEdges) {
    int n1 = std::get<1>(e);
    int n2 = std::get<2>(e);
    addEdge(ridgesMST, n1, n2);

    double w = std::get<0>(e);
    if (w < hmax) {
      reliableEdgesMST.insert(std::make_pair(n1, n2));
      reliableEdgesMST.insert(std::make_pair(n2, n1));
    }
  }
}

/*!
\brief Add an undirected edge between \p n1 and \p n2 in \p matrix.
*/
void PPA::addEdge(PPA::RidgesTreeMatrix& matrix, int n1, int n2) {
  matrix[n1].insert(n2);
  matrix[n2].insert(n1);
}

/*!
\brief Remove the undirected edge between \p n1 and \p n2 from \p matrix.
*/
void PPA::removeEdge(PPA::RidgesTreeMatrix& matrix, int n1, int n2) {
  matrix[n1].erase(n2);
  matrix[n2].erase(n1);
}

/*!
\brief Perform one iteration of leaf pruning on the MST.

Every degree-1 node whose single incident edge is not in \p reliable is removed.
The algorithm queries the original graph but edits a copy, so the removal of one
leaf does not cause its neighbour to be treated as a leaf within the same pass
(avoiding the "A–B–C → empty" problem for a line graph).
\param ridges   Input adjacency matrix (read-only).
\param reliable Set of (node1, node2) pairs whose edges must not be removed.
\return New adjacency matrix with qualifying leaves detached.
*/
PPA::RidgesTreeMatrix PPA::pruneRidgeLeaves(const RidgesTreeMatrix& ridges, const EdgeSet& reliable)
{
  // note that we make edits on the copy but always query the original
  // otherwise, unwanted effect happen. For example, imagine we have a
  // tree: A - B - C and want to do do one iteration of leaves removal
  // We should obtain: B
  // If we modify the original and B is processed after removing A,
  // it will be considered a leaf and we will obtain an empty tree

  RidgesTreeMatrix pruned(ridges);
  for (int i = 0; i < ridges.size(); i++) {
    // is this node a leaf?
    if (ridges[i].size() == 1) {
      int neigh = *ridges[i].begin();
      // unless marked as reliable, remove this segment
      if (reliable.find(std::make_pair(i, neigh)) == reliable.end()) {
        removeEdge(pruned, i, neigh);
      }
    }
  }
  return pruned;
}

/*!
\brief Remove branches shorter than \p minBranchLength nodes.

For each leaf node, traces the path toward the first bifurcation (a node with
degree ≥ 3), counting nodes along the way. If the branch contains fewer than
\p minBranchLength nodes, all its edges are removed from the copy.
\param ridges          Input adjacency matrix (read-only).
\param minBranchLength Minimum branch node count to retain.
\return New adjacency matrix with short branches removed.
*/
PPA::RidgesTreeMatrix PPA::pruneSmallBranches(const RidgesTreeMatrix& ridges, int minBranchLength)
{
  RidgesTreeMatrix pruned(ridges);

  for (int i = 0; i < ridges.size(); i++) {
    // is this node a leaf?
    if (ridges[i].size() == 1) {

      // branch until first bifurcation (a node with 3 or more neighbors)
      std::unordered_set<int> branchNodes = { i };
      int prev = i;
      int curr = *ridges[i].begin(); // we know we have exactly 1 neigh
      while (ridges[curr].size() <= 2 && branchNodes.size() < minBranchLength) {

        branchNodes.insert(curr);

        if (ridges[curr].size() < 2) // e.g. a line graph: A - B - Curr
          break;

        for (int next : ridges[curr]) {
          if (branchNodes.find(next) == branchNodes.end()) {
            prev = curr;
            curr = next;
          }
        }
      }

      // delete branch if small
      if (branchNodes.size() < minBranchLength) {
        for (int n : branchNodes) {
          for (int neigh : pruned[n]) {
            removeEdge(pruned, n, neigh);
          }
          pruned[n].clear();
        }
      }
    }
  }

  return pruned;
}
