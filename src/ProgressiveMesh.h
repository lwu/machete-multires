/*
@file ProgressiveMesh.h

ProgressiveMesh implements Hoppe's progressive meshes, using half-edge
collapses that are driven by the Garland-Heckbert error metric biased
by edge length.

It knows how to coarsen, refine, load and save models, and how to
compute multiresolution detail vectors.
*/
#ifndef PROGRESSIVEMESH_H
#define PROGRESSIVEMESH_H

// general includes
#include <vector>
#include "TriMesh.h"

extern double get_cpu_time();

// Timer helper class
class Timer {
public:
	Timer() { reset(); }
	void reset() { t0 = get_cpu_time(); }
	double get_elapsed() const { return get_cpu_time() - t0; }
private:
	double t0;	
};

typedef OpenMesh::Decimater::ModQuadricT<PM> Quadrics;
typedef OpenMesh::Decimater::DecimaterT<PM> Decimater;
typedef OpenMesh::Decimater::DecimaterT<PM>::ProgMeshInfoContainer PMInfoContainer;

class ProgressiveMesh
{

protected:

	// Compute bounding box of mesh
	void computeBoundingBox();

	// Mesh data
	PM mesh;

	PM::Point bbox_min, bbox_max;
	int minVCount, maxVCount, currentVCount;	 	

	// Progressive Mesh data
	PMInfoContainer pmInfos;
	PMInfoContainer::iterator pmIter;	

	std::vector<PM::VertexHandle> vertexOrdering;

public:

	ProgressiveMesh()
	{
		pmIter = pmInfos.end();
		minVCount = maxVCount = currentVCount=0;
		bbox_min  = PM::Point(-5, -5, -5);
		bbox_max  = PM::Point( 5,  5,  5);
	};

	virtual ~ProgressiveMesh(){};

	/// Get current level
	int getCurrentLevel(){return currentVCount;};
	int getMinLevel(){return minVCount;};
	int getMaxLevel(){return maxVCount;};

	/// Returns true iff the mesh is still refinable
	bool is_refinable();

	/// Refine one level of the progressive mesh
	PMInfoContainer::iterator refine();

	/// Refine mesh up to n vertices
	void refineToLevelN(int n);

	/// Return true iff mesh can be coarsened
	bool is_coarsenable();

	/// Simplify one level
	PMInfoContainer::iterator coarsen();

	/// Coarsen mesh down to n vertices
	void coarsenToLevelN(int n);
	

	/// Read a file
	bool readFile(const char* filename=NULL);

	/// Write a file
	bool writeFile(const char* filename=NULL);

	/// Read PM file
	bool readPMFile(const char* filename=NULL);

	/// Build the hierarchy
	void buildPM();

	/// Compute detail vectors, up to desired detail level
	void computeDetailVectors(int desiredDetailLevel);

	/// Restore detail vectors, up to desired detail level
	void restoreDetailVectors(int desiredDetailLevel);

	void stepComputeDetailVectors();

	// Compute vertex weights for relaxation operator
	typedef std::pair<PM::VertexHandle,PM::Scalar> VertexWeight;
	typedef std::vector<VertexWeight> VertexWeights;
	typedef std::pair<PM::VertexHandle,VertexWeights> VertexUpdate;
	typedef std::vector<VertexUpdate> VertexUpdateList;

	// Cached interface
	void computeVertexWeights(PM::VertexHandle vh_n, std::vector<VertexUpdate>& vertexUpdates);
	typedef std::vector<VertexUpdateList> VertexUpdateListCache;
	VertexUpdateListCache vertexWeightCache;

	/// Vertex weight calculation	
	void computeVertexWeights_calc(PM::VertexHandle vh_n, std::vector<VertexUpdate>& vertexUpdates);

	void smooth();
};

#endif
