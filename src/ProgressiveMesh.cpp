/*
@file ProgressiveMesh.cpp
*/

#include <cassert>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include "ProgressiveMesh.h"
#include "MeshOp.h"
#include "DividedDifference.h"
#include "Frame.h"

// #pragma warning(disable: 4018)  // signed/unsigned mismatch

using namespace std;

// Debugging output
void print_mesh(PM& mesh)
{
	for (PM::FaceIter f_it=mesh.faces_begin(); f_it!=mesh.faces_end(); ++f_it)
	{
		if (mesh.face(f_it.handle()).deleted())
			continue;		
		cout << "Face: ";
		for (PM::FaceVertexIter fv_it=mesh.fv_iter(f_it.handle());fv_it;++fv_it)
		{
			cout << mesh.point(*fv_it) << ", ";			
		}
		cout << endl;
	}
}

void print_vertexhandle(PM& mesh, PM::VertexHandle vh)
{
	if (vh.is_valid())
	{
		cout << mesh.point(vh);
	}
	else
	{
		cout << "(null)";
	}
}

void print_pminfo(PM& mesh, Decimater::ProgMeshInfo pminfo)
{
	cout << "v0 = ";
	print_vertexhandle(mesh, pminfo.v0);	

	cout << "\nv1 = ";
	print_vertexhandle(mesh, pminfo.v1);

	cout << "\nvl = ";
	print_vertexhandle(mesh, pminfo.vl);

	cout << "\nvr = ";
	print_vertexhandle(mesh, pminfo.vr);
	cout << endl;
}

void print_frame(Frame<PM>& frame)
{
	cout << " [" << frame.T << ", " << frame.U << ", " << frame.V << "] ";
}

void ProgressiveMesh::computeBoundingBox()
{
	PM::VertexIter vit = mesh.vertices_begin(), vend = mesh.vertices_end();

	if (vit == vend)
	{
		return;
	}

	PM::Point firstPoint = mesh.point(*vit);
	bbox_min = bbox_max = firstPoint;

	for ( ; vit != vend; ++vit)
	{		
		PM::Point p = mesh.point(*vit);
		for (int i=0; i < 3; i++)
		{
			bbox_min[i] = min(bbox_min[i], p[i]);
			bbox_max[i] = max(bbox_max[i], p[i]);
		}
	}
}

bool ProgressiveMesh::readFile(const char* filename)
{
	if (filename)
	{
		mesh.clear();
		OpenMesh::MeshIO::read_mesh(mesh, filename);
		minVCount=maxVCount=currentVCount=mesh.n_vertices();

		PM::VertexIter v_it = mesh.vertices_begin(), v_end = mesh.vertices_end();
		
		/*int boundaryCount = 0;
		for ( ; v_it != v_end; ++v_it)
		{
			if (mesh.is_boundary(mesh.handle(*v_it))) boundaryCount++;
		}

		cout << boundaryCount << " vertices on the boundary." << endl;*/

		int v = mesh.n_vertices();
		int e = mesh.n_edges();
		int f = mesh.n_faces();

		int x = v - e + f;

		cout << v << " vertices, " 
			<< e << " edges, and " 
			<< f << " faces loaded." <<endl;
		cout << "X(G) = " << x << "\n" << endl;

		computeBoundingBox();

		// Compute face normals
		mesh.update_face_normals();

		// Clear weight cache, reserve enough empty entries
		vertexWeightCache.clear();
		vertexWeightCache = VertexUpdateListCache(v);

		return true;
	}
	return false;
}

bool ProgressiveMesh::writeFile(const char* filename)
{
	if (filename)
	{
		OpenMesh::MeshIO::write_mesh(mesh, filename);
		return true;
	}
	return false;
}

// Return true iff PM is refinable
bool ProgressiveMesh::is_refinable()
{
	return (pmIter != pmInfos.begin());
}

void ProgressiveMesh::refineToLevelN(int n)
{
	while (currentVCount < n && is_refinable())
	{		
		refine();
	}
}

PMInfoContainer::iterator ProgressiveMesh::refine()
{
	assert(is_refinable());	
	--pmIter;
	
	mesh.vertex_split(pmIter->v0, pmIter->v1, pmIter->vl, pmIter->vr);	
	mesh.vertex(pmIter->v0).set_deleted(false);
	++currentVCount;
	return pmIter;
}

