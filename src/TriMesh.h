#ifndef TRIMESH_H
#define TRIMESH_H

// openmesh includes
#include <OpenMesh/IO/BinaryHelper.hh>
#include <OpenMesh/IO/MeshIO.hh>
#include <OpenMesh/Mesh/Types/TriMesh_ArrayKernelT.hh>
#ifdef _MSC_VER
#  ifndef OM_STATIC_BUILD
#  define OM_STATIC_BUILD
#  endif
#  define INCLUDE_TEMPLATES
#  include <OpenMesh/IO/IOInstances.hh>
#endif

// openmesh tools
#include <OpenMeshTools/Decimater/DecimaterT.hh>
#include <OpenMeshTools/Decimater/ModQuadricT.hh>

#include <vector>

struct RelaxationTraits : public OpenMesh::DefaultTraits
{	
	FaceAttributes(OpenMesh::DefaultAttributer::Normal);

	// Store Points as floats
	typedef OpenMesh::Vec3f Point;

	VertexTraits
	{
	public:		
		// TODO why can't inherit directly?
		OpenMesh::Geometry::QuadricT<double>  quadric;

		Point orig_point;
		std::vector<Point> detailVectors;
	};
};

OM_Merge_Traits(RelaxationTraits, OpenMesh::Decimater::DefaultTraits, PMTraits);
typedef OpenMesh::TriMesh_ArrayKernelT<PMTraits> PM;

#endif