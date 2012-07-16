/*
@file FXSmoothCanvas.h

FXSmoothCanvas supports direct manipulation and editing of a
piecewise linear frequency transfer curve.
*/
#ifndef FXSMOOTHCANVAS_H
#define FXSMOOTHCANVAS_H

#include "fx.h"
#include <vector>

typedef std::pair<double, double> Joint;
typedef std::pair<FXint, FXint> Point;

class FXSmoothCanvas : public FXCanvas
{
	FXDECLARE(FXSmoothCanvas)
private:
	// Coordinates of the peeks in the canva
	std::vector<Point> canvas_points;
	// Frequency transfer
	std::vector<Joint> values;

	Point origin, endx, endy;
	FXint factor, radius;
	// The current selected point
	FXint selected_point;
	bool update;
	std::pair<double, double> range;
	bool realtime;

public:

	FXSmoothCanvas() {}
	FXSmoothCanvas(FXComposite*, FXObject*, FXSelector, FXuint, FXApp*);
	void draw();

	// Functions to modify the joint points of the function
	void update_canvas_points();
	void selectPoint(FXint x, FXint y);
	void addDeletePoint(FXint x, FXint y);
	void movePoint(FXint prev_x, FXint prev_y, FXint x, FXint y);
	void toggleRealtime(){realtime=!realtime;}

	// Set the attributes of the class
	void setUpdate(bool b) {	update = b;		}
	void setSelectedPoint(int s) {		selected_point = s;		}
	void setYRange(double yrange) {		range.second = yrange;	}
	std::vector<Joint> getValues() {		return values;		}

	// Compute distance between two points
	double distance (Point a, Point b);
	// Compute the distance of a point to a line
	double linePointDistance(Point p0, Point p1, Point p); 
	// Convert a point on canvas to a joint of the function
	Joint convertToValue(Point pt);
	// Convert a joint of the function to a point
	Point convertToPoint(Joint j);
	// Reset the function
	void reset();
};

#endif