// Return true iff PM is coarsenable
bool ProgressiveMesh::is_coarsenable()
{
	return (pmIter != pmInfos.end());
}

PMInfoContainer::iterator ProgressiveMesh::coarsen()
{
	assert(is_coarsenable());

	PMInfoContainer::iterator iter = pmIter;

	PM::HalfedgeHandle hh = mesh.find_halfedge(pmIter->v0, pmIter->v1);
	mesh.collapse(hh);
	--currentVCount;
	++pmIter;

	return iter;
}

void ProgressiveMesh::coarsenToLevelN(int n)
{	
	while (currentVCount > n && is_coarsenable())
	{		
		coarsen();
	}
}

bool ProgressiveMesh::readPMFile(const char* filename)
{
	return false;
}

void ProgressiveMesh::buildPM()
{
	if (mesh.n_vertices() == 0) return;

	Timer t;
	cout << "Decimating... ";

	// garbage collect
	mesh.garbage_collection();

	// copy points to orig_point
	store_original_mesh(mesh);

	// 1. create decimating instance
	Decimater decimater(mesh);
	
	// 2. register modules
	OpenMesh::Decimater::ModQuadricT<PM> modQuadric(mesh);
	// clear up of these modules are in decimater
	decimater.registrate(modQuadric);

	// 3. initialize setup
	decimater.initialize();
	decimater.generate_progmesh_info(&pmInfos);	

	// 4. simplify as much as possible
	int numVertexDecimated = decimater.decimate();

	// update information
	currentVCount = minVCount = maxVCount - numVertexDecimated;

	pmIter = pmInfos.end();
	
	cout << "(" << t.get_elapsed() << "s)" << endl;

	// TODO how to we choose this?
	int minDetailLevel = maxVCount / 100;
	refineToLevelN(minDetailLevel);

	// compute
	t.reset();
	cout << "Computing detail vectors... ";
	computeDetailVectors(maxVCount);	
	cout << "(" << t.get_elapsed() << "s)\n" << endl;

	// store ordering of vertices
	vertexOrdering.clear();
	PMInfoContainer::reverse_iterator pit = pmInfos.rbegin(), pend = pmInfos.rend();
	for ( ; pit != pend; ++pit)
	{
		vertexOrdering.push_back(pit->v0);
	}
}

void ProgressiveMesh::computeVertexWeights(
	PM::VertexHandle vh_n, VertexUpdateList& vertexUpdates)
{
	// mannequin.obj:    Filtering w/o cache = 1.7s, w/cache = 0.04s
	// manifold-cow.obj: Filtering w/o cache = 7.2s, w/cache = 0.2s

	bool cacheWeights = true;

	if (cacheWeights) {
		// Make sure cache is large enough to hold vh_n data
		int index = vh_n.idx();
		assert(index >= 0);
		VertexUpdateList empty;
		while (vertexWeightCache.size() < index+1) {
			vertexWeightCache.push_back(empty);
		}

		// In cache?
		vertexUpdates = vertexWeightCache[index];
		
		// If not, compute
		if (vertexUpdates.size() == 0) {			
			computeVertexWeights_calc(vh_n, vertexUpdates);

			// Cache
			vertexWeightCache[index] = vertexUpdates;
		}		
	} else {
		return computeVertexWeights_calc(vh_n, vertexUpdates);
	}
}

