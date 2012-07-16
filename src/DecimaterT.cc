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
//   $Revision: 1.3 $
//   $Date: 2002/12/05 06:45:26 $
//                                                                            
//=============================================================================

/** \file DecimaterT.cc
 */


//=============================================================================
//
//  CLASS DecimaterT - IMPLEMENTATION
//
//=============================================================================

#define OPENMESH_DECIMATER_DECIMATERT_CC


//== INCLUDES =================================================================

#include "Config.hh"
#include <vector>
#include <float.h>
#include "DecimaterT.hh"


//== NAMESPACE =============================================================== 


namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Decimater { // BEGIN_NS_DECIMATER

   
//== IMPLEMENTATION ========================================================== 

  

/*--- get local configuration ---
  
           vl
           * 
	  / \
         /   \
        / fl  \
    v0 *------>* v1
        \ fr  /
         \   /
          \ /
	   * 
           vr
*/

template <class Mesh>
CollapseInfoT<Mesh>::
CollapseInfoT(Mesh& _mesh, typename Mesh::HalfedgeHandle _heh) :

  mesh(_mesh),
  v0v1(_heh),
  v1v0(_mesh.opposite_halfedge_handle(v0v1)),
  v0(_mesh.to_vertex_handle(v1v0)),
  v1(_mesh.to_vertex_handle(v0v1)),
  p0(_mesh.point(v0)),
  p1(_mesh.point(v1)),
  fl(_mesh.face_handle(v0v1)),
  fr(_mesh.face_handle(v1v0))

{
  // get vl
  if (fl.is_valid())
  {
    vlv1 = mesh.next_halfedge_handle(v0v1);
    v0vl = mesh.next_halfedge_handle(vlv1);
    vl   = mesh.to_vertex_handle(vlv1);
    vlv1 = mesh.opposite_halfedge_handle(vlv1);
    v0vl = mesh.opposite_halfedge_handle(v0vl);
  }


  // get vr
  if (fr.is_valid())
  {
    vrv0 = mesh.next_halfedge_handle(v1v0);
    v1vr = mesh.next_halfedge_handle(vrv0);
    vr   = mesh.to_vertex_handle(vrv0);
    vrv0 = mesh.opposite_halfedge_handle(vrv0);
    v1vr = mesh.opposite_halfedge_handle(v1vr);
  }
}  


//-----------------------------------------------------------------------------


template <class Mesh>
void
DecimaterT<Mesh>::initialize()
{
   typename ModuleList::iterator m_it, m_end = bmodules_.end();

   for (m_it=bmodules_.begin(); m_it != m_end; ++m_it)
      (*m_it)->initialize();
   cmodule_->initialize();
}


//-----------------------------------------------------------------------------

// Moved to header
/*
template <typename Mesh>
template< typename ModuleType > 
bool
DecimaterT<Mesh>::registrate(const ModuleType& _mod) 
{
   if ( _mod.is_binary() )
   {
      bmodules_.push_back( new ModuleType(_mod) );
   }
   else      
   {
      delete cmodule_;
      cmodule_ = new ModuleType(_mod);
   }
   return true;
}
*/

//-----------------------------------------------------------------------------

template <class Mesh>
bool
DecimaterT<Mesh>::is_collapse_legal(const CollapseInfo& _ci)
{

  // locked ? deleted ?
  if (mesh_.vertex(_ci.v0).locked() || 
      mesh_.vertex(_ci.v0).deleted())
    return false;



  //--- feature test ---

//   if (mesh_.vertex(_ci.v0).feature() &&
//       !mesh_.vertex(_ci.v1).feature())
//     return false;

  if (mesh_.vertex(_ci.v0).feature() &&
      !mesh_.edge(mesh_.edge_handle(_ci.v0v1)).feature())
    return false;



  //--- test one ring intersection ---

  typename Mesh::VertexVertexIter  vv_it;
  
  for (vv_it = mesh_.vv_iter(_ci.v0); vv_it; ++vv_it)
    vv_it->set_tagged(false);

  for (vv_it = mesh_.vv_iter(_ci.v1); vv_it; ++vv_it)
    vv_it->set_tagged(true);

  for (vv_it = mesh_.vv_iter(_ci.v0); vv_it; ++vv_it)
    if (vv_it->tagged() && 
	vv_it.handle() != _ci.vl && 
	vv_it.handle() != _ci.vr)
      return false;

  // if both are invalid OR equal -> fail
  if (_ci.vl == _ci.vr) return false;


  //--- test boundary cases ---

  bool b0 = mesh_.is_boundary(_ci.v0),
       b1 = mesh_.is_boundary(_ci.v1);

  if (b0)
  {
    // don't collapse a boundary vertex to an inner one
    if (!b1)  
      return false;

    else 
    {
      // edge between two boundary vertices has to be a boundary edge
      if (!(mesh_.is_boundary(_ci.v0v1) || 
	    mesh_.is_boundary(_ci.v1v0)))
	return false;
    }

    // only one one ring intersection
    if (_ci.vl.is_valid() && 
	_ci.vr.is_valid())
      return false;
  }

  // v0vl and v1vl must not both be boundary edges
  if (_ci.vl.is_valid() &&
      mesh_.is_boundary(_ci.vlv1) && 
      mesh_.is_boundary(_ci.v0v1))
    return false;
  
  // v0vr and v1vr must not be both boundary edges
  if (_ci.vr.is_valid() &&
      mesh_.is_boundary(_ci.vrv0) && 
      mesh_.is_boundary(_ci.v1vr))
    return false;
  
  // there have to be at least 2 incident faces at v0
  if (mesh_.cw_rotated_halfedge_handle(
       mesh_.cw_rotated_halfedge_handle(_ci.v0v1)) == _ci.v0v1)
    return false;


  // collapse passed all tests -> ok
  return true;
}


//-----------------------------------------------------------------------------


template <class Mesh>
float
DecimaterT<Mesh>::collapse_priority(const CollapseInfo& _ci)
{
  typename ModuleList::iterator m_it, m_end = bmodules_.end();

  for (m_it = bmodules_.begin(); m_it != m_end; ++m_it)
  {                         
     if ( (*m_it)->collapse_priority(_ci) < 0.0)
        return -1.0;
  }     
  return cmodule_->collapse_priority(_ci);
}


//-----------------------------------------------------------------------------


template <class Mesh>
void
DecimaterT<Mesh>::heap_vertex(typename Mesh::VertexHandle _vh)
{
  typename Mesh::Vertex&          v(mesh_.vertex(_vh));
  float                           prio, best_prio(FLT_MAX);
  typename Mesh::HalfedgeHandle   heh, collapse_target;


  // find best target in one ring
  typename Mesh::VertexOHalfedgeIter voh_it(mesh_, _vh);
  for (; voh_it; ++voh_it)
  {
    heh = voh_it.handle();
    CollapseInfo  ci(mesh_, heh);

    if (is_collapse_legal(ci))
    {
      prio = collapse_priority(ci);
      if (prio >= 0.0 && prio < best_prio)
      {
	best_prio = prio;
	collapse_target = heh;
      }
    }
  }



  // target found -> put vertex on heap
  if (collapse_target.is_valid())
  {
    v.collapse_target = collapse_target;
    v.priority        = best_prio;

    if (heap_->is_stored(&v))  heap_->update(&v);
    else                       heap_->insert(&v);
  }

  // not valid -> remove from heap
  else
  {
    if (heap_->is_stored(&v))  heap_->remove(&v);

    v.collapse_target = collapse_target;
    v.priority        = -1;
  }
}


//-----------------------------------------------------------------------------


template <class Mesh>
unsigned int
DecimaterT<Mesh>::decimate( unsigned int _n_collapses )
{
   typename Mesh::VertexIter         v_it, v_end(mesh_.vertices_end());
   typename Mesh::Vertex             *vp;
   typename Mesh::HalfedgeHandle     v0v1;
   typename Mesh::VertexVertexIter   vv_it;
   typename Mesh::VertexFaceIter     vf_it;
   unsigned int                      n_collapses(0);
   ProgMeshInfo                      pminfo;

   typedef std::vector<typename Mesh::VertexHandle>  Support;
   typedef typename Support::iterator                SupportIterator;
   
   Support            support(15);
   SupportIterator    s_it, s_end;


   // check _n_collapses
   if (!_n_collapses) _n_collapses = mesh_.n_vertices();

  
   // initialize heap
   heap_ = new DeciHeap();
   heap_->reserve(mesh_.n_vertices());

   for (v_it = mesh_.vertices_begin(); v_it != v_end; ++v_it)
   {
      heap_->reset_heap_position(&(*v_it));
      if (!v_it->deleted())
         heap_vertex(v_it.handle());         
   }


   // reserve mem for prog mesh info
   if (progmesh_info_)
      progmesh_info_->reserve(_n_collapses);



   // process heap
   while ((!heap_->empty()) && (n_collapses < _n_collapses))
   {
      // get 1st heap entry
      vp   = heap_->front();
      v0v1 = vp->collapse_target;
      heap_->pop_front();


      // setup collapse info
      CollapseInfo ci(mesh_, v0v1);


      // check topological correctness AGAIN !
      if (!is_collapse_legal(ci))
         continue;
    

      // store support (= one ring of *vp)
      vv_it = mesh_.vv_iter(ci.v0);
      support.clear();
      for (; vv_it; ++vv_it)
         support.push_back(vv_it.handle());
    

      // store collapse/split information
      if (progmesh_info_)
      {
         pminfo.v0 = ci.v0;
         pminfo.v1 = ci.v1;
         pminfo.vl = ci.vl;
         pminfo.vr = ci.vr;

         progmesh_info_->push_back(pminfo);
      }


      // perform collapse
      mesh_.collapse(v0v1);
      ++n_collapses;


      // update triangle normals
      vf_it = mesh_.vf_iter(ci.v1);
      for (; vf_it; ++vf_it)
         if (!vf_it->deleted())
            vf_it->set_normal(mesh_.calc_face_normal(vf_it.handle()));

    
      // update quality modules
      update_modules(ci);

      // independent sets -> lock one ring of target vertex
      if (independent_sets_)
      {
         mesh_.vertex(ci.v1).set_locked(true);
         vv_it = mesh_.vv_iter(ci.v1);
         for (; vv_it; ++vv_it)
            vv_it->set_locked(true);
      }


      // update heap (former one ring of decimated vertex)
      for (s_it = support.begin(), s_end = support.end();
           s_it != s_end; ++s_it)
      {
         assert(!mesh_.vertex(*s_it).deleted());
         heap_vertex(*s_it);
      }
   }


   // delete heap
   delete heap_;
   heap_ = NULL;


   // DON'T do garbage collection here! It's up to the application.


   return n_collapses;
}


//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================

