// Generic mesh operations, such as neighborhood finding and
// area computation.

#ifndef MESHOP_H
#define MESHOP_H

#include <OpenMesh/Mesh/Types/TriMesh_ArrayKernelT.hh>
#include <OpenMeshTools/Geometry/Algorithms.hh>
#include <vector>

/// find n ring neighbors: assumes the vector is empty, also assumes the mesh's tag are set to false except the seleced one.
template <class Mesh>
void findNRingNeighborhood(Mesh &mesh, std::vector <Mesh::VertexHandle> &initVertices, int selectionNeighborDepth,
						   std::vector < std::vector <Mesh::VertexHandle> > &selectedVertices)
{
	if (selectionNeighborDepth<=0)
		return;
	// a new vector to store a level
	std::vector <Mesh::VertexHandle> currentLevel;
	// find the 1 ring first.
	for (unsigned int i=0; i<initVertices.size(); i++)
	{
		Mesh::VertexHandle &vh=initVertices[i];
		for (Mesh::VertexVertexIter vv_it=mesh.vv_iter(vh); vv_it ; ++vv_it)
		{
			if (!vv_it->tagged())
			{
				currentLevel.push_back(vv_it.handle());
				vv_it->set_tagged(true);
			}
		}
	}
	// add vector to vector
	selectedVertices.push_back(currentLevel);
	// find 1 ring of that
	findNRingNeighborhood<Mesh>(mesh, currentLevel, selectionNeighborDepth-1, selectedVertices);
}


// Returns V_2(i)
template <class Mesh>
std::vector<Mesh::VertexHandle> findTwoRingNeighborhood(Mesh& mesh, Mesh::VertexHandle vh)
{
	Mesh::VertexOHalfedgeIter he_it = mesh.voh_iter(vh);

	typedef std::vector<Mesh::VertexHandle> Handles;
	Handles vertices;	

	// For every vertex, loop through all outgoing edges
	for ( ; he_it; ++he_it)
	{
		Mesh::HalfedgeHandle outgoing_heh = he_it.handle();

		// Get vertex handle of vertex on 1-ring neighborhood
		Mesh::VertexHandle outer_vh = mesh.to_vertex_handle(outgoing_heh);
		vertices.push_back(outer_vh);

		// If outgoing half-edge is on boundary, don't try to find 2-ring neighbor
		if ( !mesh.is_boundary(outgoing_heh) )
		{
			// Get next ccw half-edge
			Mesh::HalfedgeHandle heh = mesh.next_halfedge_handle(he_it.handle());
			// Get opposite
			Mesh::HalfedgeHandle opp_heh = mesh.opposite_halfedge_handle(heh);

			// If not on boundary, find the 2-ring neighbor
			if ( !mesh.is_boundary(opp_heh) )
			{
				opp_heh = mesh.next_halfedge_handle(opp_heh);
				Mesh::VertexHandle two_ring_vh = mesh.to_vertex_handle(opp_heh);				
				vertices.push_back(two_ring_vh);
			}
		}
	}

	// Remove duplicates
	sort(vertices.begin(), vertices.end());
	Handles::iterator it = unique(vertices.begin(), vertices.end());
	vertices.erase(it, vertices.end());

	return vertices;
}

// Returns E_2(i)
template <class Mesh>
std::vector<Mesh::HalfedgeHandle> findE2Neighborhood(Mesh& mesh, Mesh::VertexHandle vh)
{
	Mesh::VertexOHalfedgeIter he_it = mesh.voh_iter(vh);

	std::vector<Mesh::HalfedgeHandle> edges;

	// For every vertex, loop through all outgoing edges
	for ( ; he_it; ++he_it)
	{
		Mesh::HalfedgeHandle outgoing_heh = he_it.handle();

		edges.push_back(outgoing_heh);

		// If outgoing half-edge is on boundary, don't try to find 2-ring neighbor
		if ( !mesh.is_boundary(outgoing_heh) )
		{
			// Get next ccw half-edge
			Mesh::HalfedgeHandle heh = mesh.next_halfedge_handle(he_it.handle());

			edges.push_back(heh);
		}
	}
	return edges;
}

// Returns true iff omega(heh) contains vh
template <class Mesh>
bool diamondContains(Mesh& mesh, Mesh::HalfedgeHandle heh, Mesh::VertexHandle vh)
{
	if (mesh.to_vertex_handle(heh) == vh ||
		mesh.from_vertex_handle(heh) == vh)
	{
		return true;
	}

	Mesh::HalfedgeHandle opp_heh = mesh.opposite_halfedge_handle(heh);

	if ( !mesh.is_boundary(heh) ) {
		Mesh::HalfedgeHandle next_heh = mesh.next_halfedge_handle(heh);
		if (mesh.to_vertex_handle(next_heh) == vh) return true;
	}

	if ( !mesh.is_boundary(opp_heh) ) {
		Mesh::HalfedgeHandle opp_next_heh = mesh.next_halfedge_handle(opp_heh);
		if (mesh.to_vertex_handle(opp_next_heh) == vh) return true;	
	}

	return false;
}