// Compute relaxation weights for a vertex vh_n and its 1-ring neighbors
void ProgressiveMesh::computeVertexWeights_calc(
	PM::VertexHandle vh_n, VertexUpdateList& vertexUpdates)
{
	typedef std::vector<PM::VertexHandle> VertexHandles;
	VertexHandles vertices = findTwoRingNeighborhood(mesh, vh_n);

	VertexWeights weights_vh_n;
	
	for (int i=0; i<vertices.size(); i++) 
	{
		// Relax new mesh, using weights from original mesh		
		PM::Scalar weight = weight_ij(mesh, vh_n, vertices[i]);		
		weights_vh_n.push_back(make_pair(vertices[i], weight));
	}	

	// Store weights that we will use to update vh_n
	vertexUpdates.push_back(make_pair(vh_n,weights_vh_n));

	// Now relax one-ring neighborhood 
	PM::VertexVertexIter vv_it= mesh.vv_iter(vh_n);
	for ( ; vv_it; ++vv_it)
	{
		VertexWeights weights_vh_outer;

		// Get vertex handle of vertex on 1-ring neighborhood
		PM::VertexHandle vh_outer = vv_it.handle();

		// Get the V2 neighbors and compute the updated position
		VertexHandles v2_vertices = findTwoRingNeighborhood(mesh, vh_outer);
		
		// Loop over the V2 neighborhood of vh_outer
		VertexHandles::iterator hand_it, hand_end = v2_vertices.end();
		for (hand_it = v2_vertices.begin(); hand_it != hand_end; ++hand_it) 
		{
			PM::VertexHandle v2_neighbor = *hand_it;
			if (v2_neighbor == vh_n) continue;

			PM::Scalar weight = weight_ij(mesh, vh_outer, v2_neighbor);			
			weights_vh_outer.push_back(make_pair(v2_neighbor, weight));			
		}

		// Sum up the last term (w_j_n * q_n)
		PM::Scalar w_j_n = weight_ij(mesh, vh_outer, vh_n);		
		weights_vh_outer.push_back(make_pair(vh_n, w_j_n));

		// Store weights that we will use to update vh_outer		
		vertexUpdates.push_back(make_pair(vh_outer, weights_vh_outer));
	}
}

void ProgressiveMesh::computeDetailVectors(int desiredDetailLevel)
{
	// TODO remove duplication with restoreDetailVectors

	while ( is_refinable() && currentVCount < desiredDetailLevel )	
	{
		// Clear coefficient and weight hash
		clear_hashes();

		// Add new vertex
		PMInfoContainer::iterator iter = refine();

		// Get the iterator of the new vertex		
		PM::VertexHandle vh_new = iter->v0;
		PM::VertexHandle vh_old = iter->v1;
		PM::Vertex& vertex_new = mesh.vertex(vh_new);

		// Compute vertex weights
		VertexUpdateList vertexUpdates;
		computeVertexWeights(vh_new, vertexUpdates);

		// Compute local frame without using new vertex
		coarsen();
		Frame<PM> frame(mesh, vh_old);
		refine();
		
		if (true) {
			vertex_new.detailVectors.clear(); // Reset detail vectors
		}

		// Make sure to compute all new positions, before updating any of them
		//typedef pair<PM::VertexHandle, PM::Point> PointUpdate;
		//vector<PointUpdate> pointUpdates;

		bool first = true;

		// Save orig pos of vertex 0
		PM::Point vertex_new_orig_pos = mesh.point(vertex_new);

		vector<VertexUpdate>::iterator vit, vend = vertexUpdates.end();
		for (vit = vertexUpdates.begin(); vit != vend; ++vit)
		{
			PM::VertexHandle vh = vit->first;
			VertexWeights& weights = vit->second;

			// Sum weights * neighbors
			PM::Point q_n(0,0,0);
			vector<VertexWeight>::iterator wit, wend = weights.end();
			for (wit = weights.begin(); wit != wend; ++wit)
			{
				PM::VertexHandle q_k = wit->first;
				PM::Scalar w = wit->second;
				
				q_n += w * mesh.point(q_k);
				//cout << "w * q_k = " << w << " * " << mesh.point(q_k) << " = "
				//	<< (w * mesh.point(q_k)) << ", " << endl;
			}

			// store detail vector
			PM::Point new_dv = mesh.point(vh) - q_n;
			PM::Point local_dv = frame.project(new_dv);
			vertex_new.detailVectors.push_back(local_dv);

			//cout << vh.idx() << ": q_n = " << q_n << ", local_dv = " << local_dv << endl;

			if (first) {
				// new vertex gets updated immediately,
				// since 1-ring depends on its new position
				mesh.set_point(vh, q_n);				
				first = false;
			}
			
			//pointUpdates.push_back(make_pair(vh, q_n));
		}

		// Restore orig pos
		mesh.set_point(vh_new, vertex_new_orig_pos);		
		
		// Now, perform deferred updates
		//vector<PointUpdate>::iterator pu_it, pu_end(pointUpdates.end());
		//for (pu_it = pointUpdates.begin(); pu_it != pu_end; ++pu_it)
		//{
		//	mesh.set_point(pu_it->first, pu_it->second);
		//}
	}
}

