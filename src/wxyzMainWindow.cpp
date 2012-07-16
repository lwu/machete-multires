/*
@file wxyzMainWindow.cpp

(Main window UI)

Machete 3D is a multiresolution mesh editor and signal processing tool.
It implements ideas from "Multiresolution Signal Processing for Meshes",
by Guskov, Sweldens, and Schr\"oder.

Developers:

Leslie Wu
Wen Su
Zheng Sun
Jingyi Jin
*/

#include "wxyzMainWindow.h"
#include "UnitTests.h"

// Macro for the GLViewWindow class hierarchy implementation
FXIMPLEMENT(WxyzMainWindow,FXMainWindow,WxyzMainWindowMap,ARRAYNUMBER(WxyzMainWindowMap))


// Construct a GLViewWindow
WxyzMainWindow::WxyzMainWindow(FXApp* a)
:FXMainWindow(a,"Machete 3D",NULL,NULL,DECOR_ALL,0,0,800,600)
{
	initGUI();
	initVariables();
	updateScene();

	flagSphere = false;
	oldradius = -1;

}

// Create and initialize
void WxyzMainWindow::create()
{
	FXMainWindow::create();
	show(PLACEMENT_SCREEN);
}

// Destructor
WxyzMainWindow::~WxyzMainWindow()
{
	delete filemenu;
	delete helpmenu;
}

