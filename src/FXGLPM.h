/*
@file FXGLPM.h

FXGLPM is-a FXGLShape and a ProgressiveMesh. Because it is a FOX
GL scene object, it can be displayed and edited. Because it is a
ProgressiveMesh, it load 3D models, and can simplify and refine.
*/

#ifndef FXGLPM_H
#define FXGLPM_H

#include <vector>
#include "fx.h"
#include "fx3d.h"
#include "ProgressiveMesh.h"
#include "MeshOp.h"

using namespace std;

class FXAPI FXGLPM : public FXGLShape, public ProgressiveMesh
{

	FXDECLARE(FXGLPM)

protected:
	float getcurve(float x);

public:

	static int GLPointSize;

	bool shadingStyle;

	// Selection data
	int selectedVertexId;
	int selectionNeighborDepth;
	std::vector < std::vector <PM::VertexHandle> > selectedVertices;

	bool flagSphere;	// flag to test whether sphere test is turned on
	PM::Point center;		// center of the selection
	float radius;		// radius of the selected region
	float dminscale;

	float getP (bool flag, PM::Point ver);

	/// Find the new set of selected vertices
	void updateSelection();

	FXVec getVertCordFromId ( int );
	FXVec getVertCordFromId ( PM::VertexHandle );

	PM::Point getPoint(int vId) {	return mesh.point(PM::VertexHandle(vId));	}
	PM::Point getPoint(PM::VertexHandle vh)	{	return mesh.point(vh);		}

	/// Apply the smoothing or enhancement operation
	void applyOperation(std::vector<std::pair<double, double> >);

	void drawMesh();
	void drawVisuals();

	/****************Fox needed functions*************/

	/// constructor that takes a new implicit
	FXGLPM();

	/// Destructor
	virtual ~FXGLPM();

	/// Draw this object in a viewer
	virtual void drawshape(FXGLViewer* viewer);
 
	/// Return the bounds that determine the camera angle
	virtual void bounds(FXRange &box);

	/// Handles what to do with the selection buffer
	virtual FXGLObject* identify(FXuint* path);

	/// Handle dragging on a selection
	virtual FXbool drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty);

	/// Draw when it is selected
	virtual void hit(FXGLViewer* viewer);
};

#endif
