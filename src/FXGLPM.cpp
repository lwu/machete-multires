/*
@file FXGLPM.cpp
*/

#include "FXGLPM.h"

// Object implementation
FXIMPLEMENT(FXGLPM,FXGLShape,0,0)

int FXGLPM::GLPointSize=8;

// default constructor
FXGLPM::FXGLPM()
:FXGLShape(0,0,0,SHADING_NONE|STYLE_WIREFRAME)
{
	selectedVertexId=-1;
	selectionNeighborDepth=4;
	shadingStyle=false;
	flagSphere = false;
	dminscale=0.75;
}

// destructor
FXGLPM::~FXGLPM()
{
}

// Specialize generic glVertex and glNormal fns
template <class Scalar> void glVertex(const Scalar* v);
template <> void glVertex(const float* v) { glVertex3fv(v); }
template <> void glVertex(const double* v) { glVertex3dv(v); }

template <class Scalar> void glNormal(const Scalar* v);
template <> void glNormal(const float* v) { glNormal3fv(v); }
template <> void glNormal(const double* v) { glNormal3dv(v); }

// Draw this object in a viewer
void FXGLPM::drawshape(FXGLViewer* viewer)
{
	// Draw selected points and detail vectors
	drawVisuals();

	// Draw triangle mesh
	drawMesh();
	
}

void FXGLPM::drawMesh()
{
	if (shadingStyle)
	{
		glEnable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
	}

	// New points
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_TRIANGLES);
	for (PM::FaceIter f_it=mesh.faces_begin(); f_it!=mesh.faces_end();++f_it)
	{
		if (mesh.face(f_it.handle()).deleted())
			continue;

		glNormal<PM::Scalar>(mesh.calc_face_normal(f_it.handle()));//f_it->normal());
		for (PM::FaceVertexIter fv_it=mesh.fv_iter(f_it.handle());fv_it;++fv_it)
		{
			glVertex<PM::Scalar>(mesh.point(*fv_it));			
		}
	}
	glEnd();

	if (shadingStyle)
	{
		// Only triangle mesh should be lit
		glDisable(GL_LIGHTING);
	}
}

// Draw selected points and detail vectors
void FXGLPM::drawVisuals()
{
	// smooth points
	glEnable(GL_BLEND);
	glEnable(GL_POINT_SMOOTH);
	// change the size
	glPointSize(GLPointSize*0.75f);

	// draw the selections
	glBegin(GL_POINTS);
	for (unsigned int i=0; i<selectedVertices.size(); i++)
	{
		float color=1.0f/selectionNeighborDepth*i;
		glColor3f(0,1-color,0);
		std::vector <PM::VertexHandle> &currentLevel=(selectedVertices[i]);
		for (unsigned int j=0; j<currentLevel.size(); j++)
			glVertex<PM::Scalar>(mesh.point(currentLevel[j]));
	}
	glEnd();
	
	// the vertex selected by the mouse.
	glPointSize((GLfloat)GLPointSize);
	glBegin(GL_POINTS);	
	glColor3f(1,0,0);
	if (selectedVertexId!=-1)
		glVertex<PM::Scalar>(mesh.point(PM::VertexHandle(selectedVertexId)));
	glEnd();


	// draw detail vectors	
	if (pmIter != NULL && pmIter != pmInfos.end()) {
		// draw the displacement vectors of last refinement
		PM::VertexHandle vh = (pmIter)->v0;
		PM::Vertex vertex = mesh.vertex(vh);
		std::vector<PM::Point> detailVectors = vertex.detailVectors;
		glColor3d(1,0,0); // first vertex detail vector red

		// draw the displacement for the vertex being inserted
		if (detailVectors.size() > 0)
		{
			glBegin(GL_LINES);
			glVertex<PM::Scalar>(mesh.point(vh));
			glVertex<PM::Scalar>(mesh.point(vh) + detailVectors[0]);
			glEnd();
		}

		PM::VertexOHalfedgeIter he_it = mesh.voh_iter(vh);

		glColor3d(1,1,0); // other details yellow
		// For every vertex, loop through all outgoing edges
		for ( int i=1; he_it, i<detailVectors.size(); ++he_it, i++)
		{
			PM::HalfedgeHandle outgoing_heh = he_it.handle();

			// Get vertex handle of vertex on 1-ring neighborhood
			PM::VertexHandle outer_vh = mesh.to_vertex_handle(outgoing_heh);

			glBegin(GL_LINES);
			glVertex<PM::Scalar>(mesh.point(outer_vh));
			glVertex<PM::Scalar>(mesh.point(outer_vh) + detailVectors[i]);
			glEnd();
		}
	}
}

/// Return the box that bounds the object
void FXGLPM::bounds(FXRange &box)
{
	box = FXRange(bbox_min[0], bbox_max[0], bbox_min[1], bbox_max[1], bbox_min[2], bbox_max[2]);
}

/// Handles selection
void FXGLPM::hit(FXGLViewer* viewer)
{
	// draw
	glPushName(0xffffffff);

	// this is the only thing that can be selected
	// draw vertices, selection is based on vertices
	for (PM::VertexIter v_it=mesh.vertices_begin();v_it!=mesh.vertices_end();++v_it)
	{
		if (mesh.vertex(v_it.handle()).deleted())
			continue;
		// identify a selection
		int id=v_it.handle().idx();
		glLoadName(id);
		glBegin(GL_POINTS);		
		glVertex<PM::Scalar>(mesh.point(*v_it));
		glEnd();
	}
	// draw the selection again highlighted
	// clean up
	glPopName();
	
	viewer->update();
}


