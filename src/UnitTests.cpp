#include <iostream>
#include "UnitTests.h"
#include "TriMesh.h"
#include "MeshOp.h"
#include "DividedDifference.h"
#include "Frame.h"

#pragma warning(disable: 4018)  // signed/unsigned mismatch

using namespace std;

template <class Mesh>
void print(Mesh& mesh, Mesh::HalfedgeHandle heh)
{
	PM::VertexHandle vh_to = mesh.to_vertex_handle(heh);
	PM::VertexHandle vh_from = mesh.from_vertex_handle(heh);

	PM::Point p_to = mesh.point(vh_to);
	PM::Point p_from = mesh.point(vh_from);

	cout << "[" << p_from << "]" << " -> [" << p_to << "]" << endl;
}

// TODO automatic test framework?

void test_findTwoRingNeighborhood()
{
	cout << "\nTesting [test_findTwoRingNeighborhood].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "triplesquare.obj");	

	if (mesh.n_vertices() == 0) return;

	PM::VertexHandle vh = mesh.handle(*mesh.vertices_begin());
	vector<PM::VertexHandle> vertices = findTwoRingNeighborhood(mesh, vh);

	int indx = 0;
	PM::Point p;	

	for (int i=0; i < vertices.size(); i++)
	{
        indx = vertices[i].idx();
		p = mesh.point(vertices[i]);
		cout << p << endl;
	}

}

void test_findE2Neighborhood()
{
    cout << "\nTesting [test_findE2Neighborhood].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "triplesquare.obj");	

	if (mesh.n_vertices() == 0) return;

	PM::VertexHandle vh = mesh.handle(*mesh.vertices_begin());
	vector<PM::HalfedgeHandle> edges = findE2Neighborhood(mesh, vh);

	for (int i=0; i < edges.size(); i++)
	{        		
		print(mesh, edges[i]);
	}
}

void test_findDiamond()
{
    cout << "\nTesting [test_findDiamond].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "triplesquare.obj");	

	if (mesh.n_vertices() == 0) return;

	PM::HalfedgeHandle heh = mesh.halfedge_handle(mesh.handle(*mesh.vertices_begin()));
	vector<PM::VertexHandle> vertices = findDiamond(mesh, heh);

	PM::Point p;	

	cout << "The first edge: " << mesh.point(mesh.from_vertex_handle(heh)) << " " 
		<< mesh.point(mesh.to_vertex_handle(heh)) << endl;

	for (int i=0; i < vertices.size(); i++)
	{
		p = mesh.point(vertices[i]);
		cout << p << endl;
	}
}

void test_triangleArea()
{
	cout << "\nTesting [triangleArea].." << endl;

	PM::Point a(0,0,0), b(0,0,1), c(1,0,0);

	cout << "triangle area1: " << OpenMesh::Geometry::triangleArea(b, a, c) << endl;
	cout << "triangle area2: " << OpenMesh::Geometry::triangleArea(b, c, a) << endl;
}

void test_computeHingeArea()
{
	cout << "\nTesting [computeHingeArea]..." << endl;
	{ // Should be 0
		PM::Point a(0,0,0), b(0,0,1), c(1,0,0), d(-1,0,0);
		cout << "triangle hinge area: " << computeHingeArea(a, b, c, d) << endl;
	}
	{ // Should be 0
		PM::Point a(0,0,0), b(0,0,1), c(1,0,0), d(0,-1,0);
		cout << "triangle hinge area: " << computeHingeArea(a, b, c, d) << endl;
	}

	{ // Should be 1
		PM::Point a(0,0,0), b(0,0,1), c(1,0,0), d(-1,0,0);
		cout << "triangle hinge area: " << computeHingeArea(b, a, c, d) << endl;
	}
	{ // Should be 1
		PM::Point a(0,0,0), b(0,0,1), c(1,0,0), d(0,-1,0);
		cout << "triangle hinge area: " << computeHingeArea(b, a, c, d) << endl;
	}

	{ // (Plane) Should be 1/2
		PM::Point a(0,0,0), b(0,0,1), c(1,0,1), d(-1,0,0);
		cout << "triangle hinge area: " << computeHingeArea(b, a, c, d) << endl;
	}
	{ // (Non-planar) Should be 1/2
		PM::Point a(0,0,0), b(0,0,1), c(0,1,1), d(-1,0,0);
		cout << "triangle hinge area: " << computeHingeArea(b, a, c, d) << endl;
	}

	{ // (Plane) Should be 1/2
		PM::Point a(0,0,0), b(0,0,1), c(2,0,1), d(-1,0,0);
		cout << "triangle hinge area: " << computeHingeArea(a, b, c, d) << endl;
	}
	{ // (Non-planar) Should be 1/2
		PM::Point a(0,0,0), b(0,0,1), c(0,2,1), d(-1,0,0);
		cout << "triangle hinge area: " << computeHingeArea(a, b, c, d) << endl;
	}
}