void WxyzMainWindow::initGUI()
{
	FXIcon *foxicon=new FXGIFIcon(getApp(),cowiconChar);
	FXIcon *fileopenicon=new FXGIFIcon(getApp(),fileopeniconChar);
	FXIcon *filesaveicon=new FXGIFIcon(getApp(),filesaveiconChar);
	FXIcon *miloIcon=new FXGIFIcon(getApp(),miloIconChar);
	
	setIcon(foxicon);

	// Menu bar
	FXToolBarShell *dragshell1=new FXToolBarShell(this,FRAME_RAISED);
	menubar=new FXMenuBar(this,dragshell1,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_RAISED);
	new FXToolBarGrip(menubar,menubar,FXMenuBar::ID_TOOLBARGRIP,TOOLBARGRIP_DOUBLE);

	filemenu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"&File",NULL,filemenu);
	new FXMenuCommand(filemenu,"&Open...\tCtl-O\tOpen file.",fileopenicon,this,ID_OPEN);
	new FXMenuCommand(filemenu,"&Save As...\tCtl-S\tSave file.",filesaveicon,this,ID_SAVE);
	new FXMenuCommand(filemenu,"&Quit\tCtl-Q\tQuit the application.",NULL,getApp(),FXApp::ID_QUIT);

	helpmenu=new FXMenuPane(this);
	new FXMenuTitle(menubar,"&Help",NULL,helpmenu,LAYOUT_RIGHT);
	new FXMenuCommand(helpmenu,"&About...\t\tDisplay about panel.",foxicon,this,ID_ABOUT);

	// Tool bar
	FXToolBarShell *dragshell2=new FXToolBarShell(this,FRAME_RAISED);
	FXToolBar *toolbar=new FXToolBar(this,dragshell2,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_RAISED);
	new FXToolBarGrip(toolbar,toolbar,FXToolBar::ID_TOOLBARGRIP,TOOLBARGRIP_DOUBLE);
	new FXButton(toolbar,"\tOpen\tOpen file.",fileopenicon,this,ID_OPEN,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_LEFT);
	new FXButton(toolbar,"\tSave\tSave file.",filesaveicon,this,ID_SAVE,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_LEFT);
	new FXToggleButton(toolbar,"\tToggle Light\tToggle light source.",NULL,new FXGIFIcon(getApp(),nolight),new FXGIFIcon(getApp(),light),this,ID_SHADING,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_LEFT);
	new FXToggleButton(toolbar,"\tToggle Real time\tToggle Real time.",NULL,new FXGIFIcon(getApp(),parallel),new FXGIFIcon(getApp(),perspective),this,ID_REALTIME,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_TOP|LAYOUT_LEFT);

	// Make status bar
	statusbar=new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|STATUSBAR_WITH_DRAGCORNER);

	// Contents
	FXHorizontalFrame *frame=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0);
	// LEFT pane to contain the gldisplay and slider
	FXVerticalFrame *leftdisplayFrame=new FXVerticalFrame(frame,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,10,10,10,10);	
	// Drawing gldisplay	
	FXComposite *glpanel=new FXVerticalFrame(leftdisplayFrame,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0, 0,0,0,0);
	// A Visual to drag OpenGL
	glvisual=new FXGLVisual(getApp(),VISUAL_DOUBLEBUFFER|VISUAL_STEREO);
	// Drawing gldisplay
	gldisplay=new FXGLViewer(glpanel,glvisual,this,ID_GLVIEWER,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|LAYOUT_LEFT);
	gldisplay->setFieldOfView(60.0);
	// Slider
	FXHorizontalFrame *sliderFrame=new FXHorizontalFrame(leftdisplayFrame,LAYOUT_FILL_X);
	pmLevelSlider = new FXSlider(sliderFrame, this, ID_PM_LEVEL_CHANGE,
		LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0, 100,36 );
	pmLevelSlider->setHeadSize(24);
	//FXuint sliderOpt = pmLevelSlider->getSliderStyle();
	//sliderOpt |= SLIDER_INSIDE_BAR;
	//pmLevelSlider->setSliderStyle(sliderOpt);
	FXButton *reconstructButton = new FXButton(sliderFrame, 
		"Reconstruct\n&Detail", miloIcon, this, ID_RECOMPUTE_DETAIL);
	FXuint style=reconstructButton->getIconPosition();
	style|=ICON_AFTER_TEXT;
	style&=~ICON_BEFORE_TEXT;
	reconstructButton->setIconPosition(style);

	// RIGHT pane for displaying properties
	// add in LAYOUT_FILL_X to show all
	FXVerticalFrame *controlFrame = new FXVerticalFrame(frame, LAYOUT_FILL_Y,0,0,0,0,10,10,10,10);

	// ========= Upper half UI of the control panel ===============
	FXVerticalFrame *buttonFrame = new FXVerticalFrame(controlFrame, LAYOUT_FILL_X|JUSTIFY_CENTER_X, 0,0,0,0, 0,0,0,0);
	// Label above the buttons
	new FXLabel(buttonFrame,"Control",0,LAYOUT_FILL_X);
	// Horizontal divider line
	new FXHorizontalSeparator(buttonFrame,SEPARATOR_RIDGE|JUSTIFY_CENTER_X|LAYOUT_FILL_X);
	FXVerticalFrame *contents=new FXVerticalFrame(buttonFrame, LAYOUT_FILL_X);
	new FXButton(contents, "&Build Hierarchy", foxicon, this, ID_BUILD_PM);
	//pmLevelSlider = new FXSlider(contents, this, ID_PM_LEVEL_CHANGE,
	//	LAYOUT_CENTER_Y|LAYOUT_FILL_ROW|LAYOUT_FIX_WIDTH,0,0,100);		
	// new FXButton(contents, "Smooth", NULL, this, ID_SMOOTH_PM);	

	FXMatrix *selectionMatrix=new FXMatrix(contents,2,MATRIX_BY_COLUMNS);//|FRAME_THICK|FRAME_RAISED|LAYOUT_FILL_Y|LAYOUT_CENTER_X|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,10,10,10,10);
	new FXLabel(selectionMatrix, "Selection radius:");
	neighborhoodSelectionLevelSpinner= new FXSpinner(selectionMatrix, 2, this, ID_NEIGHBORHOOD_LEVEL_CHANGE);
	neighborhoodSelectionLevelSpinner->setRange(0,99);
	neighborhoodSelectionLevelSpinner->setValue(4);

	new FXButton(contents, "Set a sphere range", NULL, this, ID_SPHERE);
	radiusSlider = new FXDial(contents,this,ID_RADIUS_CHANGE,LAYOUT_CENTER_Y|LAYOUT_FILL_X|LAYOUT_FILL_ROW|LAYOUT_FIX_WIDTH|DIAL_HORIZONTAL|DIAL_HAS_NOTCH,0,0,100);

    FXMatrix* matrix=new FXMatrix(contents,2,MATRIX_BY_COLUMNS|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y);
	new FXLabel(matrix,"Input Dmin(0-1)",NULL,LAYOUT_CENTER_Y|LAYOUT_CENTER_X|JUSTIFY_RIGHT|LAYOUT_FILL_ROW);

	dminscale=0.75;
	dt.connect(dminscale);
	new FXTextField(matrix,5,&dt,FXDataTarget::ID_VALUE,TEXTFIELD_REAL|JUSTIFY_RIGHT|LAYOUT_CENTER_Y|LAYOUT_CENTER_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_ROW);


	// ========= Lower half UI of the control panel ===============
	FXHorizontalFrame *operationFrame = new FXHorizontalFrame(controlFrame, LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,100,0,0, 0,0,0,0);

	// Tab book with switchable panels
	FXTabBook* panels=new FXTabBook(operationFrame,NULL,0,LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0);

	// detail vector manipulation
	FXTabItem *item1 = new FXTabItem(panels,"Smooth\tSmooth operation\tApply smoothing to the model.");

	FXVerticalFrame *canvasFrame = new FXVerticalFrame(panels, LAYOUT_FILL_X);
	canvas=new FXSmoothCanvas(canvasFrame,this,ID_ABOUT,FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FILL_Y, getApp());
	canvas->setBackColor(this->getBackColor());
	FXHorizontalFrame *frame1 = new FXHorizontalFrame(canvasFrame, LAYOUT_FILL_X);
	new FXLabel(frame1,"Range",0,LAYOUT_FILL_Y);
	range = new FXSpinner(frame1, 2, this, ID_CHANGE_RANGE);
	range->setRange(1,10);
	range->setValue(2);
	new FXButton(frame1, "Reset", NULL, this, ID_RESET);
	new FXButton(frame1, "Apply", NULL, this, ID_APPLY);

	// the following tabs have only the purpose of making the canvas larger...
	new FXTabItem(panels,"Enhance\tEnhancement operation\tApply enhancement to the model.");
	FXMatrix *colors=new FXMatrix(panels,2,MATRIX_BY_COLUMNS|FRAME_THICK|FRAME_RAISED|LAYOUT_FILL_Y|LAYOUT_CENTER_X|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,10,10,10,10);
	new FXTabItem(panels,"Edit_____\tEditing operation\tEditing the model.");
	FXMatrix *cls=new FXMatrix(panels,2,MATRIX_BY_COLUMNS|FRAME_THICK|FRAME_RAISED|LAYOUT_FILL_Y|LAYOUT_CENTER_X|LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,10,10,10,10);
}

