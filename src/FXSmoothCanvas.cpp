#include "WxyzMainWindow.h"
#include "FXSmoothCanvas.h"

using namespace std;

// Macro for the GLViewWindow class hierarchy implementation
FXIMPLEMENT(FXSmoothCanvas,FXCanvas,0,0)

FXSmoothCanvas::FXSmoothCanvas (FXComposite* p, FXObject* tgt=NULL, FXSelector sel=0, 
								FXuint opts=FRAME_NORMAL, FXApp* ap=NULL) 
								: FXCanvas(p, tgt, sel, opts)
{
	//app=ap;
	values.clear();
	values.push_back(make_pair(0., 1.0));
	values.push_back(make_pair(1., 1.0));
	factor = 6;
	update = false;
	radius = 3;
	selected_point = -1;
	realtime=false;
	range = make_pair(1., 2.);
}

void FXSmoothCanvas::draw()
{
    // Get DC for the canvas
    FXDCWindow dc(this);

	// Clean the canvas
	dc.setForeground(this->getBackColor());
	dc.fillRectangle(0,0,getWidth(),getHeight());
	
	dc.setForeground(fxcolorfromname("Black"));

	if (!update)
		update_canvas_points();

	// Draw the axes
    dc.drawLine(origin.first, origin.second, endx.first, endx.second);
    dc.drawLine(origin.first, origin.second, endy.first, endy.second);

	// draw the frequency values
	dc.setForeground(fxcolorfromname("Red"));

	Point prev = canvas_points[0], next;
	dc.setLineWidth(4);
	for (unsigned int i=1; i<canvas_points.size(); i++) {
		next = canvas_points[i];
		dc.drawLine(prev.first, prev.second, next.first, next.second);
		prev = next;
	}
	// draw the joints
	dc.setForeground(fxcolorfromname("Blue"));
	dc.setLineWidth(1);
	for (unsigned int i=0; i<canvas_points.size(); i++) {
		Point pt = canvas_points[i];
		int d = radius + radius;
		dc.drawArc(pt.first-radius, pt.second-radius, d, d, 0, 64*360);
	}	
	// add the value on the axis
	int number = int(range.second);
	int decimal, sign;
	FXFont *f = new FXFont(this->getApp(),"Times New Roman",7);
	f->create();

	dc.setFont(f);
	for (int i=0; i<=number; i++) {
		Point p = convertToPoint(Joint(0.,i));
		dc.drawText(p.first-10, p.second+5, fcvt(double(i), 1, &decimal, &sign), 1);
	}
}

void FXSmoothCanvas::update_canvas_points()
{
	FXint width = getWidth(), height = getHeight();
	origin = make_pair(width/factor, height - height/factor);
	endx = make_pair(width - width/factor, height - height/factor);
	endy = make_pair(width/factor, height/factor);

	canvas_points.clear();
	for (unsigned int i=0; i<values.size(); i++) {
		Joint p = values[i];

		// convert the values into coordinates
		canvas_points.push_back(convertToPoint(p));
	}
	update = true;
}

void FXSmoothCanvas::addDeletePoint(FXint x, FXint y)
{
	Point prev, next, pt;

	pt = make_pair(x, y);
    // Loop over the lines and check if the point is close enough to any of them
	// Delete a point if clicked
	for (unsigned int i=1; i<canvas_points.size()-1; i++) {
		prev = canvas_points[i];
		if (distance(prev, pt) <= radius) {
			canvas_points.erase(canvas_points.begin()+i);
			values.erase(values.begin()+i);
			draw();
			return;
		}
	}

	// Add a point if there was none selected
	prev = canvas_points[0];
	for (unsigned int i=1; i<canvas_points.size(); i++) {
		next = canvas_points[i];
		// check the x range
		if (pt.first < (prev.first + radius) || pt.first > (next.first - radius)) {
			prev = next;
			continue;
		}

		// test the distance of the point to any of the lines
		double dist = linePointDistance(prev, next, pt);
		if (dist <= radius) {
			canvas_points.insert(canvas_points.begin()+i, pt);
			values.insert(values.begin()+i, convertToValue(pt));
			update = false;
			draw();
			return;
		}
		prev = next;
	}
}

void FXSmoothCanvas::selectPoint(FXint x, FXint y)
{
	Point point(x, y);
	selected_point = -1;
	for (unsigned int i=0; i<canvas_points.size(); i++) {
		Point pt = canvas_points[i];
		double dist = distance (pt, point);
		if (dist <= radius) {
			selected_point = i;
			break;
		}
	}
}

void FXSmoothCanvas::movePoint(FXint prev_x, FXint prev_y, FXint x, FXint y)
{
	if (selected_point >= 0) {
		Point pt, pc(x,y);
        // Verify the constraints - the position of the
        // mouse could be outside of the function range
        if (selected_point == 0 || selected_point == (canvas_points.size()-1))
            // constrain the movement of the end points
            pt.first = canvas_points[selected_point].first;
        else
            pt.first = origin.first > pc.first ? 
						origin.first : (endx.first < pc.first ? endx.first : pc.first);

        // constrain the lateral movements:
        // collision detection with the points on its side
	    if (selected_point > 0) {
                Point nei = canvas_points[selected_point-1];
                pt.first = (pt.first <= nei.first) ? (nei.first+1) : pt.first;
	    }
	    if (selected_point < canvas_points.size()-1) {
                Point nei = canvas_points[selected_point+1];
                pt.first = (pt.first >= nei.first) ? (nei.first-1) : pt.first;
	    }

        pt.second = origin.second < pc.second ? origin.second : 
						(endy.second > pc.second ? endy.second : pc.second);

        //update the point
        canvas_points[selected_point] = pt;
        values[selected_point] = convertToValue(pt);
        draw();

		// update real time
		if (this->target && realtime)
			((WxyzMainWindow *)(this->target))->onApplyRealTime(0,0,0);
	}
}

double FXSmoothCanvas::distance (Point a, Point b) {
	return sqrt((a.first-b.first)*(a.first-b.first)+(a.second-b.second)*(a.second-b.second));
}

double FXSmoothCanvas::linePointDistance(Point p0, Point p1, Point p) {
    Point v(p1.first-p0.first, p1.second-p0.second);
    Point w(p.first-p0.first, p.second-p0.second);
    double c1 = (double) w.first*v.first + w.second*v.second;
    double c2 = (double) v.first*v.first + v.second*v.second;
    double b = c1/c2;
    Point pb(p0.first + b*v.first, p0.second + b*v.second);

	return distance(pb, p);
}

Joint FXSmoothCanvas::convertToValue(Point pt)
{
	Joint res;
	res.first = ((double) pt.first-origin.first)/((double) endx.first-origin.first) * range.first;
	res.second = range.second - ((double) pt.second-endy.second)/((double) origin.second-endy.second) * (double) range.second;

	return res;
}

Point FXSmoothCanvas::convertToPoint(Joint j)
{
	Point res;
	res.first = origin.first + (endx.first - origin.first) * (j.first/range.first);
	res.second = origin.second + (endy.second - origin.second) * (j.second/range.second);

	return res;
}

void FXSmoothCanvas::reset()
{
	canvas_points.clear();
	values.clear();
	values.push_back(make_pair(0., 1.0));
	values.push_back(make_pair(1., 1.0));
	update = false;
}