FXGLObject* FXGLPM::identify(FXuint* path)
{
	// which edge is selected
	selectedVertexId=path[0];
	// update the effective selections
	updateSelection();

	return this;
}

FXbool FXGLPM::drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty)
{
	// see if anything is selected
	if (selectedVertexId==-1)
		return false;

	// find what should be moved.
	FXfloat zz=viewer->worldToEyeZ(position);
	FXVec wf=viewer->eyeToWorld(viewer->screenToEye(fx,fy,zz));
	FXVec wt=viewer->eyeToWorld(viewer->screenToEye(tx,ty,zz));
	FXVec delta=wt-wf;
   
	// compute the new detail vector

	// move the selections
	PM::VertexHandle vh(selectedVertexId);
	mesh.set_point(vh, mesh.point(vh) + PM::Point(delta[0],delta[1],delta[2]));
	// scale the falloff
	for(unsigned int i=0; i<selectedVertices.size();i++)
	{
		delta+=-delta/(float)selectionNeighborDepth*(float)i;
		for(unsigned int j=0; j<selectedVertices[i].size();j++)
		{
			vh=selectedVertices[i][j];
			mesh.set_point(vh, mesh.point(vh) + PM::Point(delta[0],delta[1],delta[2]));
		}
	}

	return true;
}

/// the selected one by the mouse is not in there for efficiency reasons.
void FXGLPM::updateSelection()
{
	// do some preprocessing for update.
	selectedVertices.clear();
	// clear the tag bit for the mesh.
	for (PM::VertexIter v_it=mesh.vertices_begin();v_it!=mesh.vertices_end();++v_it)
		v_it->set_tagged(false);

	if (selectedVertexId==-1) return;

	PM::VertexHandle vh(selectedVertexId);
	if (vh.is_valid())
	{
		mesh.vertex(vh).set_tagged(true);
	}

	// find the neighbors
	if (selectedVertexId!=-1)
	{
		std::vector<PM::VertexHandle> initVertices;
		initVertices.push_back(vh);
		findNRingNeighborhood <PM> (mesh, initVertices, selectionNeighborDepth, selectedVertices);
	}
}
FXVec FXGLPM::getVertCordFromId ( int vId)
{
	FXVec v;
	PM::Point p = mesh.point(PM::VertexHandle(vId));
	v[0] = (FXfloat)p[0];
	v[1] = (FXfloat)p[1];
	v[2] = (FXfloat)p[2];
	return v;
}

FXVec FXGLPM::getVertCordFromId ( PM::VertexHandle vId)
{	
	FXVec v;
	v[0] = (FXfloat)mesh.point(vId)[0];
	v[1] = (FXfloat)mesh.point(vId)[1];
	v[2] = (FXfloat)mesh.point(vId)[2];
	return v;
}

float FXGLPM::getcurve(float x)
{
	if ( x<=0 ) return 0;
	if ( x>=1 ) return 1.0;
	if ( x<=0.5 ) return x*x*2.0f;
	return 1.0f-2.0f*(x-1.0f)*(x-0.1f);
}

float FXGLPM::getP(bool flag, PM::Point ver)
{
	typedef PM::Scalar Scalar;

	if (!flagSphere) return 1.0;

	if (dminscale>=1 || dminscale<=0 ) dminscale=0.75;

	float d = (float)(ver-center).length();
	float dmax = radius;
	float dmin = radius*dminscale;

	Scalar x= (d-dmin)/(dmax-dmin);;
	if ( flag )
	{
			//	printf("......%f\n",1.0-getcurve(x));
		return 1.0-getcurve(x);
	}

	return getcurve(x);
}

void FXGLPM::applyOperation(vector<pair<double, double> > values)
{
	typedef pair<double, double> Joint;

	coarsenToLevelN(0);

	Joint prev = values[0], next;
	int prev_index=0, next_index=0;
	int detail_size = vertexOrdering.size();

	for (int i=1; i<values.size(); i++) 
	{
		next = values[i];
		next_index = next.first * detail_size;
		for (int j=prev_index; j<next_index; j++) 
		{
			// interpolated value of two closest joints of the function
			double value = prev.second + (next.second - prev.second) * double(j-prev_index)/(next_index-prev_index);

			PM::VertexHandle vh = vertexOrdering[j];
			PM::Vertex& vertex = mesh.vertex(vh);
			for (int m=0; m < vertex.detailVectors.size(); m++)
			{
				double t_value = 1 + getP(true, mesh.point(vh)) * (value - 1);
				vertex.detailVectors[m] *= t_value;
			}
		}
		prev_index = next_index;
		prev = next;
	}
	// process the last vertex of the list
	if (vertexOrdering.size() > 0) {
		PM::VertexHandle vh = vertexOrdering[vertexOrdering.size()-1];
		PM::Vertex& vertex = mesh.vertex(vh);
		for (int m=0; m < vertex.detailVectors.size(); m++)
		{
			double t_value = 1 + getP(true, mesh.point(vh)) * (prev.second - 1);
			vertex.detailVectors[m] *= t_value;
		}
	}
	restoreDetailVectors(getMaxLevel());
}