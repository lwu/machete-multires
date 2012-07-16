/*
@file FXGLGrid.h

FXGLGrid is a 2-D grid for the plane. It can be scaled, the
step size can be modified, and it knows how to draw itself.
*/
#ifndef FXGLGRID_H
#define FXGLGRID_H

// this is how FXGLObject does it.
#include "fx.h"
#include "fx3d.h"

class FXAPI FXGLGrid : public FXGLShape
{
	FXDECLARE(FXGLGrid)

public:

	//	FXVec xdirection, ydirection;
	int steps, size;
	double scale;

	/// these functions are for FXGLObject	
	FXGLGrid(double _scale=1.0, int _steps=10, int _size=1)
		:FXGLShape(0,0,0,SHADING_SMOOTH|STYLE_WIREFRAME)
	{
		steps=_steps;
		size=_size;
		scale=_scale;
	}

	void setScale(double _scale) 
	{
		scale = _scale;
	}

	/// Draw this object in a viewer
	virtual void drawshape(FXGLViewer* viewer)
	{
		// Dark gray
		glColor3f(0.46f, 0.46f, 0.46f);

		int i;
		glPushMatrix();
		// scale based on zoom, also should depend on field of view
		//double zoom=scale/viewer->getZoom();
		glScaled(scale,scale,scale);
		glBegin(GL_LINES);
		for(i=-steps;i<=steps;i++)
		{
			glVertex3d(i*size,0,-steps*size);
			glVertex3d(i*size,0,steps*size);
		}
		for(i=-steps;i<=steps;i++)
		{
			glVertex3d(-steps*size,0,i*size);
			glVertex3d(steps*size,0,i*size);
		}
		glEnd();
		glPopMatrix();
	}

	/// Draw this object in a viewer
	virtual FXbool canDrag() const{return false;}
};


#endif