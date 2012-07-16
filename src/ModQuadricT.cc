//=============================================================================
//                                                                            
//                               OpenMesh                                     
//        Copyright (C) 2002 by Computer Graphics Group, RWTH Aachen          
//                           www.openmesh.org                                 
//                                                                            
//-----------------------------------------------------------------------------
//                                                                            
//                                License                                     
//                                                                            
//   This library is free software; you can redistribute it and/or modify it 
//   under the terms of the GNU Library General Public License as published  
//   by the Free Software Foundation, version 2.                             
//                                                                             
//   This library is distributed in the hope that it will be useful, but       
//   WITHOUT ANY WARRANTY; without even the implied warranty of                
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         
//   Library General Public License for more details.                          
//                                                                            
//   You should have received a copy of the GNU Library General Public         
//   License along with this library; if not, write to the Free Software       
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                 
//                                                                            
//-----------------------------------------------------------------------------
//                                                                            
//   $Revision: 1.1 $
//   $Date: 2002/12/14 15:28:10 $
//                                                                            
//=============================================================================

/** \file ModQuadricT.c
    Bodies of template member function.
 */

//=============================================================================
//
//  CLASS ModQuadric - IMPLEMENTATION
//
//=============================================================================

#define OPENMESH_DECIMATER_MODQUADRIC_CC

//== INCLUDES =================================================================


#include "ModQuadricT.hh"


//the weight to combine the quadric error and the edge length

#define WEIGHT 1

//== NAMESPACE =============================================================== 

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Decimater { // BEGIN_NS_DECIMATER


//== IMPLEMENTATION ========================================================== 


template<class Mesh>
void
ModQuadricT<Mesh>::
initialize()
{
   using Geometry::Quadricd;
   
  // clear quadrics
  typename Mesh::VertexIter  v_it  = mesh_.vertices_begin(), 
                             v_end = mesh_.vertices_end();

  for (; v_it != v_end; ++v_it)
  {
    v_it->quadric.clear();

	//add by zheng, to combine the edge length
	Quadricd::Vec3    v;

	v = mesh_.point(*v_it);

	Quadricd q (1.0,0.0,0.0,-v[0],
		            1.0,0.0,-v[1],
					    1.0,-v[2],
						     v.sqrnorm());
	q *= WEIGHT;
	v_it->quadric = q;

  }
  
  
  // calc (normal weighted) quadric
  typename Mesh::FaceIter          f_it  = mesh_.faces_begin(),
                                   f_end = mesh_.faces_end();

  typename Mesh::FaceVertexIter    fv_it;
  typename Mesh::Vertex            *vp0, *vp1, *vp2;

  Quadricd                         q;
  Quadricd::Vec3                   v0,v1,v2,n;
  double                           a,b,c,d, area;


  for (; f_it != f_end; ++f_it)
  {
    fv_it = mesh_.fv_iter(f_it.handle());
    vp0 = &(*fv_it);  ++fv_it;
    vp1 = &(*fv_it);  ++fv_it;
    vp2 = &(*fv_it);

    v0 = mesh_.point(*vp0);
    v1 = mesh_.point(*vp1);
    v2 = mesh_.point(*vp2);

    n    =  (v1-v0) % (v2-v0);
    area = n.norm();
    if (area > FLT_MIN) 
    {
      n /= area;
      area *= 0.5;
    }

    a = n[0];
    b = n[1];
    c = n[2];
    d = -(v0|n);

    q = QuadricT<double>(a, b, c, d);
    q *= area;
    
    vp0->quadric += q;
    vp1->quadric += q;
    vp2->quadric += q;
  }
}


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