void test_calculateAreas()
{
    cout << "\nTesting [test_calculateAreas].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "hinge-sharp.obj");	

	if (mesh.n_vertices() == 0) return;	

	store_original_mesh(mesh);

	// TODO weird results when using vertex iter -> halfedge iter
	//PM::VertexIter vit = mesh.vertices_begin();
	PM::EdgeIter eit = mesh.edges_begin();
	PM::EdgeHandle eh = mesh.handle(*eit);
	PM::HalfedgeHandle heh = mesh.halfedge_handle(eh, 0);		
	PM::HalfedgeHandle heh1 = mesh.next_halfedge_handle(heh);
	PM::HalfedgeHandle heh2 = mesh.next_halfedge_handle(heh1);
	print(mesh, heh);
	print(mesh, heh1);
	print(mesh, heh2);

	PM::Scalar A_jkL1, A_jkL2, A_kL1L2, A_jL1L2;

	calculateAreas(mesh, heh, A_jkL1, A_jkL2, A_kL1L2, A_jL1L2);

	cout << "A_jkL1  = " << A_jkL1  << endl;
	cout << "A_jkL2  = " << A_jkL2  << endl;
	cout << "A_kL1L2 = " << A_kL1L2 << endl;
	cout << "A_jL1L2 = " << A_jL1L2 << endl;
}

void test_coeff()
{
    cout << "\nTesting [test_coeff].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "hinge.obj");	

	PM::EdgeIter eit = mesh.edges_begin();
	PM::EdgeHandle eh = mesh.handle(*eit);
	PM::HalfedgeHandle heh = mesh.halfedge_handle(eh, 0);
	PM::VertexHandle vh = mesh.from_vertex_handle(heh);
	PM::Scalar c = coeff(mesh, heh, vh);
}

void test_weight()
{
    cout << "\nTesting [test_weight].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "triplesquare.obj");	

	if (mesh.n_vertices() == 0) return;

	PM::EdgeIter eit = mesh.edges_begin();
	PM::EdgeHandle eh = mesh.handle(*eit);
	PM::HalfedgeHandle heh = mesh.halfedge_handle(eh, 0);
	PM::VertexHandle from = mesh.from_vertex_handle(heh);
	PM::VertexHandle to = mesh.to_vertex_handle(heh);

	print(mesh, heh);

	PM::Scalar weight = weight_ij(mesh, from, to);

	cout << "Weight_ij = " << weight << endl;
}

void test_frame() 
{
	// Test that local frame is sane
    cout << "\nTesting [test_frame].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "hinge.obj");	

	if (mesh.n_vertices() == 0) return;

	PM::EdgeIter eit = mesh.edges_begin();
	PM::EdgeHandle eh = mesh.handle(*eit);
	PM::HalfedgeHandle heh = mesh.halfedge_handle(eh, 0);
	PM::VertexHandle vh = mesh.from_vertex_handle(heh);

	Frame<PM> frame(mesh, vh);
	cout << "Frame: " << frame.U << " , " << frame.T << " , " << frame.V << endl;
}

void test_weight_sum() 
{
	// Test that weights of a vertex surrounded by planar
	// traingles sum to unity
	cout << "\nTesting [test_weight_sum].." << endl;

	PM mesh;
	OpenMesh::MeshIO::read_mesh(mesh, "funky-square.obj");	
	store_original_mesh(mesh);

	if (mesh.n_vertices() == 0) return;

	PM::Point zero(0,0,0);

	PM::VertexIter vit = mesh.vertices_begin();
	while (vit != mesh.vertices_end() && mesh.point(*vit) != zero) ++vit;

	if (vit == mesh.vertices_end())
	{
		cout << "Couldn't find 0 vertex!" << endl;
	}

	PM::VertexHandle vh_n = mesh.handle(*vit);

	typedef std::vector<PM::VertexHandle> VertexHandles;
	VertexHandles vertices = findTwoRingNeighborhood(mesh, vh_n);

	PM::Scalar sum = PM::Scalar();

	for (int i=0; i<vertices.size(); i++) 
	{
		PM::VertexHandle j = vertices[i];
		// Relax new mesh, using weights from original mesh		
		PM::Scalar weight = weight_ij(mesh, vh_n, j);		
		cout << "w = " << weight << ", p = " << mesh.point(j) << endl;

		sum += weight;
	}

	cout << "sum = " << sum << endl;
}

void run_tests()
{	
	//cout << "Running unit tests..." << endl;

	//test_frame();
	//test_weight();
	//test_coeff();
	//test_calculateAreas();
	//test_computeHingeArea();
	//test_triangleArea();
	//test_findTwoRingNeighborhood();
	//test_findE2Neighborhood();
	//test_findDiamond();
	//test_weight_sum();
}