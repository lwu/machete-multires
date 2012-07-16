/*
Frame defines a local frame, given a mesh and a vertex handle.
It does this by averaging adjacent face normals. If this is degenerate,
it uses the first face only.
*/
#ifndef FRAME_H
#define FRAME_H

#include "TriMesh.h"

template <class Mesh>
class Frame 
{
protected:
public:
	Mesh::Point U, T, V;	

	void MakeIdentity()
	{
		U = Mesh::Point(1,0,0);
		T = Mesh::Point(0,1,0);
		V = Mesh::Point(0,0,1);	
	}

	Frame(Mesh& mesh, Mesh::VertexHandle vh) 
	{
		//MakeIdentity();
		
		Mesh::Point normal(0,0,0);
		Mesh::VertexFaceIter vf_it=mesh.vf_iter(vh);
		assert(vf_it);

		for ( ; vf_it; ++vf_it)
			normal += mesh.calc_face_normal(mesh.handle(*vf_it));

		Mesh::Scalar epsilon = 0.01;

		Mesh::Scalar n = normal.norm();
		if (n > epsilon) {
			U = normal / n;
		} else {
			// If norm is small, just use first face normal
			vf_it=mesh.vf_iter(vh);
			U = mesh.calc_face_normal(mesh.handle(*vf_it));	

			n = U.norm();
			if (n < epsilon) {
				cout << "!" << endl;
				MakeIdentity();
			}
		}
		Mesh::HalfedgeHandle heh = mesh.halfedge_handle(vh);

		Mesh::Point to = mesh.point(mesh.to_vertex_handle(heh));
		T = (U % (to - mesh.point(vh)) ).normalize();
		V = U % T;
		
		// cout << "U = " << U << ", T = " << T << ", V = " << V << endl;
	}

	Mesh::Point project(Mesh::Point point)
	{
		return Mesh::Point(U|point, T|point, V|point);                
	}

	Mesh::Point unproject(Mesh::Point point)
	{
		// Since M = [U T V] is orthonormal, inverse(M) == transpose(M)
		Mesh::Point p;

        for (int i=0; i < 3; i++)
		{
            p[i] = U[i]*point[0] + T[i]*point[1] + V[i]*point[2];
		}

		return p;
	}
};

#endif
