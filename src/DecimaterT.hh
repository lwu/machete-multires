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

/** \file DecimaterT.hh
    
 */

//=============================================================================
//
//  CLASS DecimaterT
//
//=============================================================================

#ifndef OPENMESH_DECIMATER_DECIMATERT_HH
#define OPENMESH_DECIMATER_DECIMATERT_HH


//== INCLUDES =================================================================

#include "Config.hh"
// --------------------
#include <string>
// --------------------
#include <OpenMesh/Mesh/Traits.hh>
#include <OpenMesh/Mesh/Types/TriMesh_ArrayKernelT.hh>
#include <OpenMeshTools/Utils/HeapT.hh>
// --------------------
#include "ModBaseT.hh"


//== NAMESPACE ================================================================

namespace OpenMesh { // BEGIN_NS_OPENMESH
namespace Decimater { // BEGIN_NS_DECIMATER


//== CLASS DEFINITION =========================================================
	      
/** Decimater framework.
    \see BaseMod
*/

template < typename Mesh >
class DecimaterT
{
public:
   
   // Typedefs
   typedef DecimaterT< Mesh >     Self;
   
   typedef CollapseInfoT<Mesh>    CollapseInfo;
   typedef ModBaseT<Mesh>         Module;
   typedef std::vector< Module* > ModuleList;

   /// Constructor
   DecimaterT(Mesh& _mesh) : 
         mesh_(_mesh), progmesh_info_(NULL), independent_sets_(false), heap_(NULL),
         cmodule_(NULL)
   {}

   
   /// Destructor
   ~DecimaterT()
   {
      delete_modules();
      delete heap_;
   }

  
   /// Initialize (also calls Mod's initialize)
   void initialize();


   /// Registration of a module
   template< typename ModuleType > 
   bool registrate(const ModuleType& _mod) 
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

   bool registrate( const Module& _mod )
   {
	   Module* new_mod = new Module(_mod);

	   if ( new_mod->is_binary() )
	   {
		   bmodules_.push_back(new_mod);
	   }
	   else      
	   {
		   delete cmodule_;
		   cmodule_ = new_mod;
	   }
	   return true;
   }
   
   /** Decimate (perform _n_collapses collapses). Return number of 
       performed collapses. If _n_collapses is not given reduce as
       much as possible */
   unsigned int decimate( unsigned int _n_collapses = 0 );
  

   /** This piece of information defines a halfedge collapse and the
       inverse vertex split */
   struct ProgMeshInfo 
   { 
      typename Mesh::VertexHandle v0, v1, vl, vr; 
   };

   typedef std::vector<ProgMeshInfo> ProgMeshInfoContainer;


   /** Turn on/off the generation of collapse/split information. For each
       collapse an info structure ProgMeshInfo is appended to
       the provided container. Call w/o argument to turn off generation. */
   void generate_progmesh_info(ProgMeshInfoContainer* _info_container=0)
   { progmesh_info_ = _info_container; }

   void generate_progmesh_info(ProgMeshInfoContainer& _info_container)
   { generate_progmesh_info( &_info_container ); }

  
   /// Turn on/off whether independent sets should be generated. 
   void generate_independent_sets(bool _b) 
   { independent_sets_ = _b; }

private:

   void update_modules(CollapseInfo& _ci)
   {
      typename ModuleList::iterator m_it, m_end = bmodules_.end();

      for (m_it = bmodules_.begin(); m_it != m_end; ++m_it)
         (*m_it)->postprocess_collapse(_ci);
      cmodule_->postprocess_collapse(_ci);
   }

   void delete_modules(void)
   {
      typename ModuleList::iterator m_it, m_end = bmodules_.end();
      for(m_it = bmodules_.begin(); m_it != m_end; ++m_it)
      {
         Module *mod = *m_it;
         delete mod;
      }
      bmodules_.clear();
      delete cmodule_;
      cmodule_ = NULL;
   }
   
private:

   /// Insert vertex in heap
   void heap_vertex(typename Mesh::VertexHandle _vh);

   /// Is an edge collapse legal?  Performs topological test only
   bool is_collapse_legal(const CollapseInfo& _ci);

   /// Calculate priority of an halfedge collapse (using the modules)
   float collapse_priority(const CollapseInfo& _ci);


   // heap interface
   struct HeapInterface
   {
      static inline bool
      less( const typename Mesh::Vertex* _vp0,
            const typename Mesh::Vertex* _vp1 )
      { return (_vp0->priority < _vp1->priority); }
    
      static inline bool
      greater( const typename Mesh::Vertex* _vp0,
               const typename Mesh::Vertex* _vp1 )
      { return (_vp0->priority > _vp1->priority); }
    
      static inline int
      get_heap_position(const typename Mesh::Vertex* _vp)
      { return _vp->heap_position; }
    
      static inline void
      set_heap_position(typename Mesh::Vertex* _vp, int _pos)
      { _vp->heap_position = _pos; }
   };

   
   // actual heap type
   typedef Utils::HeapT<typename Mesh::Vertex*, HeapInterface>  DeciHeap;

   
   // heap
   DeciHeap*  heap_;

   
   // reference to mesh
   Mesh&  mesh_;

   
   // list of modules
   ModuleList bmodules_;
   Module*    cmodule_;
   
   // progressive mesh information
   std::vector<ProgMeshInfo>*  progmesh_info_;

   
   // produce independent sets?
   bool  independent_sets_;

private: // Noncopyable
   DecimaterT(const Self&);
   Self& operator = (const Self&);
};


//== CLASS DEFINITION =========================================================


struct DefaultTraits : public OpenMesh::DefaultTraits
{
   VertexAttributes( OpenMesh::DefaultAttributer::Status );
   EdgeAttributes( OpenMesh::DefaultAttributer::Status );
   FaceAttributes( OpenMesh::DefaultAttributer::Status |
                   OpenMesh::DefaultAttributer::Normal );
   
   VertexTraits
   {      
   public:
      typedef typename Base::Refs::HalfedgeHandle HalfedgeHandle;
      
      VertexT() : priority(-1.0), heap_position(-1) {}
      
      HalfedgeHandle  collapse_target;
      float           priority;
      int             heap_position;
      int             orig_idx;
   };
};

//=============================================================================
} // END_NS_DECIMATER
} // END_NS_OPENMESH
//=============================================================================
#if defined(INCLUDE_TEMPLATES) && !defined(OPENMESH_DECIMATER_DECIMATERT_CC)
#define OPENMESH_DECIMATER_TEMPLATES
#include "DecimaterT.cc"
#endif
//=============================================================================
#endif // OPENMESH_DECIMATER_DECIMATERT_HH defined
//=============================================================================