void WxyzMainWindow::initVariables()
{
	glGrid = new FXGLGrid(0.5);
	glAxis = new FXGLAxis(0.5);

	gldisplay->setBackgroundColor(FXHVec(0.63f,0.63f,0.63f));
	foxScene=new FXGLGroup();	
	foxScene->append(glGrid);
	foxScene->append(glAxis);
	pmMesh=new FXGLPM();
	foxScene->append(pmMesh);
}

void WxyzMainWindow::updateScene()
{
	gldisplay->setScene(foxScene);
	gldisplay->update();
}

// save
long WxyzMainWindow::onCmdSave(FXObject*,FXSelector,void*)
{
	const FXchar *patterns[]=
	{
		"OBJ Files", "*.obj",
		"Progressive Mesh Files", "*.pm",
		"All Files", "*",
		NULL
	};

	FXFileDialog save(this, "Save this model");
	save.setPatternList(patterns);
	if(save.execute())
	{		
		FXString filename = save.getFilename();
		pmMesh->writeFile(filename.text());
	}
	return 1;
}

// Open
long WxyzMainWindow::onCmdOpen(FXObject*,FXSelector,void*)
{
	const FXchar *patterns[]=
	{
		"OBJ Files", "*.obj",
		"Progressive Mesh Files", "*.pm",
		"All Files", "*",
		NULL
	};

	FXFileDialog open(this, "Load a 3D model");
	open.setPatternList(patterns);
	if(open.execute())
	{		
		// TODO figure out how to repaint text properly
		// Display "Loading..."
		FXMessageBox messageBox(this, "Loading...", "Please Wait");		
		messageBox.create();
		messageBox.show(PLACEMENT_OWNER);		
		this->repaint();	

		foxScene->remove(pmMesh);
		if (pmMesh)
			delete pmMesh;
		pmMesh=new FXGLPM();		
		foxScene->append(pmMesh);

		FXString filename = open.getFilename();
		// read a PM or a obj
		if (filename.find(".pm")!=-1)
		{
			pmMesh->readPMFile(filename.text());
		}
		else if (filename.find(".obj")!=-1)
		{
			pmMesh->readFile(filename.text());
		}

		// Update bounds from model
		FXRange box;
		pmMesh->bounds(box);
		gldisplay->setBounds(box);
		// Reset FOV
		gldisplay->setFieldOfView(60.0);
		// Change grid, axis scale
		glGrid->setScale(box.longest()/5.0f);
		glAxis->setScale(box.longest()/5.0f);		

		// Setup light
		FXLight lite;
		gldisplay->getLight(lite);
		lite.ambient = FXHVec(0.5f, 0.5f, 0.5f); // gray
		lite.diffuse = FXHVec(145.0f/255.0f, 202.0f/255.0f, 223.0f/255.0f); // bluish
		gldisplay->setLight(lite);
		
		updateScene();
	}
	return 1;
}