// Returns vertices in omega(heh)
template <class Mesh>
std::vector<Mesh::VertexHandle> findDiamond(Mesh& mesh, Mesh::HalfedgeHandle heh)
{
	std::vector<Mesh::VertexHandle> vertices;

	vertices.push_back(mesh.to_vertex_handle(heh));

	vertices.push_back(mesh.from_vertex_handle(heh));

	Mesh::HalfedgeHandle opp_heh = mesh.opposite_halfedge_handle(heh);

	if ( !mesh.is_boundary(heh) ) {
		Mesh::HalfedgeHandle next_heh = mesh.next_halfedge_handle(heh);
		vertices.push_back(mesh.to_vertex_handle(next_heh));
	}

	if ( !mesh.is_boundary(opp_heh) ) {
		Mesh::HalfedgeHandle opp_next_heh = mesh.next_halfedge_handle(opp_heh);
		vertices.push_back(mesh.to_vertex_handle(opp_next_heh));	
	}

	return vertices;
}

// Adapted from <OpenMeshTools/Geometry/Algorithms.hh>
// See http://www.magic-software.com/Documentation/DistancePointLine.pdf
template<class Vec>
typename Vec::value_type
distPointLineProj( const Vec& _p,
		      const Vec& _v0, 
			  const Vec& _v1,
		      Vec*       _min_v,
			  Vec::value_type& t)
{
	Vec d1(_p-_v0), d2(_v1-_v0), min_v(_v0);
	
	t = (d1|d2)/(d2|d2);

	// Project p to line formed by v0 + t*v1
	d1 = _p - (min_v = _v0 + d2*t);

	// Note that t may be greater than unity or less than zero

	if (_min_v) *_min_v = min_v;
	return sqrt(d1.sqrnorm());
}    

// Computes the "hinge" area formed by the triangle [j, L1, L2].
// We define this area to be always non-negative.
template <class Vec>
typename Vec::value_type
computeHingeArea(Vec j, Vec k, Vec L1, Vec L2)
{
	// Instead of rotation j such that [j, k, L1] and [j, k, L2]
	// are coplanar, we rely on similar triangles.
	//
	// We project L1 and L2 to the line j<->k. This gives us two
	// similar triangles, [L1, proj_L1, intersectionPt] and
	// [L2, proj_L2, intersectionPt], where intersectionPt is the
	// point where the lines j<->k and L1<->L2 intersect, assuming
	// that we have rotated the triangles to be coplanar.
	//
	// We do not actually rotate. We know length(L1, proj_L1) and
	// length(L2, proj_L2) as well as length(proj_L1, proj_L2). Simple
	// algebraic ratios formed by the similar triangles give us 
	// intersectionT, which gives us intersectionPt.

	typedef Vec::value_type Scalar;
    Vec proj_L1, proj_L2;	
	Scalar t1, t2;

	// Project to line j<->k
	Scalar distL1 = distPointLineProj(L1, j,k, &proj_L1, t1);
	Scalar distL2 = distPointLineProj(L2, j,k, &proj_L2, t2);

	// Compute ratio for similar triangles
	Scalar ratio = distL1/(distL1+distL2);

	// Compute intersectionT on line segment j->k.
	// Clamp to [0, 1] to ensure that weights for coplanar
	// triangles sum to unity.
	Scalar dt = t2 - t1;	Scalar intersectionT = t1 + dt * ratio;		intersectionT = ::min(intersectionT, Scalar(1.0));	if (intersectionT <= Scalar(0.0)) {		return Scalar(0.0);	} else {		Vec intersectionPt = j + (k-j)*intersectionT;		Scalar a1 = OpenMesh::Geometry::triangleArea(intersectionPt, j, L2);
		Scalar a2 = OpenMesh::Geometry::triangleArea(intersectionPt, j, L1);
		Scalar area = a1+a2;

		return area;
	}
}

//#define DEBUG_AREA 1

#if defined(DEBUG_AREA)
#include <iostream>
#endif

template <class Mesh>
void calculateAreas(Mesh& mesh, Mesh::HalfedgeHandle heh, 
					typename Mesh::Scalar& A_jkL1, 
					typename Mesh::Scalar& A_jkL2, 
					typename Mesh::Scalar& A_kL1L2, 
					typename Mesh::Scalar& A_jL1L2, 
					bool computeHinge = true)
{
	// TODO what happens for (degenerate triangles) boundary edges?
	typedef Mesh::Scalar Scalar;
	A_jkL1 = A_jkL2 = A_kL1L2 = A_jL1L2 = Scalar();

	Mesh::Point j = mesh.vertex(mesh.from_vertex_handle(heh)).orig_point;
	Mesh::Point k = mesh.vertex(mesh.to_vertex_handle(heh)).orig_point;
	Mesh::Point L1, L2;

	Mesh::HalfedgeHandle opp_heh = mesh.opposite_halfedge_handle(heh);
	bool L1_exists = !mesh.is_boundary(heh);
	bool L2_exists = !mesh.is_boundary(opp_heh);

    if (L1_exists)
	{		
		L1 = mesh.vertex(mesh.to_vertex_handle(mesh.next_halfedge_handle(heh))).orig_point;
		A_jkL1 = OpenMesh::Geometry::triangleArea(j, k, L1);
	}

	if (L2_exists)
	{
		L2 = mesh.vertex(mesh.to_vertex_handle(mesh.next_halfedge_handle(opp_heh))).orig_point;
		A_jkL2 = OpenMesh::Geometry::triangleArea(j, k, L2);		
	}

	if (computeHinge && L1_exists && L2_exists)
	{
		A_jL1L2 = computeHingeArea(j, k, L1, L2);
		A_kL1L2 = computeHingeArea(k, j, L1, L2);
	}

#if defined(DEBUG_AREA)
	std::cout << "j  = " << j  << std::endl;
	std::cout << "k  = " << k  << std::endl;
	std::cout << "L1 = " << L1 << std::endl;
	std::cout << "L2 = " << L2 << std::endl;
#endif
}

#endif