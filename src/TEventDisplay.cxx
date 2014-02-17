#include "TEventDisplay.hxx"
#include "TGUIManager.hxx"
#include "TPlotDigitsHits.hxx"
#include "TEventChangeManager.hxx"
#include "TFindResultsHandler.hxx"
#include "TTrajectoryChangeHandler.hxx"
#include "TG4HitChangeHandler.hxx"
#include "TFitChangeHandler.hxx"

#include <TCaptLog.hxx>

#include <TEveManager.h>
#include <TColor.h>
#include <TGLViewer.h>

#include <algorithm>
#include <cmath>

CP::TEventDisplay* CP::TEventDisplay::fEventDisplay = NULL;

CP::TEventDisplay& CP::TEventDisplay::Get(void) {
    if (!fEventDisplay) {
        fEventDisplay = new CP::TEventDisplay();
        fEventDisplay->Init();
    }
    return *fEventDisplay;
}

CP::TEventDisplay::TEventDisplay() {}

void CP::TEventDisplay::Init() {
    TEveManager::Create();

    TGLViewer* glViewer = gEve->GetDefaultGLViewer();
    glViewer->SetCurrentCamera(TGLViewer::kCameraPerspXOY);
    glViewer->SetGuideState(TGLUtil::kAxesEdge,kTRUE,kFALSE,0);

    // This is accessed through the GUI() method.
    fGUIManager = new TGUIManager();

    // Create the event display manager.  This needs the GUI, so it has to be
    // done after TGUIManager is created.
    fEventChangeManager = new TEventChangeManager();
    fEventChangeManager->AddNewEventHandler(new TFindResultsHandler());
    fEventChangeManager->AddUpdateHandler(new TTrajectoryChangeHandler());
    fEventChangeManager->AddUpdateHandler(new TG4HitChangeHandler());
    fEventChangeManager->AddUpdateHandler(new TFitChangeHandler());

    // Connect the class to draw digits to the GUI.
    fPlotDigitsHits = new TPlotDigitsHits();
    CP::TEventDisplay::Get().GUI().GetDrawXDigitsButton()
        ->Connect("Clicked()",
                  "CP::TPlotDigitsHits", 
                  fPlotDigitsHits,
                  "DrawDigits(=0)");
    
    CP::TEventDisplay::Get().GUI().GetDrawVDigitsButton()
        ->Connect("Clicked()",
                  "CP::TPlotDigitsHits", 
                  fPlotDigitsHits,
                  "DrawDigits(=1)");
    
    CP::TEventDisplay::Get().GUI().GetDrawUDigitsButton()
        ->Connect("Clicked()",
                  "CP::TPlotDigitsHits", 
                  fPlotDigitsHits,
                  "DrawDigits(=2)");
    
    // Create the color palette.  This is split into two halves.  The first
    // half is from dark to white and is used for negative values on the
    // digitization plots, as well has the reconstruction objects.  The second
    // halve is from white to dark and is used for positive values on the
    // digitization plot.
    Double_t s[]  = { 0.00, 0.20, 0.35, 0.43, 0.45, 0.50, 1.00};
    Double_t r[]  = { 0.00, 0.90, 1.00, 1.00, 1.00, 1.00, 0.00};
    Double_t g[]  = { 0.50, 0.10, 0.75, 0.90, 1.00, 1.00, 0.00};
    Double_t b[]  = { 0.30, 0.00, 0.00, 0.00, 0.00, 1.00, 0.00};
    fColorCount = 200;
    unsigned int abc = sizeof(s)/sizeof(s[0]);
        
    fColorBase = TColor::CreateGradientColorTable(abc, s, r, g, b, 
                                                  fColorCount);    
    
    CaptLog("Event display constructed");
}

CP::TEventDisplay::~TEventDisplay() {
    CaptLog("Event display deconstructed");
}

int CP::TEventDisplay::LinearColor(double value, double minVal, double maxVal) {
    int nCol = fColorCount/2;
    int iValue = nCol*(value - minVal)/(maxVal - minVal);
    nCol = std::max(0,std::min(iValue,nCol));
    return fColorBase + nCol;
}

int CP::TEventDisplay::LogColor(double value, double minVal, double maxVal,
    double magScale) {
    int nCol = fColorCount/2;
    double scale = std::pow(10.0,magScale);
    double nvalue = std::max(0.0,std::min((value-minVal)/(maxVal-minVal),1.0));
    double lValue = std::log10(1.0+scale*nvalue)/magScale;
    int iValue = nCol*lValue;
    iValue = std::max(0,std::min(iValue,nCol-1));
    return fColorBase + iValue;
}