void ProgressiveMesh::restoreDetailVectors(int desiredDetailLevel)
{
	Timer t;
	cout << "Restoring detail vectors... ";	

	while ( is_refinable() && currentVCount < desiredDetailLevel )	
	{
		// Clear coefficient and weight hash
		clear_hashes();

		// Add new vertex
		PMInfoContainer::iterator iter = refine();

		// Get the iterator of the new vertex		
		PM::VertexHandle vh_new = iter->v0;
		PM::VertexHandle vh_old = iter->v1;
		PM::Vertex& vertex_new = mesh.vertex(vh_new);

		if (vertex_new.detailVectors.empty()) {
			// This level has no detail vectors -- we probably did not want to compute them
			continue;
		}

		// Compute vertex weights, using original mesh points
		vector<VertexUpdate> vertexUpdates;
		computeVertexWeights(vh_new, vertexUpdates);

		// Compute local frame without using new vertex
		coarsen();
		Frame<PM> frame(mesh, vh_old);
		refine();

		// Make sure to compute all new positions, before updating any of them
		typedef pair<PM::VertexHandle, PM::Point> PointUpdate;
		vector<PointUpdate> pointUpdates;
		
		bool first = true;

		// Detail vectors are all stored on new vertex
		vector<PM::Point>::iterator detailIter = vertex_new.detailVectors.begin();

		// Relax vertices
		vector<VertexUpdate>::iterator vit, vend = vertexUpdates.end();
		for (vit = vertexUpdates.begin(); vit != vend; ++vit)
		{
			PM::VertexHandle vh = vit->first;
			VertexWeights& weights = vit->second;
			
			// Sum weights * neighbors
			PM::Point q_n(0,0,0);
			vector<VertexWeight>::iterator wit, wend = weights.end();
			for (wit = weights.begin(); wit != wend; ++wit)
			{
				PM::VertexHandle q_k = wit->first;
				PM::Scalar w = wit->second;
				
				q_n += w * mesh.point(q_k);
				//cout << "w * q_k = " << w << " * " << mesh.point(q_k) << " = "
				//	<< (w * mesh.point(q_k)) << ", " << endl;
			}

			//cout << vh.idx() << ": q_n = " << q_n << ", local_dv = " << local_dv << endl;

			assert(detailIter != vertex_new.detailVectors.end());
			PM::Point dv = *(detailIter++);
			PM::Point local_dv = frame.unproject(dv);
			PM::Point relaxed_point = q_n;
			PM::Point updated_point = relaxed_point + local_dv;

			if (first) {
				// New vertex gets updated immediately to relaxed position,
				// since 1-ring depends on that new position
				mesh.set_point(vh, relaxed_point);
				first = false;
			} 

			// All vertices need to be updated to relaxed + detail
			pointUpdates.push_back(make_pair(vh, updated_point));

		}

		// Now, perform deferred updates
		vector<PointUpdate>::iterator pu_it, pu_end(pointUpdates.end());
		for (pu_it = pointUpdates.begin(); pu_it != pu_end; ++pu_it)
		{
			mesh.set_point(pu_it->first, pu_it->second);
		}	       
	}

	cout << "(" << t.get_elapsed() << "s)" << endl;
}

void ProgressiveMesh::stepComputeDetailVectors()
{
	computeDetailVectors(currentVCount-1);
}

void ProgressiveMesh::smooth()
{
	// Deprecated
	coarsenToLevelN(0);

	for (int i=vertexOrdering.size()/2; i < vertexOrdering.size(); i++)	
	{
		PM::VertexHandle vh = vertexOrdering[i];

		PM::Vertex& vertex = mesh.vertex(vh);

		for (int j=0; j < vertex.detailVectors.size(); j++)
		{
			vertex.detailVectors[j] *= 0.9f;
		}
	}

	restoreDetailVectors(getMaxLevel());
}