long WxyzMainWindow::onCmdAbout(FXObject*,FXSelector,void*)
{
	FXMessageBox::information(this,MBOX_OK,"About Machete 3D",
		"A multiresolution mesh editor and \n"
		"geometric signal processing tool\n"		
		"\n"
		"by Leslie Wu, Wen Su, Zheng Sun, Jingyi Jin"
		"\n"
		"Version 1.0"
		);
	return 1;
}

long WxyzMainWindow::onUpdatePMLevel(FXObject*,FXSelector,void*)
{	
	int desiredDetailLevel = pmLevelSlider->getValue();

	if (pmMesh)
		if (desiredDetailLevel > pmMesh->getCurrentLevel())
			pmMesh->refineToLevelN(desiredDetailLevel);
		else if (desiredDetailLevel < pmMesh->getCurrentLevel())
			pmMesh->coarsenToLevelN(desiredDetailLevel);

	updateScene();

	return 1;
}

long WxyzMainWindow::onBuildPM(FXObject*,FXSelector,void*)
{
	// TODO figure out how to repaint text properly
	// Display status
	FXProgressDialog messageBox(this, "Building multiresolution hierarchy...", "Please Wait");
	messageBox.create();
	messageBox.show(PLACEMENT_OWNER);		
	this->repaint();	
	updateScene();

	pmMesh->buildPM();
	pmLevelSlider->setRange(pmMesh->getMinLevel(), pmMesh->getMaxLevel());
	pmLevelSlider->setValue(pmMesh->getMaxLevel());
	// update selection
	pmMesh->selectedVertexId=-1;
	pmMesh->updateSelection();
	updateScene();
	return 1;
}

/*long WxyzMainWindow::onSmoothPM(FXObject*,FXSelector,void*)
{
	//pmMesh->computeDetailVectors(pmMesh->getCurrentLevel()-1);
	
	pmMesh->smooth();	

	updateScene();

	return 1;
}*/

long WxyzMainWindow::onRecomputeDetail(FXObject*,FXSelector,void*)
{	
	// TODO figure out how to repaint text properly
	// Display status
	FXProgressDialog messageBox(this, "Reconstructing detail...", "Please Wait");
	messageBox.create();
	messageBox.show(PLACEMENT_OWNER);		
	this->repaint();	

	pmMesh->restoreDetailVectors(pmMesh->getMaxLevel());

	// Update slider
	pmLevelSlider->setValue(pmMesh->getMaxLevel());
	
	updateScene();
	return 1;
}

long WxyzMainWindow::onNeighborhoodLevelChange(FXObject*,FXSelector,void*)
{
	if ( pmMesh->selectedVertexId==-1) return 1;

	pmMesh->selectionNeighborDepth=neighborhoodSelectionLevelSpinner->getValue();
	pmMesh->updateSelection();
	if ( flagSphere ) updateSphere();
	gldisplay->update();
	return 1;
}

long WxyzMainWindow::onSphere(FXObject*,FXSelector,void*)
{
	pmMesh->dminscale = dminscale;

//	if ( pmMesh->selectedVertexId == -1 ) return 1;
	if ( flagSphere )
	{
		flagSphere = false;
		foxScene->remove ( fxSphere );
		if (fxSphere) delete fxSphere;

		pmMesh->selectedVertexId=-1;
		pmMesh->updateSelection();

		gldisplay->update();

		pmMesh->flagSphere = flagSphere;
		return 1;
	}

	if ( pmMesh->selectedVertexId != -1 )
	{

	center = pmMesh->getPoint(pmMesh->selectedVertexId);


	PM::Point v;
	float l, lmax = -1;

	for (unsigned int i=0; i<pmMesh->selectedVertices.size(); i++)
	{
		std::vector <PM::VertexHandle> &currentLevel=(pmMesh->selectedVertices[i]);
		for (unsigned int j=0; j<currentLevel.size(); j++)
		{
			v = pmMesh->getPoint(currentLevel[j]);
			l = (center-v).length();
			if ( lmax < l ) lmax = l;
		}
	}

	radius = lmax;
	oldradius=lmax;
	}

	if ( oldradius <0 ) return 1;

	flagSphere = true;

	FXMaterial mtl;
	mtl.ambient = FXHVec(0.2f,0.2f,0.8f,0.2f);
	mtl.diffuse = FXHVec(0.2f,0.2f,0.8f,0.2f);
	mtl.specular = FXHVec(0.2f,0.2f,0.8f,0.2f);
	mtl.emission = FXHVec(0.0f,0.0f,0.0f,1.0f);
	mtl.shininess=30.0f;

	fxSphere = new FXGLSphere ( center[0], center[1], center[2], radius, mtl );
	foxScene->append(fxSphere);
	gldisplay->update();

	radiusSlider->setRange(1, 150);
	radiusSlider->setValue(50);

	pmMesh->flagSphere = flagSphere;
	pmMesh->radius = radius;
	pmMesh->center = center;

	return 1;
}


