// These functions implement Guskov's divided differences and
// surface relaxation operator in the surface setting.
//
// The main function for the client programmer is weight_ij(),
// which is implemented in terms of coeff().
//
// Both of these stub functions are actually implemented in terms of
// *_calc. This is done so as to avoid recomputation through caching.

#ifndef DIVIDED_DIFFERENCE_H
#define DIVIDED_DIFFERENCE_H

#include "MeshOp.h"
#include <hash_map>
//#include <map>

// Store the original points, because we use the parameterization from
// the original progressive mesh rather than from the updated mesh
template <class Mesh>
void store_original_mesh(Mesh& mesh)
{
	// copy points to orig_point
	for (Mesh::VertexIter v_it=mesh.vertices_begin();v_it!=mesh.vertices_end();++v_it)
	{
		Mesh::Point p = mesh.point(v_it.handle());
		mesh.vertex(v_it.handle()).orig_point = p;
	}	
}

// Return true iff the halfedge contains the vertex handle
template <class Mesh>
bool contains(Mesh& mesh, Mesh::HalfedgeHandle heh, Mesh::VertexHandle vh)
{
	return (mesh.to_vertex_handle(heh) == vh) || (mesh.from_vertex_handle(heh) == vh);
}

// Returns the *original* length of a halfedge
template <class Mesh>
typename Mesh::Scalar
length(Mesh& mesh, Mesh::HalfedgeHandle heh)
{
	// How to make orig_point access cleaner?

	//Mesh::Point p0 = mesh.point(mesh.to_vertex_handle(heh));
	//Mesh::Point p1 = mesh.point(mesh.from_vertex_handle(heh));
	Mesh::Point p0 = mesh.vertex(mesh.to_vertex_handle(heh)).orig_point;
	Mesh::Point p1 = mesh.vertex(mesh.from_vertex_handle(heh)).orig_point;

	Mesh::Point dp = p1 - p0;
	return dp.length();
}

// TODO could reorganize some of this
template <class T1, class T2>
class pair_compare
{
public:
	enum
	{
		bucket_size = 4, min_buckets = 8
	};

	size_t operator()(const std::pair<T1,T2>& p) const
	{
		// TODO experiment with different hash functions
		return hash_compare<T1>()(p.first)+hash_compare<T2>()(p.second);
	}

	bool operator()(const std::pair<T1,T2>& p1, const std::pair<T1,T2>& p2) const
	{
		return p1 < p2;
	}
};

typedef std::pair<int,int> IndexPair;
typedef std::hash_map<IndexPair,double,pair_compare<int,int> > IntHash;
extern IntHash g_coeffHash;
extern IntHash g_weightHash;

void clear_hashes();

// sphere.obj takes 91s w/o hash, 51s w/ std::map, 50s w/ std::hash_map
// TODO more exp. to figure out where to cache and how much, multilevel?
#define USE_HASH 1


template <class Mesh>
typename Mesh::Scalar
coeff(Mesh& mesh, Mesh::HalfedgeHandle heh, Mesh::VertexHandle vh)
{
#if defined(USE_HASH)
	// Cache coefficient values, reuse instead of recalculate if possible
	IndexPair ipair = make_pair(heh.idx(), vh.idx());
	IntHash::iterator it = g_coeffHash.find(ipair);
	if (it != g_coeffHash.end()) {
		return it->second;
	}

	Mesh::Scalar s = coeff_calc(mesh, heh, vh);
	g_coeffHash[ipair] = s;

	return s;	
#else
    return coeff_calc(mesh, heh, vh);
#endif //(USE_HASH)
}

// Implements formula (1) in Guskov et al.
template <class Mesh>
typename Mesh::Scalar
coeff_calc(Mesh& mesh, Mesh::HalfedgeHandle heh, Mesh::VertexHandle vh)
{
	typedef Mesh::Scalar Scalar;
	Scalar zero = Scalar();
	Scalar edge_length = length(mesh, heh);

	Scalar A_jkL1, A_jkL2, A_kL1L2, A_jL1L2;		
	assert(diamondContains(mesh, heh, vh));

    if (contains(mesh, heh, vh))
	{		
		calculateAreas(mesh, heh, A_jkL1, A_jkL2, A_kL1L2, A_jL1L2);
		if (A_jkL1  == zero || A_jkL2  == zero) return zero;

        if (mesh.from_vertex_handle(heh) == vh)
		{
            // Coeff: c_e,j
			return -edge_length * A_kL1L2 / (A_jkL1 * A_jkL2);
		}
		else
		{
			// Coeff: c_e,k
			return -edge_length * A_jL1L2 / (A_jkL1 * A_jkL2);
		}
	}
	else
	{
		calculateAreas(mesh, heh, A_jkL1, A_jkL2, A_kL1L2, A_jL1L2, 
			/* computeHinge = */ false);

		Mesh::HalfedgeHandle next_heh = mesh.next_halfedge_handle(heh);
		if (mesh.to_vertex_handle(next_heh) == vh)
		{
			if (A_jkL1 == zero) return zero;
			// Coeff: c_e,l1
			return edge_length / A_jkL1;
		}
		else
		{	
			if (A_jkL2 == zero) return zero;
			// Coeff: c_e,l2
			return edge_length / A_jkL2;
		}
	}
}

// TODO could write a generic function hash cache

// Returns the weight_ij of two vertices. Note that in general,
// weight(i,j) != weight(j,i).
template <class Mesh>
typename Mesh::Scalar
weight_ij(Mesh& mesh, Mesh::VertexHandle i, Mesh::VertexHandle j)
{
#if defined(USE_HASH)
	// Cache hash values, reuse instead of recalculate if possible	
	IndexPair ipair = make_pair(i.idx(), j.idx());
	IntHash::iterator it = g_weightHash.find(ipair);
	if (it != g_weightHash.end()) {
		return it->second;
	}

	Mesh::Scalar s = weight_ij_calc(mesh, i, j);
	g_weightHash[ipair] = s;

	return s;
#else 
    return weight_ij_calc(mesh, i, j);
#endif //(USE_HASH)
}

template <class Mesh>
typename Mesh::Scalar
weight_ij_calc(Mesh& mesh, Mesh::VertexHandle i, Mesh::VertexHandle j)
{
	typedef Mesh::Scalar Scalar;

	typedef std::vector<Mesh::HalfedgeHandle> Handles;
	Handles edges = findE2Neighborhood(mesh, i);

	// Compute top_sum / bottom_sum

	Scalar zero = Scalar();
	Scalar top_sum = zero;
	Scalar bottom_sum = zero;

	// Loop through all edges e in E2 Neighborhood
	Handles::iterator hit, hend = edges.end();
	for (hit = edges.begin(); hit != hend; ++hit)
	{
		Mesh::HalfedgeHandle e = *hit;

		Scalar C_e_i = coeff(mesh, e, i);
		bottom_sum += C_e_i*C_e_i;

        if (diamondContains(mesh, e, j))
		{
			Scalar C_e_j = coeff(mesh, e, j);
			top_sum += C_e_i*C_e_j;
		}
	}	

	assert(bottom_sum != zero);

	return -top_sum/bottom_sum;
}

#endif