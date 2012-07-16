/*
@file FXGLAxis.h

FXGLAxis draws the colored coordinate axes. It can be scaled,
and knows how to draw itself.
*/
#ifndef FXGLAXIS_H
#define FXGLAXIS_H

// this is how FXGLObject does it.
#include "fx.h"
#include "fx3d.h"

class FXAPI FXGLAxis : public FXGLShape
{
	FXDECLARE(FXGLAxis)

public:

	//	FXVec xdirection, ydirection;
	double scale;

	/// these functions are for FXGLObject
	/// constructor that takes a new implicit
	FXGLAxis(double _scale=1.0)
		:FXGLShape(0,0,0,SHADING_SMOOTH|STYLE_WIREFRAME)
	{
		scale=_scale;
	}

	
	void setScale(double _scale) 
	{
		scale = _scale;
	}

	/// Draw this object in a viewer
	virtual void drawshape(FXGLViewer* viewer)
	{
		glPushMatrix();
		// scale based on zoom, also should depend on field of view
		//double zoom=scale/viewer->getZoom();
		//glScaled(zoom,zoom,zoom);
		glScaled(scale,scale,scale);

		// set line weight
		glLineWidth(2);

		glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINES);

		glColor3d(1,0,0);
		glVertex3f(0,0,0);
		glVertex3f(1,0,0);

		glColor3d(0,1,0);
		glVertex3f(0,0,0);
		glVertex3f(0,1,0);

		glColor3d(0,0,1);
		glVertex3f(0,0,0);
		glVertex3f(0,0,1);

		glEnd();
		glPopMatrix();
	}

	/// Draw this object in a viewer
	virtual FXbool canDrag() const{return false;}
};


#endif