void WxyzMainWindow::updateSphere()
{
	PM::Point v;
	float l, lmax = -1;

	for (unsigned int i=0; i<pmMesh->selectedVertices.size(); i++)
	{
		std::vector <PM::VertexHandle> &currentLevel=(pmMesh->selectedVertices[i]);
		for (unsigned int j=0; j<currentLevel.size(); j++)
		{
			v = pmMesh->getPoint(currentLevel[j]);
			l = (center-v).length();
			if ( lmax < l ) lmax = l;
		}
	}

	radius = lmax;
	oldradius=lmax;
	fxSphere->radius=radius;

	radiusSlider->setRange(1, 150);
	radiusSlider->setValue(50);

	pmMesh->radius = radius;
}

long WxyzMainWindow::onRadiusChange(FXObject*,FXSelector,void*)
{
	if (!flagSphere) return 1;
	int r = radiusSlider->getValue();
	radius = oldradius*r/50;
	fxSphere->radius=radius;
	gldisplay->update();
	pmMesh->radius = radius;
	return 1;
}

long WxyzMainWindow::onApplyRealTime(FXObject*,FXSelector,void*)
{
	pmMesh->applyOperation(canvas->getValues());
	updateScene();
	return 1;
}

long WxyzMainWindow::onApply(FXObject*,FXSelector,void*)
{
	// test if realtime update is selected.
	// TODO figure out how to repaint text properly
	// Display status
	FXProgressDialog messageBox(this, "Applying filter...", "Please Wait");
	messageBox.create();
	messageBox.show(PLACEMENT_OWNER);		
	this->repaint();	
	return onApplyRealTime(0,0,0);
}

long WxyzMainWindow::onChangeRange(FXObject*,FXSelector,void*)
{
	canvas->setYRange(range->getValue());
	canvas->setUpdate(false);
	canvas->draw();
	return 1;
}

long WxyzMainWindow::onReset(FXObject*,FXSelector,void*)
{
	canvas->reset();
	canvas->draw();
	return 1;
}

// Paint the canvas
long WxyzMainWindow::onPaint(FXObject*,FXSelector,void* ptr){
	canvas->setUpdate(false);
	canvas->draw();
	return 1;
}

// Mouse button was pressed somewhere
long WxyzMainWindow::onMouseDown(FXObject*,FXSelector,void* ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	canvas->selectPoint(ev->win_x, ev->win_y);
	return 1;
}

// Add or delete a point of the frequency function when right mouse clicked
long WxyzMainWindow::onRightBtClicked(FXObject*,FXSelector,void* ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	canvas->addDeletePoint(ev->win_x, ev->win_y);
	return 1;
}

// Move the joints of the functions in mouse movements
long WxyzMainWindow::onMouseMove(FXObject*,FXSelector,void* ptr)
{
	FXEvent *ev=(FXEvent*)ptr;
	canvas->movePoint(ev->last_x, ev->last_y, ev->win_x, ev->win_y);
	return 1;	
}

// Unselect the joint
long WxyzMainWindow::onMouseRelease(FXObject*,FXSelector,void* ptr)
{
	canvas->setSelectedPoint(-1);
	return 1;	
}

long WxyzMainWindow::onChangeShading(FXObject*,FXSelector,void* ptr)
{
	pmMesh->shadingStyle=!pmMesh->shadingStyle;
	gldisplay->update();
	return 1;
}

long WxyzMainWindow::onChangeRealtime(FXObject*,FXSelector,void* ptr)
{
	canvas->toggleRealtime();
	return 1;
}

// Here we begin
int main(int argc,char *argv[])
{
	// Run unit tests
	run_tests();

	// Make application
	FXApp application("Machete 3D","UIUC");

	// Open the display
	application.init(argc,argv);

	// Make window
	new WxyzMainWindow(&application);

	// Create the application's windows
	application.create();

	// Run the application
	return application.run();
}
